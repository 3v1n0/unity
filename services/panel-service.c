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
 *              Rodrigo Moya <rodrigo.moya@canonical.com>
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "panel-marshal.h"
#include "panel-service.h"

#include <stdlib.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <X11/extensions/XInput2.h>

#include "panel-marshal.h"

G_DEFINE_TYPE (PanelService, panel_service, G_TYPE_OBJECT);

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PANEL_TYPE_SERVICE, PanelServicePrivate))

#define NOTIFY_TIMEOUT 80
#define N_TIMEOUT_SLOTS 50

static PanelService *static_service = NULL;

struct _PanelServicePrivate
{
  GSList     *indicators;
  GHashTable *id2entry_hash;
  GHashTable *entry2indicator_hash;

  guint  initial_sync_id;
  gint32 timeouts[N_TIMEOUT_SLOTS];

  IndicatorObjectEntry *last_entry;
  GtkMenu *last_menu;
  guint32  last_menu_id;
  guint32  last_menu_move_id;
  gint32   last_x;
  gint32   last_y;
  guint32  last_menu_button;

  gint     last_menu_x;
  gint     last_menu_y;
};

/* Globals */
static gboolean suppress_signals = FALSE;

enum
{
  ENTRY_ACTIVATED = 0,
  RE_SYNC,
  ACTIVE_MENU_POINTER_MOTION,
  ENTRY_ACTIVATE_REQUEST,
  ENTRY_SHOW_NOW_CHANGED,
  GEOMETRIES_CHANGED,

  LAST_SIGNAL
};

enum
{
  SYNC_WAITING = -1,
  SYNC_NEUTRAL = 0,
};

static guint32 _service_signals[LAST_SIGNAL] = { 0 };

static gchar * indicator_order[] = {
  "libappmenu.so",
  "libapplication.so",
  "libmessaging.so",
  "libpower.so",
  "libnetwork.so",
  "libnetworkmenu.so",
  "libsoundmenu.so",
  "libdatetime.so",
  "libsession.so",
  NULL
};

/* Forwards */
static void load_indicator  (PanelService    *self,
                             IndicatorObject *object,
                             const gchar     *_name);
static void load_indicators (PanelService    *self);
static void sort_indicators (PanelService    *self);

static GdkFilterReturn event_filter (GdkXEvent    *ev,
                                     GdkEvent     *gev,
                                     PanelService *self);

/*
 * GObject stuff
 */

static void
panel_service_class_dispose (GObject *object)
{
  PanelServicePrivate *priv = PANEL_SERVICE (object)->priv;
  gint i;

  g_hash_table_destroy (priv->id2entry_hash);
  g_hash_table_destroy (priv->entry2indicator_hash);

  gdk_window_remove_filter (NULL, (GdkFilterFunc)event_filter, object);

  if (priv->initial_sync_id)
    {
      g_source_remove (priv->initial_sync_id);
      priv->initial_sync_id = 0;
    }

  for (i = 0; i < N_TIMEOUT_SLOTS; i++)
    {
      if (priv->timeouts[i] > 0)
        {
          g_source_remove (priv->timeouts[i]);
          priv->timeouts[i] = 0;
        }
    }

  G_OBJECT_CLASS (panel_service_parent_class)->dispose (object);
}

