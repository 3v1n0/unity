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

#include "DecorationStyle.h"
#include <gtk/gtk.h>
#include <NuxCore/Colors.h>
#include <NuxCore/Logger.h>
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/GLibSignal.h>
#include <math.h>

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

const std::array<std::string, size_t(WindowButtonType::Size)> WBUTTON_NAMES = { "close", "minimize", "unmaximize", "maximize" };
const std::array<std::string, size_t(WidgetState::Size)> WBUTTON_STATES = {"", "_focused_prelight", "_focused_pressed", "_unfocused",
                                                                           "_unfocused", "_unfocused_prelight", "_unfocused_pressed" };

const std::string SETTINGS_NAME = "org.gnome.desktop.wm.preferences";
const std::string FONT_KEY = "titlebar-font";
const std::string HIGH_CONTRAST_THEME_PREFIX = "HighContrast";

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

  param = g_param_spec_float("title-alignment", "Title Alignment", "", 0.0, 1.0, 0.0, G_PARAM_READABLE);
  gtk_widget_class_install_style_property(GTK_WIDGET_CLASS(klass), param);

  param = g_param_spec_uint("title-indent", "Title Indent", "", 0, G_MAXUINT, 10, G_PARAM_READABLE);
  gtk_widget_class_install_style_property(GTK_WIDGET_CLASS(klass), param);

  param = g_param_spec_uint("title-fade", "Title Fading Pixels", "", 0, G_MAXUINT, 35, G_PARAM_READABLE);
  gtk_widget_class_install_style_property(GTK_WIDGET_CLASS(klass), param);
}

Border BorderFromGtkBorder(GtkBorder* b, Border fallback = Border())
{
  if (!b)
    return fallback;

  return Border(b->top, b->left, b->right, b->bottom);
}
}

struct Style::Impl
{
  Impl(Style* parent)
    : parent_(parent)
    , ctx_(gtk_style_context_new())
    , settings_(g_settings_new(SETTINGS_NAME.c_str()))
    , pango_context_(gdk_pango_context_get_for_screen(gdk_screen_get_default()))
    , title_alignment_(0)
    , title_indent_(0)
    , title_fade_(0)
  {
    std::shared_ptr<GtkWidgetPath> widget_path(gtk_widget_path_new(), gtk_widget_path_free);
    gtk_widget_path_append_type(widget_path.get(), unity_decoration_get_type());
    gtk_style_context_set_path(ctx_, widget_path.get());

    std::shared_ptr<GtkBorder> b(GetProperty<GtkBorder*>("extents"), gtk_border_free);
    border_ = BorderFromGtkBorder(b.get(), DEFAULT_BORDER);

    b.reset(GetProperty<GtkBorder*>("input-extents"), gtk_border_free);
    input_edges_ = BorderFromGtkBorder(b.get(), DEFAULT_INPUT_EDGES);

    radius_.top = GetBorderProperty<int>(Side::TOP, WidgetState::NORMAL, "border-radius");
    radius_.left = GetBorderProperty<int>(Side::LEFT, WidgetState::NORMAL, "border-radius");
    radius_.right = GetBorderProperty<int>(Side::RIGHT, WidgetState::NORMAL, "border-radius");
    radius_.bottom = GetBorderProperty<int>(Side::BOTTOM, WidgetState::NORMAL, "border-radius");

    title_alignment_ = std::min(1.0f, std::max(0.0f, GetProperty<gfloat>("title-alignment")));
    title_indent_ = std::max<unsigned>(0, GetProperty<guint>("title-indent"));
    title_fade_ = std::max<unsigned>(0, GetProperty<guint>("title-fade"));
    parent_->theme = glib::String(GetSettingValue<gchar*>("gtk-theme-name")).Str();
    parent_->font = glib::String(GetSettingValue<gchar*>("gtk-font-name")).Str();
    parent_->title_font = glib::String(g_settings_get_string(settings_, FONT_KEY.c_str())).Str();
    UpdatePangoContext(parent_->title_font);

    GtkSettings* settings = gtk_settings_get_default();
    signals_.Add<void, GtkSettings*, GParamSpec*>(settings, "notify::gtk-theme-name", [this] (GtkSettings*, GParamSpec*) {
      gtk_style_context_invalidate(ctx_);
      parent_->theme = glib::String(GetSettingValue<gchar*>("gtk-theme-name")).Str();
      LOG_INFO(logger) << "gtk-theme-name changed to " << parent_->theme();
    });

    signals_.Add<void, GtkSettings*, GParamSpec*>(settings, "notify::gtk-font-name", [this] (GtkSettings*, GParamSpec*) {
      parent_->font = glib::String(GetSettingValue<gchar*>("gtk-font-name")).Str();
      LOG_INFO(logger) << "gtk-font-name changed to " << parent_->font();
    });

    signals_.Add<void, GtkSettings*, GParamSpec*>(settings, "notify::gtk-xft-dpi", [this] (GtkSettings*, GParamSpec*) {
      pango_context_ = gdk_pango_context_get_for_screen(gdk_screen_get_default());
      UpdatePangoContext(parent_->title_font);
      gtk_style_context_invalidate(ctx_);
      parent_->theme.changed.emit(parent_->theme());
      LOG_INFO(logger) << "gtk-xft-dpi changed to " << GetSettingValue<int>("gtk-xft-dpi");
    });

    signals_.Add<void, GSettings*, gchar*>(settings_, "changed::" + FONT_KEY, [this] (GSettings*, gchar*) {
      auto const& font = glib::String(g_settings_get_string(settings_, FONT_KEY.c_str())).Str();
      UpdatePangoContext(font);
      parent_->title_font = font;
      LOG_INFO(logger) << FONT_KEY << " changed to " << font;
    });
  }

