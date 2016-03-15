// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2013 Canonical Ltd
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
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

#include "config.h"
#include "DecorationStyle.h"

#include <math.h>
#include <NuxCore/Colors.h>
#include <NuxCore/Logger.h>
#include <UnityCore/ConnectionManager.h>
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/GLibSignal.h>

#include "GtkUtils.h"
#include "ThemeSettings.h"

namespace unity
{
namespace decoration
{
namespace
{
DECLARE_LOGGER(logger, "unity.decoration.style");

const std::array<std::string, 4> BORDER_CLASSES = {"top", "left", "right", "bottom"};
const Border DEFAULT_BORDER = {28, 1, 1, 1};
const Border DEFAULT_INPUT_EDGES = {10, 10, 10, 10};

const nux::Point DEFAULT_SHADOW_OFFSET(1, 1);

const nux::Color DEFAULT_ACTIVE_SHADOW_COLOR(nux::color::Black * 0.647);
const int DEFAULT_ACTIVE_SHADOW_RADIUS = 8;

const nux::Color DEFAULT_INACTIVE_SHADOW_COLOR(nux::color::Black * 0.647);
const int DEFAULT_INACTIVE_SHADOW_RADIUS = 5;

const float DEFAULT_TITLE_ALIGNMENT = 0.0f;
const int DEFAULT_TITLE_INDENT = 10;
const int DEFAULT_TITLE_FADING_PIXELS = 35;

const int DEFAULT_GLOW_SIZE = 10;
const nux::Color DEFAULT_GLOW_COLOR(221, 72, 20);

const std::array<std::string, size_t(WindowButtonType::Size)> WBUTTON_NAMES = { "close", "minimize", "unmaximize", "maximize" };
const std::array<std::string, size_t(WidgetState::Size)> WBUTTON_STATES = {"", "_focused_prelight", "_focused_pressed", "_unfocused",
                                                                           "_unfocused", "_unfocused_prelight", "_unfocused_pressed" };

const std::string SETTINGS_NAME = "org.gnome.desktop.wm.preferences";
const std::string FONT_KEY = "titlebar-font";
const std::string USE_SYSTEM_FONT_KEY = "titlebar-uses-system-font";
const std::string ACTION_DOUBLE_CLICK = "action-double-click-titlebar";
const std::string ACTION_MIDDLE_CLICK = "action-middle-click-titlebar";
const std::string ACTION_RIGHT_CLICK = "action-right-click-titlebar";

const std::string UNITY_SETTINGS_NAME = "com.canonical.Unity.Decorations";
const std::string GRAB_WAIT_KEY = "grab-wait";

struct UnityDecoration
{
  GtkWidget parent_instance;
};

struct UnityDecorationClass
{
  GtkWidgetClass parent_class;
};

G_DEFINE_TYPE (UnityDecoration, unity_decoration, GTK_TYPE_WIDGET);
static void unity_decoration_init(UnityDecoration*) {}

static void unity_decoration_class_init(UnityDecorationClass* klass)
{
  auto* param = g_param_spec_boxed("extents", "Border extents", "", GTK_TYPE_BORDER, G_PARAM_READABLE);
  gtk_widget_class_install_style_property(GTK_WIDGET_CLASS(klass), param);

  param = g_param_spec_boxed("input-extents", "Input Border extents", "", GTK_TYPE_BORDER, G_PARAM_READABLE);
  gtk_widget_class_install_style_property(GTK_WIDGET_CLASS(klass), param);

  param = g_param_spec_uint("shadow-offset-x", "Shadow Offset X", "", 0, G_MAXUINT, DEFAULT_SHADOW_OFFSET.x, G_PARAM_READABLE);
  gtk_widget_class_install_style_property(GTK_WIDGET_CLASS(klass), param);

  param = g_param_spec_uint("shadow-offset-y", "Shadow Offset Y", "", 0, G_MAXUINT, DEFAULT_SHADOW_OFFSET.y, G_PARAM_READABLE);
  gtk_widget_class_install_style_property(GTK_WIDGET_CLASS(klass), param);

  param = g_param_spec_boxed("active-shadow-color", "Active Window Shadow Color", "", GDK_TYPE_RGBA, G_PARAM_READABLE);
  gtk_widget_class_install_style_property(GTK_WIDGET_CLASS(klass), param);

  param = g_param_spec_uint("active-shadow-radius", "Active Window Shadow Radius", "", 0, G_MAXUINT, DEFAULT_ACTIVE_SHADOW_RADIUS, G_PARAM_READABLE);
  gtk_widget_class_install_style_property(GTK_WIDGET_CLASS(klass), param);

  param = g_param_spec_boxed("inactive-shadow-color", "Inactive Windows Shadow Color", "", GDK_TYPE_RGBA, G_PARAM_READABLE);
  gtk_widget_class_install_style_property(GTK_WIDGET_CLASS(klass), param);

  param = g_param_spec_uint("inactive-shadow-radius", "Inactive Windows Shadow Radius", "", 0, G_MAXUINT, DEFAULT_INACTIVE_SHADOW_RADIUS, G_PARAM_READABLE);
  gtk_widget_class_install_style_property(GTK_WIDGET_CLASS(klass), param);

  param = g_param_spec_float("title-alignment", "Title Alignment", "", 0.0, 1.0, DEFAULT_TITLE_ALIGNMENT, G_PARAM_READABLE);
  gtk_widget_class_install_style_property(GTK_WIDGET_CLASS(klass), param);

  param = g_param_spec_uint("title-indent", "Title Indent", "", 0, G_MAXUINT, DEFAULT_TITLE_INDENT, G_PARAM_READABLE);
  gtk_widget_class_install_style_property(GTK_WIDGET_CLASS(klass), param);

  param = g_param_spec_uint("title-fade", "Title Fading Pixels", "", 0, G_MAXUINT, DEFAULT_TITLE_FADING_PIXELS, G_PARAM_READABLE);
  gtk_widget_class_install_style_property(GTK_WIDGET_CLASS(klass), param);

  param = g_param_spec_uint("glow-size", "Selected Window Glow Size", "", 0, G_MAXUINT, DEFAULT_GLOW_SIZE, G_PARAM_READABLE);
  gtk_widget_class_install_style_property(GTK_WIDGET_CLASS(klass), param);

  param = g_param_spec_boxed("glow-color", "Selected Window Glow Color", "", GDK_TYPE_RGBA, G_PARAM_READABLE);
  gtk_widget_class_install_style_property(GTK_WIDGET_CLASS(klass), param);
}

Border BorderFromGtkBorder(GtkBorder* b, Border const& fallback = Border())
{
  if (!b)
    return fallback;

  return Border(b->top, b->left, b->right, b->bottom);
}

nux::Color ColorFromGdkRGBA(GdkRGBA* c, nux::Color const& fallback = nux::Color())
{
  if (!c)
    return fallback;

  return nux::Color(c->red, c->green, c->blue, c->alpha);
}
}

struct Style::Impl
{
  Impl(Style* parent)
    : parent_(parent)
    , ctx_(gtk_style_context_new())
    , settings_(g_settings_new(SETTINGS_NAME.c_str()))
    , usettings_(g_settings_new(UNITY_SETTINGS_NAME.c_str()))
    , title_pango_ctx_(gdk_pango_context_get())
    , menu_item_pango_ctx_(gdk_pango_context_get())
    , title_alignment_(0)
    , title_indent_(0)
    , title_fade_(0)
  {
    std::shared_ptr<GtkWidgetPath> widget_path(gtk_widget_path_new(), gtk_widget_path_free);
    gtk_widget_path_append_type(widget_path.get(), unity_decoration_get_type());
    gtk_style_context_set_path(ctx_, widget_path.get());

    auto theme_settings = theme::Settings::Get();
    parent_->theme.SetGetterFunction([theme_settings] { return theme_settings->theme(); });
    parent_->font.SetGetterFunction([theme_settings] { return theme_settings->font(); });
    parent_->font_scale = 1.0;
    SetTitleFont();

    UpdateTitlePangoContext(parent_->title_font);
    UpdateMenuItemPangoContext(parent_->font);
    UpdateThemedValues();

    connections_.Add(theme_settings->theme.changed.connect([this] (std::string const& theme) {
#if !GTK_CHECK_VERSION(3, 11, 0)
      gtk_style_context_invalidate(ctx_);
#endif
      UpdateThemedValues();
      parent_->theme.EmitChanged(theme);
      LOG_INFO(logger) << "unity theme changed to " << parent_->theme();
    }));

    connections_.Add(theme_settings->font.changed.connect([this] (std::string const& font) {
      UpdateMenuItemPangoContext(font);
      parent_->font.EmitChanged(font);

      if (g_settings_get_boolean(settings_, USE_SYSTEM_FONT_KEY.c_str()))
      {
        UpdateTitlePangoContext(parent_->font());
        parent_->title_font = parent_->font();
      }
      LOG_INFO(logger) << "unity font changed to " << parent_->font();
    }));

    parent_->font_scale.changed.connect([this] (bool scale) {
      UpdateTitlePangoContext(parent_->title_font);
      UpdateMenuItemPangoContext(parent_->font);
#if !GTK_CHECK_VERSION(3, 11, 0)
      gtk_style_context_invalidate(ctx_);
#endif
      parent_->theme.changed.emit(parent_->theme());
      LOG_INFO(logger) << "font scale changed to " << scale;
    });

    signals_.Add<void, GSettings*, gchar*>(settings_, "changed::" + FONT_KEY, [this] (GSettings*, gchar*) {
      if (g_settings_get_boolean(settings_, USE_SYSTEM_FONT_KEY.c_str()))
        return;

      auto const& font = glib::String(g_settings_get_string(settings_, FONT_KEY.c_str())).Str();
      UpdateTitlePangoContext(font);
      parent_->title_font = font;
      LOG_INFO(logger) << FONT_KEY << " changed to " << font;
    });

    signals_.Add<void, GSettings*, gchar*>(settings_, "changed::" + USE_SYSTEM_FONT_KEY, [this] (GSettings*, gchar*) {
      parent_->title_font.DisableNotifications();
      SetTitleFont();
      UpdateTitlePangoContext(parent_->title_font());
      parent_->title_font.EnableNotifications();
      parent_->title_font.changed.emit(parent_->title_font());
      LOG_INFO(logger) << USE_SYSTEM_FONT_KEY << " changed to " << g_settings_get_boolean(settings_, USE_SYSTEM_FONT_KEY.c_str());
    });

    parent_->grab_wait = g_settings_get_uint(usettings_, GRAB_WAIT_KEY.c_str());
    signals_.Add<void, GSettings*, const gchar*>(usettings_, "changed::" + GRAB_WAIT_KEY, [this] (GSettings*, const gchar*) {
      parent_->grab_wait = g_settings_get_uint(usettings_, GRAB_WAIT_KEY.c_str());
    });
  }

