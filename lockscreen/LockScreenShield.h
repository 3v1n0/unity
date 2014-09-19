// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2013 Canonical Ltd
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

#ifndef UNITY_LOCKSCREEN_SHIELD_H
#define UNITY_LOCKSCREEN_SHIELD_H

#include <UnityCore/ConnectionManager.h>
#include <UnityCore/GLibSource.h>
#include "LockScreenAbstractShield.h"

namespace unity
{
namespace lockscreen
{

class BackgroundSettings;
class UserAuthenticator;
class UserPromptView;
class Panel;
class CofView;

class Shield : public AbstractShield
{
public:
  Shield(session::Manager::Ptr const&,
         indicator::Indicators::Ptr const&,
         Accelerators::Ptr const&,
         nux::ObjectPtr<UserPromptView> const&,
         int monitor, bool is_primary);

  bool HasGrab() const override;
  bool IsIndicatorOpen() const override;
  void ActivatePanel() override;

protected:
  bool AcceptKeyNavFocus() override;
  nux::Area* FindKeyFocusArea(unsigned int, unsigned long, unsigned long) override;
  nux::Area* FindAreaUnderMouse(nux::Point const&, nux::NuxEventType) override;

private:
  void UpdateBackgroundTexture();
  void GrabScreen(bool cancel_on_failure);
  void ShowPrimaryView();
  void ShowSecondaryView();
  void UpdateScale();
  Panel* CreatePanel();

  std::shared_ptr<BackgroundSettings> bg_settings_;
  std::unique_ptr<nux::AbstractPaintLayer> background_layer_;
  nux::ObjectPtr<nux::Layout> primary_layout_;
  nux::ObjectPtr<nux::Layout> prompt_layout_;
  nux::ObjectPtr<nux::Layout> cof_layout_;
  connection::Wrapper panel_active_conn_;
  connection::Wrapper regrab_conn_;
  glib::Source::UniquePtr regrab_timeout_;
  Panel* panel_view_;
  CofView* cof_view_;
};

}
}

#endif
