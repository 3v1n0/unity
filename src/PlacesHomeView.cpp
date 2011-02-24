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
#include <gio/gdesktopappinfo.h>
#include "ubus-server.h"
#include "UBusMessages.h"

#include "PlacesHomeView.h"

#include "PlacesSimpleTile.h"

#define DESKTOP_DIR  "/desktop/gnome/applications"
#define BROWSER_DIR  DESKTOP_DIR"/browser"
#define CALENDAR_DIR DESKTOP_DIR"/calendar"
#define MEDIA_DIR    DESKTOP_DIR"/media"

enum
{
  TYPE_PLACE=0,
  TYPE_EXEC
};

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

  _client = gconf_client_get_default ();
  gconf_client_add_dir (_client,
                        BROWSER_DIR,
                        GCONF_CLIENT_PRELOAD_NONE,
                        NULL);
  gconf_client_add_dir (_client,
                        CALENDAR_DIR,
                        GCONF_CLIENT_PRELOAD_NONE,
                        NULL);
  gconf_client_add_dir (_client,
                        MEDIA_DIR,
                        GCONF_CLIENT_PRELOAD_NONE,
                        NULL);
  gconf_client_notify_add(_client,
                          BROWSER_DIR"/exec", 
                          (GConfClientNotifyFunc)OnKeyChanged,
                          this,
                          NULL, NULL);
  gconf_client_notify_add(_client,
                          CALENDAR_DIR"/exec",
                          (GConfClientNotifyFunc)OnKeyChanged,
                          this,
                          NULL, NULL);
  gconf_client_notify_add(_client,
                          MEDIA_DIR"/exec",
                          (GConfClientNotifyFunc)OnKeyChanged,
                          this,
                          NULL, NULL);

  Refresh ();
}

PlacesHomeView::~PlacesHomeView ()
{
  g_object_unref (_client);
  delete _bg_layer;
}

void
PlacesHomeView::OnKeyChanged (GConfClient    *client,
                              guint           cnxn_id,
                              GConfEntry     *entry,
                              PlacesHomeView *self)
{
  self->Refresh ();
}

void
PlacesHomeView::Refresh ()
{
  Shortcut   *shortcut = NULL;
  gchar      *markup = NULL;
  const char *temp = "<big><b>%s</b></big>";
  
  _layout->Clear ();

  // Find Media Apps
  markup = g_strdup_printf (temp, _("Find Media Apps"));
  shortcut = new Shortcut ("applications-multimedia",
                           markup,
                           96);
  shortcut->_id = TYPE_PLACE;
  shortcut->_place_id = g_strdup ("/com/canonical/unity/applicationsplace/applications");
  shortcut->_place_section = 4;
  _layout->AddView (shortcut, 1, nux::eLeft, nux::eFull);
  shortcut->sigClick.connect (sigc::mem_fun (this, &PlacesHomeView::OnShortcutClicked));
  g_free (markup);

  // Find Internet Apps
  markup = g_strdup_printf (temp, _("Find Internet Apps"));
  shortcut = new Shortcut ("applications-internet",
                           markup,
                           96);
  shortcut->_id = TYPE_PLACE;
  shortcut->_place_id = g_strdup ("/com/canonical/unity/applicationsplace/applications");
  shortcut->_place_section = 3;
  _layout->AddView (shortcut, 1, nux::eLeft, nux::eFull);
  shortcut->sigClick.connect (sigc::mem_fun (this, &PlacesHomeView::OnShortcutClicked));
  g_free (markup);

  // Find More Apps
  markup = g_strdup_printf (temp, _("Find More Apps"));
  shortcut = new Shortcut ("find",
                           markup,
                           96);
  shortcut->_id = TYPE_PLACE;
  shortcut->_place_id = g_strdup ("/com/canonical/unity/applicationsplace/applications");
  shortcut->_place_section = 0;
  _layout->AddView (shortcut, 1, nux::eLeft, nux::eFull);
  shortcut->sigClick.connect (sigc::mem_fun (this, &PlacesHomeView::OnShortcutClicked));
  g_free (markup);

  // Find Files
  markup = g_strdup_printf (temp, _("Find Files"));
  shortcut = new Shortcut ("folder-saved-search",
                           markup,
                           96);
  shortcut->_id = TYPE_PLACE;
  shortcut->_place_id = g_strdup ("/com/canonical/unity/filesplace/files");
  shortcut->_place_section = 0;
  _layout->AddView (shortcut, 1, nux::eLeft, nux::eFull);
  shortcut->sigClick.connect (sigc::mem_fun (this, &PlacesHomeView::OnShortcutClicked));
  g_free (markup);

  // Browser
  markup = gconf_client_get_string (_client, BROWSER_DIR"/exec", NULL);
  CreateShortcutFromExec (markup, _("Browse the Web"), "firefox");
  g_free (markup);

  // Photos
  // FIXME: Need to figure out the default
  CreateShortcutFromExec ("shotwell", _("View Photos"), "shotwell");

  // Email
  markup = gconf_client_get_string (_client, CALENDAR_DIR"/exec", NULL);
  CreateShortcutFromExec (markup, _("Check Email"), "evolution");
  g_free (markup);

  // Music
  markup = gconf_client_get_string (_client, MEDIA_DIR"/exec", NULL);
  CreateShortcutFromExec (markup, _("Listen to Music"), "banshee-1");
  g_free (markup);

  QueueDraw ();
  _layout->QueueDraw ();
  QueueRelayout ();
}