  void UpdateThemedValues()
  {
    std::shared_ptr<GtkBorder> b(GetProperty<GtkBorder*>("extents"), gtk_border_free);
    border_ = BorderFromGtkBorder(b.get(), DEFAULT_BORDER);

    b.reset(GetProperty<GtkBorder*>("input-extents"), gtk_border_free);
    input_edges_ = BorderFromGtkBorder(b.get(), DEFAULT_INPUT_EDGES);

    std::shared_ptr<GdkRGBA> rgba(GetProperty<GdkRGBA*>("glow-color"), gdk_rgba_free);
    glow_color_ = ColorFromGdkRGBA(rgba.get(), DEFAULT_GLOW_COLOR);
    glow_size_ = std::max<unsigned>(0, GetProperty<guint>("glow-size"));

    radius_.top = GetBorderProperty<gint>(Side::TOP, WidgetState::NORMAL, "border-radius");
    radius_.left = GetBorderProperty<gint>(Side::LEFT, WidgetState::NORMAL, "border-radius");
    radius_.right = GetBorderProperty<gint>(Side::RIGHT, WidgetState::NORMAL, "border-radius");
    radius_.bottom = GetBorderProperty<gint>(Side::BOTTOM, WidgetState::NORMAL, "border-radius");

    title_alignment_ = std::min(1.0f, std::max(0.0f, GetProperty<gfloat>("title-alignment")));
    title_indent_ = std::max<unsigned>(0, GetProperty<guint>("title-indent"));
    title_fade_ = std::max<unsigned>(0, GetProperty<guint>("title-fade"));
  }

