// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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

#include "ResultRenderer.h"

#include <gtk/gtk.h>
#include <unity-protocol.h>
#include <NuxGraphics/GdkGraphics.h>

namespace unity
{
namespace dash
{

namespace
{
#define DEFAULT_GICON ". GThemedIcon text-x-preview"

GdkPixbuf* _icon_hint_get_drag_pixbuf(std::string icon_hint, int size)
{
  GdkPixbuf *pbuf;
  GtkIconTheme *theme;
  GtkIconInfo *info;
  GError *error = NULL;
  GIcon *icon;
  if (icon_hint.empty())
    icon_hint = DEFAULT_GICON;
  if (g_str_has_prefix(icon_hint.c_str(), "/"))
  {
    pbuf = gdk_pixbuf_new_from_file_at_scale (icon_hint.c_str(),
                                              size, size, TRUE, &error);
    if (error != NULL || !pbuf || !GDK_IS_PIXBUF (pbuf))
    {
      icon_hint = "application-default-icon";
      g_error_free (error);
      error = NULL;
    }
    else
      return pbuf;
  }
  theme = gtk_icon_theme_get_default();
  icon = g_icon_new_for_string(icon_hint.c_str(), NULL);

  if (G_IS_ICON(icon))
  {
     if (UNITY_PROTOCOL_IS_ANNOTATED_ICON(icon))
     {
        UnityProtocolAnnotatedIcon *anno;
        anno = UNITY_PROTOCOL_ANNOTATED_ICON(icon);

        GIcon *base_icon = unity_protocol_annotated_icon_get_icon(anno);
        info = gtk_icon_theme_lookup_by_gicon(theme, base_icon, size, (GtkIconLookupFlags)0);
     }
     else
     {
       info = gtk_icon_theme_lookup_by_gicon(theme, icon, size, (GtkIconLookupFlags)0);
     }
     g_object_unref(icon);
  }
  else
  {
     info = gtk_icon_theme_lookup_icon(theme,
                                        icon_hint.c_str(),
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
      g_object_unref(G_OBJECT(info));
      info = gtk_icon_theme_lookup_icon(theme,
                                        "application-default-icon",
                                        size,
                                        (GtkIconLookupFlags) 0);
  }

  pbuf = gtk_icon_info_load_icon(info, &error);

  if (error != NULL)
  {
    g_error_free (error);
    pbuf = NULL;
  }

  g_object_unref(G_OBJECT(info));
  return pbuf;
}

}

NUX_IMPLEMENT_OBJECT_TYPE(ResultRenderer);

ResultRenderer::ResultRenderer(NUX_FILE_LINE_DECL)
  : InitiallyUnownedObject(NUX_FILE_LINE_PARAM)
  , width(50)
  , height(50)
{}

void ResultRenderer::Render(nux::GraphicsEngine& GfxContext,
                            Result& /*row*/,
                            ResultRendererState /*state*/,
                            nux::Geometry const& geometry, int /*x_offset*/, int /*y_offset*/,
                            nux::Color const& color,
                            float saturate)
{
  nux::GetPainter().PushDrawSliceScaledTextureLayer(GfxContext, geometry, nux::eBUTTON_NORMAL, nux::color::White, nux::eAllCorners);
}

void ResultRenderer::Preload(Result& row)
{
  // pre-load the given row
}

void ResultRenderer::Unload(Result& row)
{
  // unload any resources
}

nux::NBitmapData* ResultRenderer::GetDndImage(Result const& row) const
{
  nux::GdkGraphics graphics(_icon_hint_get_drag_pixbuf(row.icon_hint, 64));
  return graphics.GetBitmap();
}

}
}

