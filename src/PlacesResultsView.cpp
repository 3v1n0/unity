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

PlacesResultsView::PlacesResultsView (NUX_FILE_LINE_DECL)
  :   ScrollView (NUX_FILE_LINE_PARAM)
{
  EnableVerticalScrollBar (true);
  EnableHorizontalScrollBar (false);

  _layout = new nux::VLayout (NUX_TRACKER_LOCATION);

  _layout->SetContentDistribution(nux::MAJOR_POSITION_TOP);
  _layout->SetHorizontalExternalMargin (14);
  _layout->SetVerticalInternalMargin (14);

  SetLayout (_layout);

}

PlacesResultsView::~PlacesResultsView ()
{
  _layout->Clear ();
}

void
PlacesResultsView::AddGroup (PlacesGroup *group)
{
  // Reset the vertical scroll position to the top.
  ResetScrollToUp ();
  _groups.push_back (group);
  _layout->AddView (group, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
}

void
PlacesResultsView::RemoveGroup (PlacesGroup *group)
{
  // Reset the vertical scroll position to the top.
  ResetScrollToUp ();
  _groups.remove (group);
  _layout->RemoveChildObject (group);
}