  void SetTitleFont()
  {
    if (g_settings_get_boolean(settings_, USE_SYSTEM_FONT_KEY.c_str()))
      parent_->title_font = parent_->font();
    else
      parent_->title_font = glib::String(g_settings_get_string(settings_, FONT_KEY.c_str())).Str();
  }

  inline void UpdatePangoContext(glib::Object<PangoContext> const& ctx, std::string const& font)
  {
    std::shared_ptr<PangoFontDescription> desc(pango_font_description_from_string(font.c_str()), pango_font_description_free);
    pango_context_set_font_description(ctx, desc.get());
    pango_context_set_language(ctx, gtk_get_default_language());
    pango_cairo_context_set_resolution(ctx, 96.0 * parent_->font_scale());
  }

  void UpdateTitlePangoContext(std::string const& font)
  {
    UpdatePangoContext(title_pango_ctx_, font);
  }

  void UpdateMenuItemPangoContext(std::string const& font)
  {
    UpdatePangoContext(menu_item_pango_ctx_, font);
  }

  inline std::string const& GetBorderClass(Side side)
  {
    return BORDER_CLASSES[unsigned(side)];
  }

  template <typename TYPE>
  inline TYPE GetProperty(std::string const& property)
  {
    TYPE value;
    gtk_style_context_get_style(ctx_, property.c_str(), &value, nullptr);
    return value;
  }

