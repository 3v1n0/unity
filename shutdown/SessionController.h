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
* Authored by: Marco Trevisan (Treviño) <marco@ubuntu.com>
*/

#ifndef UNITYSHELL_SESSION_CONTROLLER_H
#define UNITYSHELL_SESSION_CONTROLLER_H

#include <memory>

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>
#include <Nux/HLayout.h>
#include <NuxCore/Color.h>
#include <NuxCore/Animation.h>
#include <UnityCore/SessionManager.h>

#include "SessionView.h"

namespace unity
{
namespace session
{

class Controller : public debug::Introspectable, public sigc::trackable
{
public:
  typedef std::shared_ptr<Controller> Ptr;

  Controller(session::Manager::Ptr const& manager);
  virtual ~Controller() = default;

  void Show(View::Mode mode);
  void Hide();

  bool Visible() const;

protected:
  // Introspectable
  std::string GetName() const;
  void AddProperties(debug::IntrospectionData&);

private:
  friend class TestSessionController;

  void Show(View::Mode mode, bool inhibitors);
  void CancelAndHide();
  void ConstructView();
  void EnsureView();
  void CloseWindow();
  void OnBackgroundUpdate(nux::Color const&);
  nux::Point GetOffsetPerMonitor(int monitor);

  View::Ptr view_;
  nux::ObjectPtr<nux::BaseWindow> view_window_;
  nux::Point adjustment_;
  session::Manager::Ptr manager_;

  nux::animation::AnimateValue<double> fade_animator_;
};

}
}

#endif
