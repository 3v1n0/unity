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

#include "UserPromptView.h"

#include "config.h"
#include <glib/gi18n-lib.h>

#include <boost/algorithm/string/trim.hpp>
#include <Nux/VLayout.h>

#include "LockScreenSettings.h"
#include "LockScreenButton.h"
#include "unity-shared/CairoTexture.h"
#include "unity-shared/TextInput.h"
#include "unity-shared/StaticCairoText.h"
#include "unity-shared/RawPixel.h"

namespace unity
{
namespace lockscreen
{
namespace
{
const RawPixel PADDING              = 10_em;
const RawPixel LAYOUT_MARGIN        = 10_em;
const RawPixel MSG_LAYOUT_MARGIN    = 15_em;
const RawPixel PROMPT_LAYOUT_MARGIN =  5_em;
const RawPixel BUTTON_LAYOUT_MARGIN =  5_em;
const int PROMPT_FONT_SIZE = 13;

nux::AbstractPaintLayer* CrateBackgroundLayer(double width, double height, double scale)
{
  nux::CairoGraphics cg(CAIRO_FORMAT_ARGB32, width, height);
  cairo_surface_set_device_scale(cg.GetSurface(), scale, scale);
  cairo_t* cr = cg.GetInternalContext();

  cairo_set_source_rgba(cr, 0.1, 0.1, 0.1, 0.4);

  cg.DrawRoundedRectangle(cr,
                          1.0,
                          0, 0,
                          Settings::GRID_SIZE * 0.3,
                          width/scale, height/scale);

  cairo_fill_preserve(cr);

  cairo_set_source_rgba(cr, 0.4, 0.4, 0.4, 0.4);
  cairo_set_line_width(cr, 1);
  cairo_stroke (cr);

  // Create the texture layer
  nux::TexCoordXForm texxform;

  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

  return (new nux::TextureLayer(texture_ptr_from_cairo_graphics(cg)->GetDeviceTexture(),
                                texxform,
                                nux::color::White,
                                true,
                                rop));
}

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

}

UserPromptView::UserPromptView(session::Manager::Ptr const& session_manager)
  : AbstractUserPromptView(session_manager)
  , session_manager_(session_manager)
  , username_(nullptr)
  , msg_layout_(nullptr)
  , prompt_layout_(nullptr)
  , button_layout_(nullptr)
  , prompted_(false)
  , unacknowledged_messages_(false)
{
  user_authenticator_.echo_on_requested.connect([this](std::string const& message, PromiseAuthCodePtr const& promise){
    prompted_ = true;
    unacknowledged_messages_ = false;
    AddPrompt(message, /* visible */ true, promise);
  });

  user_authenticator_.echo_off_requested.connect([this](std::string const& message, PromiseAuthCodePtr const& promise){
    prompted_ = true;
    unacknowledged_messages_ = false;
    AddPrompt(message, /* visible */ false, promise);
  });

  user_authenticator_.message_requested.connect([this](std::string const& message){
    unacknowledged_messages_ = true;
    AddMessage(message, nux::color::White);
  });

  user_authenticator_.error_requested.connect([this](std::string const& message){
    unacknowledged_messages_ = true;
    AddMessage(message, nux::color::Red);
  });

  user_authenticator_.clear_prompts.connect([this](){
    ResetLayout();
  });

  scale.changed.connect(sigc::hide(sigc::mem_fun(this, &UserPromptView::UpdateSize)));

  UpdateSize();
  ResetLayout();

  StartAuthentication();
}

void UserPromptView::UpdateSize()
{
  auto width = 8 * Settings::GRID_SIZE.CP(scale);
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

  if (button_layout_)
  {
    button_layout_->SetVerticalInternalMargin(BUTTON_LAYOUT_MARGIN.CP(scale));

    for (auto* area : button_layout_->GetChildren())
    {
      auto* button = static_cast<LockScreenButton*>(area);
      button->SetMinimumHeight(Settings::GRID_SIZE.CP(scale));
      button->SetMaximumHeight(Settings::GRID_SIZE.CP(scale));
      button->scale = scale();
    }
  }

  bg_layer_.reset();

  ComputeContentSize();
  QueueRelayout();
  QueueDraw();
}

bool UserPromptView::InspectKeyEvent(unsigned int eventType, unsigned int key_sym, const char* character)
{
  if ((eventType == nux::NUX_KEYDOWN) && (key_sym == NUX_VK_ESCAPE))
  {
    if (!focus_queue_.empty())
      focus_queue_.front()->text_entry()->SetText("");

    return true;
  }

  return false;
}

void UserPromptView::ResetLayout()
{
  bool keep_msg_layout = msg_layout_ && (!prompted_ || unacknowledged_messages_);

  focus_queue_.clear();

  if (keep_msg_layout)
    msg_layout_->Reference();

  SetLayout(new nux::VLayout());

  GetLayout()->SetLeftAndRightPadding(PADDING.CP(scale));
  GetLayout()->SetTopAndBottomPadding(PADDING.CP(scale));
  static_cast<nux::VLayout*>(GetLayout())->SetVerticalInternalMargin(LAYOUT_MARGIN.CP(scale));

  auto const& real_name = session_manager_->RealName();
  auto const& name = (real_name.empty() ? session_manager_->UserName() : real_name);

  username_ = new unity::StaticCairoText(name);
  username_->SetScale(scale);
  username_->SetFont("Ubuntu "+std::to_string(PROMPT_FONT_SIZE));
  GetLayout()->AddView(username_);

  if (!keep_msg_layout)
  {
    msg_layout_ = new nux::VLayout();
    msg_layout_->SetVerticalInternalMargin(MSG_LAYOUT_MARGIN.CP(scale));
    msg_layout_->SetReconfigureParentLayoutOnGeometryChange(true);
  }

  GetLayout()->AddLayout(msg_layout_);

  if (keep_msg_layout)
    msg_layout_->UnReference();

  prompt_layout_ = new nux::VLayout();
  prompt_layout_->SetVerticalInternalMargin(PROMPT_LAYOUT_MARGIN.CP(scale));
  prompt_layout_->SetReconfigureParentLayoutOnGeometryChange(true);
  GetLayout()->AddLayout(prompt_layout_);

  button_layout_ = new nux::VLayout();
  button_layout_->SetVerticalInternalMargin(BUTTON_LAYOUT_MARGIN.CP(scale));
  button_layout_->SetReconfigureParentLayoutOnGeometryChange(true);

  QueueRelayout();
  QueueDraw();
}

void UserPromptView::AuthenticationCb(bool is_authenticated)
{
  ResetLayout();

  if (is_authenticated)
  {
    if (prompted_ && !unacknowledged_messages_)
      DoUnlock();
    else
      ShowAuthenticated(true);
  }
  else
  {
    if (prompted_)
    {
      AddMessage(_("Invalid password, please try again"), nux::color::Red);
      StartAuthentication();
    }
    else
    {
      AddMessage(_("Failed to authenticate"), nux::color::Red);
      ShowAuthenticated(false);
    }
  }
}

void UserPromptView::EnsureBGLayer()
{
  auto const& geo = GetGeometry();

  if (bg_layer_)
  {
    auto const& layer_geo = bg_layer_->GetGeometry();

    if (layer_geo.width == geo.width && layer_geo.height == geo.height)
      return;
  }

  bg_layer_.reset(CrateBackgroundLayer(geo.width, geo.height, scale));
}

void UserPromptView::Draw(nux::GraphicsEngine& graphics_engine, bool /* force_draw */)
{
  nux::Geometry const& geo = GetGeometry();

  graphics_engine.PushClippingRectangle(geo);
  nux::GetPainter().PaintBackground(graphics_engine, geo);

  EnsureBGLayer();
  nux::GetPainter().PushDrawLayer(graphics_engine, geo, bg_layer_.get());

  nux::GetPainter().PopBackground();
  graphics_engine.PopClippingRectangle();
}

void UserPromptView::DrawContent(nux::GraphicsEngine& graphics_engine, bool force_draw)
{
  nux::Geometry const& geo = GetGeometry();
  graphics_engine.PushClippingRectangle(geo);

  if (!IsFullRedraw())
  {
    EnsureBGLayer();
    nux::GetPainter().PushLayer(graphics_engine, geo, bg_layer_.get());
  }

  if (GetLayout())
    GetLayout()->ProcessDraw(graphics_engine, force_draw);

  if (!IsFullRedraw())
    nux::GetPainter().PopBackground();

  graphics_engine.PopClippingRectangle();
}

nux::View* UserPromptView::focus_view()
{
  if (focus_queue_.empty())
  {
    if (button_layout_ && button_layout_->GetChildren().size() > 0)
    {
      return static_cast<nux::View*>(button_layout_->GetChildren().front());
    }
    else
    {
      return nullptr;
    }
  }

  for (auto* view : focus_queue_)
    if (view->text_entry()->HasKeyboardFocus())
      return view;

  return focus_queue_.front()->text_entry();
}

void UserPromptView::AddPrompt(std::string const& message, bool visible, PromiseAuthCodePtr const& promise)
{
  auto* text_input = new unity::TextInput();
  auto* text_entry = text_input->text_entry();

  text_input->scale = scale();
  text_input->input_hint = SanitizeMessage(message);
  text_input->hint_font_size = PROMPT_FONT_SIZE;
  text_input->show_lock_warnings = true;
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

void UserPromptView::AddMessage(std::string const& message, nux::Color const& color)
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

void UserPromptView::AddButton(std::string const& text, std::function<void()> const& cb)
{
  auto* button = new LockScreenButton (text, NUX_TRACKER_LOCATION);
  button->scale = scale();
  button_layout_->AddView(button, 1, nux::MINOR_POSITION_START, nux::MINOR_SIZE_FULL);

  button->state_change.connect ([cb] (nux::View*) {
    cb();
  });

  GetLayout()->ComputeContentPosition(0, 0);
  ComputeContentSize();
  QueueRelayout();
  QueueDraw();
}

void UserPromptView::ShowAuthenticated(bool successful)
{
  prompted_ = true;
  unacknowledged_messages_ = false;

  if (successful)
    AddButton(_("Unlock"), sigc::mem_fun(this, &UserPromptView::DoUnlock));
  else
    AddButton(_("Retry"), sigc::mem_fun(this, &UserPromptView::StartAuthentication));

  GetLayout()->AddLayout(button_layout_);
}

void UserPromptView::StartAuthentication()
{
  prompted_ = false;
  unacknowledged_messages_ = false;

  user_authenticator_.AuthenticateStart(session_manager_->UserName(),
                                        sigc::mem_fun(this, &UserPromptView::AuthenticationCb));
}

void UserPromptView::DoUnlock()
{
  session_manager_->unlock_requested.emit();
}

}
}
