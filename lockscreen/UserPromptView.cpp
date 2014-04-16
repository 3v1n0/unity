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

#include <boost/algorithm/string/trim.hpp>
#include <Nux/VLayout.h>
#include <X11/XKBlib.h>

#include "LockScreenSettings.h"
#include "unity-shared/CairoTexture.h"
#include "unity-shared/DashStyle.h"
#include "unity-shared/PreviewStyle.h"
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
const RawPixel PROMPT_LAYOUT_MARGIN = 5_em;

const int PROMPT_FONT_SIZE     = 13;

nux::AbstractPaintLayer* CrateBackgroundLayer(int width, int height)
{
  nux::CairoGraphics cg(CAIRO_FORMAT_ARGB32, width, height);
  cairo_t* cr = cg.GetInternalContext();

  cairo_set_source_rgba(cr, 0.1, 0.1, 0.1, 0.4);

  cg.DrawRoundedRectangle(cr,
                          1.0,
                          0, 0,
                          Settings::GRID_SIZE * 0.3,
                          width, height);

  cairo_fill_preserve(cr);

  cairo_set_source_rgba (cr, 0.4, 0.4, 0.4, 0.4);
  cairo_set_line_width (cr, 1);
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

nux::AbstractPaintLayer* CreateWarningLayer(nux::BaseTexture* texture)
{
  // Create the texture layer
  nux::TexCoordXForm texxform;

  texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
  texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
  texxform.min_filter = nux::TEXFILTER_LINEAR;
  texxform.mag_filter = nux::TEXFILTER_LINEAR;

  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

  return (new nux::TextureLayer(texture->GetDeviceTexture(),
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
  : nux::View(NUX_TRACKER_LOCATION)
  , session_manager_(session_manager)
  , caps_lock_on_(false)
{
  user_authenticator_.echo_on_requested.connect([this](std::string const& message, PromiseAuthCodePtr const& promise){
    AddPrompt(message, /* visible */ true, promise);
  });

  user_authenticator_.echo_off_requested.connect([this](std::string const& message, PromiseAuthCodePtr const& promise){
    AddPrompt(message, /* visible */ false, promise);
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

  dash::previews::Style& preview_style = dash::previews::Style::Instance();

  warning_ = preview_style.GetWarningIcon();
  ResetLayout();

  user_authenticator_.AuthenticateStart(session_manager_->UserName(),
                                        sigc::mem_fun(this, &UserPromptView::AuthenticationCb));

  // When we get to HiDPI changes here, we will need to update this width
  dash::Style& style = dash::Style::Instance();
  spin_icon_width_ = style.GetSearchSpinIcon()->GetWidth();

  CheckIfCapsLockOn();
}

void UserPromptView::CheckIfCapsLockOn()
{
  Display *dpy = nux::GetGraphicsDisplay()->GetX11Display();
  unsigned int state = 0;
  XkbGetIndicatorState(dpy, XkbUseCoreKbd, &state);

  // Caps is on 0x1, couldn't find any #define in /usr/include/X11
  if ((state & 0x1) == 1)
    caps_lock_on_ = true;
  else
    caps_lock_on_ = false;
}

bool UserPromptView::InspectKeyEvent(unsigned int eventType, unsigned int key_sym, const char* character)
{
  if ((eventType == nux::NUX_KEYDOWN) && (key_sym == NUX_VK_ESCAPE))
  {
    if (!focus_queue_.empty())
      focus_queue_.front()->SetText("");

    return true;
  }

  return false;
}

void UserPromptView::ResetLayout()
{
  focus_queue_.clear();

  SetLayout(new nux::VLayout());

  GetLayout()->SetLeftAndRightPadding(PADDING);
  GetLayout()->SetTopAndBottomPadding(PADDING);
  static_cast<nux::VLayout*>(GetLayout())->SetVerticalInternalMargin(LAYOUT_MARGIN);

  auto const& real_name = session_manager_->RealName();
  auto const& name = (real_name.empty() ? session_manager_->UserName() : real_name);

  unity::StaticCairoText* username = new unity::StaticCairoText(name);
  username->SetFont("Ubuntu "+std::to_string(PROMPT_FONT_SIZE));
  GetLayout()->AddView(username);

  msg_layout_ = new nux::VLayout();
  msg_layout_->SetVerticalInternalMargin(MSG_LAYOUT_MARGIN);
  msg_layout_->SetReconfigureParentLayoutOnGeometryChange(true);
  GetLayout()->AddLayout(msg_layout_);

  prompt_layout_ = new nux::VLayout();
  prompt_layout_->SetVerticalInternalMargin(PROMPT_LAYOUT_MARGIN);
  prompt_layout_->SetReconfigureParentLayoutOnGeometryChange(true);
  GetLayout()->AddLayout(prompt_layout_);

  QueueRelayout();
  QueueDraw();
}

void UserPromptView::AuthenticationCb(bool authenticated)
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
                                          sigc::mem_fun(this, &UserPromptView::AuthenticationCb));
  }
}

void UserPromptView::Draw(nux::GraphicsEngine& graphics_engine, bool /* force_draw */)
{
  nux::Geometry const& geo = GetGeometry();

  graphics_engine.PushClippingRectangle(geo);
  nux::GetPainter().PaintBackground(graphics_engine, geo);

  bg_layer_.reset(CrateBackgroundLayer(geo.width, geo.height));
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
    bg_layer_.reset(CrateBackgroundLayer(geo.width, geo.height));
    nux::GetPainter().PushLayer(graphics_engine, geo, bg_layer_.get());
  }

  if (caps_lock_on_)
  {
    for (auto const& text_entry : focus_queue_)
      PaintWarningIcon(graphics_engine, text_entry->GetGeometry());

    if (focus_queue_.empty())
      PaintWarningIcon(graphics_engine, cached_focused_geo_);
  }

  if (GetLayout())
    GetLayout()->ProcessDraw(graphics_engine, force_draw);

  if (!IsFullRedraw())
    nux::GetPainter().PopBackground();

  graphics_engine.PopClippingRectangle();
}

void UserPromptView::PaintWarningIcon(nux::GraphicsEngine& graphics_engine, nux::Geometry const& geo)
{
  nux::Geometry warning_geo = {geo.x + geo.width - GetWarningIconOffset(),
                               geo.y, warning_->GetWidth(), warning_->GetHeight()};

  nux::GetPainter().PushLayer(graphics_engine, warning_geo, CreateWarningLayer(warning_));
}

int UserPromptView::GetWarningIconOffset()
{
  return warning_->GetWidth() + spin_icon_width_;
}

nux::View* UserPromptView::focus_view()
{
  if (focus_queue_.empty())
    return nullptr;

  for (auto* view : focus_queue_)
    if (view->HasKeyboardFocus())
      return view;

  return focus_queue_.front();
}

void UserPromptView::ToggleCapsLockBool()
{
  caps_lock_on_ = !caps_lock_on_;
  QueueDraw();
}

void UserPromptView::RecvKeyUp(unsigned keysym,
                               unsigned long keycode,
                               unsigned long state)
{
  if (!caps_lock_on_ && keysym == NUX_VK_CAPITAL)
  {
    ToggleCapsLockBool();
  }
  else if (caps_lock_on_ && keysym == NUX_VK_CAPITAL)
  {
    ToggleCapsLockBool();
  }
}

void UserPromptView::AddPrompt(std::string const& message, bool visible, PromiseAuthCodePtr const& promise)
{
  auto* text_input = new unity::TextInput();
  auto* text_entry = text_input->text_entry();

  text_input->input_hint = SanitizeMessage(message);
  text_input->hint_font_size = PROMPT_FONT_SIZE;
  text_entry->SetPasswordMode(!visible);
  text_entry->SetPasswordChar("•");
  text_entry->SetToggleCursorVisibilityOnKeyFocus(true);

  text_entry->key_up.connect(sigc::mem_fun(this, &UserPromptView::RecvKeyUp));

  text_input->SetMinimumHeight(Settings::GRID_SIZE);
  text_input->SetMaximumHeight(Settings::GRID_SIZE);
  prompt_layout_->AddView(text_input, 1);
  focus_queue_.push_back(text_entry);

  CheckIfCapsLockOn();

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
  auto* view = new unity::StaticCairoText("");
  view->SetFont(Settings::Instance().font_name());
  view->SetTextColor(color);
  view->SetText(message);

  msg_layout_->AddView(view);

  GetLayout()->ComputeContentPosition(0, 0);
  ComputeContentSize();
  QueueRelayout();
  QueueDraw();
}

}
}
