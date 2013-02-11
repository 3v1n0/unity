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

#include "unity-shared/UnityWindowView.h"

namespace unity
{
namespace session
{

class View : public ui::UnityWindowView
{
  NUX_DECLARE_OBJECT_TYPE(View, ui::UnityWindowView);
public:
  typedef nux::ObjectPtr<View> Ptr;

  View(Manager::Ptr const& manager);

protected:
  void DrawOverlay(nux::GraphicsEngine& GfxContext, bool force_draw, nux::Geometry clip);
  nux::Geometry GetBackgroundGeometry();

  // Introspectable methods
  std::string GetName() const;

private:
  Manager::Ptr manager_;
};

} // namespace session

} // namespace unity

#endif // UNITYSHELL_SESSION_VIEW_H

