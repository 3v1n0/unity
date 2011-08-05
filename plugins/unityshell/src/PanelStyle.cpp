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

namespace
{

PanelStyle* _style = NULL;

nux::Color ColorFromGdkRGBA(GdkRGBA const& color)
{
  return nux::Color(color.red,
                    color.green,
                    color.blue,
                    color.alpha);
}

}

PanelStyle::PanelStyle()
  : _theme_name(NULL)
{
  _style_context = gtk_style_context_new();

  GtkWidgetPath* widget_path = gtk_widget_path_new();
  gtk_widget_path_iter_set_name(widget_path, -1 , "UnityPanelWidget");
  gtk_widget_path_append_type(widget_path, GTK_TYPE_WINDOW);

  gtk_style_context_set_path(_style_context, widget_path);
  gtk_style_context_add_class(_style_context, "gnome-panel-menu-bar");
  gtk_style_context_add_class(_style_context, "unity-panel");

  gtk_widget_path_free(widget_path);

  _gtk_theme_changed_id = g_signal_connect(gtk_settings_get_default(), "notify::gtk-theme-name",
                                           G_CALLBACK(PanelStyle::OnStyleChanged), this);

  Refresh();
}

PanelStyle::~PanelStyle()
{
  if (_gtk_theme_changed_id)
    g_signal_handler_disconnect(gtk_settings_get_default(),
                                _gtk_theme_changed_id);

  g_object_unref(_style_context);

  if (_style == this)
    _style = NULL;

  g_free(_theme_name);
}

PanelStyle*
PanelStyle::GetDefault()
{
  if (G_UNLIKELY(!_style))
    _style = new PanelStyle();

  return _style;
}

void
PanelStyle::Refresh()
{
  GdkRGBA rgba_text;

  if (_theme_name)
    g_free(_theme_name);

  _theme_name = NULL;
  g_object_get(gtk_settings_get_default(), "gtk-theme-name", &_theme_name, NULL);

  gtk_style_context_invalidate(_style_context);

  gtk_style_context_get_color(_style_context, GTK_STATE_FLAG_NORMAL, &rgba_text);

  _text = ColorFromGdkRGBA(rgba_text);

  changed.emit();
}

GtkStyleContext*
PanelStyle::GetStyleContext()
{
  return _style_context;
}

void
PanelStyle::OnStyleChanged(GObject*    gobject,
                           GParamSpec* pspec,
                           gpointer    data)
{
  PanelStyle* self = (PanelStyle*) data;

  self->Refresh();
}

nux::NBitmapData*
PanelStyle::GetBackground(int width, int height)
{
  nux::CairoGraphics* context = new nux::CairoGraphics(CAIRO_FORMAT_ARGB32, width, height);

  cairo_t* cr = context->GetContext();

  gtk_render_background(_style_context, cr, 0, 0, width, height);

  nux::NBitmapData* bitmap = context->GetBitmap();

  delete context;

  return bitmap;
}

nux::BaseTexture*
PanelStyle::GetWindowButton(WindowButtonType type, WindowState state)
{
#define ICON_LOCATION "/usr/share/themes/%s/metacity-1/%s%s.png"
  nux::BaseTexture* texture = NULL;
  const char* names[] = { "close", "minimize", "unmaximize" };
  const char* states[] = { "", "_focused_prelight", "_focused_pressed" };

  // I wish there was a magic bullet here, but not all themes actually set the panel to be
  // the same style as the window titlebars (e.g. Clearlooks) so we can just grab the
  // metacity window buttons as that would look horrible
  if (IsAmbianceOrRadiance())
  {
    char*      filename;
    GdkPixbuf* pixbuf;
    GError*    error = NULL;

    filename = g_strdup_printf(ICON_LOCATION, _theme_name, names[type], states[state]);

    pixbuf = gdk_pixbuf_new_from_file(filename, &error);
    if (error)
    {
      g_warning("Unable to load window button %s: %s", filename, error->message);
      g_error_free(error);
      error = NULL;
    }
    else
      texture = nux::CreateTexture2DFromPixbuf(pixbuf, true);

    g_free(filename);
    g_object_unref(pixbuf);
  }
  else
  {
    texture = GetWindowButtonForTheme(type, state);
  }

  return texture;
}

bool
PanelStyle::IsAmbianceOrRadiance()
{
  return g_strcmp0(_theme_name, "Ambiance") == 0 || g_strcmp0(_theme_name, "Radiance") == 0;
}

nux::BaseTexture*
PanelStyle::GetWindowButtonForTheme(WindowButtonType type, WindowState state)
{
  nux::BaseTexture* texture = NULL;
  int width = 18, height = 18;
  float w = width / 3.0f;
  float h = height / 3.0f;
  nux::CairoGraphics cairo_graphics(CAIRO_FORMAT_ARGB32, 22, 22);
  cairo_t* cr;
  nux::Color main = _text;

  if (type == WINDOW_BUTTON_CLOSE)
  {
    main = nux::Color(1.0f, 0.3f, 0.3f, 0.8f);
  }

  if (state == WINDOW_STATE_PRELIGHT)
    main = main * 1.2f;
  else if (state == WINDOW_STATE_PRESSED)
    main = main * 0.8f;

  cr  = cairo_graphics.GetContext();
  cairo_translate(cr, 0.5, 0.5);
  cairo_set_line_width(cr, 1.5f);

  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

  cairo_set_source_rgba(cr, main.red, main.green, main.blue, main.alpha);

  cairo_arc(cr, width / 2.0f, height / 2.0f, (width - 2) / 2.0f, 0.0f, 360 * (M_PI / 180));
  cairo_stroke(cr);

  if (type == WINDOW_BUTTON_CLOSE)
  {
    cairo_move_to(cr, w, h);
    cairo_line_to(cr, width - w, height - h);
    cairo_move_to(cr, width - w, h);
    cairo_line_to(cr, w, height - h);
  }
  else if (type == WINDOW_BUTTON_MINIMIZE)
  {
    cairo_move_to(cr, w, height / 2.0f);
    cairo_line_to(cr, width - w, height / 2.0f);
  }
  else
  {
    cairo_move_to(cr, w, h);
    cairo_line_to(cr, width - w, h);
    cairo_line_to(cr, width - w, height - h);
    cairo_line_to(cr, w, height - h);
    cairo_close_path(cr);
  }

  cairo_stroke(cr);

  cairo_destroy(cr);

  nux::NBitmapData* bitmap =  cairo_graphics.GetBitmap();
  texture = nux::GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableTexture();
  texture->Update(bitmap);
  delete bitmap;

  return texture;
}

GdkPixbuf*
PanelStyle::GetHomeButton()
{
  GdkPixbuf* pixbuf = NULL;

  pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
                                    "start-here",
                                    24,
                                    (GtkIconLookupFlags)0,
                                    NULL);
  if (pixbuf == NULL)
    pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
                                      "distributor-logo",
                                      24,
                                      (GtkIconLookupFlags)0,
                                      NULL);
  return pixbuf;
}
