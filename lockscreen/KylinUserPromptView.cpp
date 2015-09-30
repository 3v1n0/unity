// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2014 Canonical Ltd
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 3 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
* Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
*/

#include "KylinUserPromptView.h"

#include "config.h"
#include <glib/gi18n-lib.h>

#include <boost/algorithm/string/trim.hpp>
#include <Nux/VLayout.h>
#include <Nux/HLayout.h>
#include <NuxCore/Logger.h>
#include "Variant.h"

#include "LockScreenSettings.h"
#include "unity-shared/CairoTexture.h"
#include "unity-shared/TextInput.h"
#include "unity-shared/StaticCairoText.h"
#include "unity-shared/RawPixel.h"
#include "unity-shared/IconTexture.h"
#include "unity-shared/TextureCache.h"

namespace unity
{
namespace lockscreen
{
DECLARE_LOGGER(logger,"unity.lockscreen.kylinprompt");
namespace
{
const RawPixel PADDING              = 10_em;
const RawPixel LAYOUT_MARGIN        = 20_em;
const RawPixel MSG_LAYOUT_MARGIN    = 15_em;
const RawPixel PROMPT_LAYOUT_MARGIN =  5_em;
const RawPixel ICON_SIZE            = 32_em;
const RawPixel Avatar_SIZE          = 128_em;
const int PROMPT_FONT_SIZE = 14;

std::string SanitizeMessage(std::string const& message)
{
  std::string msg = boost::algorithm::trim_copy(message);

  if (msg.empty())
    return msg;

  if (msg[msg.size()-1] == ':')
    msg = msg.substr(0, msg.size()-1);

  if (msg == "Password")
    return _("Password");

  if (msg == "login")
    return _("Username");

  return msg;
}

bool DoUserSwitch()
{
  glib::Error error;
  glib::Object<GDBusConnection> bus(g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error));

  if(error)
  {
    LOG_ERROR(logger)<<"Failed to get system bus: "<< error;
    return false;
  }

  glib::Variant result(g_dbus_connection_call_sync(bus,
                                         "org.freedesktop.DisplayManager",
                                         g_getenv("XDG_SEAT_PATH"),
                                         "org.freedesktop.DisplayManager.Seat",
                                         "SwitchToGreeter",
                                         g_variant_new("()"),
                                         G_VARIANT_TYPE("()"),
                                         G_DBUS_CALL_FLAGS_NONE,
                                         -1,
                                         nullptr,
                                         &error));
  if(error)
  {
    LOG_ERROR(logger)<<"Failed to switch to greeter: "<< error;
    return false;
  }

  return result.GetBool();
}

std::string GetUserImage()
{
  glib::Error error;
  glib::Object<GDBusConnection> system_bus(g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error));

  if (error)
  {
    LOG_ERROR(logger)<<"Unable to get system bus: "<< error;
    return "";
  }

  glib::Variant user_icon(g_dbus_connection_call_sync (system_bus,
                                           "org.freedesktop.Accounts",
                                           g_strdup_printf("/org/freedesktop/Accounts/User%i", getuid()),
                                           "org.freedesktop.DBus.Properties",
                                           "Get",
                                           g_variant_new("(ss)",
                                                         "org.freedesktop.Accounts.User",
                                                         "IconFile"),
                                           G_VARIANT_TYPE("(v)"),
                                           G_DBUS_CALL_FLAGS_NONE,
                                           -1,
                                           nullptr,
                                           &error));

  if (error)
  {
    LOG_ERROR(logger)<<"Couldn't find user icon in accounts service: "<< error;
    return "";
  }

  glib::Variant icon_file = user_icon.GetVariant();

  return icon_file.GetString();
}

}

KylinUserPromptView::KylinUserPromptView(session::Manager::Ptr const& session_manager)
  : AbstractUserPromptView(session_manager)
  , scale(1.0)
  , session_manager_(session_manager)
  , username_(nullptr)
  , msg_layout_(nullptr)
  , prompt_layout_(nullptr)
{
    user_authenticator_.echo_on_requested.connect([this](std::string const& message, PromiseAuthCodePtr const& promise){
        AddPrompt(message, true, promise);
    });

    user_authenticator_.echo_off_requested.connect([this](std::string const& message, PromiseAuthCodePtr const& promise){
        AddPrompt(message, false, promise);
    });

    user_authenticator_.message_requested.connect([this](std::string const& message){
        AddMessage(message, nux::color::White);
    });

    user_authenticator_.error_requested.connect([this](std::string const& message){
        AddMessage(message, nux::color::Red);
    });

    user_authenticator_.clear_prompts.connect([this](){
        ResetLayout();
    });

    scale.changed.connect(sigc::hide(sigc::mem_fun(this, &KylinUserPromptView::UpdateSize)));

    UpdateSize();
    ResetLayout();

    user_authenticator_.AuthenticateStart(session_manager_->UserName(),
                                          sigc::mem_fun(this, &KylinUserPromptView::AuthenticationCb));
}

