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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#include "PanelTray.h"
#include "PanelStyle.h"

#include <NuxCore/Logger.h>

namespace
{
nux::logging::Logger logger("unity.panel");
const std::string SETTINGS_NAME = "com.canonical.Unity.Panel";
const int PADDING = 3;
}

namespace unity
{

PanelTray::PanelTray()
  : View(NUX_TRACKER_LOCATION),
    _window(0),
    _tray(NULL),
    _last_x(0),
    _last_y(0),
    _tray_icon_added_id(0)
{
  _settings = g_settings_new(SETTINGS_NAME.c_str());
  _whitelist = g_settings_get_strv(_settings, "systray-whitelist");

  RealInit();
}

unsigned int
PanelTray::xid ()
{
  if (!_window)
    return 0;

  return gdk_x11_window_get_xid (gtk_widget_get_window (_window));
}

void PanelTray::RealInit()
{
  int panel_height = panel::Style::Instance().panel_height;
  _window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_type_hint(GTK_WINDOW(_window), GDK_WINDOW_TYPE_HINT_DOCK);
  gtk_window_set_has_resize_grip(GTK_WINDOW(_window), FALSE);
  gtk_window_set_keep_above(GTK_WINDOW(_window), TRUE);
  gtk_window_set_skip_pager_hint(GTK_WINDOW(_window), TRUE);
  gtk_window_set_skip_taskbar_hint(GTK_WINDOW(_window), TRUE);
  gtk_window_resize(GTK_WINDOW(_window), 1, panel_height);
  gtk_window_move(GTK_WINDOW(_window), -panel_height,-panel_height);
  gtk_widget_set_name(_window, "UnityPanelApplet");

  gtk_widget_set_visual(_window, gdk_screen_get_rgba_visual(gdk_screen_get_default()));
  gtk_widget_realize(_window);
  gtk_widget_set_app_paintable(_window, TRUE);
  _tray_expose_id = g_signal_connect(_window, "draw", G_CALLBACK(PanelTray::OnTrayDraw), this);

  if (!g_getenv("UNITY_PANEL_TRAY_DISABLE"))
  {
    _tray = na_tray_new_for_screen(gdk_screen_get_default(),
                                   GTK_ORIENTATION_HORIZONTAL,
                                   (NaTrayFilterCallback)FilterTrayCallback,
                                   this);
    na_tray_set_icon_size(_tray, panel_height);

    _tray_icon_added_id = g_signal_connect(na_tray_get_manager(_tray), "tray_icon_removed",
                                           G_CALLBACK(PanelTray::OnTrayIconRemoved), this);

    gtk_container_add(GTK_CONTAINER(_window), GTK_WIDGET(_tray));
    gtk_widget_show(GTK_WIDGET(_tray));
  }

  SetMinMaxSize(1, panel_height);
}

PanelTray::~PanelTray()
{
  if (_tray)
  {
    g_signal_handler_disconnect(na_tray_get_manager(_tray), _tray_icon_added_id);
    _tray = NULL;
  }

  g_idle_remove_by_data(this);

  if (_tray_expose_id)
    g_signal_handler_disconnect(_window, _tray_expose_id);

  gtk_widget_destroy(_window);
  g_strfreev(_whitelist);
  g_object_unref(_settings);
}

void
PanelTray::Draw(nux::GraphicsEngine& gfx_context, bool force_draw)
{
  nux::Geometry geo(GetAbsoluteGeometry());

  gfx_context.PushClippingRectangle(geo);
  nux::GetPainter().PaintBackground(gfx_context, geo);
  gfx_context.PopClippingRectangle();

  if (geo.x != _last_x || geo.y != _last_y)
  {
    _last_x = geo.x;
    _last_y = geo.y;

    gtk_window_move(GTK_WINDOW(_window), geo.x + PADDING, geo.y);
  }
}

void
PanelTray::Sync()
{
  if (_tray)
  {
    SetMinMaxSize(WidthOfTray() + (PADDING * 2), panel::Style::Instance().panel_height);
    QueueRelayout();
    QueueDraw();

    if (_children.size())
      gtk_widget_show(_window);
    else
      gtk_widget_hide(_window);
  }
}

gboolean
PanelTray::FilterTrayCallback(NaTray* tray, NaTrayChild* icon, PanelTray* self)
{
  char* title;
  char* res_name = NULL;
  char* res_class = NULL;
  char* name;
  int   i = 0;
  bool  accept = false;

  title = na_tray_child_get_title(icon);
  na_tray_child_get_wm_class(icon, &res_name, &res_class);

  while ((name = self->_whitelist[i]))
  {
    if (g_strcmp0(name, "all") == 0)
    {
      accept = true;
      break;
    }
    else if (!name || g_strcmp0(name, "") == 0)
    {
      accept = false;
      break;
    }
    else if ((title && g_str_has_prefix(title, name))
             || (res_name && g_str_has_prefix(res_name, name))
             || (res_class && g_str_has_prefix(res_class, name)))
    {
      accept = true;
      break;
    }

    i++;
  }

  if (accept)
  {
    if (na_tray_child_has_alpha(icon))
      na_tray_child_set_composited(icon, TRUE);

    self->_children.push_back(icon);
    g_idle_add((GSourceFunc)IdleSync, self);
  }

  LOG_DEBUG(logger) << "TrayChild "
                    << (accept ? "Accepted: " : "Rejected: ")
                    << na_tray_child_get_title(icon) << " "
                    << res_name << " " << res_class;

  g_free(res_name);
  g_free(res_class);
  g_free(title);

  return accept ? TRUE : FALSE;
}

void
PanelTray::OnTrayIconRemoved(NaTrayManager* manager, NaTrayChild* child, PanelTray* self)
{
  for (auto it = self->_children.begin(); it != self->_children.end(); ++it)
  {
    if (*it == child)
    {
      g_idle_add((GSourceFunc)IdleSync, self);
      self->_children.erase(it);
      break;
    }
  }
}

gboolean
PanelTray::IdleSync(PanelTray* self)
{
  int width = self->WidthOfTray();
  gtk_window_resize(GTK_WINDOW(self->_window), width, panel::Style::Instance().panel_height);
  self->Sync();
  return FALSE;
}

int PanelTray::WidthOfTray()
{
  int width = 0;
  for (auto child: _children)
  {
    int w = gtk_widget_get_allocated_width(GTK_WIDGET(child));
    width += w > 24 ? w : 24;
  }
  return width;
}

gboolean
PanelTray::OnTrayDraw(GtkWidget* widget, cairo_t* cr, PanelTray* tray)
{
  GtkAllocation alloc;

  gtk_widget_get_allocation(widget, &alloc);

  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  cairo_set_source_rgba(cr, 0.0f, 0.0f, 0.0f, 0.0f);
  cairo_rectangle(cr, 0, 0, alloc.width, alloc.height);
  cairo_fill(cr);

  gtk_container_propagate_draw(GTK_CONTAINER(widget),
                               gtk_bin_get_child(GTK_BIN(widget)),
                               cr);

  return FALSE;
}

std::string
PanelTray::GetName() const
{
  return "PanelTray";
}

void
PanelTray::AddProperties(GVariantBuilder* builder)
{

}

} // namespace unity
