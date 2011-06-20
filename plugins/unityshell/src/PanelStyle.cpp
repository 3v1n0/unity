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
 * Authored by: Mirco Müller <mirco.mueller@canonical.com>
 *              Neil Jagdish Patel <neil.patel@canonical.com>
 */

#include "config.h"

#include <math.h>
#include <gtk/gtk.h>

#include <Nux/Nux.h>
#include <NuxGraphics/GraphicsEngine.h>
#include <NuxImage/CairoGraphics.h>

#include "PanelStyle.h"

namespace {

PanelStyle *_style = NULL;

nux::Color ColorFromGdkColor(GdkColor const& gc)
{
  return nux::Color(gc.red / static_cast<float>(0xffff),
                    gc.green / static_cast<float>(0xffff),
                    gc.blue / static_cast<float>(0xffff));
}

}

PanelStyle::PanelStyle ()
: _theme_name (NULL)
{
  _offscreen = gtk_offscreen_window_new ();
  gtk_widget_set_name (_offscreen, "UnityPanelWidget");
  gtk_widget_set_size_request (_offscreen, 100, 24);
  gtk_widget_show_all (_offscreen);

  _gtk_theme_changed_id = g_signal_connect (gtk_settings_get_default (), "notify::gtk-theme-name",
                                            G_CALLBACK (PanelStyle::OnStyleChanged), this);

  Refresh ();
}

PanelStyle::~PanelStyle ()
{
  if (_gtk_theme_changed_id)
    g_signal_handler_disconnect (gtk_settings_get_default (),
                                 _gtk_theme_changed_id);
  
  gtk_widget_destroy (_offscreen);

  if (_style == this)
    _style = NULL;

  g_free (_theme_name);
}

PanelStyle *
PanelStyle::GetDefault ()
{
  if (G_UNLIKELY (!_style))
    _style = new PanelStyle ();

  return _style;
}

void
PanelStyle::Refresh ()
{
  GtkStyle*  style    = NULL;

  if (_theme_name)
    g_free (_theme_name);

  _theme_name = NULL;
  g_object_get (gtk_settings_get_default (), "gtk-theme-name", &_theme_name, NULL);

  style = gtk_widget_get_style (_offscreen);

  _text = ColorFromGdkColor(style->text[0]);
  _text_shadow = ColorFromGdkColor(style->text[3]);
  _line = ColorFromGdkColor(style->dark[0]);
  _bg_top = ColorFromGdkColor(style->bg[1]);
  _bg_bottom = ColorFromGdkColor(style->bg[0]);

  changed.emit ();
}

nux::Color const& PanelStyle::GetTextColor() const
{
  return _text;
}

nux::Color const& PanelStyle::GetBackgroundTop() const
{
  return _bg_top;
}

nux::Color const& PanelStyle::GetBackgroundBottom() const
{
  return _bg_bottom;
}

nux::Color const& PanelStyle::GetTextShadow() const
{
  return _text_shadow;
}

nux::Color const& PanelStyle::GetLineColor() const
{
  return _line;
}

void
PanelStyle::OnStyleChanged (GObject*    gobject,
                            GParamSpec* pspec,
                            gpointer    data)
{
  PanelStyle* self = (PanelStyle*) data;

  self->Refresh ();
}

GdkPixbuf *
PanelStyle::GetBackground (int width, int height)
{
  gtk_widget_set_size_request (_offscreen, width, height);
  gdk_window_process_updates (gtk_widget_get_window (_offscreen), TRUE);
  
  return gtk_offscreen_window_get_pixbuf (GTK_OFFSCREEN_WINDOW (_offscreen));
}