static void
panel_service_class_init (PanelServiceClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  obj_class->dispose = panel_service_class_dispose;

  /* Signals */
  _service_signals[ENTRY_ACTIVATED] =
    g_signal_new ("entry-activated",
                  G_OBJECT_CLASS_TYPE (obj_class),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1, G_TYPE_STRING);

  _service_signals[RE_SYNC] =
    g_signal_new ("re-sync",
                  G_OBJECT_CLASS_TYPE (obj_class),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1, G_TYPE_STRING);

 _service_signals[ACTIVE_MENU_POINTER_MOTION] =
    g_signal_new ("active-menu-pointer-motion",
                  G_OBJECT_CLASS_TYPE (obj_class),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

 _service_signals[ENTRY_ACTIVATE_REQUEST] =
    g_signal_new ("entry-activate-request",
                  G_OBJECT_CLASS_TYPE (obj_class),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1, G_TYPE_STRING);
 _service_signals[GEOMETRIES_CHANGED] =
    g_signal_new ("geometries-changed",
      G_OBJECT_CLASS_TYPE (obj_class),
      G_SIGNAL_RUN_LAST,
      0,
      NULL, NULL,
      panel_marshal_VOID__OBJECT_POINTER_INT_INT_INT_INT,
      G_TYPE_NONE, 6,
      G_TYPE_OBJECT, G_TYPE_POINTER,
      G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT);

 _service_signals[ENTRY_SHOW_NOW_CHANGED] =
    g_signal_new ("entry-show-now-changed",
                  G_OBJECT_CLASS_TYPE (obj_class),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  panel_marshal_VOID__STRING_BOOLEAN,
                  G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_BOOLEAN);


  g_type_class_add_private (obj_class, sizeof (PanelServicePrivate));
}

static GdkFilterReturn
event_filter (GdkXEvent *ev, GdkEvent *gev, PanelService *self)
{
  XEvent *e = (XEvent *)ev;
  GdkFilterReturn ret = GDK_FILTER_CONTINUE;

  if (!PANEL_IS_SERVICE (self))
  {
    g_warning ("%s: Invalid PanelService instance", G_STRLOC);
    return ret;
  }

  if (!GTK_IS_WIDGET (self->priv->last_menu))
    return ret;

  /* Use XI2 to read the event data */
  XGenericEventCookie *cookie = &e->xcookie;
  if (cookie->type == GenericEvent)
    {
      XIDeviceEvent *event = cookie->data;
            
      if (event->evtype == XI_ButtonRelease &&
          self->priv->last_menu_button != 0) //FocusChange
        {
          gint       x=0, y=0, width=0, height=0, x_root=0, y_root=0;
          GdkWindow *window = gtk_widget_get_window (GTK_WIDGET (self->priv->last_menu));
          if (window == NULL)
          {
            g_warning ("%s: gtk_widget_get_window  (self->priv->last_menu) == NULL", G_STRLOC);
            return GDK_FILTER_CONTINUE;
          }
          
          Window     xwindow = gdk_x11_window_get_xid (window);
         
          if (xwindow == 0)
          {
            g_warning ("%s: gdk_x11_window_get_xid (last_menu->window) == 0", G_STRLOC);
            return GDK_FILTER_CONTINUE;
          }

          Window     root = 0, child = 0;
          int        win_x=0, win_y = 0;
          guint32    mask_return = 0;

          XQueryPointer (gdk_x11_display_get_xdisplay (gdk_display_get_default ()),
                         xwindow,
                         &root,
                         &child,
                         &x_root,
                         &y_root,
                         &win_x,
                         &win_y,
                         &mask_return);

          gdk_window_get_geometry (window, &x, &y, &width, &height);
          gdk_window_get_origin (window, &x, &y);

          if (x_root > x
              && x_root < x + width
              && y_root > y
              && y_root < y + height)
            {
              ret = GDK_FILTER_CONTINUE;
            }
          else
            {
              ret = GDK_FILTER_REMOVE;
            }

          self->priv->last_menu_button = 0;
        }

      // FIXME: THIS IS HORRIBLE AND WILL BE CHANGED BEFORE RELEASE
      // ITS A WORKAROUND SO I CAN TEST THE PANEL SCRUBBING
      // DONT HATE ME
      // --------------------------------------------------------------------------
      //FIXME-GTK3 - i'm not porting this, fix your code :P
      
      else if (event->evtype == XI_Motion)
        {
          int       x_root=0, y_root=0;
          GdkWindow *window = gtk_widget_get_window (GTK_WIDGET (self->priv->last_menu));
          Window     xwindow = gdk_x11_window_get_xid (window);
          Window     root = 0, child = 0;
          int        win_x=0, win_y = 0;
          guint32    mask_return = 0;

          XQueryPointer (gdk_x11_display_get_xdisplay (gdk_display_get_default ()),
                         xwindow,
                         &root,
                         &child,
                         &x_root,
                         &y_root,
                         &win_x,
                         &win_y,
                         &mask_return);

          self->priv->last_menu_x = x_root;
          self->priv->last_menu_y = y_root;

          if (y_root <= self->priv->last_y)
            {
              g_signal_emit (self, _service_signals[ACTIVE_MENU_POINTER_MOTION], 0);
            }
        }
      // -> I HATE YOU
      // /DONT HATE ME
      // /FIXME
      // --------------------------------------------------------------------------
    }

  return ret;
}

static gboolean
initial_resync (PanelService *self)
{
  if (PANEL_IS_SERVICE (self))
    {
      g_signal_emit (self, _service_signals[RE_SYNC], 0, "");
      self->priv->initial_sync_id = 0;
    }
  return FALSE;
}

static void
panel_service_init (PanelService *self)
{
  PanelServicePrivate *priv;
  priv = self->priv = GET_PRIVATE (self);

  gdk_window_add_filter (NULL, (GdkFilterFunc)event_filter, self);

  priv->id2entry_hash = g_hash_table_new_full (g_str_hash, g_str_equal,
                                               g_free, NULL);
  priv->entry2indicator_hash = g_hash_table_new (g_direct_hash, g_direct_equal);

  suppress_signals = TRUE;
  load_indicators (self);
  sort_indicators (self);
  suppress_signals = FALSE;

  priv->initial_sync_id = g_idle_add ((GSourceFunc)initial_resync, self);
}

PanelService *
panel_service_get_default ()
{
  if (static_service == NULL || !PANEL_IS_SERVICE (static_service))
    static_service = g_object_new (PANEL_TYPE_SERVICE, NULL);

  return static_service;
}

PanelService *
panel_service_get_default_with_indicators (GList *indicators)
{
  PanelService *service = panel_service_get_default ();
  GList        *i;

  for (i = indicators; i; i = i->next)
    {
      IndicatorObject *object = i->data;
      if (INDICATOR_IS_OBJECT (object))
          load_indicator (service, object, NULL);
    }

  return service;
}
guint
panel_service_get_n_indicators (PanelService *self)
{
  g_return_val_if_fail (PANEL_IS_SERVICE (self), 0);

  return g_slist_length (self->priv->indicators);
}

IndicatorObject *
panel_service_get_indicator (PanelService *self, guint position)
{
  g_return_val_if_fail (PANEL_IS_SERVICE (self), NULL);

  return (IndicatorObject *) g_slist_nth_data (self->priv->indicators, position);
}

/*
 * Private Methods
 */
static gboolean
actually_notify_object (IndicatorObject *object)
{
  PanelService *self;
  PanelServicePrivate *priv;
  gint position;

  if (!PANEL_IS_SERVICE (static_service))
    return FALSE;

  if (!INDICATOR_IS_OBJECT (object))
    return FALSE;

  self = panel_service_get_default ();
  priv = self->priv;

  position = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (object), "position"));
  priv->timeouts[position] = SYNC_WAITING;

  if (!suppress_signals)
    g_signal_emit (self, _service_signals[RE_SYNC],
                   0, g_object_get_data (G_OBJECT (object), "id"));

  return FALSE;
}

