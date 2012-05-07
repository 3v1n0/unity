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
 *              Marco Trevisan <3v1n0@ubuntu.com>
 */

#include "config.h"

#include <math.h>
#include <gtk/gtk.h>
#include <gconf/gconf-client.h>
#include <boost/algorithm/string/predicate.hpp>

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

nux::logging::Logger logger("unity.panel.style");

const std::string METACITY_SETTINGS_PATH("/apps/metacity/general/");
const std::string PANEL_TITLE_FONT_KEY("/apps/metacity/general/titlebar_font");
const std::string HIGH_CONTRAST_THEME_PREFIX("HighContrast");

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
  , _style_context(gtk_style_context_new())
{
  if (style_instance)
  {
    LOG_ERROR(logger) << "More than one panel::Style created.";
  }
  else
  {
    style_instance = this;
  }

  GtkWidgetPath* widget_path = gtk_widget_path_new();
  gint pos = gtk_widget_path_append_type(widget_path, GTK_TYPE_WINDOW);
  gtk_widget_path_iter_set_name(widget_path, pos, "UnityPanelWidget");

  gtk_style_context_set_path(_style_context, widget_path);
  gtk_style_context_add_class(_style_context, "gnome-panel-menu-bar");
  gtk_style_context_add_class(_style_context, "unity-panel");

  gtk_widget_path_free(widget_path);

  GtkSettings* settings = gtk_settings_get_default();

  _style_changed_signal.Connect(settings, "notify::gtk-theme-name",
  [&] (GtkSettings*, GParamSpec*) {
    Refresh();
  });

  _font_changed_signal.Connect(settings, "notify::gtk-font-name",
  [&] (GtkSettings*, GParamSpec*) {
    changed.emit();
  });

  _dpi_changed_signal.Connect(settings, "notify::gtk-xft-dpi",
  [&] (GtkSettings*, GParamSpec*) {
    changed.emit();
  });

  GConfClient* client = gconf_client_get_default();
  gconf_client_add_dir(client, METACITY_SETTINGS_PATH.c_str(), GCONF_CLIENT_PRELOAD_NONE, nullptr);
  _gconf_notify_id = gconf_client_notify_add(client, PANEL_TITLE_FONT_KEY.c_str(),
    [] (GConfClient*,guint,GConfEntry*, gpointer data)
    {
      auto self = static_cast<Style*>(data);
      self->changed.emit();
    }, this, nullptr, nullptr);

  Refresh();
}

Style::~Style()
{
  if (style_instance == this)
    style_instance = nullptr;

  if (_gconf_notify_id)
    gconf_client_notify_remove(gconf_client_get_default(), _gconf_notify_id);
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
  GdkRGBA rgba_text_color;
  glib::String theme_name;
  bool updated = false;
  
  GtkSettings* settings = gtk_settings_get_default();
  g_object_get(settings, "gtk-theme-name", &theme_name, nullptr);

  if (_theme_name != theme_name.Str())
  {
    _theme_name = theme_name.Str();
    updated = true;
  }

  gtk_style_context_invalidate(_style_context);
  gtk_style_context_get_color(_style_context, GTK_STATE_FLAG_NORMAL, &rgba_text_color);
  nux::Color const& new_text_color = ColorFromGdkRGBA(rgba_text_color);

  if (_text_color != new_text_color)
  {
    _text_color = new_text_color;
    updated = true;
  }

  if (updated)
    changed.emit();
}

GtkStyleContext* Style::GetStyleContext()
{
  return _style_context;
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
  std::string names[] = { "close", "minimize", "unmaximize", "maximize" };
  std::string states[] = { "", "_focused_prelight", "_focused_pressed", "_unfocused",
                           "_unfocused", "_unfocused_prelight", "_unfocused_pressed"};

  std::ostringstream subpath;
  subpath << "unity/" << names[static_cast<int>(type)]
          << states[static_cast<int>(state)] << ".png";

  // Look in home directory
  const char* home_dir = g_get_home_dir();
  if (home_dir)
  {
    glib::String filename(g_build_filename(home_dir, ".themes", _theme_name.c_str(), subpath.str().c_str(), NULL));

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

    glib::String filename(g_build_filename(var, "share", "themes", _theme_name.c_str(), subpath.str().c_str(), NULL));

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
  int width = 17, height = 17;
  int canvas_w = 19, canvas_h = 19;

  if (boost::starts_with(_theme_name, HIGH_CONTRAST_THEME_PREFIX))
  {
    width = 20, height = 20;
    canvas_w = 22, canvas_h = 22;
  }

  float w = width / 3.0f;
  float h = height / 3.0f;
  nux::CairoGraphics cairo_graphics(CAIRO_FORMAT_ARGB32, canvas_w, canvas_h);
  nux::Color main = (state != WindowState::UNFOCUSED) ? _text_color : nux::color::Gray;
  cairo_t* cr = cairo_graphics.GetContext();

  if (type == WindowButtonType::CLOSE)
  {
    double alpha = (state != WindowState::UNFOCUSED) ? 0.8f : 0.5;
    main = nux::Color(1.0f, 0.3f, 0.3f, alpha);
  }

  switch (state)
  {
    case WindowState::PRELIGHT:
      main = main * 1.2f;
      break;
    case WindowState::UNFOCUSED_PRELIGHT:
      main = main * 0.9f;
      break;
    case WindowState::PRESSED:
      main = main * 0.8f;
      break;
    case WindowState::UNFOCUSED_PRESSED:
      main = main * 0.7f;
      break;
    case WindowState::DISABLED:
      main = main * 0.5f;
      break;
    default:
      break;
  }

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

glib::Object<GdkPixbuf> Style::GetHomeButton()
{
  glib::Object<GdkPixbuf> pixbuf;

  pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
                                    "start-here",
                                    panel_height,
                                    (GtkIconLookupFlags)0,
                                    NULL);
  if (!pixbuf)
    pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
                                      "distributor-logo",
                                      panel_height,
                                      (GtkIconLookupFlags)0,
                                      NULL);
  return pixbuf;
}

std::string Style::GetFontDescription(PanelItem item)
{
  switch (item)
  {
    case PanelItem::INDICATOR:
    case PanelItem::MENU:
    {
      glib::String font_name;
      g_object_get(gtk_settings_get_default(), "gtk-font-name", &font_name, nullptr);
      return font_name.Str();
    }
    case PanelItem::TITLE:
    {
      GConfClient* client = gconf_client_get_default();
      glib::String font_name(gconf_client_get_string(client, PANEL_TITLE_FONT_KEY.c_str(), nullptr));
      return font_name.Str();
    }
  }

  return "";
}

int Style::GetTextDPI()
{
  int dpi = 0;
  g_object_get(gtk_settings_get_default(), "gtk-xft-dpi", &dpi, nullptr);

  return dpi;
}

} // namespace panel
} // namespace unity
