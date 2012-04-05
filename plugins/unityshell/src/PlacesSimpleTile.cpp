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

#include "PlacesSimpleTile.h"

#include <NuxImage/GdkGraphics.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <UnityCore/Variant.h>

#include "DashStyle.h"
#include "ubus-server.h"
#include "UBusMessages.h"


namespace unity
{
NUX_IMPLEMENT_OBJECT_TYPE(PlacesSimpleTile);

PlacesSimpleTile::PlacesSimpleTile(std::string const& icon_name,
                                   std::string const& label,
                                   int         icon_size,
                                   bool defer_icon_loading,
                                   const void* id)
  : PlacesTile(NUX_TRACKER_LOCATION, id),
    _label(label),
    _icon(icon_name),
    _idealiconsize(icon_size)
{
  dash::Style& style = dash::Style::Instance();
  nux::VLayout* layout = new nux::VLayout("", NUX_TRACKER_LOCATION);

  _icontex = new IconTexture(_icon, icon_size, defer_icon_loading);
  _icontex->SetMinMaxSize(style.GetTileWidth(), icon_size);
  AddChild(_icontex);

  _cairotext = new nux::StaticCairoText("");
  _cairotext->SetMaximumWidth(style.GetTileWidth());
  _cairotext->SetTextEllipsize(nux::StaticCairoText::NUX_ELLIPSIZE_START);
  _cairotext->SetTextAlignment(nux::StaticCairoText::NUX_ALIGN_CENTRE);
  _cairotext->SetText(_label);

  layout->AddLayout(new nux::SpaceLayout(0, 0, 12, 12));
  layout->AddView(_icontex, 0, nux::eCenter, nux::eFull);
  layout->AddLayout(new nux::SpaceLayout(0, 0, 12, 12));
  layout->AddView(_cairotext, 0, nux::eCenter, nux::eFull);

  SetMinMaxSize(style.GetTileWidth(), style.GetTileHeight());

  SetLayout(layout);

  SetDndEnabled(true, false);
}

bool
PlacesSimpleTile::DndSourceDragBegin()
{
  Reference();
  ubus_server_send_message(ubus_server_get_default(),
                           UBUS_PLACE_VIEW_CLOSE_REQUEST,
                           NULL);
  return true;
}

nux::NBitmapData*
PlacesSimpleTile::DndSourceGetDragImage()
{
  nux::NBitmapData* result = 0;
  GdkPixbuf* pbuf;
  GtkIconTheme* theme;
  GtkIconInfo* info;
  GError* error = NULL;
  GIcon* icon;

  std::string icon_name = _icon;
  int size = 64;

  if (icon_name.empty())
    icon_name = "application-default-icon";

  theme = gtk_icon_theme_get_default();
  icon = g_icon_new_for_string(icon_name.c_str(), NULL);

  if (G_IS_ICON(icon))
  {
    info = gtk_icon_theme_lookup_by_gicon(theme, icon, size, (GtkIconLookupFlags)0);
    g_object_unref(icon);
  }
  else
  {
    info = gtk_icon_theme_lookup_icon(theme,
                                      icon_name.c_str(),
                                      size,
                                      (GtkIconLookupFlags) 0);
  }

  if (!info)
  {
    info = gtk_icon_theme_lookup_icon(theme,
                                      "application-default-icon",
                                      size,
                                      (GtkIconLookupFlags) 0);
  }

  if (gtk_icon_info_get_filename(info) == NULL)
  {
    gtk_icon_info_free(info);
    info = gtk_icon_theme_lookup_icon(theme,
                                      "application-default-icon",
                                      size,
                                      (GtkIconLookupFlags) 0);
  }

  pbuf = gtk_icon_info_load_icon(info, &error);
  gtk_icon_info_free(info);

  if (GDK_IS_PIXBUF(pbuf))
  {
    nux::GdkGraphics graphics(pbuf);
    result = graphics.GetBitmap();
  }

  return result;
}

std::list<const char*> PlacesSimpleTile::DndSourceGetDragTypes()
{
  std::list<const char*> result;
  result.push_back("text/uri-list");
  return result;
}

const char* PlacesSimpleTile::DndSourceGetDataForType(const char* type, int* size, int* format)
{
  *format = 8;

  if (!_uri.empty())
  {
    *size = _uri.size();
    return _uri.c_str();
  }
  else
  {
    *size = 0;
    return 0;
  }
}

void PlacesSimpleTile::DndSourceDragFinished(nux::DndAction result)
{
  UnReference();
}

nux::Geometry PlacesSimpleTile::GetHighlightGeometry()
{
  nux::Geometry base = GetGeometry();
  int width = 0, height = 0;

  _icontex->GetTextureSize(&width, &height);

  _highlight_geometry.width = MAX(width, _idealiconsize);
  _highlight_geometry.height = MAX(height, _idealiconsize);
  _highlight_geometry.x = (base.width - _highlight_geometry.width) / 2;
  _highlight_geometry.y = 12;

  return _highlight_geometry;
}

std::string PlacesSimpleTile::GetLabel() const
{
  return _label;
}

std::string PlacesSimpleTile::GetIcon() const
{
  return _icon;
}

std::string PlacesSimpleTile::GetURI() const
{
  return _uri;
}

void PlacesSimpleTile::SetURI(std::string const& uri)
{
  _uri = uri;
}

std::string PlacesSimpleTile::GetName() const
{
  return "PlacesTile";
}

void PlacesSimpleTile::AddProperties(GVariantBuilder* builder)
{
  unity::variant::BuilderWrapper(builder).add(GetGeometry());
}

void PlacesSimpleTile::LoadIcon()
{
  _icontex->LoadIcon();

  QueueDraw();
}

} // namespace unity
