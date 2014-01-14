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
#include <array>
#include <gtk/gtk.h>
#include <boost/algorithm/string/predicate.hpp>

#include <Nux/Nux.h>
#include <NuxGraphics/GraphicsEngine.h>
#include <NuxGraphics/CairoGraphics.h>
#include <NuxCore/Logger.h>

#include "CairoTexture.h"
#include "PanelStyle.h"

#include <UnityCore/GLibWrapper.h>
#include "unity-shared/DecorationStyle.h"
#include "unity-shared/TextureCache.h"
#include "unity-shared/UnitySettings.h"

namespace unity
{
namespace panel
{
DECLARE_LOGGER(logger, "unity.panel.style");
namespace
{
Style* style_instance = nullptr;

const std::string SETTINGS_NAME("org.gnome.desktop.wm.preferences");
const std::string PANEL_TITLE_FONT_KEY("titlebar-font");
const std::string HIGH_CONTRAST_THEME_PREFIX("HighContrast");
const int BUTTONS_SIZE = 16;
const int BUTTONS_PADDING = 1;

nux::Color ColorFromGdkRGBA(GdkRGBA const& color)
{
  return nux::Color(color.red, color.green, color.blue, color.alpha);
}

}

Style::Style()
  : panel_height(24)
  , _style_context(gtk_style_context_new())
  , _gsettings(g_settings_new(SETTINGS_NAME.c_str()))
{
  if (style_instance)
  {
    LOG_ERROR(logger) << "More than one panel::Style created.";
  }
  else
  {
    style_instance = this;
  }

  if (Settings::Instance().form_factor() == FormFactor::TV)
    panel_height = 0;

  Settings::Instance().form_factor.changed.connect([this](FormFactor form_factor)
  {
    if (form_factor == FormFactor::TV)
      panel_height = 0;
  });

  GtkWidgetPath* widget_path = gtk_widget_path_new();
  gint pos = gtk_widget_path_append_type(widget_path, GTK_TYPE_WINDOW);
  gtk_widget_path_iter_set_name(widget_path, pos, "UnityPanelWidget");

  gtk_style_context_set_path(_style_context, widget_path);
  gtk_style_context_add_class(_style_context, "gnome-panel-menu-bar");
  gtk_style_context_add_class(_style_context, "unity-panel");

  gtk_widget_path_free(widget_path);

  GtkSettings* settings = gtk_settings_get_default();

  _style_changed_signal.Connect(settings, "notify::gtk-theme-name",
  [this] (GtkSettings*, GParamSpec*) {
    Refresh();
  });

  _font_changed_signal.Connect(settings, "notify::gtk-font-name",
  [this] (GtkSettings*, GParamSpec*) {
    changed.emit();
  });

  _dpi_changed_signal.Connect(settings, "notify::gtk-xft-dpi",
  [this] (GtkSettings*, GParamSpec*) {
    changed.emit();
  });

  _settings_changed_signal.Connect(_gsettings, "changed::" + PANEL_TITLE_FONT_KEY,
  [this] (GSettings*, gchar*) {
    changed.emit();
  });

  Refresh();
}

Style::~Style()
{
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
  {
    _bg_texture.Release();
    changed.emit();
  }
}

GtkStyleContext* Style::GetStyleContext()
{
  return _style_context;
}

BaseTexturePtr Style::GetBackground()
{
  if (_bg_texture)
    return _bg_texture;

  int width = 1;
  int height = panel_height();

  nux::CairoGraphics context(CAIRO_FORMAT_ARGB32, width, height);

  // Use the internal context as we know it is good and shiny new.
  cairo_t* cr = context.GetInternalContext();
  gtk_render_background(_style_context, cr, 0, 0, width, height);
  gtk_render_frame(_style_context, cr, 0, 0, width, height);

  _bg_texture = texture_ptr_from_cairo_graphics(context);

  return _bg_texture;
}

BaseTexturePtr Style::GetWindowButton(WindowButtonType type, WindowState state)
{
  auto texture_factory = [this, type, state] (std::string const&, int, int) {
    nux::BaseTexture* texture = nullptr;
    auto const& file = decoration::Style::Get()->WindowButtonFile(type, state);
    texture = nux::CreateTexture2DFromFile(file.c_str(), -1, true);
    if (texture)
      return texture;

    auto const& fallback = GetFallbackWindowButton(type, state);
    texture = fallback.GetPointer();
    texture->Reference();

    return texture;
  };

  auto& cache = TextureCache::GetDefault();
  std::string texture_id = "window-button-";
  texture_id += std::to_string(static_cast<int>(type));
  texture_id += std::to_string(static_cast<int>(state));

  return cache.FindTexture(texture_id, 0, 0, texture_factory);
}

BaseTexturePtr Style::GetFallbackWindowButton(WindowButtonType type, WindowState state)
{
  nux::CairoGraphics cairo_graphics(CAIRO_FORMAT_ARGB32, BUTTONS_SIZE + BUTTONS_PADDING*2, BUTTONS_SIZE + BUTTONS_PADDING*2);
  cairo_t* ctx = cairo_graphics.GetInternalContext();
  cairo_translate(ctx, BUTTONS_PADDING, BUTTONS_PADDING);
  decoration::Style::Get()->DrawWindowButton(type, state, ctx, BUTTONS_SIZE, BUTTONS_SIZE);
  return texture_ptr_from_cairo_graphics(cairo_graphics);
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
      glib::String font_name(g_settings_get_string(_gsettings, PANEL_TITLE_FONT_KEY.c_str()));
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

