// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2014-2015 Canonical Ltd
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

#ifndef UNITY_LOCKSCREEN_BASE_SHIELD_H
#define UNITY_LOCKSCREEN_BASE_SHIELD_H

#include <NuxCore/Property.h>
#include "UnityCore/ConnectionManager.h"
#include "UnityCore/SessionManager.h"
#include "UnityCore/GLibSource.h"
#include "unity-shared/MockableBaseWindow.h"

#include "LockScreenAccelerators.h"

namespace unity
{
namespace lockscreen
{
class BackgroundSettings;
class AbstractUserPromptView;
class CofView;

class BaseShield : public MockableBaseWindow
{
public:
  BaseShield(session::Manager::Ptr const&, Accelerators::Ptr const&,
             nux::ObjectPtr<AbstractUserPromptView> const&,
             int monitor_num, bool is_primary);

  nux::Property<bool> primary;
  nux::Property<int> monitor;
  nux::Property<double> scale;

  bool HasGrab() const;
  virtual bool IsIndicatorOpen() const { return false; }
  virtual void ActivatePanel() {}
  using MockableBaseWindow::RemoveLayout;

  sigc::signal<void> grabbed;
  sigc::signal<void> grab_failed;

protected:
  virtual bool AcceptKeyNavFocus() { return false; }
  virtual void ShowPrimaryView() = 0;
  virtual void ShowSecondaryView();

  nux::Area* FindKeyFocusArea(unsigned int, unsigned long, unsigned long) override;
  nux::Area* FindAreaUnderMouse(nux::Point const& mouse, nux::NuxEventType event_type) override;

  void GrabScreen(bool cancel_on_failure);
  void UpdateBackgroundTexture();
  void UpdateScale();

  session::Manager::Ptr session_manager_;
  Accelerators::Ptr accelerators_;
  nux::ObjectPtr<AbstractUserPromptView> prompt_view_;
  std::shared_ptr<BackgroundSettings> bg_settings_;
  std::unique_ptr<nux::AbstractPaintLayer> background_layer_;
  nux::ObjectPtr<nux::Layout> primary_layout_;
  nux::ObjectPtr<nux::Layout> prompt_layout_;
  nux::ObjectPtr<nux::Layout> cof_layout_;
  CofView* cof_view_;
  connection::Wrapper regrab_conn_;
  glib::Source::UniquePtr regrab_timeout_;
};

} // lockscreen
} // unity

#endif // UNITY_LOCKSCREEN_BASE_SHIELD_H
