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
#include <X11/Xatom.h>
#include "DecorationsForceQuitDialog.h"
#include "DecorationStyle.h"
#include "WindowManager.h"

namespace unity
{
namespace decoration
{
namespace
{
DECLARE_LOGGER(logger, "unity.decoration.forcequit.dialog");

const std::string CLOSE_BUTTON_INACTIVE_FILE = "sheet_style_close_focused";
const std::string CLOSE_BUTTON_FOCUSED_FILE = "sheet_style_close_focused_prelight";
const std::string CLOSE_BUTTON_ACTIVE_FILE = "sheet_style_close_focused_pressed";


// Dialog
struct SheetStyleWindow
{
  GtkWindow parent_instance;
};

struct SheetStyleWindowClass
{
  GtkWindowClass parent_class;
};

G_DEFINE_TYPE(SheetStyleWindow, sheet_style_window, GTK_TYPE_WINDOW);
static void sheet_style_window_init(SheetStyleWindow*) {}
static void sheet_style_window_class_init(SheetStyleWindowClass*);

// Box
struct SheetStyleDialog
{
  GtkBox parent_instance;
};

struct SheetStyleDialogClass
{
  GtkBoxClass parent_class;
};

G_DEFINE_TYPE(SheetStyleDialog, sheet_style_dialog, GTK_TYPE_BOX);
GtkWidget* sheet_style_dialog_new(ForceQuitDialog*, Window, long);
static void sheet_style_dialog_init(SheetStyleDialog*) {}
static void sheet_style_dialog_class_init(SheetStyleDialogClass*);

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

#define CLOSE_BUTTON(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), close_button_get_type(), CloseButton))

G_DEFINE_TYPE(CloseButton, close_button, GTK_TYPE_BUTTON);
GtkWidget* close_button_new();
static void close_button_init(CloseButton*);
static void close_button_class_init(CloseButtonClass*);

bool gdk_error_trap_pop_with_output(std::string const& prefix)
{
  if (int error_code = gdk_error_trap_pop())
  {
    gchar tmp[1024];
    XGetErrorText(gdk_x11_get_default_xdisplay(), error_code, tmp, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';
    LOG_ERROR(logger) << (prefix.empty() ? "X error: " : prefix+": ") << tmp;
    return true;
  }

  return false;
}

// Window implementation
GtkWidget* sheet_style_window_new(ForceQuitDialog* main_dialog, Window parent_xid)
{
  auto* dpy = gdk_x11_get_default_xdisplay();
  auto* self = GTK_WINDOW(g_object_new(sheet_style_window_get_type(), nullptr));
  gtk_window_set_skip_taskbar_hint(self, TRUE);
  gtk_window_set_skip_pager_hint(self, TRUE);
  gtk_window_set_position(self, GTK_WIN_POS_CENTER_ON_PARENT);
  gtk_window_set_type_hint(self, GDK_WINDOW_TYPE_HINT_DIALOG);
  gtk_window_set_decorated(self, FALSE);
  gtk_window_set_resizable(self, FALSE);
  gtk_window_set_urgency_hint(self, TRUE);
  gtk_window_set_deletable(self, FALSE);
  gtk_window_set_title(self, "Force Quit Dialog");

  gdk_error_trap_push();
  XClassHint parent_class = {nullptr, nullptr};
  XGetClassHint(dpy, parent_xid, &parent_class);

  if (!gdk_error_trap_pop_with_output("Impossible to get window class"))
    gtk_window_set_wmclass(self, parent_class.res_name, parent_class.res_class);

  XFree(parent_class.res_class);
  XFree(parent_class.res_name);

  Atom WM_PID = gdk_x11_get_xatom_by_name("_NET_WM_PID");
  Atom WM_CLIENT_MACHINE = gdk_x11_get_xatom_by_name("WM_CLIENT_MACHINE");
  Atom WM_CLIENT_LEADER = gdk_x11_get_xatom_by_name("WM_CLIENT_LEADER");

  gdk_error_trap_push();
  auto& wm = WindowManager::Default();
  auto parent_hostname = wm.GetStringProperty(parent_xid, WM_CLIENT_MACHINE);
  long parent_pid = 0;

  char current_hostname[256];
  if (gethostname(current_hostname, sizeof(current_hostname)) > -1)
  {
    current_hostname[sizeof(current_hostname)-1] = '\0';

    if (current_hostname == parent_hostname)
    {
      auto const& pid_list = wm.GetCardinalProperty(parent_xid, WM_PID);

      if (!pid_list.empty())
        parent_pid = pid_list.front();
    }
  }

  gdk_error_trap_pop_with_output("Impossible to get window client machine and PID");

  auto const& deco_style = decoration::Style::Get();
  auto const& offset = deco_style->ShadowOffset();
  int max_offset = std::max(std::abs(offset.x * 4), std::abs(offset.y * 4));
  gtk_container_set_border_width(GTK_CONTAINER(self), deco_style->ActiveShadowRadius()+max_offset);

  auto* screen = gtk_window_get_screen(self);
  gtk_widget_set_visual(GTK_WIDGET(self), gdk_screen_get_rgba_visual(screen));
  gtk_widget_realize(GTK_WIDGET(self));

  glib::Object<GtkCssProvider> style(gtk_css_provider_new());
  gtk_css_provider_load_from_data(style, R"(
    * { background-color: transparent; }
  )", -1, nullptr);

  auto* style_ctx = gtk_widget_get_style_context(GTK_WIDGET(self));
  gtk_style_context_add_provider(style_ctx, glib::object_cast<GtkStyleProvider>(style), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  gtk_container_add(GTK_CONTAINER(self), sheet_style_dialog_new(main_dialog, parent_xid, parent_pid));

  gtk_window_set_modal(self, TRUE);
  gtk_window_set_destroy_with_parent(self, TRUE);

  auto* gwindow = gtk_widget_get_window(GTK_WIDGET(self));
  gdk_window_set_functions(gwindow, GDK_FUNC_CLOSE);
  gtk_widget_realize(GTK_WIDGET(self));

  gdk_error_trap_push();
  auto xid = gdk_x11_window_get_xid(gwindow);
  XSetTransientForHint(dpy, xid, parent_xid);
  XSync(dpy, False);
  gdk_error_trap_pop_with_output("Impossible to reparent dialog");

  XChangeProperty(dpy, xid, WM_CLIENT_MACHINE, XA_STRING, 8, PropModeReplace,
                  (unsigned char *) parent_hostname.c_str(), parent_hostname.size());
  XChangeProperty(dpy, xid, WM_PID, XA_CARDINAL, 32, PropModeReplace,
                  (unsigned char *) &parent_pid, 1);
  XChangeProperty(dpy, xid, WM_CLIENT_LEADER, XA_WINDOW, 32, PropModeReplace,
                  (unsigned char *) &parent_xid, 1);
  XSync(dpy, False);

  return GTK_WIDGET(self);
}

static void sheet_style_window_class_init(SheetStyleWindowClass* klass)
{
  GTK_WIDGET_CLASS(klass)->delete_event = [] (GtkWidget* self, GdkEventAny*) {
    // Don't destroy the window on delete event
    return TRUE;
  };

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
    GTK_WIDGET_CLASS(sheet_style_window_parent_class)->size_allocate(self, a);

    int border = gtk_container_get_border_width(GTK_CONTAINER(self));
    cairo_rectangle_int_t rect = {border, border, a->width-border*2, a->height-border*2};

    auto* region = cairo_region_create_rectangle(&rect);
    gtk_widget_input_shape_combine_region(GTK_WIDGET(self), region);
    cairo_region_destroy(region);
  };
}

// Dialog content Implementation
void on_force_quit_clicked(GtkButton *button, gint64* kill_data)
{
  Display* dpy = gdk_x11_get_default_xdisplay();
  GtkWidget* top_level = gtk_widget_get_toplevel(GTK_WIDGET(button));
  Window parent_xid = kill_data[0];
  long parent_pid = kill_data[1];

  gtk_widget_hide(top_level);

  gdk_error_trap_push();
  XSync(dpy, False);
  XKillClient(dpy, parent_xid);
  XSync(dpy, False);
  gdk_error_trap_pop_with_output("Impossible to kill window "+std::to_string(parent_xid));

  if (parent_pid > 0)
    kill(parent_pid, 9);
}

void on_wait_clicked(GtkButton *button, ForceQuitDialog* main_dialog)
{
  main_dialog->close_request.emit();
}

GtkWidget* sheet_style_dialog_new(ForceQuitDialog* main_dialog, Window parent_xid, long parent_pid)
{
  auto* self = GTK_WIDGET(g_object_new(sheet_style_dialog_get_type(), nullptr));
  gtk_orientable_set_orientation(GTK_ORIENTABLE(self), GTK_ORIENTATION_VERTICAL);

  glib::Object<GtkCssProvider> style(gtk_css_provider_new());

  auto const& deco_style = decoration::Style::Get();
  auto const& radius = deco_style->CornerRadius();
  auto const& offset = deco_style->ShadowOffset();
  auto const& color = deco_style->ActiveShadowColor();
  auto const& backcolor = deco_style->InactiveShadowColor();
  int decoration_radius = std::max({radius.top, radius.left, radius.right, radius.bottom});

  gtk_css_provider_load_from_data(style, (R"(
  SheetStyleDialog, sheet-style-dialog {
    background-color: #f7f6f5;
    color: #4a4a4a;
    border-radius: )"+std::to_string(decoration_radius)+R"(px;
    box-shadow: )"+std::to_string(2 * offset.x)+"px "+std::to_string(2 * offset.y)+"px "+
                   std::to_string(deco_style->ActiveShadowRadius())+"px "+
                   "rgba("+std::to_string(int(color.red * 255.0))+", "+
                           std::to_string(int(color.green * 255.0))+", "+
                           std::to_string(int(color.blue * 255.0))+", "+
                           std::to_string(int(color.alpha))+'.'+std::to_string(int(color.alpha*10000.0))+')'+R"(;
  }

  SheetStyleDialog:backdrop, sheet-style-dialog:backdrop {
    background-color: shade(#f7f6f5, 1.2);
    color: shade(#4a4a4a, 1.5);
    border-radius: )"+std::to_string(decoration_radius)+R"(px;
    box-shadow: )"+std::to_string(2 * offset.x)+"px "+std::to_string(2 * offset.y)+"px "+
                   std::to_string(deco_style->InactiveShadowRadius())+"px "+
                   "rgba("+std::to_string(int(backcolor.red * 255.0))+", "+
                           std::to_string(int(backcolor.green * 255.0))+", "+
                           std::to_string(int(backcolor.blue * 255.0))+", "+
                           std::to_string(int(backcolor.alpha))+'.'+std::to_string(int(backcolor.alpha*10000.0))+')'+R"(;
  })").c_str(), -1, nullptr);

  auto* style_ctx = gtk_widget_get_style_context(self);
  gtk_style_context_add_provider(style_ctx, glib::object_cast<GtkStyleProvider>(style), GTK_STYLE_PROVIDER_PRIORITY_FALLBACK);
  gtk_style_context_add_class(style_ctx, "unity-force-quit");

  auto* main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_container_set_border_width(GTK_CONTAINER(main_box), 4);

  auto *close_button = close_button_new();
  g_signal_connect(close_button, "clicked", G_CALLBACK(on_wait_clicked), main_dialog);
  gtk_box_pack_start(GTK_BOX(main_box), close_button, TRUE, TRUE, 0);

  auto* content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_container_set_border_width(GTK_CONTAINER(content_box), 10);
#if GTK_CHECK_VERSION(3, 11, 0)
  gtk_widget_set_margin_start(content_box, 20);
  gtk_widget_set_margin_end(content_box, 20);
#else
  gtk_widget_set_margin_left(content_box, 20);
  gtk_widget_set_margin_right(content_box, 20);
#endif
  gtk_container_add(GTK_CONTAINER(main_box), content_box);

  auto* title = gtk_label_new(_("This window is not responding"));
  glib::Object<GtkCssProvider> title_style(gtk_css_provider_new());
  gtk_css_provider_load_from_data(title_style, (R"(* { font-size: 17px; })"), -1, nullptr);
  style_ctx = gtk_widget_get_style_context(title);
  gtk_style_context_add_provider(style_ctx, glib::object_cast<GtkStyleProvider>(title_style), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  gtk_style_context_add_class(style_ctx, "unity-force-quit");

  gtk_widget_set_halign(title, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(content_box), title, FALSE, FALSE, 0);

  auto* subtitle = gtk_label_new(_("Do you want to force the application to exit, or wait for it to respond?"));
  gtk_widget_set_halign(title, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(content_box), subtitle, FALSE, FALSE, 0);

  auto* buttons_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_box_set_spacing(GTK_BOX(buttons_box), 15);
  gtk_container_set_border_width(GTK_CONTAINER(buttons_box), 5);
  gtk_button_box_set_layout(GTK_BUTTON_BOX(buttons_box), GTK_BUTTONBOX_END);
  gtk_widget_set_halign(GTK_WIDGET(buttons_box), GTK_ALIGN_END);
  gtk_widget_set_valign(GTK_WIDGET(buttons_box), GTK_ALIGN_END);
  gtk_widget_set_margin_top(GTK_WIDGET(buttons_box), 20);

  auto* wait_button = gtk_button_new_with_mnemonic(_("_Wait"));
  gtk_container_add(GTK_CONTAINER(buttons_box), wait_button);
  g_signal_connect(wait_button, "clicked", G_CALLBACK(on_wait_clicked), main_dialog);

  auto *kill_data = g_new(gint64, 2);
  kill_data[0] = parent_xid;
  kill_data[1] = parent_pid;
  auto* quit_button = gtk_button_new_with_mnemonic(_("_Force Quit"));
  gtk_container_add(GTK_CONTAINER(buttons_box), quit_button);
  g_signal_connect_data(quit_button, "clicked", G_CALLBACK(on_force_quit_clicked),
                        kill_data, [] (gpointer data, GClosure*) { g_free(data); },
                        static_cast<GConnectFlags>(0));

  gtk_container_add(GTK_CONTAINER(content_box), buttons_box);
  gtk_container_add(GTK_CONTAINER(self), main_box);

  return self;
}

static void sheet_style_dialog_class_init(SheetStyleDialogClass* klass)
{
  GTK_WIDGET_CLASS(klass)->draw = [] (GtkWidget* self, cairo_t* cr) {
    GtkAllocation a;
    gtk_widget_get_allocation(self, &a);
    gtk_render_background(gtk_widget_get_style_context(self), cr, 0, 0, a.width, a.height);
    return GTK_WIDGET_CLASS(sheet_style_dialog_parent_class)->draw(self, cr);
  };

#if GTK_CHECK_VERSION(3, 20, 0)
  gtk_widget_class_set_css_name(GTK_WIDGET_CLASS(klass), "sheet-style-dialog");
#endif
}

// Close button
GtkWidget* close_button_new()
{
  auto* self = GTK_WIDGET(g_object_new(close_button_get_type(), nullptr));
  gtk_button_set_relief(GTK_BUTTON(self), GTK_RELIEF_NONE);
  gtk_widget_set_can_focus(self, FALSE);
  gtk_widget_set_halign(self, GTK_ALIGN_START);

  auto const& file = decoration::Style::Get()->ThemedFilePath(CLOSE_BUTTON_INACTIVE_FILE, {PKGDATADIR});
  auto* img = gtk_image_new_from_file(file.c_str());
  gtk_container_add(GTK_CONTAINER(self), img);
  CLOSE_BUTTON(self)->priv->img = GTK_IMAGE(img);

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
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, close_button_get_type(), CloseButtonPrivate);
}

static void close_button_class_init(CloseButtonClass* klass)
{
  GTK_WIDGET_CLASS(klass)->draw = [] (GtkWidget* self, cairo_t* cr) {
    gtk_widget_draw(GTK_WIDGET(CLOSE_BUTTON(self)->priv->img), cr);
    return TRUE;
  };

  GTK_WIDGET_CLASS(klass)->state_flags_changed = [] (GtkWidget* self, GtkStateFlags prev_state) {
    auto* img = CLOSE_BUTTON(self)->priv->img;
    if (!img) return;

    auto new_flags = gtk_widget_get_state_flags(self);
    auto const& deco_style = decoration::Style::Get();
    auto file = deco_style->ThemedFilePath(CLOSE_BUTTON_INACTIVE_FILE, {PKGDATADIR});

    if (((new_flags & GTK_STATE_FLAG_PRELIGHT) && !gtk_widget_get_can_focus(self)) ||
        (new_flags & GTK_STATE_FLAG_FOCUSED))
    {
      auto const& basename = (new_flags & GTK_STATE_FLAG_ACTIVE) ? CLOSE_BUTTON_ACTIVE_FILE : CLOSE_BUTTON_FOCUSED_FILE;
      file = deco_style->ThemedFilePath(basename, {PKGDATADIR});
    }

    gtk_image_set_from_file(img, file.c_str());

    return GTK_WIDGET_CLASS(close_button_parent_class)->state_flags_changed(self, prev_state);
  };

  G_OBJECT_CLASS(klass)->finalize = [] (GObject* self) {
    CLOSE_BUTTON(self)->priv->img = nullptr;
    return G_OBJECT_CLASS(close_button_parent_class)->finalize(self);
  };
}

}

struct ForceQuitDialog::Impl : sigc::trackable
{
  Impl(ForceQuitDialog* parent, CompWindow* win)
    : parent_(parent)
    , win_(win)
    , dialog_(sheet_style_window_new(parent, win_->id()))
  {
    parent_->time.changed.connect(sigc::mem_fun(this, &Impl::UpdateWindowTime));
    UpdateWindowTime(parent_->time());
  }

  ~Impl()
  {
    gtk_widget_destroy(dialog_);
  }

  void UpdateWindowTime(Time time)
  {
    auto gwindow = gtk_widget_get_window(dialog_);
    gdk_x11_window_set_user_time(gwindow, time);
    gtk_widget_show_all(dialog_);

    auto* dpy = gdk_x11_get_default_xdisplay();
    auto xid = gdk_x11_window_get_xid(gwindow);
    if (XWMHints *wmhints = XGetWMHints(dpy, xid))
    {
      wmhints->window_group = win_->id();
      XSetWMHints(dpy, xid, wmhints);
      XFree(wmhints);
    }
  }

  void UpdateDialogPosition()
  {
    auto const& win_geo = win_->inputRect();
    nux::Size walloc(gtk_widget_get_allocated_width(dialog_), gtk_widget_get_allocated_height(dialog_));
    gtk_window_move(GTK_WINDOW(dialog_), win_geo.centerX() - walloc.width/2, win_geo.centerY() - walloc.height/2);
  }

  ForceQuitDialog* parent_;
  CompWindow* win_;
  GtkWidget* dialog_;
};

ForceQuitDialog::ForceQuitDialog(CompWindow* win, Time tm)
  : time(tm)
  , impl_(new Impl(this, win))
{}

ForceQuitDialog::~ForceQuitDialog()
{}

void ForceQuitDialog::UpdateDialogPosition()
{
  impl_->UpdateDialogPosition();
}

} // decoration namespace
} // unity namespace