void
PlacesHomeView::CreateShortcutFromExec (const char *exec,
                                        const char *name,
                                        const char *icon_hint)
{
  Shortcut        *shortcut = NULL;
  gchar           *id;
  gchar           *markup;
  gchar           *icon;
  gchar           *real_exec;
  GDesktopAppInfo *info;

  markup = g_strdup_printf ("<big><b>%s</b></big>", name);

  // We're going to try and create a desktop id from a exec string. Now, this is hairy at the
  // best of times but the following is the closest best-guess without having to do D-Bus
  // roundtrips to BAMF.
  if (exec)
  {
    char *basename;

    if (exec[0] == '/')
      basename = g_path_get_basename (exec);
    else
      basename = g_strdup (exec);

    id = g_strdup_printf ("%s.desktop", basename);

    g_free (basename);
  }
  else
  {
    id = g_strdup_printf ("%s.desktop", icon_hint);
  }
  
  info = g_desktop_app_info_new (id);
  if (G_IS_DESKTOP_APP_INFO (info))
  {
    icon = g_icon_to_string (g_app_info_get_icon (G_APP_INFO (info)));
    real_exec = g_strdup (g_app_info_get_executable (G_APP_INFO (info)));

    g_object_unref (info);
  }
  else
  {
    icon = g_strdup (icon_hint);
    real_exec = g_strdup ("firefox");
  }

  shortcut = new Shortcut (icon, markup, 96);
  shortcut->_id = TYPE_EXEC;
  shortcut->_exec = real_exec; //shorcut will free
  _layout->AddView (shortcut, 1, nux::eLeft, nux::eFull);
  shortcut->sigClick.connect (sigc::mem_fun (this, &PlacesHomeView::OnShortcutClicked));

  g_free (id);
  g_free (markup);
  g_free (icon);
}

void
PlacesHomeView::OnShortcutClicked (PlacesTile *tile)
{
  Shortcut *shortcut = static_cast<Shortcut *> (tile);
  int id = shortcut->_id;

  if (id == TYPE_PLACE)
  {
    ubus_server_send_message (ubus_server_get_default (),
                              UBUS_PLACE_ENTRY_ACTIVATE_REQUEST,
                              g_variant_new ("(sus)",
                                             shortcut->_place_id,
                                             shortcut->_place_section,
                                             ""));
  }
  else if (id == TYPE_EXEC)
  {
    GError *error = NULL;

    if (!g_spawn_command_line_async (shortcut->_exec, &error))
    {
      g_warning ("%s: Unable to launch %s: %s",
                 G_STRFUNC,
                 shortcut->_exec,
                 error->message);
      g_error_free (error);
    }
    
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
