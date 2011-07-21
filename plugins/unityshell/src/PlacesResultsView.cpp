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

#include "Nux/Nux.h"
#include "Nux/Layout.h"
#include "Nux/WindowCompositor.h"
#include "Nux/VScrollBar.h"
#include "Nux/HScrollBar.h"
#include "Nux/Panel.h"
#include "Nux/GridHLayout.h"
#include "Nux/GridVLayout.h"
#include "PlacesGroup.h"
#include "PlacesResultsView.h"
#include "PlacesVScrollBar.h"

PlacesResultsView::PlacesResultsView(NUX_FILE_LINE_DECL)
  :   ScrollView(NUX_FILE_LINE_PARAM)
{
  _layout = new nux::VLayout(NUX_TRACKER_LOCATION);

  _layout->SetContentDistribution(nux::MAJOR_POSITION_TOP);
  _layout->SetHorizontalExternalMargin(0);
  _layout->SetVerticalInternalMargin(0);
  setBorder(0);
  setTopBorder(0);

  SetLayout(_layout);

  PlacesVScrollBar* vscrollbar = new PlacesVScrollBar();
  SetVScrollBar(vscrollbar);
  EnableVerticalScrollBar(true);
  EnableHorizontalScrollBar(false);
  _idle_id = 0;
}

PlacesResultsView::~PlacesResultsView()
{
  _layout->Clear();
  if (_idle_id != 0)
  {
    g_source_remove(_idle_id);
    _idle_id = 0;
  }
}

void
PlacesResultsView::AddGroup(PlacesGroup* group)
{
  // Reset the vertical scroll position to the top.
  ResetScrollToUp();
  _groups.push_back(group);
  _layout->AddView(group, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);

  if (_idle_id == 0)
    _idle_id = g_idle_add((GSourceFunc)OnIdleFocus, this);
}

gboolean
PlacesResultsView::OnIdleFocus(PlacesResultsView* self)
{
  self->_idle_id = 0;

  if (self->GetFocused())
  {
    self->SetFocused(false);  // unset focus on all children
    self->SetFocused(true);  // set focus on first child
  }

  return FALSE;
}
void
PlacesResultsView::RemoveGroup(PlacesGroup* group)
{
  // Reset the vertical scroll position to the top.
  ResetScrollToUp();
  _groups.remove(group);
  _layout->RemoveChildObject(group);
  if (_idle_id == 0)
    _idle_id = g_idle_add((GSourceFunc)OnIdleFocus, this);
}

void
PlacesResultsView::Clear()
{
  _layout->Clear();
}

