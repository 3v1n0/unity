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
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

#ifndef UNITY_LOCKSCREEN_ABSTRACT_SHIELD_H
#define UNITY_LOCKSCREEN_ABSTRACT_SHIELD_H

#include <NuxCore/Property.h>
#include <UnityCore/SessionManager.h>
#include <UnityCore/Indicators.h>

#include "BackgroundSettings.h"
#include "unity-shared/MockableBaseWindow.h"
#include "unity-shared/UnitySettings.h"
#include "unity-shared/UScreen.h"
#include "unity-shared/WindowManager.h"
#include "LockScreenAccelerators.h"

namespace unity
{
namespace lockscreen
{
namespace
{
const unsigned MAX_GRAB_WAIT = 100;
}

class AbstractUserPromptView;

class AbstractShield : public MockableBaseWindow
{
public:
  AbstractShield(session::Manager::Ptr const& session,
                 indicator::Indicators::Ptr const& indicators,
                 Accelerators::Ptr const& accelerators,
                 nux::ObjectPtr<AbstractUserPromptView> const& prompt_view,
                 int monitor_num, bool is_primary)
    : MockableBaseWindow("Unity Lockscreen")
    , primary(is_primary)
    , monitor(monitor_num)
    , scale(1.0)
    , session_manager_(session)
    , indicators_(indicators)
    , accelerators_(accelerators)
    , prompt_view_(prompt_view)
    , bg_settings_(std::make_shared<BackgroundSettings>())
  {}

  nux::Property<bool> primary;
  nux::Property<int> monitor;
  nux::Property<double> scale;

  using MockableBaseWindow::RemoveLayout;
  virtual bool HasGrab() const
  {
    auto& wc = nux::GetWindowCompositor();
    return (wc.GetPointerGrabArea() == this && wc.GetKeyboardGrabArea() == this);
  }
  virtual bool IsIndicatorOpen() const { return false; }
  virtual void ActivatePanel() {}

  sigc::signal<void> grabbed;
  sigc::signal<void> grab_failed;
  sigc::signal<void, int, int> grab_motion;
  sigc::signal<void, unsigned long, unsigned long> grab_key;

protected:
  virtual bool AcceptKeyNavFocus() { return false; }
  virtual nux::Area* FindAreaUnderMouse(nux::Point const& mouse, nux::NuxEventType event_type)
  {
    nux::Area* area = BaseWindow::FindAreaUnderMouse(mouse, event_type);

    if (!area && primary)
      return this;

    return area;
  }
  virtual void GrabScreen(bool cancel_on_failure)
  {
    auto& wc = nux::GetWindowCompositor();

    if (wc.GrabPointerAdd(this) && wc.GrabKeyboardAdd(this))
    {
      regrab_conn_->disconnect();
      regrab_timeout_.reset();
      grabbed.emit();
    }
    else
    {
      auto const& retry_cb = sigc::bind(sigc::mem_fun(this, &AbstractShield::GrabScreen), false);
      regrab_conn_ = WindowManager::Default().screen_ungrabbed.connect(retry_cb);

      if (cancel_on_failure)
      {
        regrab_timeout_.reset(new glib::Timeout(MAX_GRAB_WAIT, [this] {
          grab_failed.emit();
          return false;
        }));
      }
    }
  }
  virtual void UpdateBackgroundTexture()
  {
    auto const& monitor_geo = UScreen::GetDefault()->GetMonitorGeometry(monitor);

    if (!background_layer_ || monitor_geo != background_layer_->GetGeometry())
    {
      auto background_texture = bg_settings_->GetBackgroundTexture(monitor);
      background_layer_.reset(new nux::TextureLayer(background_texture->GetDeviceTexture(), nux::TexCoordXForm(), nux::color::White, true));
      SetBackgroundLayer(background_layer_.get());
    }
  }
  virtual void UpdateScale()
  {
    scale = Settings::Instance().em(monitor)->DPIScale();
  }

  session::Manager::Ptr session_manager_;
  indicator::Indicators::Ptr indicators_;
  Accelerators::Ptr accelerators_;
  nux::ObjectPtr<AbstractUserPromptView> prompt_view_;
  std::shared_ptr<BackgroundSettings> bg_settings_;
  std::unique_ptr<nux::AbstractPaintLayer> background_layer_;
  connection::Wrapper regrab_conn_;
  glib::Source::UniquePtr regrab_timeout_;
};

} // lockscreen
} // unity

#endif // UNITY_LOCKSCREEN_ABSTRACT_SHIELD_H