  template <typename TYPE>
  inline TYPE GetBorderProperty(Side s, WidgetState ws, std::string const& property)
  {
    TYPE value;
    gtk_style_context_save(ctx_);
    AddContextClasses(s, ws);
    gtk_style_context_get(ctx_, GtkStateFromWidgetState(ws), property.c_str(), &value, nullptr);
    gtk_style_context_restore(ctx_);
    return value;
  }

  WMAction WMActionFromString(std::string const& action) const
  {
    if (action == "toggle-shade")
      return WMAction::TOGGLE_SHADE;
    else if (action == "toggle-maximize")
      return WMAction::TOGGLE_MAXIMIZE;
    else if (action == "toggle-maximize-horizontally")
      return WMAction::TOGGLE_MAXIMIZE_HORIZONTALLY;
    else if (action == "toggle-maximize-vertically")
      return WMAction::TOGGLE_MAXIMIZE_VERTICALLY;
    else if (action == "minimize")
      return WMAction::MINIMIZE;
    else if (action == "shade")
      return WMAction::SHADE;
    else if (action == "menu")
      return WMAction::MENU;
    else if (action == "lower")
      return WMAction::LOWER;

    return WMAction::NONE;
  }

  WMAction WindowManagerAction(WMEvent event) const
  {
    std::string action_setting;

    switch (event)
    {
      case WMEvent::DOUBLE_CLICK:
        action_setting = ACTION_DOUBLE_CLICK;
        break;
      case WMEvent::MIDDLE_CLICK:
        action_setting = ACTION_MIDDLE_CLICK;
        break;
      case WMEvent::RIGHT_CLICK:
        action_setting = ACTION_RIGHT_CLICK;
        break;
    }

    glib::String action_string(g_settings_get_string(settings_, action_setting.c_str()));
    return WMActionFromString(action_string.Str());
  }

  inline GtkStateFlags GtkStateFromWidgetState(WidgetState ws)
  {
    switch (ws)
    {
      case WidgetState::NORMAL:
        return GTK_STATE_FLAG_NORMAL;
      case WidgetState::PRELIGHT:
        return GTK_STATE_FLAG_PRELIGHT;
      case WidgetState::PRESSED:
        return GTK_STATE_FLAG_ACTIVE;
      case WidgetState::DISABLED:
        return GTK_STATE_FLAG_INSENSITIVE;
      case WidgetState::BACKDROP:
        return GTK_STATE_FLAG_BACKDROP;
      case WidgetState::BACKDROP_PRELIGHT:
        return static_cast<GtkStateFlags>(GTK_STATE_FLAG_BACKDROP|GTK_STATE_FLAG_PRELIGHT);
      case WidgetState::BACKDROP_PRESSED:
        return static_cast<GtkStateFlags>(GTK_STATE_FLAG_BACKDROP|GTK_STATE_FLAG_ACTIVE);
      case WidgetState::Size:
        break;
    }

    return GTK_STATE_FLAG_NORMAL;
  }

