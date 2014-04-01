// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2014 Canonical Ltd
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

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <glib/gi18n-lib.h>
#include <NuxCore/Logger.h>
#include <UnityCore/GLibWrapper.h>
#include "DecorationsForceQuitDialog.h"
#include "DecorationStyle.h"

namespace unity
{
namespace decoration
{
namespace
{
DECLARE_LOGGER(logger, "unity.decoration.forcequit.dialog");

const gchar* CLOSE_BUTTON_INACTIVE_FILE = "/usr/share/themes/Radiance/unity/close_focused.svg";
const gchar* CLOSE_BUTTON_FOCUSED_FILE = "/usr/share/themes/Radiance/unity/close_focused_prelight.svg";
const gchar* CLOSE_BUTTON_ACTIVE_FILE = "/usr/share/themes/Radiance/unity/close_focused_pressed.svg";


// Dialog
struct SheetStyleDialog
{
  GtkWindow parent_instance;
};

struct SheetStyleDialogClass
{
  GtkWindowClass parent_class;
};

G_DEFINE_TYPE(SheetStyleDialog, sheet_style_dialog, GTK_TYPE_WINDOW);
static void sheet_style_dialog_init(SheetStyleDialog*) {}
static void sheet_style_dialog_class_init(SheetStyleDialogClass*);

// Box
struct SheetStyleBox
{
  GtkBox parent_instance;
};

struct SheetStyleBoxClass
{
  GtkBoxClass parent_class;
};

G_DEFINE_TYPE(SheetStyleBox, sheet_style_box, GTK_TYPE_BOX);
GtkWidget* sheet_style_box_new();
static void sheet_style_box_init(SheetStyleBox*) {}
static void sheet_style_box_class_init(SheetStyleBoxClass*);

// Close button
struct CloseButtonPrivate
{
  GtkImage* img;
};

struct CloseButton
{
  GtkButton parent_instance;
  CloseButtonPrivate* priv;
};

struct CloseButtonClass
{
  GtkButtonClass parent_class;
};

#define CLOSE_BUTTON_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), close_button_get_type(), CloseButtonPrivate))

G_DEFINE_TYPE(CloseButton, close_button, GTK_TYPE_BUTTON);
GtkWidget* close_button_new();
static void close_button_init(CloseButton*);
static void close_button_class_init(CloseButtonClass*);

// Dialog implementation
GtkWidget* sheet_style_dialog_new(Window parent_xid)
{
  auto* self = GTK_WINDOW(g_object_new(sheet_style_dialog_get_type(), nullptr));
  gtk_window_set_skip_taskbar_hint(self, TRUE);
  gtk_window_set_skip_pager_hint(self, TRUE);
  gtk_window_set_position(self, GTK_WIN_POS_CENTER_ON_PARENT);
  gtk_window_set_type_hint(self, GDK_WINDOW_TYPE_HINT_DIALOG);
  gtk_window_set_decorated(self, FALSE);
  gtk_window_set_resizable(self, FALSE);
  gtk_window_set_title(self, "Force Quit Dialog");

  auto const& deco_style = decoration::Style::Get();
  auto const& offset = deco_style->ShadowOffset();
  int max_offset = std::max(std::abs(offset.x), std::abs(offset.y));
  gtk_container_set_border_width(GTK_CONTAINER(self), deco_style->ActiveShadowRadius()+max_offset);

  auto* screen = gtk_window_get_screen(self);
  gtk_widget_set_visual(GTK_WIDGET(self), gdk_screen_get_rgba_visual(screen));
  gtk_widget_realize(GTK_WIDGET(self));
  gtk_widget_override_background_color(GTK_WIDGET(self), GTK_STATE_FLAG_NORMAL, nullptr);

  gtk_container_add(GTK_CONTAINER(self), sheet_style_box_new());

  gtk_window_set_modal(self, TRUE);
  gtk_window_set_destroy_with_parent(self, TRUE);

  auto* gwindow = gtk_widget_get_window(GTK_WIDGET(self));
  gdk_window_set_functions(gwindow, GDK_FUNC_CLOSE);
  gtk_widget_realize(GTK_WIDGET(self));

  gdk_error_trap_push();
  auto* dpy = gdk_x11_get_default_xdisplay();
  auto xid = gdk_x11_window_get_xid(gwindow);
  XSetTransientForHint(dpy, xid, parent_xid);
  XSync(dpy, False);

  if (int error_code = gdk_error_trap_pop())
  {
    gchar tmp[1024];
    XGetErrorText(dpy, error_code, tmp, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';
    LOG_ERROR(logger) << "Impossible to reparent dialog: " << tmp;
  }

  return GTK_WIDGET(self);
}

static void sheet_style_dialog_class_init(SheetStyleDialogClass* klass)
{
  GTK_WIDGET_CLASS(klass)->draw = [] (GtkWidget* self, cairo_t* cr) {
    GtkAllocation a;
    gtk_widget_get_allocation(self, &a);
    cairo_rectangle(cr, 0, 0, a.width, a.height);

    cairo_save(cr);
    int border = gtk_container_get_border_width(GTK_CONTAINER(self));
    cairo_translate(cr, border, border);
    gtk_container_forall(GTK_CONTAINER(self), [] (GtkWidget* child, gpointer cr) {
      gtk_widget_draw(child, static_cast<cairo_t*>(cr));
    }, cr);
    cairo_restore(cr);

    return TRUE;
  };

  GTK_WIDGET_CLASS(klass)->size_allocate = [] (GtkWidget* self, GtkAllocation* a) {
    GTK_WIDGET_CLASS(sheet_style_dialog_parent_class)->size_allocate(self, a);

    int border = gtk_container_get_border_width(GTK_CONTAINER(self));
    cairo_rectangle_int_t rect = {border, border, a->width-border*2, a->height-border*2};

    auto* region = cairo_region_create_rectangle(&rect);
    gtk_widget_input_shape_combine_region(GTK_WIDGET(self), region);
    cairo_region_destroy(region);
  };
}

// BOX Implementation
GtkWidget* sheet_style_box_new()
{
  auto* self = GTK_WIDGET(g_object_new(sheet_style_box_get_type(), nullptr));
  gtk_orientable_set_orientation(GTK_ORIENTABLE(self), GTK_ORIENTATION_VERTICAL);

  glib::Object<GtkCssProvider> style(gtk_css_provider_new());

  auto const& deco_style = decoration::Style::Get();
  auto const& radius = deco_style->CornerRadius();
  auto const& offset = deco_style->ShadowOffset();
  auto const& color = deco_style->ActiveShadowColor();
  int decoration_radius = std::max({radius.top, radius.left, radius.right, radius.bottom});

  gtk_css_provider_load_from_data(style, (R"(SheetStyleBox {
    background-color: #f7f6f5;
    color: #4a4a4a;
    border-radius: )"+std::to_string(decoration_radius)+R"(px;
    box-shadow: )"+std::to_string(offset.x)+"px "+std::to_string(offset.y)+"px "+
                   std::to_string(deco_style->ActiveShadowRadius())+"px "+
                   "rgba("+std::to_string(int(color.red * 255.0))+", "+
                           std::to_string(int(color.green * 255.0))+", "+
                           std::to_string(int(color.blue * 255.0))+", "+
                           std::to_string(int(color.alpha))+'.'+std::to_string(int(color.alpha*10000.0))+')'+R"(;
  })").c_str(), -1, nullptr);

  auto* style_ctx = gtk_widget_get_style_context(self);
  gtk_style_context_add_provider(style_ctx, glib::object_cast<GtkStyleProvider>(style), GTK_STYLE_PROVIDER_PRIORITY_FALLBACK);

  auto* main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_container_set_border_width(GTK_CONTAINER(main_box), 4);

  auto *button = close_button_new();
  gtk_box_pack_start(GTK_BOX(main_box), button, TRUE, TRUE, 0);

  auto* content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_container_set_border_width(GTK_CONTAINER(content_box), 10);
  gtk_widget_set_margin_left(content_box, 20);
  gtk_widget_set_margin_right(content_box, 20);
  gtk_container_add(GTK_CONTAINER(main_box), content_box);

  auto* title = gtk_label_new(_("This window is not responding"));
  auto* font_desc = pango_font_description_from_string("Ubuntu 17");
  gtk_widget_override_font(title, font_desc);
  pango_font_description_free(font_desc);
  gtk_widget_set_halign(title, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(content_box), title, FALSE, FALSE, 0);

  auto* subtitle = gtk_label_new(_("Do you want to force the application to exit, or wait for it to respond?"));
  gtk_widget_set_halign(title, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(content_box), subtitle, FALSE, FALSE, 0);

  auto* buttons_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_box_set_spacing(GTK_BOX(buttons_box), 15);
  gtk_container_set_border_width(GTK_CONTAINER(buttons_box), 5);
  gtk_button_box_set_layout(GTK_BUTTON_BOX(buttons_box), GTK_BUTTONBOX_END);

  auto* wait_button = gtk_button_new_with_mnemonic(_("_Wait"));
  gtk_container_add(GTK_CONTAINER(buttons_box), wait_button);

  auto* quit_button = gtk_button_new_with_mnemonic(_("_Force Quit"));
  gtk_container_add(GTK_CONTAINER(buttons_box), quit_button);

  auto* buttons_aligment = gtk_alignment_new(1, 1, 0, 0);
  gtk_alignment_set_padding(GTK_ALIGNMENT(buttons_aligment), 20, 0, 0, 0);
  gtk_container_add(GTK_CONTAINER(buttons_aligment), buttons_box);
  gtk_container_add(GTK_CONTAINER(content_box), buttons_aligment);

  gtk_container_add(GTK_CONTAINER(self), main_box);

  return self;
}

static void sheet_style_box_class_init(SheetStyleBoxClass* klass)
{
  GTK_WIDGET_CLASS(klass)->draw = [] (GtkWidget* self, cairo_t* cr) {
    GtkAllocation a;
    gtk_widget_get_allocation(self, &a);
    gtk_render_background(gtk_widget_get_style_context(self), cr, 0, 0, a.width, a.height);
    return GTK_WIDGET_CLASS(sheet_style_box_parent_class)->draw(self, cr);
  };
}

// Close button
GtkWidget* close_button_new()
{
  auto* self = GTK_WIDGET(g_object_new(close_button_get_type(), nullptr));
  gtk_button_set_relief(GTK_BUTTON(self), GTK_RELIEF_NONE);
  gtk_button_set_focus_on_click(GTK_BUTTON(self), FALSE);
  gtk_widget_set_can_focus(self, FALSE);
  gtk_widget_set_halign(self, GTK_ALIGN_START);

  auto* img = gtk_image_new_from_file(CLOSE_BUTTON_INACTIVE_FILE);
  gtk_container_add(GTK_CONTAINER(self), img);
  CLOSE_BUTTON_GET_PRIVATE(self)->img = GTK_IMAGE(img);

  glib::Object<GtkCssProvider> style(gtk_css_provider_new());
  gtk_css_provider_load_from_data(style, R"(
    * {padding: 0px 0px 0px 0px; border: 0px; }
  )", -1, nullptr);

  auto* style_ctx = gtk_widget_get_style_context(self);
  gtk_style_context_add_provider(style_ctx, glib::object_cast<GtkStyleProvider>(style), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  return self;
}

static void close_button_init(CloseButton* self)
{
  self->priv = CLOSE_BUTTON_GET_PRIVATE(self);
}

static void close_button_class_init(CloseButtonClass* klass)
{
  GTK_WIDGET_CLASS(klass)->draw = [] (GtkWidget* self, cairo_t* cr) {
    gtk_widget_draw(GTK_WIDGET(CLOSE_BUTTON_GET_PRIVATE(self)->img), cr);
    return TRUE;
  };

  GTK_WIDGET_CLASS(klass)->state_flags_changed = [] (GtkWidget* self, GtkStateFlags prev_state) {
    auto new_flags = gtk_widget_get_state_flags(self);
    const gchar* file = CLOSE_BUTTON_INACTIVE_FILE;

    if (((new_flags & GTK_STATE_FLAG_PRELIGHT) && !gtk_widget_get_can_focus(self)) ||
        (new_flags & GTK_STATE_FLAG_FOCUSED))
    {
      file = (new_flags & GTK_STATE_FLAG_ACTIVE) ? CLOSE_BUTTON_ACTIVE_FILE : CLOSE_BUTTON_FOCUSED_FILE;
    }

    gtk_image_set_from_file(CLOSE_BUTTON_GET_PRIVATE(self)->img, file);

    return GTK_WIDGET_CLASS(close_button_parent_class)->state_flags_changed(self, prev_state);
  };
}

}

struct ForceQuitDialog::Impl
{
  Impl(CompWindow* win)
    : win_(win)
    , dialog_(sheet_style_dialog_new())
  {
    gtk_widget_show_all(glib::object_cast<GtkWidget>(dialog_));
  }

  ~Impl()
  {
    gtk_widget_destroy(glib::object_cast<GtkWidget>(dialog_));
  }

  CompWindow* win_;
  glib::Object<GtkWindow> dialog_;
};

ForceQuitDialog::ForceQuitDialog(CompWindow* win)
  : impl_(new Impl(win))
{}

ForceQuitDialog::~ForceQuitDialog()
{}

} // decoration namespace
} // unity namespace
