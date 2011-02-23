// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 */

#ifndef PLACES_RESULTS_VIEW_H
#define PLACES_RESULTS_VIEW_H

#include <sigc++/sigc++.h>

#include <Nux/Nux.h>
#include <Nux/VLayout.h>
#include <Nux/ScrollView.h>
#include <Nux/BaseWindow.h>
#include <NuxCore/Math/MathInc.h>

#include "PlacesGroup.h"
#include "Introspectable.h"

#include <sigc++/trackable.h>
#include <sigc++/signal.h>
#include <sigc++/functors/ptr_fun.h>
#include <sigc++/functors/mem_fun.h>

class PlacesResultsView : public nux::ScrollView
{
public:
  PlacesResultsView (NUX_FILE_LINE_PROTO);
  ~PlacesResultsView ();

  sigc::signal<void, char *> sigClick; // returns ID of item clicked
  sigc::signal<void> sigToggled;
  sigc::signal<void, bool> sigStateChanged;

  void AddGroup (PlacesGroup *group);
  void RemoveGroup (PlacesGroup *group);

protected:
  virtual void ScrollLeft (float stepx, int mousedx);
  virtual void ScrollRight (float stepx, int mousedx);
  virtual void ScrollUp (float stepy, int mousedy);
  virtual void ScrollDown (float stepy, int mousedy);

private:
  nux::Layout *_layout;
  std::list<PlacesGroup *> _groups;

protected:

  virtual long ProcessEvent (nux::IEvent &ievent, long TraverseInfo, long ProcessEventInfo);
  virtual void Draw (nux::GraphicsEngine &GfxContext, bool force_draw);
  virtual void DrawContent (nux::GraphicsEngine &GfxContext, bool force_draw);
  virtual void PostDraw (nux::GraphicsEngine &GfxContext, bool force_draw);

  virtual void PreLayoutManagement ();
  virtual long PostLayoutManagement (long LayoutResult);
  virtual void PositionChildLayout (float offsetX, float offsetY);

};

#endif // PLACE_RESULTS_VIEW_H