static void
notify_object (IndicatorObject *object)
{
  PanelService        *self;
  PanelServicePrivate *priv;
  gint                 position;

  if (suppress_signals)
    return;

  self = panel_service_get_default ();
  priv = self->priv;

  position = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (object), "position"));

  if (priv->timeouts[position] == SYNC_WAITING)
    {
      /* No need to ping again as we're waiting for the client to sync anyway */
      return;
    }
  else if (priv->timeouts[position] != SYNC_NEUTRAL)
    {
      /* We were going to signal that a sync was needed, but since there's been another change let's
       * hold off a little longer so we're not flooding the client
       */
      g_source_remove (priv->timeouts[position]);
    }

  priv->timeouts[position] = g_timeout_add (NOTIFY_TIMEOUT,
                                            (GSourceFunc)actually_notify_object,
                                            object);
}

static void
on_entry_property_changed (GObject        *o,
                           GParamSpec      *pspec,
                           IndicatorObject *object)
{
  notify_object (object);
}

static void
on_entry_changed (GObject *o,
                  IndicatorObject *object)
{
  notify_object (object);
}

static void
on_entry_added (IndicatorObject      *object,
                IndicatorObjectEntry *entry,
                PanelService         *self)
{
  PanelServicePrivate *priv;
  gchar *id;

  g_return_if_fail (PANEL_IS_SERVICE (self));
  g_return_if_fail (entry != NULL);
  priv = self->priv;

  id = g_strdup_printf ("%p", entry);
  g_hash_table_insert (priv->id2entry_hash, id, entry);
  g_hash_table_insert (priv->entry2indicator_hash, entry, object);

  if (GTK_IS_LABEL (entry->label))
    {
      g_signal_connect (entry->label, "notify::label",
                        G_CALLBACK (on_entry_property_changed), object);

      g_signal_connect (entry->label, "notify::sensitive",
                        G_CALLBACK (on_entry_property_changed), object);
      g_signal_connect (entry->label, "show",
                        G_CALLBACK (on_entry_changed), object);
      g_signal_connect (entry->label, "hide",
                        G_CALLBACK (on_entry_changed), object);

    }
  if (GTK_IS_IMAGE (entry->image))
    {
      g_signal_connect (entry->image, "notify::storage-type",
                        G_CALLBACK (on_entry_property_changed), object);
      g_signal_connect (entry->image, "notify::file",
                        G_CALLBACK (on_entry_property_changed), object);
      g_signal_connect (entry->image, "notify::gicon",
                        G_CALLBACK (on_entry_property_changed), object);
      g_signal_connect (entry->image, "notify::icon-name",
                        G_CALLBACK (on_entry_property_changed), object);
      g_signal_connect (entry->image, "notify::pixbuf",
                        G_CALLBACK (on_entry_property_changed), object);
      g_signal_connect (entry->image, "notify::stock",
                        G_CALLBACK (on_entry_property_changed), object);

      g_signal_connect (entry->image, "notify::sensitive",
                        G_CALLBACK (on_entry_property_changed), object);
      g_signal_connect (entry->image, "show",
                        G_CALLBACK (on_entry_changed), object);
      g_signal_connect (entry->image, "hide",
                        G_CALLBACK (on_entry_changed), object);

    }

  notify_object (object);
}

