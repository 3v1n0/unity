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
                                           const GList **windows,
                                           gint          from,
                                           gint          to,
                                           MetaMotionDirection direction);
static void unity_mutter_kill_effect (MutterPlugin  *self,
                                      MutterWindow  *window,
                                      gulong         events);
static gboolean unity_mutter_xevent_filter (MutterPlugin *self,
                                      XEvent      *event);

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
  mut_class->kill_effect      = unity_mutter_kill_effect;
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
  ClutterBackend       *backend;
  cairo_font_options_t *font_options;

  clutter_set_font_flags (clutter_get_font_flags () & ~CLUTTER_FONT_MIPMAPPING);
  
  backend = clutter_get_default_backend ();
  font_options = cairo_font_options_create ();
  cairo_font_options_set_antialias (font_options, CAIRO_ANTIALIAS_GRAY);
  clutter_backend_set_font_options (backend, font_options);
  cairo_font_options_destroy (font_options);

  self->plugin = unity_plugin_new ();
  unity_plugin_set_plugin (self->plugin, MUTTER_PLUGIN (self));
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
                               const GList **windows,
                               gint          from,
                               gint          to,
                               MetaMotionDirection direction)
{
  ;
}

static void
unity_mutter_kill_effect (MutterPlugin  *self,
                          MutterWindow  *window,
                          gulong         events)
{
  unity_plugin_kill_effect (UNITY_MUTTER (self)->plugin, window, events);
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