  void UpdatePangoContext(std::string const& font)
  {
    std::shared_ptr<PangoFontDescription> desc(pango_font_description_from_string(font.c_str()), pango_font_description_free);
    pango_context_set_font_description(pango_context_, desc.get());
    pango_context_set_language(pango_context_, gtk_get_default_language());
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

  template <typename TYPE>
  inline TYPE GetSettingValue(std::string const& name)
  {
    TYPE value;
    g_object_get(gtk_settings_get_default(), name.c_str(), &value, nullptr);
    return value;
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

  void AddContextClasses(Side s, WidgetState ws)
  {
    gtk_style_context_add_class(ctx_, "gnome-panel-menu-bar");
    if (s == Side::TOP) { gtk_style_context_add_class(ctx_, "header-bar"); }
    gtk_style_context_add_class(ctx_, GetBorderClass(s).c_str());
    gtk_style_context_set_state(ctx_, GtkStateFromWidgetState(ws));
  }

  void DrawSide(Side s, WidgetState ws, cairo_t* cr, int w, int h)
  {
    gtk_style_context_save(ctx_);
    AddContextClasses(s, ws);
    gtk_render_background(ctx_, cr, 0, 0, w, h);
    gtk_render_frame(ctx_, cr, 0, 0, w, h);
    gtk_style_context_restore(ctx_);
  }

  std::string WindowButtonFile(WindowButtonType type, WidgetState state) const
  {
    auto const& theme = parent_->theme();
    auto filename = WBUTTON_NAMES[unsigned(type)] + WBUTTON_STATES[unsigned(state)] + ".png";
    glib::String subpath(g_build_filename(theme.c_str(), "unity", filename.c_str(), nullptr));

    // Look in home directory
    const char* home_dir = g_get_home_dir();
    if (home_dir)
    {
      glib::String local_file(g_build_filename(home_dir, ".local", "share", "themes", subpath.Value(), nullptr));

      if (g_file_test(local_file, G_FILE_TEST_EXISTS))
        return local_file.Str();

      glib::String home_file(g_build_filename(home_dir, ".themes", subpath.Value(), nullptr));

      if (g_file_test(home_file, G_FILE_TEST_EXISTS))
        return home_file.Str();
    }

    const char* var = g_getenv("GTK_DATA_PREFIX");
    if (!var)
      var = "/usr";

    glib::String path(g_build_filename(var, "share", "themes", subpath.Value(), nullptr));

    if (g_file_test(path, G_FILE_TEST_EXISTS))
      return path.Str();

    LOG_WARN(logger) << "No Window button file for '"<< subpath.Str() << "'";
    return std::string();
  }

  void DrawWindowButton(WindowButtonType type, WidgetState ws, cairo_t* cr, int width, int height)
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
        color = nux::Color(rgba->red, rgba->green, rgba->blue, rgba->alpha);
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

  glib::Object<PangoLayout> BuildPangoLayout(std::string const& text)
  {
    glib::Object<PangoLayout> layout(pango_layout_new(pango_context_));
    pango_layout_set_height(layout, -1); //avoid wrap lines
    pango_layout_set_text(layout, text.c_str(), -1);
    return layout;
  }

  nux::Size TitleNaturalSize(std::string const& text)
  {
    nux::Size extents;
    auto const& layout = BuildPangoLayout(text);
    pango_layout_context_changed(layout);
    pango_layout_get_pixel_size(layout, &extents.width, &extents.height);

    return extents;
  }

  void DrawTitle(std::string const& text, WidgetState ws, cairo_t* cr, int w, int h)
  {
    gtk_style_context_save(ctx_);
    AddContextClasses(Side::TOP, ws);

    auto const& layout = BuildPangoLayout(text);

    nux::Size extents;
    pango_layout_get_pixel_size(layout, &extents.width, &extents.height);
    pango_layout_set_height(layout, (h > 0) ? h * PANGO_SCALE : -1);

    if (extents.width > w)
    {
      int out_pixels = extents.width - w;
      int fading_width = std::min<int>(title_fade_, out_pixels);

      cairo_push_group(cr);
      gtk_render_layout(ctx_, cr, 0, 0, layout);
      cairo_pop_group_to_source(cr);

      std::shared_ptr<cairo_pattern_t> linpat(cairo_pattern_create_linear(w - fading_width, 0, w, 0),
                                              cairo_pattern_destroy);

      cairo_pattern_add_color_stop_rgba(linpat.get(), 0, 0, 0, 0, 1);
      cairo_pattern_add_color_stop_rgba(linpat.get(), 1, 0, 0, 0, 0);
      cairo_mask(cr, linpat.get());
    }
    else
    {
      pango_layout_set_width(layout, w * PANGO_SCALE);
      gtk_render_layout(ctx_, cr, 0, 0, layout);
    }

    gtk_style_context_restore(ctx_);
  }

  Style* parent_;
  glib::SignalManager signals_;
  glib::Object<GtkStyleContext> ctx_;
  glib::Object<GSettings> settings_;
  glib::Object<PangoContext> pango_context_;
  decoration::Border border_;
  decoration::Border input_edges_;
  decoration::Border radius_;
  float title_alignment_;
  unsigned title_indent_;
  unsigned title_fade_;
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

void Style::DrawSide(Side s, WidgetState ws, cairo_t* cr, int w, int h)
{
  impl_->DrawSide(s, ws, cr, w, h);
}

void Style::DrawTitle(std::string const& t, WidgetState ws, cairo_t* cr, int w, int h)
{
  impl_->DrawTitle(t, ws, cr, w, h);
}

std::string Style::WindowButtonFile(WindowButtonType type, WidgetState state) const
{
  return impl_->WindowButtonFile(type, state);
}

void Style::DrawWindowButton(WindowButtonType type, WidgetState state, cairo_t* cr, int width, int height)
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
  return decoration::Border(impl_->GetBorderProperty<int>(s, ws, "padding-top"),
                            impl_->GetBorderProperty<int>(s, ws, "padding-left"),
                            impl_->GetBorderProperty<int>(s, ws, "padding-right"),
                            impl_->GetBorderProperty<int>(s, ws, "padding-bottom"));
}

int Style::DoubleClickMaxDistance() const
{
  return impl_->GetSettingValue<int>("gtk-double-click-distance");
}

int Style::DoubleClickMaxTimeDelta() const
{
  return impl_->GetSettingValue<int>("gtk-double-click-time");
}

nux::Size Style::TitleNaturalSize(std::string const& text)
{
  return impl_->TitleNaturalSize(text);
}

} // decoration namespace
} // unity namespace