static void
on_entry_removed (IndicatorObject      *object,
                  IndicatorObjectEntry *entry,
                  PanelService         *self)
{
  PanelServicePrivate *priv;
  gchar *id;

  g_return_if_fail (PANEL_IS_SERVICE (self));
  g_return_if_fail (entry != NULL);

  priv = self->priv;

  id = g_strdup_printf ("%p", entry);
  g_hash_table_remove (priv->entry2indicator_hash, entry);
  g_hash_table_remove (priv->id2entry_hash, id);
  g_free (id);

  notify_object (object);
}

static void
on_entry_moved (IndicatorObject      *object,
                IndicatorObjectEntry *entry,
                PanelService         *self)
{
  notify_object (object);
}

static void
on_indicator_menu_show (IndicatorObject      *object,
                        IndicatorObjectEntry *entry,
                        guint32               timestamp,
                        PanelService         *self)
{
  gchar *entry_id;
  
  g_return_if_fail (PANEL_IS_SERVICE (self));
  if (entry == NULL)
    {
      g_warning ("on_indicator_menu_show() called with a NULL entry");
      return;
    }

  entry_id = g_strdup_printf ("%p", entry);

  g_signal_emit (self, _service_signals[ENTRY_ACTIVATE_REQUEST], 0, entry_id);

  g_free (entry_id);
}

static void
on_indicator_menu_show_now_changed (IndicatorObject      *object,
                                    IndicatorObjectEntry *entry,
                                    gboolean              show_now_changed,
                                    PanelService         *self)
{
  gchar *entry_id;
  
  g_return_if_fail (PANEL_IS_SERVICE (self));
  if (entry == NULL)
    {
      g_warning ("on_indicator_menu_show_now_changed() called with a NULL entry");
      return;
    }

  entry_id = g_strdup_printf ("%p", entry);

  g_signal_emit (self, _service_signals[ENTRY_SHOW_NOW_CHANGED], 0, entry_id, show_now_changed);

  g_free (entry_id);
}

static const gchar * indicator_environment[] = {
  "unity",
  "unity-3d",
  "unity-panel-service",
  NULL
};

static void
load_indicator (PanelService *self, IndicatorObject *object, const gchar *_name)
{
  PanelServicePrivate *priv = self->priv;
  gchar *name;
  GList *entries, *entry;

  indicator_object_set_environment(object, (const GStrv)indicator_environment);

  if (_name != NULL)
    name = g_strdup (_name);
  else
    name = g_strdup_printf ("%p", object);

  priv->indicators = g_slist_append (priv->indicators, object);

  g_object_set_data_full (G_OBJECT (object), "id", g_strdup (name), g_free);

  g_signal_connect (object, INDICATOR_OBJECT_SIGNAL_ENTRY_ADDED,
                    G_CALLBACK (on_entry_added), self);
  g_signal_connect (object, INDICATOR_OBJECT_SIGNAL_ENTRY_REMOVED,
                    G_CALLBACK (on_entry_removed), self);
  g_signal_connect (object, INDICATOR_OBJECT_SIGNAL_ENTRY_MOVED,
                    G_CALLBACK (on_entry_moved), self);
  g_signal_connect (object, INDICATOR_OBJECT_SIGNAL_MENU_SHOW,
                    G_CALLBACK (on_indicator_menu_show), self);
  g_signal_connect (object, INDICATOR_OBJECT_SIGNAL_SHOW_NOW_CHANGED,
                    G_CALLBACK (on_indicator_menu_show_now_changed), self);

  entries = indicator_object_get_entries (object);
  for (entry = entries; entry != NULL; entry = entry->next)
    {
      on_entry_added (object, entry->data, self);
    }
  g_list_free (entries);

  g_free (name);
}

