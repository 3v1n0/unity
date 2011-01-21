// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010 Canonical Ltd
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

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>
#include <Nux/HLayout.h>
#include <Nux/Layout.h>
#include <Nux/WindowCompositor.h>

#include <NuxImage/CairoGraphics.h>
#include <NuxImage/ImageSurface.h>

#include <NuxGraphics/GLThread.h>
#include <NuxGraphics/RenderingPipe.h>

#include <glib.h>
#include <glib/gi18n-lib.h>

#include "ubus-server.h"
#include "UBusMessages.h"

#include "PlacesHomeView.h"

#include "PlacesSimpleTile.h"

typedef struct
{
  gchar *name;
  gchar *icon;
  gchar *exec;

} TileInfo;

static TileInfo tile_infos[] = {
  {
    (gchar*)_("Find Media Apps"),
    (gchar*)"applications-multimedia",
    (gchar*)"xdg-open /usr/share/applications"
  },
  {
    (gchar*)_("Find Internet Apps"),
    (gchar*)"applications-internet",
    (gchar*)"xdg-open /usr/share/applications"
  },
  {
    (gchar*)_("Find More Apps"),
    (gchar*)"find",
    (gchar*)"xdg-open /usr/share/applications"
  },
  {
    (gchar*)_("Find Files"),
    (gchar*)"folder-saved-search",
    (gchar*)"xdg-open ~"
  },
  {
    (gchar*)_("Browse the Web"),
    (gchar*)"firefox",
    (gchar*)"firefox"
  },
  {
    (gchar*)_("View Photos"),
    (gchar*)"shotwell",
    (gchar*)"shotwell"
  },
  {
    (gchar*)_("Check Email"),
    (gchar*)"evolution",
    (gchar*)"evolution"
  },
  {
    (gchar*)_("Listen to Music"),
    (gchar*)"media-player-banshee",
    (gchar*)"banshee"
  }
};

PlacesHomeView::PlacesHomeView (NUX_FILE_LINE_DECL)
:   View (NUX_FILE_LINE_PARAM)
{
  _bg_layer = new nux::ColorLayer (nux::Color (0xff999893), true);

  _layout = new nux::GridHLayout (NUX_TRACKER_LOCATION);
  SetCompositionLayout (_layout);
 
  for (guint i = 0; i < G_N_ELEMENTS (tile_infos); i++)
  {
    gchar *markup = g_strdup_printf ("<big><b>%s</b></big>", tile_infos[i].name);

    PlacesSimpleTile *tile = new PlacesSimpleTile (tile_infos[i].icon,
                                                   markup,
                                                   96);
    _layout->AddView (tile, 1, nux::eLeft, nux::eFull);

    tile->sigClick.connect (sigc::mem_fun (this, &PlacesHomeView::OnTileClicked));

    g_free (markup);
  }

  _layout->ForceChildrenSize (true);
  _layout->SetChildrenSize (186, 186);
  _layout->EnablePartialVisibility (false);

  _layout->SetVerticalExternalMargin (48);
  _layout->SetHorizontalExternalMargin (48);
  _layout->SetVerticalInternalMargin (32);
  _layout->SetHorizontalInternalMargin (32);
}

PlacesHomeView::~PlacesHomeView ()
{
  delete _bg_layer;
}

void
PlacesHomeView::OnTileClicked (PlacesTile *_tile)
{
  PlacesSimpleTile *tile = static_cast<PlacesSimpleTile *> (_tile);
  
  for (guint i = 0; i < G_N_ELEMENTS (tile_infos); i++)
  {
    if (g_strcmp0 (tile->GetIcon (), tile_infos[i].icon) == 0)
    {
      GError *error = NULL;

      g_spawn_command_line_async (tile_infos[i].exec, &error);
      if (error)
      {
        g_warning ("Unable to launch tile: %s", error->message);
        g_error_free (error);
      }
    }
  }

  ubus_server_send_message (ubus_server_get_default (),
                            UBUS_PLACE_VIEW_CLOSE_REQUEST,
                            NULL);
}

const gchar* PlacesHomeView::GetName ()
{
	return "PlacesHomeView";
}

const gchar *
PlacesHomeView::GetChildsName ()
{
  return "";
}

void PlacesHomeView::AddProperties (GVariantBuilder *builder)
{
  nux::Geometry geo = GetGeometry ();

  g_variant_builder_add (builder, "{sv}", "x", g_variant_new_int32 (geo.x));
  g_variant_builder_add (builder, "{sv}", "y", g_variant_new_int32 (geo.y));
  g_variant_builder_add (builder, "{sv}", "width", g_variant_new_int32 (geo.width));
  g_variant_builder_add (builder, "{sv}", "height", g_variant_new_int32 (geo.height));
}