nux::BaseTexture *
PanelStyle::GetWindowButton (WindowButtonType type, WindowState state)
{
#define ICON_LOCATION "/usr/share/themes/%s/metacity-1/%s%s.png"
  nux::BaseTexture * texture = NULL;
  const char *names[] = { "close", "minimize", "unmaximize" };
  const char *states[] = { "", "_focused_prelight", "_focused_pressed" };

  // I wish there was a magic bullet here, but not all themes actually set the panel to be
  // the same style as the window titlebars (e.g. Clearlooks) so we can just grab the 
  // metacity window buttons as that would look horrible
  if (IsAmbianceOrRadiance ())
  {
    char      *filename;
    GdkPixbuf *pixbuf;
    GError    *error = NULL;

    filename = g_strdup_printf (ICON_LOCATION, _theme_name, names[type], states[state]);

    pixbuf = gdk_pixbuf_new_from_file (filename, &error);
    if (error)
    {
      g_warning ("Unable to load window button %s: %s", filename, error->message);
      g_error_free (error);
      error = NULL;
    }
    else
      texture = nux::CreateTexture2DFromPixbuf (pixbuf, true);

    g_free (filename);
    g_object_unref (pixbuf);
  }
  else
  {
    texture = GetWindowButtonForTheme (type, state);
  }

  return texture;
}

bool
PanelStyle::IsAmbianceOrRadiance() {
  return g_strcmp0 (_theme_name, "Ambiance") == 0 || g_strcmp0 (_theme_name, "Radiance") == 0;
}

nux::BaseTexture *
PanelStyle::GetWindowButtonForTheme (WindowButtonType type, WindowState state)
{
  nux::BaseTexture *texture = NULL;
  int width = 18, height = 18;
  float w = width/3.0f;
  float h = height/3.0f;
  nux::CairoGraphics cairo_graphics(CAIRO_FORMAT_ARGB32, 22, 22);
  cairo_t *cr;
  nux::Color main = _text;

  if (type == WINDOW_BUTTON_CLOSE)
  {
    main = nux::Color (1.0f, 0.3f, 0.3f, 0.8f);
  }

  if (state == WINDOW_STATE_PRELIGHT)
    main = main * 1.2f;
  else if (state == WINDOW_STATE_PRESSED)
    main = main * 0.8f;

  cr  = cairo_graphics.GetContext();
  cairo_translate (cr, 0.5, 0.5);
  cairo_set_line_width (cr, 1.5f);

  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);

  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

  cairo_set_source_rgba (cr, main.red, main.green, main.blue, main.alpha);

  cairo_arc (cr, width/2.0f, height/2.0f, (width - 2)/2.0f, 0.0f, 360 * (M_PI/180));
  cairo_stroke (cr);

  if (type == WINDOW_BUTTON_CLOSE)
  {
    cairo_move_to (cr, w, h);
    cairo_line_to (cr, width - w, height - h);
    cairo_move_to (cr, width -w, h);
    cairo_line_to (cr, w, height - h);
  }
  else if (type == WINDOW_BUTTON_MINIMIZE)
  {
    cairo_move_to (cr, w, height/2.0f);
    cairo_line_to (cr, width - w, height/2.0f);
  }
  else
  {
    cairo_move_to (cr, w, h);
    cairo_line_to (cr, width - w, h);
    cairo_line_to (cr, width - w, height - h);
    cairo_line_to (cr, w, height -h);
    cairo_close_path (cr);
  }

  cairo_stroke (cr);

  cairo_destroy (cr);

  nux::NBitmapData* bitmap =  cairo_graphics.GetBitmap();
  texture = nux::GetGraphicsDisplay ()->GetGpuDevice ()->CreateSystemCapableTexture ();
  texture->Update(bitmap);
  delete bitmap;

  return texture;
}

GdkPixbuf *
PanelStyle::GetHomeButton ()
{
  GdkPixbuf *pixbuf = NULL;

  pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                     "start-here",
                                     24,
                                     (GtkIconLookupFlags)0,
                                     NULL); 
  if (pixbuf == NULL)
    pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                       "distributor-logo",
                                       24,
                                       (GtkIconLookupFlags)0,
                                       NULL);
  return pixbuf;
}
