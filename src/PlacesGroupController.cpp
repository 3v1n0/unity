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

PlacesGroupController::PlacesGroupController (PlaceEntry *entry, PlaceEntryGroup& group)
: _entry (entry),
  _group (NULL),
  _check_tiles_id (0)
{
  PlacesStyle *style = PlacesStyle::GetDefault ();

  _id = group.GetId ();

  _group = new PlacesGroup (NUX_TRACKER_LOCATION);
  _group->SetName(group.GetName ());
  _group->SetIcon (group.GetIcon ());

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

  _group->expanded.connect (sigc::mem_fun (this, &PlacesGroupController::CheckTiles));
  style->columns_changed.connect (sigc::mem_fun (this, &PlacesGroupController::CheckTiles));
}

PlacesGroupController::~PlacesGroupController ()
{
  if (_check_tiles_id)
    g_source_remove (_check_tiles_id);
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
PlacesGroupController::AddTile (PlaceEntry       *ignore, 
                                PlaceEntryGroup&  group,
                                PlaceEntryResult& result)
{
  PlacesStyle *style = PlacesStyle::GetDefault ();

  gchar            *result_name;
  const gchar      *result_icon;
  PlacesSimpleTile *tile;

  result_name = g_markup_escape_text (result.GetName (), -1);
  result_icon = result.GetIcon ();

  tile = new PlacesSimpleTile (result_icon,
                               result_name,
                               style->GetTileIconSize ());
  tile->SetURI (result.GetURI ());
  tile->QueueRelayout ();

  _id_to_tile[result.GetId ()] = tile;
  
  _group->GetChildLayout ()->AddView (tile);
  _group->Relayout ();
  _group->SetVisible (true);

  g_free (result_name);
}

void
PlacesGroupController::AddResult (PlaceEntryGroup& group, PlaceEntryResult& result)
{
  PlacesStyle *style = PlacesStyle::GetDefault ();

  _queue.push_back (result.GetId ());
  
  if (_group->GetExpanded ()
      || _id_to_tile.size () != (guint)style->GetDefaultNColumns ())
  {
    AddTile (_entry, group, result);
  }

  _group->SetCounts (style->GetDefaultNColumns (), _queue.size ());
}

void
PlacesGroupController::RemoveResult (PlaceEntryGroup& group, PlaceEntryResult& result)
{
  std::vector<const void *>::iterator it;
  PlacesTile  *tile = NULL;

  it = std::find (_queue.begin (), _queue.end (), result.GetId ());

  if (it != _queue.end ())
  {
    _queue.erase (it);
  }

  if ((tile = _id_to_tile[result.GetId ()]))
  {
    _group->GetChildLayout ()->RemoveChildObject (tile);
    _group->Relayout ();
  }
  
  _id_to_tile.erase (result.GetId ());

  if (!_check_tiles_id)
    _check_tiles_id = g_timeout_add (0, (GSourceFunc)CheckTilesTimeout, this);

  _group->SetVisible (_queue.size ());
  _group->SetCounts (PlacesStyle::GetDefault ()->GetDefaultNColumns (),
                     _queue.size ());
}

void
PlacesGroupController::Clear ()
{

}

void
PlacesGroupController::CheckTiles ()
{
  PlacesStyle *style = PlacesStyle::GetDefault ();
  guint          n_to_show;

  if (_group->GetExpanded ())
    n_to_show = _queue.size ();
  else
    n_to_show = style->GetDefaultNColumns ();

  if (_id_to_tile.size () == n_to_show)
  {
    // Hoorah
  }
  else if (_id_to_tile.size () < n_to_show)
  {
    std::vector<const void *>::iterator it = _queue.begin ();

    if (_queue.size () >= n_to_show)
    {
      it += _id_to_tile.size ();

      while (_id_to_tile.size () < n_to_show && it != _queue.end ())
      {
        _entry->GetResult ((*it), sigc::mem_fun (this, &PlacesGroupController::AddTile));
        it++;
      }
    }
  }
  else // Remove some
  {
    std::vector<const void *>::iterator it, eit = _queue.end ();

    for (it = _queue.begin () + n_to_show; it != eit; ++it)
    {
      PlacesTile *tile = _id_to_tile[*it];

      if (tile)
        _group->GetChildLayout ()->RemoveChildObject (tile);

      _id_to_tile.erase (*it);
    }

  }
  _group->Relayout ();
}

gboolean
PlacesGroupController::CheckTilesTimeout (PlacesGroupController *self)
{
  self->_check_tiles_id = 0;
  self->CheckTiles ();

  return FALSE;
}

bool
PlacesGroupController::ActivateFirst ()
{
  std::vector<const void *>::iterator it = _queue.begin ();

  if (it != _queue.end ())
  {
    PlacesTile *tile = _id_to_tile[*it];

    if (tile)
    {
      nux::Geometry geo = tile->GetGeometry ();
      tile->OnMouseClick.emit (geo.x, geo.y, 0, 0);
      return true;
    }
  }

  return false;
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
