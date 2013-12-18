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
#include <UnityCore/GLibWrapper.h>

namespace unity
{
namespace decoration
{
namespace
{
const std::array<std::string, 4> BORDER_CLASSES = {"top", "left", "right", "bottom"};
const Border DEFAULT_BORDER = {28, 1, 1, 1};
const Border DEFAULT_INPUT_EDGES = {10, 10, 10, 10};

const std::array<std::string, size_t(WindowButtonType::Size)> WBUTTON_NAMES = { "close", "minimize", "unmaximize", "maximize" };
const std::array<std::string, size_t(WidgetState::Size)> WBUTTON_STATES = {"", "_focused_prelight", "_focused_pressed", "_unfocused",
                                                                           "_unfocused", "_unfocused_prelight", "_unfocused_pressed" };

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
  Impl()
    : ctx_(gtk_style_context_new())
    , title_alignment_(0)
  {
    std::shared_ptr<GtkWidgetPath> widget_path(gtk_widget_path_new(), gtk_widget_path_free);
    gtk_widget_path_append_type(widget_path.get(), unity_decoration_get_type());
    gtk_style_context_set_path(ctx_, widget_path.get());

    std::shared_ptr<GtkBorder> b(GetProperty<GtkBorder*>("extents"), gtk_border_free);
    border_ = BorderFromGtkBorder(b.get(), DEFAULT_BORDER);

    b.reset(GetProperty<GtkBorder*>("input-extents"), gtk_border_free);
    input_edges_ = BorderFromGtkBorder(b.get(), DEFAULT_INPUT_EDGES);

    title_alignment_ = GetProperty<gfloat>("title-alignment");
    theme_name_ = glib::String(GetSettingValue<gchar*>("gtk-theme-name")).Str();
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
    gtk_style_context_add_class(ctx_, GetBorderClass(s).c_str());
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

  void DrawSide(Side s, WidgetState ws, cairo_t* cr, int w, int h)
  {
    gtk_style_context_save(ctx_);
    gtk_style_context_add_class(ctx_, GetBorderClass(s).c_str());
    gtk_style_context_set_state(ctx_, GtkStateFromWidgetState(ws));
    gtk_render_background(ctx_, cr, 0, 0, w, h);
    gtk_render_frame(ctx_, cr, 0, 0, w, h);
    gtk_style_context_restore(ctx_);
  }

  std::string WindowButtonFile(WindowButtonType type, WidgetState state) const
  {
    auto filename = WBUTTON_NAMES[unsigned(type)] + WBUTTON_STATES[unsigned(state)] + ".png";
    glib::String subpath(g_build_filename(theme_name_.c_str(), "unity", filename.c_str(), nullptr));

    // Look in home directory
    const char* home_dir = g_get_home_dir();
    if (home_dir)
    {
      glib::String local_file(g_build_filename(home_dir, ".local", "share", "themes", subpath.Value(), nullptr));

      if (g_file_test(local_file, G_FILE_TEST_EXISTS))
        return local_file.Str();

      glib::String home_file(g_build_filename(home_dir, ".themes", theme_name_.c_str(), subpath.Value(), nullptr));

      if (g_file_test(home_file, G_FILE_TEST_EXISTS))
        return home_file.Str();
    }

    const char* var = g_getenv("GTK_DATA_PREFIX");
    if (!var)
      var = "/usr";

    glib::String path(g_build_filename(var, "share", "themes", theme_name_.c_str(), subpath.Value(), nullptr));

    if (g_file_test(path, G_FILE_TEST_EXISTS))
      return path.Str();

    return "";
  }

  glib::Object<GtkStyleContext> ctx_;
  std::string theme_name_;
  decoration::Border border_;
  decoration::Border input_edges_;
  float title_alignment_;
};

Style::Ptr const& Style::Get()
{
  static Style::Ptr style(new Style());
  return style;
}

Style::Style()
  : impl_(new Impl())
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

void Style::DrawSide(Side s, WidgetState ws, cairo_t* cr, int w, int h)
{
  impl_->DrawSide(s, ws, cr, w, h);
}

std::string Style::WindowButtonFile(WindowButtonType type, WidgetState state) const
{
  return impl_->WindowButtonFile(type, state);
}

Border const& Style::Border() const
{
  return impl_->border_;
}

Border const& Style::InputBorder() const
{
  return impl_->input_edges_;
}

Border Style::Padding(Side s, WidgetState ws) const
{
  return decoration::Border(impl_->GetBorderProperty<int>(s, ws, "padding-top"),
                            impl_->GetBorderProperty<int>(s, ws, "padding-left"),
                            impl_->GetBorderProperty<int>(s, ws, "padding-right"),
                            impl_->GetBorderProperty<int>(s, ws, "padding-bottom"));
}

} // decoration namespace
} // unity namespace