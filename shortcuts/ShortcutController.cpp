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
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 */

#include "ShortcutController.h"

#include "unity-shared/UBusMessages.h"

namespace na = nux::animation;

namespace unity
{
namespace shortcut
{
namespace
{
const unsigned int SUPER_TAP_DURATION = 650;
const unsigned int FADE_DURATION = 100;
}

Controller::Controller(std::list<AbstractHint::Ptr> const& hints,
                       BaseWindowRaiser::Ptr const& base_window_raiser)
  : model_(std::make_shared<Model>(hints))
  , base_window_raiser_(base_window_raiser)
  , visible_(false)
  , enabled_(true)
  , bg_color_(0.0, 0.0, 0.0, 0.5)
  , fade_animator_(FADE_DURATION)
{
  model_->Fill();

  ubus_manager_.RegisterInterest(UBUS_BACKGROUND_COLOR_CHANGED,
                                 sigc::mem_fun(this, &Controller::OnBackgroundUpdate));

  ubus_manager_.RegisterInterest(UBUS_LAUNCHER_START_KEY_SWITCHER, [this] (GVariant*) {
                                   enabled_ = false;
                                 });

  ubus_manager_.RegisterInterest(UBUS_LAUNCHER_END_KEY_SWITCHER, [this] (GVariant*) {
                                   enabled_ = true;
                                 });

  ubus_manager_.RegisterInterest(UBUS_OVERLAY_SHOWN, [this] (GVariant*) {
                                   Hide();
                                 });

  ubus_manager_.SendMessage(UBUS_BACKGROUND_REQUEST_COLOUR_EMIT);

  fade_animator_.updated.connect([this] (double opacity) {
    SetOpacity(opacity);
  });
}

Controller::~Controller()
{}

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
  if (enabled_)
  {
    EnsureView();

    show_timer_.reset(new glib::Timeout(SUPER_TAP_DURATION, sigc::mem_fun(this, &Controller::OnShowTimer)));
    model_->Fill();
    visible_ = true;

    return true;
  }

  return false;
}

bool Controller::OnShowTimer()
{
  if (!enabled_)
    return false;

  base_window_raiser_->Raise(view_window_);

  nux::Geometry geo;
  if (!view_->GetBaseGeometry(geo))
    return false;

  view_window_->SetGeometry(geo);

  if (visible_)
  {
    view_->SetupBackground(true);

    if (fade_animator_.CurrentState() == na::Animation::State::Running)
    {
      if (fade_animator_.GetFinishValue() == 0.0f)
        fade_animator_.Reverse();
    }
    else
    {
      fade_animator_.SetStartValue(0.0f).SetFinishValue(1.0f).Start();
    }
  }

  return false;
}

void Controller::ConstructView()
{
  view_ = View::Ptr(new View());
  AddChild(view_.GetPointer());
  view_->SetModel(model_);
  view_->background_color = bg_color_;

  if (!view_window_)
  {
    main_layout_ = new nux::HLayout(NUX_TRACKER_LOCATION);
    main_layout_->SetVerticalExternalMargin(0);
    main_layout_->SetHorizontalExternalMargin(0);

    view_window_ = new nux::BaseWindow("ShortcutHint");
    view_window_->SetLayout(main_layout_);
    view_window_->SetBackgroundColor(nux::Color(0x00000000));
  }

  main_layout_->AddView(view_.GetPointer());

  view_->SetupBackground(false);
  view_window_->ShowWindow(true);
  SetOpacity(0.0);
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
  show_timer_.reset();

  if (view_window_ && view_window_->GetOpacity() > 0.0f)
  {
    view_->SetupBackground(false);

    if (fade_animator_.CurrentState() == na::Animation::State::Running)
    {
      if (fade_animator_.GetFinishValue() == 1.0f)
        fade_animator_.Reverse();
    }
    else
    {
      fade_animator_.SetStartValue(1.0f).SetFinishValue(0.0f).Start();
    }
  }
}

bool Controller::Visible() const
{
  return visible_;
}

bool Controller::IsEnabled() const
{
  return enabled_;
}

void Controller::SetEnabled(bool enabled)
{
  enabled_ = enabled;
}

void Controller::SetOpacity(double value)
{
  view_window_->SetOpacity(value);
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
  bool animating = (fade_animator_.CurrentState() == na::Animation::State::Running);

  unity::variant::BuilderWrapper(builder)
  .add("timeout_duration", SUPER_TAP_DURATION + FADE_DURATION)
  .add("enabled", IsEnabled())
  .add("about_to_show", (Visible() && animating && fade_animator_.GetFinishValue() == 1.0f))
  .add("about_to_hide", (Visible() && animating && fade_animator_.GetFinishValue() == 0.0f))
  .add("visible", (Visible() && view_window_ && view_window_->GetOpacity() == 1.0f));
}

} // namespace shortcut
} // namespace unity
