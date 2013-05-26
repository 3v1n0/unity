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

#ifndef UNITYSHELL_SESSION_VIEW_H
#define UNITYSHELL_SESSION_VIEW_H

#include <Nux/Nux.h>
#include <Nux/View.h>
#include <Nux/HLayout.h>

#include "UnityCore/SessionManager.h"
#include "unity-shared/UnityWindowView.h"

namespace unity
{
class StaticCairoText;
namespace session
{
class Button;

class View : public ui::UnityWindowView
{
  NUX_DECLARE_OBJECT_TYPE(View, ui::UnityWindowView);
public:
  typedef nux::ObjectPtr<View> Ptr;

  enum class Mode
  {
    FULL,
    SHUTDOWN,
    LOGOUT
  };

  View(nux::BaseWindow *parent, Manager::Ptr const& manager);

  nux::Property<Mode> mode;
  nux::Property<bool> have_inhibitors;
  nux::ROProperty<nux::InputArea*> key_focus_area;

  sigc::signal<void> request_hide;

protected:
  void PreLayoutManagement();
  void DrawOverlay(nux::GraphicsEngine&, bool force, nux::Geometry const&);
  nux::Geometry GetBackgroundGeometry();

  nux::Area* FindKeyFocusArea(unsigned etype, unsigned long key, unsigned long mod);
  nux::Area* KeyNavIteration(nux::KeyNavDirection);

  // Introspectable methods
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

private:
  friend class TestSessionView;

  void UpdateText();
  void Populate();
  void AddButton(Button*);

  Manager::Ptr manager_;
  StaticCairoText* title_;
  StaticCairoText* subtitle_;
  nux::HLayout* buttons_layout_;
  nux::InputArea* key_focus_area_;
};

} // namespace session

} // namespace unity

#endif // UNITYSHELL_SESSION_VIEW_H

