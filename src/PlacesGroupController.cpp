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
* Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
*/

#include "PlacesGroupController.h"

#include <Nux/GridHLayout.h>

#include "PlacesStyle.h"
#include "PlacesSimpleTile.h"

static const guint kPadding = 4;

PlacesGroupController::PlacesGroupController (PlaceEntryGroup& group)
: _group (NULL),
  _load_icons_id (0)
{
  PlacesStyle *style = PlacesStyle::GetDefault ();

  _id = group.GetId ();

  _group = new PlacesGroup (NUX_TRACKER_LOCATION);
  _group->SetName(group.GetName ());
  _group->SetIcon (group.GetIcon ());
  _group->SetChildUnexpandHeight (style->GetTileHeight () + kPadding * 3);

  nux::GridHLayout *layout = new nux::GridHLayout (NUX_TRACKER_LOCATION);
  layout->ForceChildrenSize (true);
  layout->SetChildrenSize (style->GetTileWidth (), style->GetTileHeight ());
  layout->EnablePartialVisibility (false);

  layout->SetVerticalExternalMargin (kPadding);
  layout->SetHorizontalExternalMargin (kPadding);
  layout->SetVerticalInternalMargin (kPadding);
  layout->SetHorizontalInternalMargin (kPadding);
  layout->SetHeightMatchContent (true);

  _group->SetChildLayout (layout);
  _group->SetVisible (false);
  _group->SetExpanded (false);

  _group->expanded.connect (sigc::mem_fun (this, &PlacesGroupController::LoadIcons));
  style->columns_changed.connect (sigc::mem_fun (this, &PlacesGroupController::LoadIcons));
}

PlacesGroupController::~PlacesGroupController ()
{
  if (_load_icons_id)
    g_source_remove (_load_icons_id);
}

const void *
PlacesGroupController::GetId ()
{
  return _id;
}

PlacesGroup *
PlacesGroupController::GetGroup ()
{
  return _group;
}

void
PlacesGroupController::AddResult (PlaceEntryGroup& group, PlaceEntryResult& result)
{
  gchar            *result_name;
  const gchar      *result_icon;
  PlacesSimpleTile *tile;
  PlacesStyle      *style = PlacesStyle::GetDefault ();
  bool              defer_load;

  if (_group->GetExpanded ())
    defer_load = false;
  else
    defer_load = ((int) _id_to_tile.size () + 1) > (int) style->GetDefaultNColumns ();
  
  result_name = g_markup_escape_text (result.GetName (), -1);
  result_icon = result.GetIcon ();

  tile = new PlacesSimpleTile (result_icon,
                               result_name,
                               style->GetTileIconSize (),
                               defer_load);
  tile->SetURI (result.GetURI ());
  tile->QueueRelayout ();

  _id_to_tile[result.GetId ()] = tile;
  
  _group->GetChildLayout ()->AddView (tile);
  _group->Relayout ();
  _group->SetVisible (true);
  _group->SetCounts (6, _id_to_tile.size ());

  g_free (result_name);
}

void
PlacesGroupController::RemoveResult (PlaceEntryGroup& group, PlaceEntryResult& result)
{
  PlacesTile  *tile;

  tile = _id_to_tile[result.GetId ()];
  if (!tile)
    return;
  
  _id_to_tile.erase (result.GetId ());

  _group->GetChildLayout ()->RemoveChildObject (tile);
  _group->Relayout ();
  _group->SetCounts (6, _id_to_tile.size ());
  _group->SetVisible (_id_to_tile.size ());

  if (!_load_icons_id)
    _load_icons_id = g_timeout_add (0, (GSourceFunc)LoadIconsTimeout, this);
}

void
PlacesGroupController::Clear ()
{

}

void
PlacesGroupController::LoadIcons ()
{
  PlacesStyle *style = PlacesStyle::GetDefault ();
  int          n_to_show, i = 0;
//  std::map<const void *, PlacesTile *>::iterator it, eit = _id_to_tile.end ();
  nux::GridHLayout *layout = static_cast<nux::GridHLayout *> (_group->GetChildLayout ());
  std::list<nux::Area *>::iterator it, eit = layout->GetChildren ().end ();

  if (_group->GetExpanded ())
    n_to_show = _id_to_tile.size ();
  else
    n_to_show = style->GetDefaultNColumns ();

  for (it = layout->GetChildren ().begin (); it != eit; ++it)
  {
    //static_cast<PlacesSimpleTile *> (it->second)->LoadIcon ();
    static_cast<PlacesSimpleTile *> (*it)->LoadIcon ();

    i++;
    if (i > n_to_show)
      break;
  }
}

gboolean
PlacesGroupController::LoadIconsTimeout (PlacesGroupController *self)
{
  self->_load_icons_id = 0;
  self->LoadIcons ();

  return FALSE;
}

//
// Introspectable
//
const gchar *
PlacesGroupController::GetName ()
{
  return "PlacesGroupController";
}

void
PlacesGroupController::AddProperties (GVariantBuilder *builder)
{
}