static void
load_indicators (PanelService *self)
{
  GDir        *dir;
  const gchar *name;

  if (!g_file_test (INDICATORDIR,
                    G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR))
    {
      g_warning ("%s does not exist, cannot read any indicators", INDICATORDIR);
      gtk_main_quit ();
    }

  dir = g_dir_open (INDICATORDIR, 0, NULL);
  while ((name = g_dir_read_name (dir)) != NULL)
    {
      IndicatorObject *object;
      gchar           *path;

      if (!g_str_has_suffix (name, ".so"))
        continue;

      path = g_build_filename (INDICATORDIR, name, NULL);
      g_debug ("Loading: %s", path);

      object = indicator_object_new_from_file (path);
      if (object == NULL)
        {
          g_warning ("Unable to load '%s'", path);
          g_free (path);
          continue;
        }
      load_indicator (self, object, name);

      g_free (path);
    }

  g_dir_close (dir);
}

static gint
name2order (const gchar * name)
{
  int i;

  for (i = 0; indicator_order[i] != NULL; i++)
    {
      if (g_strcmp0(name, indicator_order[i]) == 0)
        {
          return i;
        }
    }
  return -1;
}

static int
indicator_compare_func (IndicatorObject *o1, IndicatorObject *o2)
{
  gchar *s1;
  gchar *s2;
  int    i1;
  int    i2;

  s1 = g_object_get_data (G_OBJECT (o1), "id");
  s2 = g_object_get_data (G_OBJECT (o2), "id");

  i1 = name2order (s1);
  i2 = name2order (s2);

  return i1 - i2;
}

static void
sort_indicators (PanelService *self)
{
  GSList *i;
  int     k = 0;

  self->priv->indicators = g_slist_sort (self->priv->indicators,
                                         (GCompareFunc)indicator_compare_func);

  for (i = self->priv->indicators; i; i = i->next)
    {
      g_object_set_data (G_OBJECT (i->data), "position", GINT_TO_POINTER (k));
      self->priv->timeouts[k] = SYNC_NEUTRAL;
      k++;
    }
}

static gchar *
gtk_image_to_data (GtkImage *image)
{
  GtkImageType type = gtk_image_get_storage_type (image);
  gchar *ret = NULL;

  if (type == GTK_IMAGE_PIXBUF)
    {
      GdkPixbuf  *pixbuf;
      gchar      *buffer = NULL;
      gsize       buffer_size = 0;
      GError     *error = NULL;

      pixbuf = gtk_image_get_pixbuf (image);

      if (gdk_pixbuf_save_to_buffer (pixbuf, &buffer, &buffer_size, "png", &error, NULL))
        {
          ret = g_base64_encode ((const guchar *)buffer, buffer_size);
          g_free (buffer);
        }
      else
        {
          g_warning ("Unable to convert pixbuf to png data: '%s'", error ? error->message : "unknown");
          if (error)
            g_error_free (error);

          ret = g_strdup ("");
        }
    }
  else if (type == GTK_IMAGE_STOCK)
    {
      g_object_get (G_OBJECT (image), "stock", &ret, NULL);
    }
  else if (type == GTK_IMAGE_ICON_NAME)
    {
      g_object_get (G_OBJECT (image), "icon-name", &ret, NULL);
    }
  else if (type == GTK_IMAGE_GICON)
    {
      GIcon *icon = NULL;
      gtk_image_get_gicon (image, &icon, NULL);
      if (G_IS_ICON (icon))
        {
          ret = g_icon_to_string (icon);
        }
    }
  else
    {
      ret = g_strdup ("");
      g_warning ("Unable to support GtkImageType: %d", type);
    }

  return ret;
}

static void
indicator_entry_to_variant (IndicatorObjectEntry *entry,
                            const gchar          *id,
                            const gchar          *indicator_id,
                            GVariantBuilder      *b)
{
  gboolean is_label = GTK_IS_LABEL (entry->label);
  gboolean is_image = GTK_IS_IMAGE (entry->image);
  gchar *image_data = NULL;

  g_variant_builder_add (b, "(sssbbusbb)",
                         indicator_id,
                         id,
                         is_label ? gtk_label_get_label (entry->label) : "",
                         is_label ? gtk_widget_get_sensitive (GTK_WIDGET (entry->label)) : FALSE,
                         is_label ? gtk_widget_get_visible (GTK_WIDGET (entry->label)) : FALSE,
                         is_image ? (guint32)gtk_image_get_storage_type (entry->image) : (guint32) 0,
                         is_image ? (image_data = gtk_image_to_data (entry->image)) : "",
                         is_image ? gtk_widget_get_sensitive (GTK_WIDGET (entry->image)) : FALSE,
                         is_image ? gtk_widget_get_visible (GTK_WIDGET (entry->image)) : FALSE);

  g_free (image_data);
}

