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

#include <Nux/Nux.h>
#include <NuxCore/Logger.h>

#include "config.h"
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
const std::string PANEL_STYLE_CSS_NAME = "UnityPanelWidget";

inline std::string button_id(std::string const& prefix, double scale, WindowButtonType type, WindowState ws)
{
  std::string texture_id = prefix;
  texture_id += std::to_string(scale);
  texture_id += std::to_string(static_cast<int>(type));
  texture_id += std::to_string(static_cast<int>(ws));
  return texture_id;
}

inline std::string win_button_id(double scale, WindowButtonType type, WindowState ws)
{
  return button_id("window-button-", scale, type, ws);
}

inline std::string dash_button_id(double scale, WindowButtonType type, WindowState ws)
{
  return button_id("dash-win-button-", scale, type, ws);
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

  std::shared_ptr<GtkWidgetPath> widget_path(gtk_widget_path_new(), gtk_widget_path_free);
  gtk_widget_path_append_type(widget_path.get(), GTK_TYPE_WIDGET);
  gtk_widget_path_iter_set_name(widget_path.get(), -1, PANEL_STYLE_CSS_NAME.c_str());

  gtk_style_context_set_path(style_context_, widget_path.get());
  gtk_style_context_add_class(style_context_, "background");
  gtk_style_context_add_class(style_context_, "gnome-panel-menu-bar");
  gtk_style_context_add_class(style_context_, "unity-panel");

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
  auto& settings = Settings::Instance();

  for (unsigned monitor = 0; monitor < monitors::MAX; ++monitor)
  {
    for (unsigned button = 0; button < unsigned(WindowButtonType::Size); ++button)
    {
      for (unsigned state = 0; state < unsigned(WindowState::Size); ++state)
      {
        cache.Invalidate(win_button_id(settings.em(monitor)->DPIScale(), WindowButtonType(button), WindowState(state)));
        cache.Invalidate(dash_button_id(settings.em(monitor)->DPIScale(), WindowButtonType(button), WindowState(state)));
      }
    }
  }

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
#if !GTK_CHECK_VERSION(3, 11, 0)
  gtk_style_context_invalidate(style_context_);
#endif
  bg_textures_.assign(monitors::MAX, BaseTexturePtr());

  changed.emit();
}

GtkStyleContext* Style::GetStyleContext(PanelItem type)
{
  auto const* current_path = gtk_style_context_get_path(style_context_);

  switch (type)
  {
    case PanelItem::MENU:
    case PanelItem::INDICATOR:
      if (gtk_widget_path_is_type(current_path, GTK_TYPE_MENU_ITEM))
        return style_context_;
      break;
    case PanelItem::TITLE:
      if (gtk_widget_path_get_object_type(current_path) == GTK_TYPE_WIDGET)
        return style_context_;
      break;
  }

  std::shared_ptr<GtkWidgetPath> widget_path(gtk_widget_path_new(), gtk_widget_path_free);
  gtk_widget_path_append_type(widget_path.get(), GTK_TYPE_WIDGET);

  switch (type)
  {
    case PanelItem::MENU:
    case PanelItem::INDICATOR:
      gtk_widget_path_append_type(widget_path.get(), GTK_TYPE_MENU_BAR);
      gtk_widget_path_append_type(widget_path.get(), GTK_TYPE_MENU_ITEM);
      break;
    case PanelItem::TITLE:
      gtk_widget_path_append_type(widget_path.get(), GTK_TYPE_WIDGET);
      break;
  }

  gtk_widget_path_iter_set_name(widget_path.get(), -1, PANEL_STYLE_CSS_NAME.c_str());
  gtk_style_context_set_path(style_context_, widget_path.get());

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
  GtkStyleContext* style_ctx = GetStyleContext(PanelItem::TITLE);
  gtk_render_background(style_ctx, cr, 0, 0, width, height);
  gtk_render_frame(style_ctx, cr, 0, 0, width, height);

  bg_texture = texture_ptr_from_cairo_graphics(context);
  return bg_texture;
}

BaseTexturePtr GetFallbackWindowButton(WindowButtonType type, WindowState state, int monitor = 0)
{
  double scale = Settings::Instance().em(monitor)->DPIScale();
  int button_size = std::round((BUTTONS_SIZE + BUTTONS_PADDING*2) * scale);
  nux::CairoGraphics cairo_graphics(CAIRO_FORMAT_ARGB32, button_size, button_size);
  cairo_t* ctx = cairo_graphics.GetInternalContext();
  cairo_surface_set_device_scale(cairo_graphics.GetSurface(), scale, scale);
  cairo_translate(ctx, BUTTONS_PADDING, BUTTONS_PADDING);
  decoration::Style::Get()->DrawWindowButton(type, state, ctx, BUTTONS_SIZE, BUTTONS_SIZE);
  return texture_ptr_from_cairo_graphics(cairo_graphics);
}

nux::BaseTexture* ButtonFactory(std::string const& file, WindowButtonType type, WindowState state, int monitor, double scale)
{
  nux::Size size;
  nux::BaseTexture* texture = nullptr;
  gdk_pixbuf_get_file_info(file.c_str(), &size.width, &size.height);

  size.width = std::round(size.width * scale);
  size.height = std::round(size.height * scale);
  texture = nux::CreateTexture2DFromFile(file.c_str(), std::max(size.width, size.height), true);

  if (texture)
    return texture;

  auto const& fallback = GetFallbackWindowButton(type, state, monitor);
  texture = fallback.GetPointer();
  texture->Reference();

  return texture;
}

BaseTexturePtr Style::GetWindowButton(WindowButtonType type, WindowState state, int monitor)
{
  double scale = Settings::Instance().em(monitor)->DPIScale();
  auto texture_factory = [this, type, state, scale, monitor] (std::string const&, int, int) {
    auto const& file = decoration::Style::Get()->WindowButtonFile(type, state);
    return ButtonFactory(file, type, state, monitor, scale);
  };

  auto& cache = TextureCache::GetDefault();
  return cache.FindTexture(win_button_id(scale, type, state), 0, 0, texture_factory);
}

BaseTexturePtr Style::GetDashWindowButton(WindowButtonType type, WindowState state, int monitor)
{
  double scale = Settings::Instance().em(monitor)->DPIScale();

  auto texture_factory = [this, type, state, monitor, scale] (std::string const&, int, int) {
    static const std::array<std::string, 4> names = { "close_dash", "minimize_dash", "unmaximize_dash", "maximize_dash" };
    static const std::array<std::string, 4> states = { "", "_prelight", "_pressed", "_disabled" };

    auto base_filename = names[unsigned(type)] + states[unsigned(state)];
    auto const& file = decoration::Style::Get()->ThemedFilePath(base_filename, {PKGDATADIR});

    if (file.empty())
      return static_cast<nux::BaseTexture*>(nullptr);

    return ButtonFactory(file, type, state, monitor, scale);
  };

  auto& cache = TextureCache::GetDefault();
  return cache.FindTexture(dash_button_id(scale, type, state), 0, 0, texture_factory);
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

