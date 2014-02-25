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

#include <NuxCore/Logger.h>

#include "PanelStyle.h"
#include "CairoTexture.h"

#include "unity-shared/DecorationStyle.h"
#include "unity-shared/TextureCache.h"
#include "unity-shared/UnitySettings.h"

#include "MultiMonitor.h"

namespace unity
{
namespace panel
{
namespace
{
Style* style_instance = nullptr;

DECLARE_LOGGER(logger, "unity.panel.style");
const int BUTTONS_SIZE = 16;
const int BUTTONS_PADDING = 1;
const int BASE_PANEL_HEIGHT = 24;

std::string button_id(WindowButtonType type, WindowState ws)
{
  std::string texture_id = "window-button-";
  texture_id += std::to_string(static_cast<int>(type));
  texture_id += std::to_string(static_cast<int>(ws));
  return texture_id;
}

}

Style::Style()
  : style_context_(gtk_style_context_new())
  , bg_textures_(monitors::MAX)
  , panel_heights_(monitors::MAX, 0)
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

  gtk_style_context_set_path(style_context_, widget_path);
  gtk_style_context_add_class(style_context_, "gnome-panel-menu-bar");
  gtk_style_context_add_class(style_context_, "unity-panel");

  gtk_widget_path_free(widget_path);

  Settings::Instance().dpi_changed.connect(sigc::mem_fun(this, &Style::DPIChanged));
  auto const& deco_style = decoration::Style::Get();
  auto refresh_cb = sigc::hide(sigc::mem_fun(this, &Style::RefreshContext));
  deco_style->theme.changed.connect(sigc::mem_fun(this, &Style::OnThemeChanged));
  deco_style->font.changed.connect(refresh_cb);
  deco_style->title_font.changed.connect(refresh_cb);
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

void Style::OnThemeChanged(std::string const&)
{
  auto& cache = TextureCache::GetDefault();

  for (unsigned button = 0; button < unsigned(WindowButtonType::Size); ++button)
    for (unsigned state = 0; state < unsigned(WindowState::Size); ++state)
      cache.Invalidate(button_id(WindowButtonType(button), WindowState(state)));

  RefreshContext();
}

int Style::PanelHeight(int monitor) const
{
  if (monitor < 0 || monitor >= (int)monitors::MAX)
  {
    LOG_ERROR(logger) << "Invalid monitor index: " << monitor;
    return 0;
  }

  auto& panel_height = panel_heights_[monitor];

  if (panel_height > 0)
    return panel_height;

  panel_height = Settings::Instance().em(monitor)->CP(BASE_PANEL_HEIGHT);
  return panel_height;
}

void Style::DPIChanged()
{
  bg_textures_.assign(monitors::MAX, BaseTexturePtr());
  panel_heights_.assign(monitors::MAX, 0);
  changed.emit();
}

void Style::RefreshContext()
{
  gtk_style_context_invalidate(style_context_);
  bg_textures_.assign(monitors::MAX, BaseTexturePtr());

  changed.emit();
}

GtkStyleContext* Style::GetStyleContext()
{
  return style_context_;
}

BaseTexturePtr Style::GetBackground(int monitor)
{
  auto& bg_texture = bg_textures_[monitor];

  if (bg_texture)
    return bg_texture;

  const int width = 1;
  int height = PanelHeight(monitor);
  nux::CairoGraphics context(CAIRO_FORMAT_ARGB32, width, height);

  // Use the internal context as we know it is good and shiny new.
  cairo_t* cr = context.GetInternalContext();
  gtk_render_background(style_context_, cr, 0, 0, width, height);
  gtk_render_frame(style_context_, cr, 0, 0, width, height);

  bg_texture = texture_ptr_from_cairo_graphics(context);
  return bg_texture;
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
  return cache.FindTexture(button_id(type, state), 0, 0, texture_factory);
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
      return decoration::Style::Get()->font();
    case PanelItem::TITLE:
      return decoration::Style::Get()->title_font();
  }

  return std::string();
}

} // namespace panel
} // namespace unity