static void
indicator_entry_null_to_variant (const gchar     *indicator_id,
                                 GVariantBuilder *b)
{
  g_variant_builder_add (b, "(sssbbusbb)",
                         indicator_id,
                         "",
                         "",
                         FALSE,
                         FALSE,
                         (guint32) 0,
                         "",
                         FALSE,
                         FALSE);
}

static void
indicator_object_to_variant (IndicatorObject *object, const gchar *indicator_id, GVariantBuilder *b)
{
  GList *entries, *e;

  entries = indicator_object_get_entries (object);
  if (entries)
    {
      for (e = entries; e; e = e->next)
        {
          IndicatorObjectEntry *entry = e->data;
          gchar *id = g_strdup_printf ("%p", entry);
          indicator_entry_to_variant (entry, id, indicator_id, b);
          g_free (id);
        }
    }
  else
    {
      /* Add a null entry to indicate that there is an indicator here, it's just empty */
      indicator_entry_null_to_variant (indicator_id, b);
    }
  g_list_free (entries);
}

static void
positon_menu (GtkMenu  *menu,
              gint     *x,
              gint     *y,
              gboolean *push,
              gpointer  user_data)
{
  PanelService *self = PANEL_SERVICE (user_data);
  PanelServicePrivate *priv = self->priv;

  *x = priv->last_x;
  *y = priv->last_y;
  *push = TRUE;
}

static void
on_active_menu_hidden (GtkMenu *menu, PanelService *self)
{
  PanelServicePrivate *priv = self->priv;

  priv->last_x = 0;
  priv->last_y = 0;
  priv->last_menu_button = 0;

  g_signal_handler_disconnect (priv->last_menu, priv->last_menu_id);
  g_signal_handler_disconnect (priv->last_menu, priv->last_menu_move_id);
  priv->last_menu = NULL;
  priv->last_menu_id = 0;
  priv->last_menu_move_id = 0;
  priv->last_entry = NULL;

  g_signal_emit (self, _service_signals[ENTRY_ACTIVATED], 0, "");
}

/*
 * Public Methods
 */
GVariant *
panel_service_sync (PanelService *self)
{
  GVariantBuilder b;
  GSList *i;

  g_variant_builder_init (&b, G_VARIANT_TYPE ("(a(sssbbusbb))"));
  g_variant_builder_open (&b, G_VARIANT_TYPE ("a(sssbbusbb)"));

  for (i = self->priv->indicators; i; i = i->next)
    {
      const gchar *indicator_id = g_object_get_data (G_OBJECT (i->data), "id");
      gint position;

      /* Set the sync back to neutral */
      position = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (i->data), "position"));
      self->priv->timeouts[position] = SYNC_NEUTRAL;

      indicator_object_to_variant (i->data, indicator_id, &b);
    }

  g_variant_builder_close (&b);
  return g_variant_builder_end (&b);
}

GVariant *
panel_service_sync_one (PanelService *self, const gchar *indicator_id)
{
  GVariantBuilder b;
  GSList *i;

  g_variant_builder_init (&b, G_VARIANT_TYPE ("(a(sssbbusbb))"));
  g_variant_builder_open (&b, G_VARIANT_TYPE ("a(sssbbusbb)"));

  for (i = self->priv->indicators; i; i = i->next)
    {
      if (g_strcmp0 (indicator_id,
                     g_object_get_data (G_OBJECT (i->data), "id")) == 0)
        {
          gint position;

          /* Set the sync back to neutral */
          position = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (i->data), "position"));
          self->priv->timeouts[position] = SYNC_NEUTRAL;

          indicator_object_to_variant (i->data, indicator_id, &b);
          break;
        }
    }

  g_variant_builder_close (&b);
  return g_variant_builder_end (&b);
}

void
panel_service_sync_geometry (PanelService *self,
           const gchar *indicator_id,
           const gchar *entry_id,
           gint x,
           gint y,
           gint width,
           gint height)
{
  PanelServicePrivate *priv = self->priv;
  IndicatorObjectEntry *entry = g_hash_table_lookup (priv->id2entry_hash, entry_id);
  IndicatorObject *object = g_hash_table_lookup (priv->entry2indicator_hash, entry);

  g_signal_emit (self, _service_signals[GEOMETRIES_CHANGED], 0, object, entry, x, y, width, height);
}

