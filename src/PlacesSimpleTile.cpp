/*
 * Copyright 2011 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 *              Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */

#include "PlacesSettings.h"
#include "ubus-server.h"
#include "UBusMessages.h"

#include "PlacesSimpleTile.h"

PlacesSimpleTile::PlacesSimpleTile (const char *icon_name, const char *label, int icon_size, bool defer_icon_loading)
: PlacesTile (NUX_TRACKER_LOCATION),
  _label (NULL),
  _icon (NULL),
  _uri (NULL)
{
  nux::VLayout *layout = new nux::VLayout ("", NUX_TRACKER_LOCATION);

  _label = g_strdup (label);
  _icon = g_strdup (icon_name);

  _icontex = new IconTexture (_icon, icon_size, defer_icon_loading);
  _icontex->SetMinMaxSize (PlacesSettings::GetDefault ()->GetDefaultTileWidth (), icon_size);
  _icontex->SinkReference ();
  AddChild (_icontex);

  _cairotext = new nux::StaticCairoText (_label);
  _cairotext->SinkReference ();
  _cairotext->SetFont ("Ubuntu normal 9");
  _cairotext->SetTextEllipsize (nux::StaticCairoText::NUX_ELLIPSIZE_START);
  _cairotext->SetTextAlignment (nux::StaticCairoText::NUX_ALIGN_CENTRE);
  _cairotext->SetMaximumWidth (140);

  layout->AddLayout (new nux::SpaceLayout (0, 0, 12, 12));
  layout->AddView (_icontex, 0, nux::eCenter, nux::eFull);
  layout->AddLayout (new nux::SpaceLayout (0, 0, 12, 12));
  layout->AddView (_cairotext, 0, nux::eCenter, nux::eFull);

  SetMinMaxSize (160, 128);

  SetLayout (layout);

  OnMouseClick.connect (sigc::mem_fun (this, &PlacesSimpleTile::Clicked));
  
  SetDndEnabled (true, false);
}


PlacesSimpleTile::~PlacesSimpleTile ()
{
  _icontex->UnReference ();
  _cairotext->UnReference ();

  g_free (_label);
  g_free (_icon);
  g_free (_uri);
}

nux::NBitmapData * 
PlacesSimpleTile::DndSourceGetDragImage ()
{
  return 0;
}

std::list<const char *> 
PlacesSimpleTile::DndSourceGetDragTypes ()
{
  std::list<const char*> result;
  result.push_back ("text/uri-list");
  return result;
}

const char *
PlacesSimpleTile::DndSourceGetDataForType (const char *type, int *size, int *format)
{
  *format = 8;

  if (_uri)
  {
    *size = strlen (_uri);
    return _uri;
  }
  else
  {
    *size = 0;
    return 0;
  }
}

void
PlacesSimpleTile::DndSourceDragFinished (nux::DndAction result)
{
  return;
}

nux::Geometry
PlacesSimpleTile::GetHighlightGeometry ()
{
  nux::Geometry base = GetGeometry ();
  int width = 0, height = 0;

  _icontex->GetTextureSize (&width, &height);
  
  _highlight_geometry.x = (base.width - width) / 2;
  _highlight_geometry.y = 12;
  _highlight_geometry.width = width;
  _highlight_geometry.height = height;

  return _highlight_geometry;
}

const char *
PlacesSimpleTile::GetLabel ()
{
  return _label;
}

const char *
PlacesSimpleTile::GetIcon ()
{
  return _icon;
}

const char *
PlacesSimpleTile::GetURI ()
{
  return _uri;
}

void
PlacesSimpleTile::SetURI (const char *uri)
{
  if (_uri)
    g_free (_uri);
  
  _uri = NULL;

  if (uri)
    _uri = g_strdup (uri);
}

const gchar*
PlacesSimpleTile::GetName ()
{
	return "PlacesTile";
}

const gchar *
PlacesSimpleTile::GetChildsName ()
{
  return "PlacesTileContents";
}

void
PlacesSimpleTile::AddProperties (GVariantBuilder *builder)
{
  nux::Geometry geo = GetGeometry ();

  g_variant_builder_add (builder, "{sv}", "x", g_variant_new_int32 (geo.x));
  g_variant_builder_add (builder, "{sv}", "y", g_variant_new_int32 (geo.y));
  g_variant_builder_add (builder, "{sv}", "width", g_variant_new_int32 (geo.width));
  g_variant_builder_add (builder, "{sv}", "height", g_variant_new_int32 (geo.height));
}

void
PlacesSimpleTile::Clicked (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  if (_uri)
  {
    ubus_server_send_message (ubus_server_get_default (),
                              UBUS_PLACE_TILE_ACTIVATE_REQUEST,
                              g_variant_new_string (_uri));
  }
}

void
PlacesSimpleTile::LoadIcon ()
{
  _icontex->LoadIcon ();

  QueueDraw ();
}