  void AddContextClasses(Side s, WidgetState ws, GtkStyleContext* ctx = nullptr)
  {
    ctx = ctx ? ctx : ctx_;
    gtk_style_context_add_class(ctx, "gnome-panel-menu-bar");
    if (s == Side::TOP) { gtk_style_context_add_class(ctx, "header-bar"); }
    gtk_style_context_add_class(ctx, GetBorderClass(s).c_str());
    gtk_style_context_set_state(ctx, GtkStateFromWidgetState(ws));
  }

  void DrawSide(Side s, WidgetState ws, cairo_t* cr, double w, double h)
  {
    gtk_style_context_save(ctx_);
    AddContextClasses(s, ws);
    gtk_render_background(ctx_, cr, 0, 0, w, h);
    gtk_render_frame(ctx_, cr, 0, 0, w, h);
    gtk_style_context_restore(ctx_);
  }

  std::string WindowButtonFile(WindowButtonType type, WidgetState state) const
  {
    auto base_filename = WBUTTON_NAMES[unsigned(type)] + WBUTTON_STATES[unsigned(state)];
    auto const& file_path = parent_->ThemedFilePath(base_filename);

    if (!file_path.empty())
      return file_path;

    LOG_WARN(logger) << "No Window button file for '"<< base_filename << "'";
    return std::string();
  }

  void DrawWindowButton(WindowButtonType type, WidgetState ws, cairo_t* cr, double width, double height)
  {
    nux::Color color;
    float w = width / 3.5f;
    float h = height / 3.5f;

    if (type == WindowButtonType::CLOSE)
    {
      double alpha = (ws != WidgetState::BACKDROP) ? 0.8f : 0.5;
      color = nux::Color(1.0f, 0.3f, 0.3f, alpha);
    }
    else
    {
      if (ws != WidgetState::BACKDROP)
      {
        std::shared_ptr<GdkRGBA> rgba(GetBorderProperty<GdkRGBA*>(Side::TOP, WidgetState::NORMAL, "color"), gdk_rgba_free);
        color = ColorFromGdkRGBA(rgba.get());
      }
      else
      {
        color = nux::color::Gray;
      }
    }

    switch (ws)
    {
      case WidgetState::PRELIGHT:
        color = color * 1.2f;
        break;
      case WidgetState::BACKDROP_PRELIGHT:
        color = color * 0.9f;
        break;
      case WidgetState::PRESSED:
        color = color * 0.8f;
        break;
      case WidgetState::BACKDROP_PRESSED:
        color = color * 0.7f;
        break;
      case WidgetState::DISABLED:
        color = color * 0.5f;
        break;
      default:
        break;
    }

    cairo_set_line_width(cr, 1);
    cairo_set_source_rgba(cr, color.red, color.green, color.blue, color.alpha);
    cairo_arc(cr, width / 2.0f, height / 2.0f, (width - 2) / 2.0f, 0.0f, 360 * (M_PI / 180));
    cairo_fill_preserve(cr);
    cairo_stroke(cr);

    switch (type)
    {
      case WindowButtonType::CLOSE:
        cairo_move_to(cr, w, h);
        cairo_line_to(cr, width - w, height - h);
        cairo_move_to(cr, width - w, h);
        cairo_line_to(cr, w, height - h);
        break;
      case WindowButtonType::MINIMIZE:
        cairo_move_to(cr, w, height / 2.0f);
        cairo_line_to(cr, width - w, height / 2.0f);
        break;
      case WindowButtonType::UNMAXIMIZE:
        cairo_move_to(cr, w, h + h/5.0f);
        cairo_line_to(cr, width - w, h + h/5.0f);
        cairo_line_to(cr, width - w, height - h - h/5.0f);
        cairo_line_to(cr, w, height - h - h/5.0f);
        cairo_close_path(cr);
        break;
      case WindowButtonType::MAXIMIZE:
        cairo_move_to(cr, w, h);
        cairo_line_to(cr, width - w, h);
        cairo_line_to(cr, width - w, height - h);
        cairo_line_to(cr, w, height - h);
        cairo_close_path(cr);
        break;
      default:
        break;
    }

    cairo_set_line_width(cr, 1);
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_stroke(cr);
  }

