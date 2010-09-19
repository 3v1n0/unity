/*
 * Copyright (C) 2009 Canonical Ltd
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
 * Authored by Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */

#include <mutter-plugin.h>
#include <mutter-window.h>
#include <glib.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <clutter/x11/clutter-x11.h>

#include "unity-mutter.h"

#define UNITY_TYPE_MUTTER (unity_mutter_get_type ())

#define UNITY_MUTTER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
	UNITY_TYPE_MUTTER, UnityMutter))

#define UNITY_MUTTER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass),\
	UNITY_TYPE_MUTTER, UnityMutterClass))

#define UNITY_IS_MUTTER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
	UNITY_TYPE_MUTTER))

#define UNITY_IS_MUTTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
	UNITY_TYPE_MUTTER))

#define UNITY_MUTTER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),\
	UNITY_TYPE_MUTTER, UnityMutterClass))

typedef struct _UnityMutter UnityMutter;
typedef struct _UnityMutterClass UnityMutterClass;

struct _UnityMutter
{
  MutterPlugin parent;

  UnityPlugin *plugin;
};

struct _UnityMutterClass
{
 MutterPluginClass parent_class;
};

/* .c */
MUTTER_PLUGIN_DECLARE(UnityMutter, unity_mutter);

static void unity_mutter_constructed (GObject *object);

static void unity_mutter_minimize    (MutterPlugin *self,
                                      MutterWindow *window);
static void unity_mutter_maximize    (MutterPlugin *self,
                                      MutterWindow *window,
                                      gint          x,
                                      gint          y,
                                      gint          width,
                                      gint          height);
static void unity_mutter_unmaximize  (MutterPlugin  *self,
                                      MutterWindow  *window,
                                      gint           x,
                                      gint           y,
                                      gint           width,
                                      gint           height);
static void unity_mutter_map        (MutterPlugin   *self,
                                     MutterWindow   *window);
static void unity_mutter_destroy    (MutterPlugin   *self,
                                     MutterWindow   *window);
static void unity_mutter_switch_workspace (MutterPlugin *self,
                                           gint          from,
                                           gint          to,
                                           MetaMotionDirection direction);
static void unity_mutter_kill_window_effects (MutterPlugin  *self,
                                              MutterWindow  *window);
                                             
static void unity_mutter_kill_switch_workspace (MutterPlugin *self);
                                             
static gboolean unity_mutter_xevent_filter (MutterPlugin *self,
                                      XEvent      *event);

static void on_restore_input_region (UnityPlugin *plugin, gboolean fullscreen);

static const MutterPluginInfo * unity_mutter_plugin_info (MutterPlugin *self);

static void
unity_mutter_class_init (UnityMutterClass *klass)
{
  GObjectClass      *obj_class = G_OBJECT_CLASS (klass);
  MutterPluginClass *mut_class = MUTTER_PLUGIN_CLASS (klass);

  obj_class->constructed = unity_mutter_constructed;

  mut_class->minimize         = unity_mutter_minimize;
  mut_class->maximize         = unity_mutter_maximize;
  mut_class->unmaximize       = unity_mutter_unmaximize;
  mut_class->map              = unity_mutter_map;
  mut_class->destroy          = unity_mutter_destroy;
  mut_class->switch_workspace = unity_mutter_switch_workspace;
  mut_class->kill_window_effects      = unity_mutter_kill_window_effects;
  mut_class->kill_switch_workspace    = unity_mutter_kill_switch_workspace;
  mut_class->xevent_filter    = unity_mutter_xevent_filter;
  mut_class->plugin_info      = unity_mutter_plugin_info;
}

static void
unity_mutter_init (UnityMutter *self)
{
  ;
}

static void
unity_mutter_constructed (GObject *object)
{
  UnityMutter          *self = UNITY_MUTTER (object);

  self->plugin = unity_plugin_new ();
  g_signal_connect (self->plugin, "restore-input-region",
                    G_CALLBACK (on_restore_input_region), self);

  unity_plugin_set_plugin (self->plugin, MUTTER_PLUGIN (self));
}