long
PlacesHomeView::ProcessEvent (nux::IEvent &ievent, long TraverseInfo, long ProcessEventInfo)
{
  nux::Geometry geo = GetGeometry ();

  long ret = TraverseInfo;
  ret = _layout->ProcessEvent (ievent, ret, ProcessEventInfo);

  return ret;
}

void
PlacesHomeView::Draw (nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::GetPainter().PaintBackground (GfxContext, GetGeometry() );

  _bg_layer->SetGeometry (GetGeometry ());
  nux::GetPainter().RenderSinglePaintLayer (GfxContext, GetGeometry(), _bg_layer);
}

void
PlacesHomeView::DrawContent (nux::GraphicsEngine &GfxContext, bool force_draw)
{
  _layout->ProcessDraw (GfxContext, force_draw);
}

void
PlacesHomeView::PreLayoutManagement ()
{
  nux::View::PreLayoutManagement ();
}

long
PlacesHomeView::PostLayoutManagement (long LayoutResult)
{
  // I'm imagining this is a good as time as any to update the background
  UpdateBackground ();

  return nux::View::PostLayoutManagement (LayoutResult);
}

void
PlacesHomeView::DrawRoundedRectangle (cairo_t* cr,
                                      double   aspect,
                                      double   x,
                                      double   y,
                                      double   cornerRadius,
                                      double   width,
                                      double   height)
{
    double radius = cornerRadius / aspect;

    // top-left, right of the corner
    cairo_move_to (cr, x + radius, y);

    // top-right, left of the corner
    cairo_line_to (cr, x + width - radius, y);

    // top-right, below the corner
    cairo_arc (cr,
               x + width - radius,
               y + radius,
               radius,
               -90.0f * G_PI / 180.0f,
               0.0f * G_PI / 180.0f);

    // bottom-right, above the corner
    cairo_line_to (cr, x + width, y + height - radius);

    // bottom-right, left of the corner
    cairo_arc (cr,
               x + width - radius,
               y + height - radius,
               radius,
               0.0f * G_PI / 180.0f,
               90.0f * G_PI / 180.0f);

    // bottom-left, right of the corner
    cairo_line_to (cr, x + radius, y + height);

    // bottom-left, above the corner
    cairo_arc (cr,
               x + radius,
               y + height - radius,
               radius,
               90.0f * G_PI / 180.0f,
               180.0f * G_PI / 180.0f);

    // top-left, right of the corner
    cairo_arc (cr,
               x + radius,
               y + radius,
               radius,
               180.0f * G_PI / 180.0f,
               270.0f * G_PI / 180.0f);
}

void
PlacesHomeView::UpdateBackground ()
{
#define PADDING 24
#define RADIUS  6
  int x, y, width, height;
  nux::Geometry geo = GetGeometry ();

  if (geo.width == _last_width && geo.height == _last_height)
    return;

  _last_width = geo.width;
  _last_height = geo.height;

  x = y = PADDING;
  width = _last_width - (2*PADDING);
  height = _last_height - (2*PADDING);

  nux::CairoGraphics cairo_graphics(CAIRO_FORMAT_ARGB32, _last_width, _last_height);
  cairo_t *cr = cairo_graphics.GetContext();

  cairo_translate (cr, 0.5, 0.5);
  cairo_set_line_width (cr, 1.0);

  cairo_set_source_rgba (cr, 0.5f, 0.5f, 0.5f, 0.2f);

  DrawRoundedRectangle (cr, 1.0f, x, y, RADIUS, width, height);

  cairo_close_path (cr);

  cairo_fill_preserve (cr);

  cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 0.2f);
  cairo_stroke (cr);

  cairo_destroy (cr);

  nux::NBitmapData* bitmap =  cairo_graphics.GetBitmap();

  nux::BaseTexture* texture2D = nux::GetThreadGLDeviceFactory ()->CreateSystemCapableTexture ();
  texture2D->Update(bitmap);
  delete bitmap;

  nux::TexCoordXForm texxform;
  texxform.SetTexCoordType (nux::TexCoordXForm::OFFSET_COORD);
  texxform.SetWrap (nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
  if (_bg_layer)
    delete _bg_layer;

  nux::ROPConfig rop; 
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
  
  _bg_layer = new nux::TextureLayer (texture2D->GetDeviceTexture(),
                                     texxform,
                                     nux::Color::White,
                                     false,
                                     rop);

  texture2D->UnReference ();

  NeedRedraw ();
}
