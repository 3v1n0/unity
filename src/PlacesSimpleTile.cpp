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
 *
 */

#include "Nux/Nux.h"
#include "PlacesSimpleTile.h"

#include "IconTexture.h"
#include "Variant.h"

PlacesSimpleTile::PlacesSimpleTile (const char *icon_name, const char *label, int icon_size)
: PlacesTile (NUX_TRACKER_LOCATION),
  _label (NULL),
  _icon (NULL),
  _uri (NULL)
{
  _layout = new nux::VLayout ("", NUX_TRACKER_LOCATION);

  _label = g_strdup (label);
  _icon = g_strdup (icon_name);

  _icontex = new IconTexture (_icon, icon_size);
  _icontex->SinkReference ();

  _cairotext = new nux::StaticCairoText (_label);
  _cairotext->SinkReference ();
  _cairotext->SetFont ("Ubuntu normal 9");
  _cairotext->SetTextEllipsize (nux::StaticCairoText::NUX_ELLIPSIZE_START);
  _cairotext->SetTextAlignment (nux::StaticCairoText::NUX_ALIGN_CENTRE);
  _cairotext->SetMaximumWidth (140);

  _layout->AddLayout (new nux::SpaceLayout (0, 0, 12, 12));
  _layout->AddView (_icontex, 0, nux::eCenter, nux::eFull);
  _layout->AddSpace (6, 0);
  _layout->AddView (_cairotext, 0, nux::eCenter, nux::eFull);

  SetMinimumSize (160, 128);
  SetMaximumSize (160, 128);

  int textwidth, textheight;
  textwidth = textheight = 0;
  _cairotext->GetTextExtents (textwidth, textheight);

  AddChild (_icontex);

  SetCompositionLayout (_layout);
}


PlacesSimpleTile::~PlacesSimpleTile ()
{
  _icontex->UnReference ();
  _cairotext->UnReference ();

  g_free (_label);
  g_free (_icon);
  g_free (_uri);
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
  unity::variant::BuilderWrapper(builder).add(GetGeometry());
}

