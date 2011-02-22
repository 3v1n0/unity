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

/*
typedef struct
{
  gchar *name;
  gchar *icon;
  gchar *exec;

} TileInfo;

static TileInfo tile_infos[] = {
  {
    (gchar*)_("Find Media Apps"v),
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

*/

class Shortcut : public PlacesSimpleTile
{
public:
  Shortcut (const char *icon, const char *name, int size)
  : PlacesSimpleTile (icon, name, size),
    _id (0),
    _place_id (NULL),
    _place_section (0),
    _exec (NULL)
  {
  }

  ~Shortcut ()
  {
    g_free (_place_id);
    g_free (_exec);
  }

  int      _id;
  gchar   *_place_id;
  guint32  _place_section;
  char    *_exec;
};

PlacesHomeView::PlacesHomeView (NUX_FILE_LINE_DECL)
:   View (NUX_FILE_LINE_PARAM)
{
  _bg_layer = new nux::ColorLayer (nux::Color (0xff999893), true);

  _layout = new nux::GridHLayout (NUX_TRACKER_LOCATION);
  SetCompositionLayout (_layout);
 
  _layout->ForceChildrenSize (true);
  _layout->SetChildrenSize (186, 186);
  _layout->EnablePartialVisibility (false);

  _layout->SetVerticalExternalMargin (48);
  _layout->SetHorizontalExternalMargin (48);
  _layout->SetVerticalInternalMargin (32);
  _layout->SetHorizontalInternalMargin (32);

  Refresh ();
}

PlacesHomeView::~PlacesHomeView ()
{
  delete _bg_layer;
}

void
PlacesHomeView::Refresh ()
{
  Shortcut   *shortcut = NULL;
  gchar      *markup = NULL;
  const char *temp = "<big><b>%s</b></big>";
  
  if (_layout->GetChildren ().size () == 0)
    _layout->Clear ();

  markup = g_strdup_printf (temp, _("Find Media Apps"));
  shortcut = new Shortcut ("applications-multimedia",
                           markup,
                           96);
  shortcut->_id = 0;
  shortcut->_place_id = g_strdup ("/com/canonical/unity/applicationsplace/applications");
  shortcut->_place_section = 4;
  _layout->AddView (shortcut, 1, nux::eLeft, nux::eFull);
  shortcut->sigClick.connect (sigc::mem_fun (this, &PlacesHomeView::OnShortcutClicked));
  g_free (markup);
}

void
PlacesHomeView::OnShortcutClicked (PlacesTile *tile)
{
  Shortcut *shortcut = static_cast<Shortcut *> (tile);
  int id = shortcut->_id;

  if (id < 2 || id == 3)
  {
    ubus_server_send_message (ubus_server_get_default (),
                              UBUS_PLACE_ENTRY_ACTIVATE_REQUEST,
                              g_variant_new ("(sus)",
                                             shortcut->_place_id,
                                             shortcut->_place_section,
                                             ""));
  }

  if (shortcut->_id > 3)
  {
     ubus_server_send_message (ubus_server_get_default (),
                               UBUS_PLACE_VIEW_CLOSE_REQUEST,
                               NULL);
  }
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
  UpdateBackground ();

  _bg_layer->SetGeometry (GetGeometry ());
  nux::GetPainter().PushDrawLayer (GfxContext, GetGeometry(), _bg_layer);
  nux::GetPainter().PopBackground ();
}

void
PlacesHomeView::DrawContent (nux::GraphicsEngine &GfxContext, bool force_draw)
{
  nux::GetPainter().PushLayer (GfxContext, GetGeometry(), _bg_layer);
  _layout->ProcessDraw (GfxContext, force_draw);
  nux::GetPainter().PopBackground ();
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
