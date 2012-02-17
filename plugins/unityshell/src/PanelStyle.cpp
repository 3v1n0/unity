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
#include <NuxCore/Logger.h>

#include "CairoTexture.h"
#include "PanelStyle.h"

#include <UnityCore/GLibWrapper.h>

namespace unity
{
namespace panel
{
namespace
{
Style* style_instance = nullptr;

nux::logging::Logger logger("unity.panel");

nux::Color ColorFromGdkRGBA(GdkRGBA const& color)
{
  return nux::Color(color.red,
                    color.green,
                    color.blue,
                    color.alpha);
}

}

Style::Style()
  : panel_height(24)
  , _theme_name(NULL)
{
  if (style_instance)
  {
    LOG_ERROR(logger) << "More than one panel::Style created.";
  }
  else
  {
    style_instance = this;
  }

  _style_context = gtk_style_context_new();

  GtkWidgetPath* widget_path = gtk_widget_path_new();
  gint pos = gtk_widget_path_append_type(widget_path, GTK_TYPE_WINDOW);
  gtk_widget_path_iter_set_name(widget_path, pos, "UnityPanelWidget");

  gtk_style_context_set_path(_style_context, widget_path);
  gtk_style_context_add_class(_style_context, "gnome-panel-menu-bar");
  gtk_style_context_add_class(_style_context, "unity-panel");

  gtk_widget_path_free(widget_path);

  _gtk_theme_changed_id = g_signal_connect(gtk_settings_get_default(), "notify::gtk-theme-name",
                                           G_CALLBACK(Style::OnGtkThemeChanged), this);

  Refresh();
}

Style::~Style()
{
  if (_gtk_theme_changed_id)
    g_signal_handler_disconnect(gtk_settings_get_default(),
                                _gtk_theme_changed_id);

  g_object_unref(_style_context);
  g_free(_theme_name);

  if (style_instance == this)
    style_instance = nullptr;
}

Style& Style::Instance()
{
  if (!style_instance)
  {
    LOG_ERROR(logger) << "No panel::Style created yet.";
  }

  return *style_instance;
}


void Style::Refresh()
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

GtkStyleContext* Style::GetStyleContext()
{
  return _style_context;
}

void Style::OnGtkThemeChanged(GObject*    gobject,
                              GParamSpec* pspec,
                              gpointer    data)
{
  Style* self = (Style*) data;

  self->Refresh();
}

nux::NBitmapData* Style::GetBackground(int width, int height, float opacity)
{
  nux::CairoGraphics context(CAIRO_FORMAT_ARGB32, width, height);

  // Use the internal context as we know it is good and shiny new.
  cairo_t* cr = context.GetInternalContext();
  cairo_push_group(cr);
  gtk_render_background(_style_context, cr, 0, 0, width, height);
  gtk_render_frame(_style_context, cr, 0, 0, width, height);
  cairo_pop_group_to_source(cr);
  cairo_paint_with_alpha(cr, opacity);

  return context.GetBitmap();
}

nux::BaseTexture* Style::GetWindowButton(WindowButtonType type, WindowState state)
{
  nux::BaseTexture* texture = NULL;
  const char* names[] = { "close", "minimize", "unmaximize" };
  const char* states[] = { "", "_focused_prelight", "_focused_pressed" };

  std::ostringstream subpath;
  subpath << "unity/" << names[static_cast<int>(type)]
          << states[static_cast<int>(state)] << ".png";

  // Look in home directory
  const char* home_dir = g_get_home_dir();
  if (home_dir)
  {
    glib::String filename(g_build_filename(home_dir, ".themes", _theme_name, subpath.str().c_str(), NULL));

    if (g_file_test(filename.Value(), G_FILE_TEST_EXISTS))
    {
      glib::Error error;

      // Found a file, try loading the pixbuf
      glib::Object<GdkPixbuf> pixbuf(gdk_pixbuf_new_from_file(filename.Value(), &error));
      if (error)
        LOG_WARNING(logger) << "Unable to load window button " << filename.Value() << ": " << error.Message();
      else
        texture = nux::CreateTexture2DFromPixbuf(pixbuf, true);
    }
  }

  // texture is NULL if the pixbuf is not loaded
  if (!texture)
  {
    const char* var = g_getenv("GTK_DATA_PREFIX");
    if (!var)
      var = "/usr";

    glib::String filename(g_build_filename(var, "share", "themes", _theme_name, subpath.str().c_str(), NULL));

    if (g_file_test(filename.Value(), G_FILE_TEST_EXISTS))
    {
      glib::Error error;

      // Found a file, try loading the pixbuf
      glib::Object<GdkPixbuf> pixbuf(gdk_pixbuf_new_from_file(filename.Value(), &error));
      if (error)
        LOG_WARNING(logger) << "Unable to load window button " << filename.Value() << ": " << error.Message();
      else
        texture = nux::CreateTexture2DFromPixbuf(pixbuf, true);
    }
  }

  if (!texture)
    texture = GetFallbackWindowButton(type, state);

  return texture;
}

nux::BaseTexture* Style::GetFallbackWindowButton(WindowButtonType type,
                                                 WindowState state)
{
  int width = 18, height = 18;
  float w = width / 3.0f;
  float h = height / 3.0f;
  nux::CairoGraphics cairo_graphics(CAIRO_FORMAT_ARGB32, 22, 22);
  cairo_t* cr;
  nux::Color main = _text;

  if (type == WindowButtonType::CLOSE)
  {
    main = nux::Color(1.0f, 0.3f, 0.3f, 0.8f);
  }

  if (state == WindowState::PRELIGHT)
    main = main * 1.2f;
  else if (state == WindowState::PRESSED)
    main = main * 0.8f;
  else if (state == WindowState::DISABLED)
    main = main * 0.5f;

  cr  = cairo_graphics.GetContext();
  cairo_translate(cr, 0.5, 0.5);
  cairo_set_line_width(cr, 1.5f);

  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

  cairo_set_source_rgba(cr, main.red, main.green, main.blue, main.alpha);

  cairo_arc(cr, width / 2.0f, height / 2.0f, (width - 2) / 2.0f, 0.0f, 360 * (M_PI / 180));
  cairo_stroke(cr);

  if (type == WindowButtonType::CLOSE)
  {
    cairo_move_to(cr, w, h);
    cairo_line_to(cr, width - w, height - h);
    cairo_move_to(cr, width - w, h);
    cairo_line_to(cr, w, height - h);
  }
  else if (type == WindowButtonType::MINIMIZE)
  {
    cairo_move_to(cr, w, height / 2.0f);
    cairo_line_to(cr, width - w, height / 2.0f);
  }
  else if (type == WindowButtonType::UNMAXIMIZE)
  {
    cairo_move_to(cr, w, h + h/5.0f);
    cairo_line_to(cr, width - w, h + h/5.0f);
    cairo_line_to(cr, width - w, height - h - h/5.0f);
    cairo_line_to(cr, w, height - h - h/5.0f);
    cairo_close_path(cr);
  }
  else // if (type == WindowButtonType::MAXIMIZE)
  {
    cairo_move_to(cr, w, h);
    cairo_line_to(cr, width - w, h);
    cairo_line_to(cr, width - w, height - h);
    cairo_line_to(cr, w, height - h);
    cairo_close_path(cr);
  }

  cairo_stroke(cr);

  cairo_destroy(cr);

  return texture_from_cairo_graphics(cairo_graphics);
}

GdkPixbuf* Style::GetHomeButton()
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

} // namespace panel
} // namespace unity
