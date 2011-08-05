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
#include <glib/gi18n-lib.h>

#include "PlacesStyle.h"
#include "PlacesSimpleTile.h"
#include "PlacesHorizontalTile.h"

static const guint kPadding = 4;

PlacesGroupController::PlacesGroupController(PlaceEntry* entry, PlaceEntryGroup& group)
  : _type(RENDERER_TYPE_DEFAULT),
    _entry(entry),
    _group(NULL),
    _check_tiles_id(0)
{
  PlacesStyle* style = PlacesStyle::GetDefault();

  _id = group.GetId();

  _group = new PlacesGroup(NUX_TRACKER_LOCATION);
  _group->SetName(group.GetName());
  _group->SetIcon(group.GetIcon());

  if (g_strcmp0(group.GetRenderer(), "UnityHorizontalTileRenderer") == 0)
    _type = RENDERER_TYPE_HORI_TILE;

  nux::GridHLayout* layout = new nux::GridHLayout(NUX_TRACKER_LOCATION);
  layout->SetReconfigureParentLayoutOnGeometryChange(true);
  layout->ForceChildrenSize(true);
  layout->EnablePartialVisibility(false);
  layout->SetVerticalExternalMargin(kPadding);
  layout->SetHorizontalExternalMargin(kPadding);
  layout->SetVerticalInternalMargin(kPadding);
  layout->SetHorizontalInternalMargin(kPadding);
  layout->SetHeightMatchContent(true);

  if (_type == RENDERER_TYPE_HORI_TILE)
    layout->SetChildrenSize(style->GetTileWidth() * 2, style->GetTileIconSize() + 24);    //padding
  else
    layout->SetChildrenSize(style->GetTileWidth(), style->GetTileHeight());

  _group->SetChildLayout(layout);
  _group->SetVisible(false);
  _group->SetExpanded(false);

  _group->expanded.connect(sigc::mem_fun(this, &PlacesGroupController::CheckTiles));
  style->columns_changed.connect(sigc::mem_fun(this, &PlacesGroupController::CheckTiles));

  if (_type == RENDERER_TYPE_HORI_TILE)
    _more_tile = new PlacesHorizontalTile("gtk-add",
                                          _("Load more results..."),
                                          "",
                                          style->GetTileIconSize(),
                                          false,
                                          "more-tile");
  else
    _more_tile = new PlacesSimpleTile("gtk-add",
                                      _("Load more results..."),
                                      style->GetTileIconSize(),
                                      false,
                                      "more-tile");
  _more_tile->Reference();
  _more_tile->sigClick.connect(sigc::mem_fun(this, &PlacesGroupController::MoreTileClicked));
}

PlacesGroupController::~PlacesGroupController()
{
  if (_check_tiles_id)
    g_source_remove(_check_tiles_id);
  if (_more_tile)
    _more_tile->UnReference();
}

const void*
PlacesGroupController::GetId()
{
  return _id;
}

PlacesGroup*
PlacesGroupController::GetGroup()
{
  return _group;
}

void
PlacesGroupController::AddTile(PlaceEntry*       ignore,
                               PlaceEntryGroup&  group,
                               PlaceEntryResult& result)
{
  PlacesStyle* style = PlacesStyle::GetDefault();

  gchar*            result_name;
  const gchar*      result_icon;
  gchar*            result_comment;
  PlacesTile*       tile;

  result_name = g_markup_escape_text(result.GetName(), -1);
  result_comment = g_markup_escape_text(result.GetComment(), -1);
  result_icon = result.GetIcon();

  if (_type == RENDERER_TYPE_HORI_TILE)
  {
    tile = new PlacesHorizontalTile(result_icon,
                                    result_name,
                                    result_comment,
                                    style->GetTileIconSize(),
                                    false,
                                    result.GetId());
    static_cast<PlacesHorizontalTile*>(tile)->SetURI(result.GetURI());
  }
  else
  {
    tile = new PlacesSimpleTile(result_icon,
                                result_name,
                                style->GetTileIconSize(),
                                false,
                                result.GetId());
    static_cast<PlacesSimpleTile*>(tile)->SetURI(result.GetURI());
  }
  tile->QueueRelayout();
  tile->sigClick.connect(sigc::mem_fun(this, &PlacesGroupController::TileClicked));

  _id_to_tile[result.GetId()] = tile;

  _group->GetChildLayout()->AddView(tile);
  _group->Relayout();
  _group->SetVisible(true);

  g_free(result_name);
  g_free(result_comment);
}

void
PlacesGroupController::TileClicked(PlacesTile* tile)
{
  if (_entry)
  {
    _entry->ActivateResult(tile->GetId());
  }
}

void
PlacesGroupController::MoreTileClicked(PlacesTile* tile)
{
  if (!_check_tiles_id)
    _check_tiles_id = g_timeout_add(150, (GSourceFunc)CheckTilesTimeout, this);
}