  inline glib::Object<PangoLayout> BuildPangoLayout(glib::Object<PangoContext> const& ctx, std::string const& text)
  {
    glib::Object<PangoLayout> layout(pango_layout_new(ctx));
    pango_layout_set_height(layout, -1); //avoid wrap lines
    pango_layout_set_text(layout, text.c_str(), -1);
    return layout;
  }

  nux::Size TextNaturalSize(glib::Object<PangoContext> const& pctx, std::string const& text)
  {
    nux::Size extents;
    auto const& layout = BuildPangoLayout(pctx, text);
    pango_layout_get_pixel_size(layout, &extents.width, &extents.height);
    return extents;
  }

  void DrawTextBackground(GtkStyleContext* ctx, cairo_t* cr, glib::Object<PangoLayout> const& layout, nux::Rect const& bg_geo)
  {
    if (bg_geo.IsNull())
      return;

    // We need to render the background under the text glyphs, or the edge
    // of the text won't be correctly anti-aliased. See bug #723167
    cairo_push_group(cr);
    gtk_render_layout(ctx, cr, 0, 0, layout);
    std::shared_ptr<cairo_pattern_t> pat(cairo_pop_group(cr), cairo_pattern_destroy);

    cairo_push_group(cr);
    gtk_render_background(ctx, cr, bg_geo.x, bg_geo.y, bg_geo.width, bg_geo.height);
    cairo_pop_group_to_source(cr);
    cairo_mask(cr, pat.get());
  }

  void DrawTitle(std::string const& text, WidgetState ws, cairo_t* cr, double w, double h, nux::Rect const& bg_geo, GtkStyleContext* ctx)
  {
    gtk_style_context_save(ctx);
    AddContextClasses(Side::TOP, ws, ctx);

    auto const& layout = BuildPangoLayout(title_pango_ctx_, text);

    nux::Size extents;
    pango_layout_get_pixel_size(layout, &extents.width, &extents.height);
    pango_layout_set_height(layout, (h >= 0) ? h * PANGO_SCALE : -1);

    if (w >= 0 && extents.width > w)
    {
      double out_pixels = extents.width - w;
      double fading_width = std::min<double>(title_fade_, out_pixels);

      cairo_push_group(cr);
      DrawTextBackground(ctx, cr, layout, bg_geo);
      gtk_render_layout(ctx, cr, 0, 0, layout);
      cairo_pop_group_to_source(cr);

      std::shared_ptr<cairo_pattern_t> linpat(cairo_pattern_create_linear(w - fading_width, 0, w, 0),
                                              cairo_pattern_destroy);

      cairo_pattern_add_color_stop_rgba(linpat.get(), 0, 0, 0, 0, 1);
      cairo_pattern_add_color_stop_rgba(linpat.get(), 1, 0, 0, 0, 0);
      cairo_mask(cr, linpat.get());
    }
    else
    {
      pango_layout_set_width(layout, (w >= 0) ? w * PANGO_SCALE : -1);
      DrawTextBackground(ctx, cr, layout, bg_geo);
      gtk_render_layout(ctx, cr, 0, 0, layout);
    }

    gtk_style_context_restore(ctx);
  }

  inline void AddContextClassesForMenuItem(WidgetState ws)
  {
    AddContextClasses(Side::TOP, ws);

    gtk_style_context_add_class(ctx_, GTK_STYLE_CLASS_MENUBAR);
    gtk_style_context_add_class(ctx_, GTK_STYLE_CLASS_MENUITEM);
  }

