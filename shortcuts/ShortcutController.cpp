/*
 * Copyright (C) 2011-2013 Canonical Ltd
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
 *              Marco Trevisan <marco.trevisan@canonical.com>
 */

#include "ShortcutController.h"
#include "ShortcutModel.h"

#include "unity-shared/AnimationUtils.h"
#include "unity-shared/UBusMessages.h"
#include "unity-shared/UScreen.h"
#include "unity-shared/WindowManager.h"

namespace unity
{
namespace shortcut
{
namespace
{
const unsigned int SUPER_TAP_DURATION = 650;
const unsigned int FADE_DURATION = 100;
}

Controller::Controller(BaseWindowRaiser::Ptr const& base_window_raiser,
                       AbstractModeller::Ptr const& modeller)
  : modeller_(modeller)
  , base_window_raiser_(base_window_raiser)
  , visible_(false)
  , enabled_(true)
  , fade_animator_(FADE_DURATION)
{
  ubus_manager_.RegisterInterest(UBUS_LAUNCHER_START_KEY_SWITCHER, [this] (GVariant*)
                                 { SetEnabled(false); });
  ubus_manager_.RegisterInterest(UBUS_LAUNCHER_END_KEY_SWITCHER, [this] (GVariant*)
                                 { SetEnabled(true); });
  ubus_manager_.RegisterInterest(UBUS_OVERLAY_SHOWN,
                                 sigc::hide(sigc::mem_fun(this, &Controller::Hide)));

  WindowManager::Default().average_color.changed.connect(sigc::mem_fun(this, &Controller::OnBackgroundUpdate));
  fade_animator_.updated.connect(sigc::mem_fun(this, &Controller::SetOpacity));
  modeller_->model_changed.connect(sigc::mem_fun(this, &Controller::OnModelUpdated));
}

Controller::~Controller()
{}

void Controller::OnBackgroundUpdate(nux::Color const& new_color)
{
  if (view_)
    view_->background_color = new_color;
}

void Controller::OnModelUpdated(Model::Ptr const& model)
{
  if (!view_)
    return;

  view_->SetModel(model);

  if (Visible())
  {
    model->Fill();
    auto uscreen = UScreen::GetDefault();
    int monitor = uscreen->GetMonitorAtPosition(view_window_->GetX(), view_window_->GetX());
    auto const& offset = GetOffsetPerMonitor(monitor);

    if (offset.x < 0 || offset.y < 0)
    {
      Hide();
      return;
    }

    view_window_->SetXY(offset.x, offset.y);
  }
}

bool Controller::Show()
{
  if (enabled_ && modeller_->GetCurrentModel())
  {
    show_timer_.reset(new glib::Timeout(SUPER_TAP_DURATION, sigc::mem_fun(this, &Controller::OnShowTimer)));
    visible_ = true;

    return true;
  }

  return false;
}

bool Controller::OnShowTimer()
{
  if (!enabled_ || !modeller_->GetCurrentModel())
    return false;

  modeller_->GetCurrentModel()->Fill();
  EnsureView();

  int monitor = UScreen::GetDefault()->GetMonitorWithMouse();
  auto const& offset = GetOffsetPerMonitor(monitor);

  if (offset.x < 0 || offset.y < 0)
    return false;

  base_window_raiser_->Raise(view_window_);
  view_window_->SetXY(offset.x, offset.y);

  if (visible_)
  {
    view_->live_background = true;
    animation::StartOrReverse(fade_animator_, animation::Direction::FORWARD);
  }

  return false;
}

nux::Point Controller::GetOffsetPerMonitor(int monitor)
{
  EnsureView();

  view_window_->ComputeContentSize();
  auto const& view_geo = view_->GetAbsoluteGeometry();
  auto const& monitor_geo = UScreen::GetDefault()->GetMonitorGeometry(monitor);

  if (adjustment_.x + view_geo.width > monitor_geo.width ||
      adjustment_.y + view_geo.height > monitor_geo.height)
  {
    // Invalid position
    return nux::Point(std::numeric_limits<int>::min(), std::numeric_limits<int>::min());
  }

  nux::Point offset(adjustment_.x + monitor_geo.x, adjustment_.y + monitor_geo.y);
  offset.x += (monitor_geo.width - view_geo.width - adjustment_.x) / 2;
  offset.y += (monitor_geo.height - view_geo.height - adjustment_.y) / 2;

  return offset;
}

void Controller::ConstructView()
{
  view_ = View::Ptr(new View());
  AddChild(view_.GetPointer());
  view_->SetModel(modeller_->GetCurrentModel());
  view_->background_color = WindowManager::Default().average_color();

  if (!view_window_)
  {
    main_layout_ = new nux::HLayout(NUX_TRACKER_LOCATION);
    main_layout_->SetVerticalExternalMargin(0);
    main_layout_->SetHorizontalExternalMargin(0);

    view_window_ = new nux::BaseWindow("ShortcutHint");
    view_window_->SetLayout(main_layout_);
    view_window_->SetBackgroundColor(nux::color::Transparent);
    view_window_->SetWindowSizeMatchLayout(true);
  }

  main_layout_->AddView(view_.GetPointer());

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
  adjustment_.x = x;
  adjustment_.y = y;
}

void Controller::Hide()
{
  if (!visible_)
    return;

  visible_ = false;
  show_timer_.reset();

  if (view_window_ && view_window_->GetOpacity() > 0.0f)
  {
    view_->live_background = false;
    animation::StartOrReverse(fade_animator_, animation::Direction::BACKWARD);
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
  auto direction = animation::GetDirection(fade_animator_);

  unity::variant::BuilderWrapper(builder)
  .add("timeout_duration", SUPER_TAP_DURATION + FADE_DURATION)
  .add("enabled", IsEnabled())
  .add("about_to_show", (Visible() && animating && direction == animation::Direction::FORWARD))
  .add("about_to_hide", (Visible() && animating && direction == animation::Direction::BACKWARD))
  .add("visible", (Visible() && view_window_ && view_window_->GetOpacity() == 1.0f));
}

} // namespace shortcut
} // namespace unity