void
PlacesGroupController::AddResult(PlaceEntryGroup& group, PlaceEntryResult& result)
{
  PlacesStyle* style = PlacesStyle::GetDefault();

  _queue.push_back(result.GetId());

  if (_group->GetExpanded()
      || _id_to_tile.size() != (guint)style->GetDefaultNColumns())
  {
    AddTile(_entry, group, result);
  }

  _group->SetCounts(style->GetDefaultNColumns(), _queue.size());
}

void
PlacesGroupController::RemoveResult(PlaceEntryGroup& group, PlaceEntryResult& result)
{
  std::vector<const void*>::iterator it;
  PlacesTile*  tile = NULL;

  it = std::find(_queue.begin(), _queue.end(), result.GetId());

  if (it != _queue.end())
  {
    _queue.erase(it);
  }

  if ((tile = _id_to_tile[result.GetId()]))
  {
    _group->GetChildLayout()->RemoveChildObject(tile);
    _group->Relayout();
  }

  _id_to_tile.erase(result.GetId());

  if (!_check_tiles_id)
    _check_tiles_id = g_timeout_add(0, (GSourceFunc)CheckTilesTimeout, this);

  _group->SetVisible(_queue.size());
  _group->SetCounts(PlacesStyle::GetDefault()->GetDefaultNColumns(),
                    _queue.size());
}

void
PlacesGroupController::Clear()
{
  if (_group->GetChildLayout())
    _group->GetChildLayout()->Clear();
}

void
PlacesGroupController::CheckTiles()
{
  PlacesStyle* style = PlacesStyle::GetDefault();
  guint          n_to_show;

  if (_group->GetExpanded())
    n_to_show = _queue.size();
  else
    n_to_show = style->GetDefaultNColumns();

  if (_more_tile->GetParentObject())
    _group->GetChildLayout()->RemoveChildObject(_more_tile);

  if (_id_to_tile.size() == n_to_show)
  {
    // Hoorah
  }
  else if (_id_to_tile.size() < n_to_show)
  {
    std::vector<const void*>::iterator it = _queue.begin();
    int max_rows_to_add = style->GetDefaultNColumns() * 15;
    int max_rows_per_click = max_rows_to_add * 3;
    int n_tiles = _id_to_tile.size();

    if (_queue.size() >= n_to_show)
    {
      it += _id_to_tile.size();

      while (_id_to_tile.size() < n_to_show && it != _queue.end())
      {
        _entry->GetResult((*it), sigc::mem_fun(this, &PlacesGroupController::AddTile));
        it++;
        n_tiles++;

        if (n_tiles % max_rows_to_add == 0)
        {
          // What we do over here is add a "Load more results..." button which will then kick off
          // another fill cycle. Generally the user will never need to see this, but in the case
          // of large results sets, this gives us a chance of not blocking the entire WM for
          // 20 secs as we load in 2000+ results.
          if (n_tiles % max_rows_per_click == 0)
          {
            _group->GetChildLayout()->AddView(_more_tile);
          }
          else
          {
            // The idea here is that we increase the timeouts of adding tiles in relation to
            // how big the view is getting. In theory it shouldn't be needed but in reality with
            // Nux having to do so many calculations it is needed. Ideally we'd be rendering
            // the entire results view as a scene with re-usable tiles but that would have been
            // too difficult to get right with a11y, so instead we need to just deal with this
            // the best way we can.
            if (!_check_tiles_id)
              _check_tiles_id = g_timeout_add(350 * n_tiles / max_rows_to_add,
                                              (GSourceFunc)CheckTilesTimeout,
                                              this);
          }
          return;
        }
      }
    }
  }
  else // Remove some
  {
    std::vector<const void*>::iterator it, eit = _queue.end();

    for (it = _queue.begin() + n_to_show; it != eit; ++it)
    {
      PlacesTile* tile = _id_to_tile[*it];

      if (tile)
        _group->GetChildLayout()->RemoveChildObject(tile);

      _id_to_tile.erase(*it);
    }

  }
  _group->Relayout();
}

gboolean
PlacesGroupController::CheckTilesTimeout(PlacesGroupController* self)
{
  self->_check_tiles_id = 0;
  self->CheckTiles();

  return FALSE;
}

bool
PlacesGroupController::ActivateFirst()
{
  std::vector<const void*>::iterator it = _queue.begin();

  if (it != _queue.end())
  {
    PlacesTile* tile = _id_to_tile[*it];

    if (tile)
    {
      tile->sigClick.emit(tile);
      return true;
    }
  }

  return false;
}

//
// Introspectable
//
const gchar*
PlacesGroupController::GetName()
{
  return "PlacesGroupController";
}

void
PlacesGroupController::AddProperties(GVariantBuilder* builder)
{
}

int
PlacesGroupController::GetTotalResults()
{
  return _queue.size();
}