  void DrawMenuItem(WidgetState ws, cairo_t* cr, double w, double h)
  {
    gtk_style_context_save(ctx_);
    AddContextClassesForMenuItem(ws);

    gtk_render_background(ctx_, cr, 0, 0, w, h);
    gtk_render_frame(ctx_, cr, 0, 0, w, h);

    gtk_style_context_restore(ctx_);
  }

  void DrawMenuItemEntry(std::string const& text, WidgetState ws, cairo_t* cr, double w, double h, nux::Rect const& bg_geo)
  {
    gtk_style_context_save(ctx_);
    AddContextClassesForMenuItem(ws);

    auto s = text;
    s.erase(std::remove(s.begin(), s.end(), '_'), s.end());
    auto const& layout = BuildPangoLayout(menu_item_pango_ctx_, s);

    if (ws == WidgetState::PRESSED || ws == WidgetState::BACKDROP_PRESSED)
    {
      PangoAttrList* text_attribs = nullptr;
      pango_parse_markup(text.c_str(), -1, '_', &text_attribs, nullptr, nullptr, nullptr);
      pango_layout_set_attributes(layout, text_attribs);
      pango_attr_list_unref(text_attribs);
    }

    pango_layout_set_width(layout, (w >= 0) ? w * PANGO_SCALE : -1);
    pango_layout_set_height(layout, (h >= 0) ? h * PANGO_SCALE : -1);
    DrawTextBackground(ctx_, cr, layout, bg_geo);
    gtk_render_layout(ctx_, cr, 0, 0, layout);

    gtk_style_context_restore(ctx_);
  }

  void DrawMenuItemIcon(std::string const& icon, WidgetState ws, cairo_t* cr, int size)
  {
    gtk_style_context_save(ctx_);
    AddContextClassesForMenuItem(ws);

    auto* theme = gtk_icon_theme_get_default();
    GtkIconLookupFlags flags = GTK_ICON_LOOKUP_FORCE_SIZE;
    glib::Error error;
    glib::Object<GdkPixbuf> pixbuf(gtk_icon_theme_load_icon(theme, icon.c_str(), size, flags, &error));

    if (error)
    {
      LOG_ERROR(logger) << "Error when loading icon " << icon << " at size "
                        << size << ": " << error;
    }

    if (pixbuf)
      gtk_render_icon(ctx_, cr, pixbuf, 0, 0);

    gtk_style_context_restore(ctx_);
  }

