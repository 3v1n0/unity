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
const std::array<int, 4> DEFAULT_BORDERS = {28, 1, 1, 1};

typedef struct _UnityDecorationPrivate UnityDecorationPrivate;
struct UnityDecoration
{
  GtkWidget parent_instance;
  UnityDecorationPrivate* priv;
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

  param = g_param_spec_float("title-alignment", "Title Alignment", "", 0.0, 1.0, 0.0, G_PARAM_READABLE);
  gtk_widget_class_install_style_property(GTK_WIDGET_CLASS(klass), param);
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
    border_size_[unsigned(Side::TOP)] = b ? b->top : DEFAULT_BORDERS[unsigned(Side::TOP)];
    border_size_[unsigned(Side::LEFT)] = b ? b->left : DEFAULT_BORDERS[unsigned(Side::LEFT)];
    border_size_[unsigned(Side::RIGHT)] = b ? b->right : DEFAULT_BORDERS[unsigned(Side::RIGHT)];
    border_size_[unsigned(Side::BOTTOM)] = b ? b->bottom : DEFAULT_BORDERS[unsigned(Side::BOTTOM)];

    title_alignment_ = GetProperty<gfloat>("title-alignment");
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
  inline TYPE GetBorderProperty(Side s, GtkStateFlags flags, std::string const& property)
  {
    TYPE value;

    gtk_style_context_save(ctx_);
    gtk_style_context_add_class(ctx_, GetBorderClass(s).c_str());
    gtk_style_context_get(ctx_, flags, property.c_str(), &value, nullptr);
    gtk_style_context_restore(ctx_);

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

  glib::Object<GtkStyleContext> ctx_;
  std::array<int, 4> border_size_;
  float title_alignment_;
};

Style::Ptr Style::Get()
{
  static Style::Ptr style(new Style());
  return style;
}

Style::Style()
  : impl_(new Impl())
{}

Style::~Style()
{}

int Style::BorderWidth(Side side) const
{
  return impl_->border_size_[unsigned(side)];
}

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

} // decoration namespace
} // unity namespace