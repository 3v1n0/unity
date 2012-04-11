/*
 * Copyright (C) 2011 Canonical Ltd
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
 * Authored by: Andrea Azzarone <azzaronea@gmail.com>
 */

#include "ShortcutController.h"

#include "UBusMessages.h"
#include "WindowManager.h"

namespace unity
{
namespace shortcut
{
namespace
{
const unsigned int SUPER_TAP_DURATION = 650;
const unsigned int FADE_DURATION = 100;
} // anonymouse namespace;

Controller::Controller(std::list<AbstractHint::Ptr>& hints)
  : view_window_(0)
  , visible_(false)
  , enabled_(true)
  , show_timer_(0)
  , fade_in_animator_(FADE_DURATION)
  , fade_out_animator_(FADE_DURATION)
{
  bg_color_ = nux::Color(0.0, 0.0, 0.0, 0.5);

  ubus_manager_.RegisterInterest(UBUS_BACKGROUND_COLOR_CHANGED,
                                 sigc::mem_fun(this, &Controller::OnBackgroundUpdate));

  ubus_manager_.RegisterInterest(UBUS_LAUNCHER_START_KEY_SWTICHER, [&] (GVariant*) {
                                   enabled_ = false;
                                 });

  ubus_manager_.RegisterInterest(UBUS_LAUNCHER_END_KEY_SWTICHER, [&] (GVariant*) {
                                   enabled_ = true;
                                 });

  ubus_manager_.RegisterInterest(UBUS_OVERLAY_SHOWN, [&] (GVariant*) {
                                   Hide();
                                 });

  ubus_manager_.SendMessage(UBUS_BACKGROUND_REQUEST_COLOUR_EMIT);

  model_.reset(new Model(hints));

  model_->Fill();

  fade_in_animator_.animation_updated.connect(sigc::mem_fun(this, &Controller::OnFadeInUpdated));
  fade_in_animator_.animation_ended.connect(sigc::mem_fun(this, &Controller::OnFadeInEnded));
  fade_out_animator_.animation_updated.connect(sigc::mem_fun(this, &Controller::OnFadeOutUpdated));
  fade_out_animator_.animation_ended.connect(sigc::mem_fun(this, &Controller::OnFadeOutEnded));
}

Controller::~Controller()
{
  if (view_window_)
    view_window_->UnReference();

  view_.Release();
}


void Controller::OnFadeInUpdated(double opacity)
{
  view_window_->SetOpacity(opacity);
}

void Controller::OnFadeInEnded()
{
  view_window_->SetOpacity(1.0);
}

void Controller::OnFadeOutUpdated(double progress)
{
  double opacity = CLAMP(1.0f - progress, 0.0f, 1.0f);
  view_window_->SetOpacity(opacity);
}

void Controller::OnFadeOutEnded()
{
  view_window_->SetOpacity(0.0);
}


void Controller::OnBackgroundUpdate(GVariant* data)
{
  gdouble red, green, blue, alpha;
  g_variant_get(data, "(dddd)", &red, &green, &blue, &alpha);
  bg_color_ = nux::Color(red, green, blue, alpha);

  if (view_)
    view_->background_color = bg_color_;
}

bool Controller::Show()
{
  if (show_timer_)
    g_source_remove(show_timer_);
  show_timer_ = 0;

  if (enabled_)
  {
    show_timer_ = g_timeout_add(SUPER_TAP_DURATION, &Controller::OnShowTimer, this);
    model_->Fill();
    visible_ = true;

    return true;
  }

  return false;
}

gboolean Controller::OnShowTimer(gpointer data)
{
  Controller* self = static_cast<Controller*>(data);

  if (!self->enabled_)
  {
    return FALSE;
  }

  self->EnsureView();

  nux::Geometry geo;
  if (!self->view_->GetBaseGeometry(geo))
    return FALSE;
  self->view_window_->SetGeometry(geo);

  if (self->visible_)
  {
    self->view_->SetupBackground(true);
    self->fade_out_animator_.Stop();
    self->fade_in_animator_.Start(self->view_window_->GetOpacity());
  }

  self->show_timer_ = 0;
  return FALSE;
}

void Controller::ConstructView()
{
  view_ = View::Ptr(new View());
  view_->SetModel(model_);
  view_->background_color = bg_color_;

  if (!view_window_)
  {
    main_layout_ = new nux::HLayout(NUX_TRACKER_LOCATION);
    main_layout_->SetVerticalExternalMargin(0);
    main_layout_->SetHorizontalExternalMargin(0);

    view_window_ = new nux::BaseWindow("ShortcutHint");
    view_window_->SinkReference();
    view_window_->SetLayout(main_layout_);
    view_window_->SetBackgroundColor(nux::Color(0x00000000));
  }

  main_layout_->AddView(view_.GetPointer());

  view_->SetupBackground(false);
  view_window_->SetOpacity(0.0);
  view_window_->ShowWindow(true);
}

void Controller::EnsureView()
{
  if (!view_window_)
    ConstructView();
}

void Controller::SetAdjustment(int x, int y)
{
  EnsureView();

  view_->SetAdjustment(x, y);
}


void Controller::Hide()
{
  if (!visible_)
    return;

  visible_ = false;

  if (show_timer_)
  {
    g_source_remove(show_timer_);
    show_timer_ = 0;
  }

  if (view_window_)
  {
    view_->SetupBackground(false);
    fade_in_animator_.Stop();
    fade_out_animator_.Start(1.0 - view_window_->GetOpacity());
  }
}

bool Controller::Visible()
{
  return visible_;
}

bool Controller::IsEnabled()
{
  return enabled_;
}

void Controller::SetEnabled(bool enabled)
{
  enabled_ = enabled;
}

//
// Introspection
//
std::string Controller::GetName() const
{
  return "ShortcutController";
}

void Controller::AddProperties(GVariantBuilder* builder)
{
  unity::variant::BuilderWrapper(builder)
  .add("timeout_duration", SUPER_TAP_DURATION + FADE_DURATION)
  .add("enabled", IsEnabled())
  .add("about_to_show", (Visible() && !fade_out_animator_.IsRunning() && view_window_ && view_window_->GetOpacity() != 1.0f))
  .add("about_to_hide", (Visible() && !fade_in_animator_.IsRunning() && view_window_ && view_window_->GetOpacity() != 1.0f))
  .add("visible", (Visible() && view_window_ && view_window_->GetOpacity() == 1.0f));
}

} // namespace shortcut
} // namespace unity