static gboolean
should_skip_menu (IndicatorObjectEntry *entry)
{
  gboolean label_ok = FALSE;
  gboolean image_ok = FALSE;

  g_return_val_if_fail (entry != NULL, TRUE);

  if (GTK_IS_LABEL (entry->label))
    {
      label_ok = gtk_widget_get_visible (GTK_WIDGET (entry->label))
        && gtk_widget_is_sensitive (GTK_WIDGET (entry->label));
    }

  if (GTK_IS_IMAGE (entry->image))
    {
      image_ok = gtk_widget_get_visible (GTK_WIDGET (entry->image))
        && gtk_widget_is_sensitive (GTK_WIDGET (entry->image));
    }

  return !label_ok && !image_ok;
}

static void
activate_next_prev_menu (PanelService         *self,
                         IndicatorObject      *object,
                         IndicatorObjectEntry *entry,
                         GtkMenuDirectionType  direction)
{
  PanelServicePrivate *priv = self->priv;
  GSList *indicators = priv->indicators;
  GList  *entries;
  gint    n_entries;
  IndicatorObjectEntry *new_entry;
  gchar  *id;

  entries = indicator_object_get_entries (object);
  n_entries = g_list_length (entries);
  // all of these are for switching between independant indicators (for example, sound to messaging. 
  // As opposed to batter to appmenu)
  if (n_entries == 1
      || (g_list_index (entries, entry) == 0 && direction == GTK_MENU_DIR_PARENT)
      || (g_list_index (entries, entry) == n_entries - 1 && direction == GTK_MENU_DIR_CHILD))
    {
      int              n_indicators;
      IndicatorObject *new_object;
      GList           *new_entries;
      
      n_indicators = g_slist_length (priv->indicators);

      // changing from first indicator to last indicator
      if (g_slist_index (indicators, object) == 0 && direction == GTK_MENU_DIR_PARENT)
        {
          new_object = g_slist_nth_data (indicators, n_indicators - 1);
        }
      // changing from last indicator to first indicator
      else if (g_slist_index (indicators, object) == n_indicators -1 && direction == GTK_MENU_DIR_CHILD)
        {
          new_object = g_slist_nth_data (indicators, 0);
        }
      else
        {
          gint cur_object_index = g_slist_index (indicators, object);
          gint new_object_index = cur_object_index + (direction == GTK_MENU_DIR_CHILD ? 1 : -1);
          new_object = g_slist_nth_data (indicators, new_object_index);
        }

      if (!INDICATOR_IS_OBJECT (new_object))
        return;
      new_entries = indicator_object_get_entries (new_object);
      // if the indicator has no entries, move to the next/prev one until we find one with entries
      while (new_entries == NULL)
        {
          gint cur_object_index = g_slist_index (indicators, new_object);
          gint new_object_index = cur_object_index + (direction == GTK_MENU_DIR_CHILD ? 1 : -1);
          new_object = g_slist_nth_data (indicators, new_object_index);
          if (!INDICATOR_IS_OBJECT (new_object))
            return;
          new_entries = indicator_object_get_entries (new_object);
        }

      new_entry = g_list_nth_data (new_entries, direction == GTK_MENU_DIR_PARENT ? g_list_length (new_entries) - 1 : 0);

      g_list_free (entries);
      g_list_free (new_entries);

      if (should_skip_menu (new_entry))
        {	  
          activate_next_prev_menu (self, new_object, new_entry, direction);
      	  return;
        }
    }
  // changing within a group of indicators (for example, entries within appmenu)
  else
    {
      new_entry = g_list_nth_data (entries, g_list_index (entries, entry) + (direction == GTK_MENU_DIR_CHILD ? 1 : -1));
      g_list_free (entries);

      if (should_skip_menu (new_entry))
        { 
          activate_next_prev_menu (self, object, new_entry, direction);
          return;
        }
    }

  id = g_strdup_printf ("%p", new_entry);
  g_signal_emit (self, _service_signals[ENTRY_ACTIVATE_REQUEST], 0, id);
  g_free (id);
}

