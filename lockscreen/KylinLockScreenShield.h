// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2015 Canonical Ltd
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
* Authored by: handsome_feng <jianfengli@ubuntukylin.com>
*/

#ifndef UNITY_KYLIN_LOCKSCREEN_SHIELD_H
#define UNITY_KYLIN_LOCKSCREEN_SHIELD_H

#include <UnityCore/ConnectionManager.h>
#include <UnityCore/GLibSource.h>
#include "LockScreenAbstractShield.h"

namespace unity
{
namespace lockscreen
{

class BackgroundSettings;
class UserAuthenticator;
class AbstractUserPromptView;
class CofView;

class KylinShield : public AbstractShield
{
public:
  KylinShield(session::Manager::Ptr const&,
         Accelerators::Ptr const&,
         nux::ObjectPtr<AbstractUserPromptView> const&,
         int monitor, bool is_primary);

  bool HasGrab() const override;

protected:
  nux::Area* FindKeyFocusArea(unsigned int, unsigned long, unsigned long) override;
  nux::Area* FindAreaUnderMouse(nux::Point const&, nux::NuxEventType) override;

private:
  void UpdateBackgroundTexture();
  void GrabScreen(bool cancel_on_failure);
  void ShowPrimaryView();
  void ShowSecondaryView();

  std::shared_ptr<BackgroundSettings> bg_settings_;
  std::unique_ptr<nux::AbstractPaintLayer> background_layer_;
  nux::ObjectPtr<nux::Layout> primary_layout_;
  nux::ObjectPtr<nux::Layout> prompt_layout_;
  nux::ObjectPtr<nux::Layout> cof_layout_;
  connection::Wrapper regrab_conn_;
  glib::Source::UniquePtr regrab_timeout_;
  CofView* cof_view_;
};

}
}

#endif