void KylinUserPromptView::ResetLayout()
{
  focus_queue_.clear();

  SetLayout(new nux::HLayout());

  static_cast<nux::HLayout*>(GetLayout())->SetHorizontalInternalMargin(LAYOUT_MARGIN.CP(scale));

  nux::Layout* switch_layout = new nux::HLayout();

  TextureCache& cache = TextureCache::GetDefault();
  SwitchIcon_ = new IconTexture(cache.FindTexture("cof.png", ICON_SIZE, ICON_SIZE));
  switch_layout->AddView(SwitchIcon_);
  SwitchIcon_->mouse_click.connect([this](int x, int y, unsigned long button_flags, unsigned long key_flags) {
    DoUserSwitch();
  });

  unity::StaticCairoText* switch_label = new unity::StaticCairoText(_("Switch User"));
  switch_label->SetScale(scale);
  switch_layout->AddView(switch_label);

  switch_layout->SetMaximumSize(128, ICON_SIZE);

  GetLayout()->AddLayout(switch_layout);

  Avatar_ = new IconTexture(GetUserImage(), Avatar_SIZE);
  Avatar_->SetMinimumWidth(Avatar_SIZE);
  Avatar_->SetMaximumWidth(Avatar_SIZE);

  GetLayout()->AddView(Avatar_);

  nux::Layout* prompt_layout = new nux::VLayout();

  auto const& real_name = session_manager_->RealName();
  auto const& name = (real_name.empty() ? session_manager_->UserName() : real_name);

  username_ = new unity::StaticCairoText(name);
  username_->SetScale(scale);
  username_->SetFont("Ubuntu "+std::to_string(PROMPT_FONT_SIZE));
  prompt_layout->AddView(username_);

  msg_layout_ = new nux::VLayout();
  msg_layout_->SetVerticalInternalMargin(MSG_LAYOUT_MARGIN.CP(scale));
  prompt_layout->AddLayout(msg_layout_);

  prompt_layout_ = new nux::VLayout();
  prompt_layout_->SetVerticalInternalMargin(PROMPT_LAYOUT_MARGIN.CP(scale));
  prompt_layout->AddLayout(prompt_layout_);

  GetLayout()->AddLayout(prompt_layout);
  QueueRelayout();
  QueueDraw();
}

void KylinUserPromptView::UpdateSize()
{
  auto width = 15 * Settings::GRID_SIZE.CP(scale);
  auto height = 3 * Settings::GRID_SIZE.CP(scale);

  SetMinimumWidth(width);
  SetMaximumWidth(width);
  SetMinimumHeight(height);

  if (nux::Layout* layout = GetLayout())
  {
    layout->SetLeftAndRightPadding(PADDING.CP(scale));
    layout->SetTopAndBottomPadding(PADDING.CP(scale));
    static_cast<nux::VLayout*>(layout)->SetVerticalInternalMargin(LAYOUT_MARGIN.CP(scale));
  }

  if (username_)
    username_->SetScale(scale);

  if (msg_layout_)
  {
    msg_layout_->SetVerticalInternalMargin(MSG_LAYOUT_MARGIN.CP(scale));

    for (auto* area : msg_layout_->GetChildren())
    {
      area->SetMaximumWidth(width);
      static_cast<StaticCairoText*>(area)->SetScale(scale);
    }
  }

  if (prompt_layout_)
  {
    prompt_layout_->SetVerticalInternalMargin(PROMPT_LAYOUT_MARGIN.CP(scale));

    for (auto* area : prompt_layout_->GetChildren())
    {
      auto* text_input = static_cast<TextInput*>(area);
      text_input->SetMinimumHeight(Settings::GRID_SIZE.CP(scale));
      text_input->SetMaximumHeight(Settings::GRID_SIZE.CP(scale));
      text_input->scale = scale();
    }
  }

  ComputeContentSize();
  QueueRelayout();
  QueueDraw();
}