static void
on_active_menu_move_current (GtkMenu              *menu,
                             GtkMenuDirectionType  direction,
                             PanelService         *self)
{
  PanelServicePrivate *priv;
  IndicatorObject     *object;

  g_return_if_fail (PANEL_IS_SERVICE (self));
  priv = self->priv;

  /* Not interested in up or down */
  if (direction == GTK_MENU_DIR_NEXT
      || direction == GTK_MENU_DIR_PREV)
    return;

  /* We don't want to distrupt going into submenus */
  if (direction == GTK_MENU_DIR_CHILD)
    {
      GList               *children, *c;
      children = gtk_container_get_children (GTK_CONTAINER (menu));
      for (c = children; c; c = c->next)
        {
          GtkWidget *item = (GtkWidget *)c->data;

          if (GTK_IS_MENU_ITEM (item)
              && gtk_widget_get_state (item) == GTK_STATE_PRELIGHT
              && gtk_menu_item_get_submenu (GTK_MENU_ITEM (item)))
            {
              /* Skip direction due to there being a submenu,
               * and we don't want to inhibit going into that */
              return;
            }
        }
      g_list_free (children);
    }

  /* Find the next/prev indicator */
  object = g_hash_table_lookup (priv->entry2indicator_hash, priv->last_entry);
  if (object == NULL)
    {
      g_warning ("Unable to find IndicatorObject for entry");
      return;
    }

  activate_next_prev_menu (self, object, priv->last_entry, direction);
}

void
panel_service_show_entry (PanelService *self,
                          const gchar  *entry_id,
                          guint32       timestamp,
                          gint32        x,
                          gint32        y,
                          gint32        button)
{
  PanelServicePrivate  *priv = self->priv;
  IndicatorObjectEntry *entry = g_hash_table_lookup (priv->id2entry_hash, entry_id);
  IndicatorObject      *object = g_hash_table_lookup (priv->entry2indicator_hash, entry);
  GtkWidget            *last_menu;

  if (priv->last_entry == entry)
    return;

  last_menu = GTK_WIDGET (priv->last_menu);
  
  if (GTK_IS_MENU (priv->last_menu))
    {
      priv->last_x = 0;
      priv->last_y = 0;

      g_signal_handler_disconnect (priv->last_menu, priv->last_menu_id);
      g_signal_handler_disconnect (priv->last_menu, priv->last_menu_move_id);

      priv->last_entry = NULL;
      priv->last_menu = NULL;
      priv->last_menu_id = 0;
      priv->last_menu_move_id = 0;
      priv->last_menu_button = 0;
    }

  if (entry != NULL)
    {
      if (GTK_IS_MENU (entry->menu))
        {
          priv->last_menu = entry->menu;
        }
      else
        {
          /* For some reason, this entry doesn't have a menu.  To simplify the
             rest of the code and to keep scrubbing fluidly, we'll create a
             stub menu for the duration of this scrub. */
          priv->last_menu = GTK_MENU (gtk_menu_new ());
          g_signal_connect (priv->last_menu, "deactivate",
                            G_CALLBACK (gtk_widget_destroy), NULL);
          g_signal_connect (priv->last_menu, "destroy",
                            G_CALLBACK (gtk_widget_destroyed), &priv->last_menu);
        }

      priv->last_entry = entry;
      priv->last_x = x;
      priv->last_y = y;
      priv->last_menu_button = button;
      priv->last_menu_id = g_signal_connect (priv->last_menu, "hide",
                                             G_CALLBACK (on_active_menu_hidden), self);
      priv->last_menu_move_id = g_signal_connect_after (priv->last_menu, "move-current",
                                                        G_CALLBACK (on_active_menu_move_current), self);

      indicator_object_entry_activate (object, entry, CurrentTime);
      gtk_menu_popup (priv->last_menu, NULL, NULL, positon_menu, self, 0, CurrentTime);

      g_signal_emit (self, _service_signals[ENTRY_ACTIVATED], 0, entry_id);
    }

  /* We popdown the old one last so we don't accidently send key focus back to the
   * active application (which will make it change colour (as state changes), which
   * then looks like flickering to the user.
   */
  if (GTK_MENU (last_menu))
    gtk_menu_popdown (GTK_MENU (last_menu));
}

void
panel_service_scroll_entry (PanelService   *self,
                            const gchar    *entry_id,
                            gint32         delta)
{
  PanelServicePrivate  *priv = self->priv;
  IndicatorObjectEntry *entry = g_hash_table_lookup (priv->id2entry_hash, entry_id);
  IndicatorObject *object = g_hash_table_lookup (priv->entry2indicator_hash, entry);
  GdkScrollDirection direction = delta > 0 ? GDK_SCROLL_DOWN : GDK_SCROLL_UP;

  g_signal_emit_by_name(object, "scroll", abs(delta/120), direction);
  g_signal_emit_by_name(object, "scroll-entry", entry, abs(delta/120), direction);
}

void
panel_service_get_last_xy   (PanelService  *self,
                             gint          *x,
                             gint          *y)
{
  *x = self->priv->last_menu_x;
  *y = self->priv->last_menu_y;
}