static void
on_restore_input_region (UnityPlugin *plugin, gboolean fullscreen)
{
  MutterPlugin  *self;
  MetaScreen    *screen;
  MetaDisplay   *display;
  Display       *xdisplay;
  XRectangle    *rects;
  gint           width=0, height=0;
  XserverRegion  region;

  g_return_if_fail (UNITY_IS_PLUGIN (plugin));

  self = unity_plugin_get_plugin (plugin);

  screen = mutter_plugin_get_screen (self);
  display = meta_screen_get_display (screen);
  xdisplay = meta_display_get_xdisplay (display);

  mutter_plugin_query_screen_size (self, &width, &height);

  if (fullscreen)
    {
      rects = g_new (XRectangle, 1);

      /* Whole Screen */
      rects[0].x = 0;
      rects[0].y = 0;
      rects[0].width = width;
      rects[0].height = height;

      region = XFixesCreateRegion (xdisplay, rects, 1);
      mutter_plugin_set_stage_input_region (self, region);
    }
  else
    {
      rects = g_new (XRectangle, 2);

      /* Panel first */
      rects[0].x = plugin->primary_monitor.x;
      rects[0].y = plugin->primary_monitor.y;
      rects[0].width = plugin->primary_monitor.width;
      rects[0].height = unity_plugin_get_panel_height (plugin);

      /* Launcher */
      rects[1].x = plugin->primary_monitor.y;
      rects[1].y = rects[0].height;
      rects[1].width = unity_plugin_get_launcher_width (plugin) + 1;
      rects[1].height = plugin->primary_monitor.height - rects[0].height;

      /* Update region */
      region = XFixesCreateRegion (xdisplay, rects, 2);
      mutter_plugin_set_stage_input_region (self, region);
    }

  g_free (rects);
  XFixesDestroyRegion (xdisplay, region);
}

static void
unity_mutter_minimize (MutterPlugin *self,
                       MutterWindow *window)
{
  unity_plugin_minimize (UNITY_MUTTER (self)->plugin, window);
}

static void
unity_mutter_maximize (MutterPlugin *self,
                       MutterWindow *window,
                       gint          x,
                       gint          y,
                       gint          width,
                       gint          height)
{
  unity_plugin_maximize (UNITY_MUTTER(self)->plugin, window, x, y, width, height);
}

static void
unity_mutter_unmaximize (MutterPlugin  *self,
                         MutterWindow  *window,
                         gint           x,
                         gint           y,
                         gint           width,
                         gint           height)
{
  unity_plugin_unmaximize (UNITY_MUTTER(self)->plugin, window, x, y, width, height);
}

static void
unity_mutter_map (MutterPlugin   *self,
                  MutterWindow   *window)
{
  unity_plugin_map (UNITY_MUTTER(self)->plugin, window);
}

static void
unity_mutter_destroy (MutterPlugin   *self,
                      MutterWindow   *window)
{
  unity_plugin_destroy (UNITY_MUTTER(self)->plugin, window);
}

static void
unity_mutter_switch_workspace (MutterPlugin *self,
                               gint          from,
                               gint          to,
                               MetaMotionDirection direction)
{
  unity_plugin_switch_workspace (UNITY_MUTTER(self)->plugin, from, to, direction);
}

static void
unity_mutter_kill_window_effects (MutterPlugin  *self,
                                  MutterWindow  *window)
{
  unity_plugin_on_kill_window_effects (UNITY_MUTTER (self)->plugin, window);
}

static void
unity_mutter_kill_switch_workspace (MutterPlugin *self)
{
  unity_plugin_on_kill_switch_workspace (UNITY_MUTTER (self)->plugin);
}

static gboolean
unity_mutter_xevent_filter (MutterPlugin *self,
                            XEvent      *event)
{
  return clutter_x11_handle_event (event) != CLUTTER_X11_FILTER_CONTINUE;
}

static const MutterPluginInfo *
unity_mutter_plugin_info (MutterPlugin *self)
{
  static const MutterPluginInfo info = {
    .name = "Unity",
    .version = "0.1.2",
    .author = "Various Artists",
    .license = "GPLv3",
    .description = "Copyright (C) Canonical Ltd 2009"
  };

  return &info;
}

#if 0
#include "unity-mutter.h"

G_MODULE_EXPORT MutterPluginVersion mutter_plugin_version =
  {
    MUTTER_MAJOR_VERSION,
    MUTTER_MINOR_VERSION,
    MUTTER_MICRO_VERSION,
    MUTTER_PLUGIN_API_VERSION
  };

G_MODULE_EXPORT GType
mutter_plugin_register_type (GTypeModule *type_module)
{
  return unity_plugin_get_type ();
}
#endif