bool KylinUserPromptView::InspectKeyEvent(unsigned int eventType, unsigned int key_sym, const char* character)
{
  if ((eventType == nux::NUX_KEYDOWN) && (key_sym == NUX_VK_ESCAPE))
  {
    if (!focus_queue_.empty())
      focus_queue_.front()->text_entry()->SetText("");

    return true;
  }

  return false;
}

void KylinUserPromptView::AuthenticationCb(bool authenticated)
{
  ResetLayout();

  if (authenticated)
  {
    session_manager_->unlock_requested.emit();
  }
  else
  {
    AddMessage(_("Invalid password, please try again"), nux::color::Red);

    user_authenticator_.AuthenticateStart(session_manager_->UserName(),
                                          sigc::mem_fun(this, &KylinUserPromptView::AuthenticationCb));
  }
}

void KylinUserPromptView::Draw(nux::GraphicsEngine& graphics_engine, bool force_draw)
{
  nux::Geometry const& geo = GetGeometry();

  graphics_engine.PushClippingRectangle(geo);
  nux::GetPainter().PaintBackground(graphics_engine, geo);

  graphics_engine.PopClippingRectangle();
}

void KylinUserPromptView::DrawContent(nux::GraphicsEngine& graphics_engine, bool force_draw)
{
  nux::Geometry const& geo = GetGeometry();
  graphics_engine.PushClippingRectangle(geo);

  if (GetLayout())
    GetLayout()->ProcessDraw(graphics_engine, force_draw);

  graphics_engine.PopClippingRectangle();
}

nux::View* KylinUserPromptView::focus_view()
{
  if (focus_queue_.empty())
    return nullptr;

  for (auto* view : focus_queue_)
    if (view->text_entry()->HasKeyboardFocus())
      return view;

  return focus_queue_.front()->text_entry();
}

void KylinUserPromptView::AddPrompt(std::string const& message, bool visible, PromiseAuthCodePtr const& promise)
{
  auto* text_input = new unity::TextInput();
  auto* text_entry = text_input->text_entry();

  text_input->scale = scale();
  text_input->input_hint = SanitizeMessage(message);
  text_input->hint_font_size = PROMPT_FONT_SIZE;
  text_input->show_caps_lock = true;
  text_input->show_activator = true;
  text_entry->SetPasswordMode(!visible);
  text_entry->SetPasswordChar("•");
  text_entry->SetToggleCursorVisibilityOnKeyFocus(true);
  text_entry->clipboard_enabled = false;

  text_input->SetMinimumHeight(Settings::GRID_SIZE.CP(scale));
  text_input->SetMaximumHeight(Settings::GRID_SIZE.CP(scale));
  prompt_layout_->AddView(text_input, 1);
  focus_queue_.push_back(text_input);

  // Don't remove it, it helps with a11y.
  if (focus_queue_.size() == 1)
    nux::GetWindowCompositor().SetKeyFocusArea(text_entry);

  text_entry->activated.connect([this, text_input, promise](){
    auto* text_entry = text_input->text_entry();

    if (!text_entry->GetInputEventSensitivity())
      return;

    if (focus_queue_.size() == 1)
    {
      text_input->SetSpinnerVisible(true);
      text_input->SetSpinnerState(STATE_SEARCHING);
    }

    focus_queue_.pop_front();
    cached_focused_geo_ = text_entry->GetGeometry();
    text_entry->SetInputEventSensitivity(false);
    QueueRelayout();
    QueueDraw();

    std::string const& password = text_entry->GetText();
    if (promise)
      promise->set_value(password);
  });

  GetLayout()->ComputeContentPosition(0, 0);
  ComputeContentSize();
  QueueRelayout();
  QueueDraw();
}

void KylinUserPromptView::AddMessage(std::string const& message, nux::Color const& color)
{
  nux::Geometry const& geo = GetGeometry();
  auto* view = new unity::StaticCairoText("");
  view->SetScale(scale);
  view->SetFont(Settings::Instance().font_name());
  view->SetTextColor(color);
  view->SetText(message);
  view->SetMaximumWidth(geo.width - PADDING.CP(scale)*2);
  msg_layout_->AddView(view);

  GetLayout()->ComputeContentPosition(0, 0);
  ComputeContentSize();
  QueueRelayout();
  QueueDraw();
}


}
}