  Style* parent_;
  glib::SignalManager signals_;
  glib::Object<GtkStyleContext> ctx_;
  glib::Object<GSettings> settings_;
  glib::Object<GSettings> usettings_;
  glib::Object<PangoContext> title_pango_ctx_;
  glib::Object<PangoContext> menu_item_pango_ctx_;
  connection::Manager connections_;
  decoration::Border border_;
  decoration::Border input_edges_;
  decoration::Border radius_;
  nux::Color glow_color_;
  float title_alignment_;
  unsigned title_indent_;
  unsigned title_fade_;
  unsigned glow_size_;
};

Style::Ptr const& Style::Get()
{
  static Style::Ptr style(new Style());
  return style;
}

Style::Style()
  : impl_(new Impl(this))
{}

Style::~Style()
{}

Alignment Style::TitleAlignment() const
{
  if (impl_->title_alignment_ == 0.0f)
    return Alignment::LEFT;

  if (impl_->title_alignment_ == 0.5f)
    return Alignment::CENTER;

  if (impl_->title_alignment_ == 1.0f)
    return Alignment::RIGHT;

  return Alignment::FLOATING;
}

float Style::TitleAlignmentValue() const
{
  return impl_->title_alignment_;
}

int Style::TitleIndent() const
{
  return impl_->title_indent_;
}

void Style::DrawSide(Side s, WidgetState ws, cairo_t* cr, double w, double h)
{
  impl_->DrawSide(s, ws, cr, w, h);
}

void Style::DrawTitle(std::string const& t, WidgetState ws, cairo_t* cr, double w, double h, nux::Rect const& bg_geo, GtkStyleContext* ctx)
{
  impl_->DrawTitle(t, ws, cr, w, h, bg_geo, ctx ? ctx : impl_->ctx_);
}

void Style::DrawMenuItem(WidgetState ws, cairo_t* cr, double w, double h)
{
  impl_->DrawMenuItem(ws, cr, w, h);
}

void Style::DrawMenuItemEntry(std::string const& t, WidgetState ws, cairo_t* cr, double w, double h, nux::Rect const& bg_geo)
{
  impl_->DrawMenuItemEntry(t, ws, cr, w, h, bg_geo);
}

void Style::DrawMenuItemIcon(std::string const& i, WidgetState ws, cairo_t* cr, int s)
{
  impl_->DrawMenuItemIcon(i, ws, cr, s);
}

std::string Style::ThemedFilePath(std::string const& basename, std::vector<std::string> const& extra_folders) const
{
  return theme::Settings::Get()->ThemedFilePath(basename, extra_folders);
}

std::string Style::WindowButtonFile(WindowButtonType type, WidgetState state) const
{
  return impl_->WindowButtonFile(type, state);
}

void Style::DrawWindowButton(WindowButtonType type, WidgetState state, cairo_t* cr, double width, double height)
{
  return impl_->DrawWindowButton(type, state, cr, width, height);
}

Border const& Style::Border() const
{
  return impl_->border_;
}

Border const& Style::InputBorder() const
{
  return impl_->input_edges_;
}

Border const& Style::CornerRadius() const
{
  return impl_->radius_;
}

Border Style::Padding(Side s, WidgetState ws) const
{
  return decoration::Border(impl_->GetBorderProperty<gint>(s, ws, "padding-top"),
                            impl_->GetBorderProperty<gint>(s, ws, "padding-left"),
                            impl_->GetBorderProperty<gint>(s, ws, "padding-right"),
                            impl_->GetBorderProperty<gint>(s, ws, "padding-bottom"));
}

nux::Point Style::ShadowOffset() const
{
  return nux::Point(std::max<unsigned>(0, impl_->GetProperty<guint>("shadow-offset-x")),
                    std::max<unsigned>(0, impl_->GetProperty<guint>("shadow-offset-y")));
}

nux::Color Style::ActiveShadowColor() const
{
  std::shared_ptr<GdkRGBA> rgba(impl_->GetProperty<GdkRGBA*>("active-shadow-color"), gdk_rgba_free);
  return ColorFromGdkRGBA(rgba.get(), DEFAULT_ACTIVE_SHADOW_COLOR);
}

unsigned Style::ActiveShadowRadius() const
{
  return std::max<unsigned>(0, impl_->GetProperty<guint>("active-shadow-radius"));
}

nux::Color Style::InactiveShadowColor() const
{
  std::shared_ptr<GdkRGBA> rgba(impl_->GetProperty<GdkRGBA*>("inactive-shadow-color"), gdk_rgba_free);
  return ColorFromGdkRGBA(rgba.get(), DEFAULT_INACTIVE_SHADOW_COLOR);
}

unsigned Style::InactiveShadowRadius() const
{
  return std::max<unsigned>(0, impl_->GetProperty<guint>("inactive-shadow-radius"));
}

unsigned Style::GlowSize() const
{
  return impl_->glow_size_;
}

nux::Color const& Style::GlowColor() const
{
  return impl_->glow_color_;
}

WMAction Style::WindowManagerAction(WMEvent event) const
{
  return impl_->WindowManagerAction(event);
}

int Style::DoubleClickMaxDistance() const
{
  return gtk::GetSettingValue<int>("gtk-double-click-distance");
}

int Style::DoubleClickMaxTimeDelta() const
{
  return gtk::GetSettingValue<int>("gtk-double-click-time");
}

nux::Size Style::TitleNaturalSize(std::string const& text)
{
  return impl_->TextNaturalSize(impl_->title_pango_ctx_, text);
}

nux::Size Style::MenuItemNaturalSize(std::string const& text)
{
  auto s = text;
  s.erase(std::remove(s.begin(), s.end(), '_'), s.end());
  return impl_->TextNaturalSize(impl_->menu_item_pango_ctx_, s);
}

} // decoration namespace
} // unity namespace