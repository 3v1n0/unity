// -*- Mode: C; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010-2012 Canonical Ltd
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
 *              Marco Trevisan (Treviño) <mail@3v1n0.net>
 */

#include "config.h"
#include "panel-service.h"
#include "panel-service-private.h"

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <glib/gi18n-lib.h>
#include <libindicator/indicator-ng.h>

#include <X11/XKBlib.h>
#include <X11/XF86keysym.h>
#include <X11/extensions/XInput2.h>

#include <upstart/upstart-dbus.h>

G_DEFINE_TYPE (PanelService, panel_service, G_TYPE_OBJECT);

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PANEL_TYPE_SERVICE, PanelServicePrivate))

#define NOTIFY_TIMEOUT 80
#define N_TIMEOUT_SLOTS 50
#define MAX_INDICATOR_ENTRIES 500

#define NUX_VERTICAL_SCROLL_DELTA 120
#define NUX_HORIZONTAL_SCROLL_DELTA (NUX_VERTICAL_SCROLL_DELTA ^ 2)

#define COMPIZ_OPTION_SCHEMA "org.compiz.unityshell"
#define COMPIZ_OPTION_PATH "/org/compiz/profiles/unity/plugins/"
#define MENU_TOGGLE_KEYBINDING_KEY "panel-first-menu"
#define SHOW_DASH_KEY "show-launcher"
#define SHOW_HUD_KEY "show-hud"

static PanelService *static_service = NULL;
static gboolean lockscreen_mode = FALSE;

struct _PanelServicePrivate
{
  GSList     *indicators;
  GSList     *dropdown_entries;
  GSList     *removed_entries;
  GHashTable *id2entry_hash;
  GHashTable *panel2entries_hash;
  IndicatorObject *appmenu_indicator;

  guint timeouts[N_TIMEOUT_SLOTS];
  guint remove_idle;

  IndicatorObjectEntry *last_entry;
  IndicatorObjectEntry *last_dropdown_entry;
  const gchar *last_panel;
  GtkMenu *last_menu;
  gint32   last_x;
  gint32   last_y;
  gint     last_left;
  gint     last_top;
  gint     last_right;
  gint     last_bottom;
  guint32  last_menu_button;

  GSettings *gsettings;
  KeyBinding menu_toggle;
  KeyBinding show_dash;
  KeyBinding show_hud;

  IndicatorObjectEntry *pressed_entry;
  gboolean use_event;
};

/* Globals */
static gboolean suppress_signals = FALSE;

enum
{
  ENTRY_ACTIVATED = 0,
  RE_SYNC,
  ENTRY_ACTIVATE_REQUEST,
  GEOMETRIES_CHANGED,
  INDICATORS_CLEARED,

  LAST_SIGNAL
};

enum
{
  SYNC_WAITING = G_MAXUINT,
  SYNC_NEUTRAL = 0,
};

static guint32 _service_signals[LAST_SIGNAL] = { 0 };

static const gchar * indicator_order[][2] = {
  {APPMENU_INDICATOR_NAME, NULL},             /* indicator-appmenu" */
  {"libapplication.so", NULL},                /* indicator-application" */
  {"floating-indicators", NULL},              /* position-less NG indicators */
  {"libprintersmenu.so", NULL},               /* indicator-printers */
  {"libapplication.so", "gsd-keyboard-xkb"},  /* keyboard layout selector */
  {"libmessaging.so", NULL},                  /* indicator-messages */
  {"libpower.so", NULL},                      /* indicator-power */
  {"libbluetooth.so", NULL},                  /* indicator-bluetooth */
  {"libnetwork.so", NULL},                    /* indicator-network */
  {"libnetworkmenu.so", NULL},                /* indicator-network */
  {"libapplication.so", "nm-applet"},         /* network manager */
  {"libsoundmenu.so", NULL},                  /* indicator-sound */
  {"libdatetime.so", NULL},                   /* indicator-datetime */
  {"libsession.so", NULL},                    /* indicator-session */
  {NULL, NULL}
};

/* Forwards */
static void load_indicator  (PanelService *, IndicatorObject *, const gchar *);
static void load_indicators (PanelService *);
static void load_indicators_from_indicator_files (PanelService *);
static void sort_indicators (PanelService *);
static void notify_object (IndicatorObject *object);
static void update_keybinding (GSettings *, const gchar *, gpointer);
static void emit_upstart_event (const gchar *);
static gchar * get_indicator_entry_id_by_entry (IndicatorObjectEntry *entry);
static IndicatorObjectEntry * get_indicator_entry_by_id (PanelService *self, const gchar *entry_id);
static GdkFilterReturn event_filter (GdkXEvent *, GdkEvent *, PanelService *);

/*
 * GObject stuff
 */

static void
panel_service_class_dispose (GObject *self)
{
  PanelServicePrivate *priv = PANEL_SERVICE (self)->priv;
  gint i;

  g_idle_remove_by_data (self);
  gdk_window_remove_filter (NULL, (GdkFilterFunc)event_filter, self);

  if (!lockscreen_mode)
    emit_upstart_event ("indicator-services-end");

  if (GTK_IS_WIDGET (priv->last_menu) &&
      gtk_widget_get_realized (GTK_WIDGET (priv->last_menu)))
    {
      gtk_menu_popdown (GTK_MENU (priv->last_menu));
      g_signal_handlers_disconnect_by_data (priv->last_menu, self);
      priv->last_menu = NULL;
    }

  for (i = 0; i < N_TIMEOUT_SLOTS; i++)
    {
      if (priv->timeouts[i] > 0 && priv->timeouts[i] != SYNC_WAITING)
        {
          g_source_remove (priv->timeouts[i]);
          priv->timeouts[i] = 0;
        }
    }

  if (G_IS_OBJECT (priv->gsettings))
    {
      g_signal_handlers_disconnect_matched (priv->gsettings, G_SIGNAL_MATCH_FUNC,
                                            0, 0, NULL, update_keybinding, NULL);
      g_object_unref (priv->gsettings);
      priv->gsettings = NULL;
    }

  if (priv->dropdown_entries)
    {
      g_slist_free_full (priv->dropdown_entries, g_free);
      priv->dropdown_entries = NULL;
    }

  G_OBJECT_CLASS (panel_service_parent_class)->dispose (self);
}

static void
panel_service_class_finalize (GObject *object)
{
  PanelServicePrivate *priv = PANEL_SERVICE (object)->priv;

  g_hash_table_destroy (priv->id2entry_hash);
  g_hash_table_destroy (priv->panel2entries_hash);

  static_service = NULL;

  G_OBJECT_CLASS (panel_service_parent_class)->finalize (object);
}

static void
panel_service_class_init (PanelServiceClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  obj_class->dispose = panel_service_class_dispose;
  obj_class->finalize = panel_service_class_finalize;

  /* Signals */
  _service_signals[ENTRY_ACTIVATED] =
    g_signal_new ("entry-activated",
                  G_OBJECT_CLASS_TYPE (obj_class),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 6, G_TYPE_STRING, G_TYPE_STRING,
                  G_TYPE_INT, G_TYPE_INT, G_TYPE_UINT, G_TYPE_UINT);

  _service_signals[RE_SYNC] =
    g_signal_new ("re-sync",
                  G_OBJECT_CLASS_TYPE (obj_class),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1, G_TYPE_STRING);

 _service_signals[ENTRY_ACTIVATE_REQUEST] =
    g_signal_new ("entry-activate-request",
                  G_OBJECT_CLASS_TYPE (obj_class),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1, G_TYPE_STRING);

 _service_signals[GEOMETRIES_CHANGED] =
    g_signal_new ("geometries-changed",
                  G_OBJECT_CLASS_TYPE (obj_class),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 6,
                  G_TYPE_OBJECT, G_TYPE_POINTER,
                  G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT);

  _service_signals[INDICATORS_CLEARED] =
    g_signal_new ("indicators-cleared",
                  G_OBJECT_CLASS_TYPE (obj_class),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  g_type_class_add_private (obj_class, sizeof (PanelServicePrivate));
}

static gboolean
rect_contains_point (GdkRectangle* rect, gint x, gint y)
{
  if (!rect)
    return FALSE;

  return (x >= rect->x && x <= (rect->x + rect->width) &&
          y >= rect->y && y <= (rect->y + rect->height));
}

gboolean
entry_has_dropdown_id (const gchar *entry_id)
{
  return g_str_has_suffix (entry_id, "-dropdown");
}

IndicatorObjectEntry *
get_entry_at (PanelService *self, gint x, gint y)
{
  GHashTableIter panel_iter, entries_iter;
  gpointer key, value, k, v;

  g_hash_table_iter_init (&panel_iter, self->priv->panel2entries_hash);
  while (g_hash_table_iter_next (&panel_iter, &key, &value))
    {
      GHashTable *entry2geometry_hash = value;
      g_hash_table_iter_init (&entries_iter, entry2geometry_hash);

      while (g_hash_table_iter_next (&entries_iter, &k, &v))
        {
          IndicatorObjectEntry *entry = k;
          GdkRectangle *geo = v;

          if (rect_contains_point (geo, x, y))
            {
              return entry;
            }
        }
    }

  return NULL;
}

static IndicatorObjectEntry *
get_entry_at_panel (PanelService *self, const gchar *panel, gint x, gint y)
{
  GHashTable *entry2geometry_hash;
  GHashTableIter entries_iter;
  gpointer key, value;

  if (!panel)
    return NULL;

  entry2geometry_hash = g_hash_table_lookup (self->priv->panel2entries_hash, panel);

  if (!entry2geometry_hash)
    return NULL;

  g_hash_table_iter_init (&entries_iter, entry2geometry_hash);
  while (g_hash_table_iter_next (&entries_iter, &key, &value))
    {
      IndicatorObjectEntry *entry = key;
      GdkRectangle *geo = value;

      if (rect_contains_point (geo, x, y))
        {
          return entry;
        }
    }

  return NULL;
}

static const gchar*
get_panel_for_parent_at (PanelService *self, guint parent, gint x, gint y)
{
  GHashTableIter panel_iter, entries_iter;
  gpointer key, value, k, v;

  g_hash_table_iter_init (&panel_iter, self->priv->panel2entries_hash);
  while (g_hash_table_iter_next (&panel_iter, &key, &value))
    {
      const gchar *panel_id = key;
      GHashTable *entry2geometry_hash = value;
      g_hash_table_iter_init (&entries_iter, entry2geometry_hash);

      while (g_hash_table_iter_next (&entries_iter, &k, &v))
        {
          IndicatorObjectEntry *entry = k;
          GdkRectangle *geo = v;

          /* The entry might be invalid at this point (as it could have been
           * removed, but still not synced), so it's better to double check */
          if (g_slist_find (self->priv->removed_entries, entry))
            continue;

          if (!parent || entry->parent_window == parent)
            {
              if (rect_contains_point (geo, x, y))
                {
                  return panel_id;
                }
            }
        }
    }

  return NULL;
}

static IndicatorObject *
get_entry_parent_indicator (IndicatorObjectEntry *entry)
{
  if (!entry || !INDICATOR_IS_OBJECT (entry->parent_object))
    return NULL;

  return INDICATOR_OBJECT (entry->parent_object);
}

static gchar *
get_indicator_entry_id_by_entry (IndicatorObjectEntry *entry)
{
  gchar *entry_id = NULL;

  if (g_slist_find (static_service->priv->dropdown_entries, entry))
    {
      return g_strdup (entry->name_hint);
    }

  if (entry)
    entry_id = g_strdup_printf ("%p", entry);

  return entry_id;
}

static IndicatorObjectEntry *
get_indicator_entry_by_id (PanelService *self, const gchar *entry_id)
{
  IndicatorObjectEntry *entry;

  entry = g_hash_table_lookup (self->priv->id2entry_hash, entry_id);

  if (entry)
    {
      if (g_slist_find (self->priv->indicators, entry->parent_object))
        {
          if (!INDICATOR_IS_OBJECT (entry->parent_object))
            entry = NULL;
        }
      else
        {
          entry = NULL;
        }

      if (!entry)
        {
          g_warning("The entry id '%s' you're trying to lookup is not a valid IndicatorObjectEntry!", entry_id);
        }
    }

  if (!entry && entry_has_dropdown_id (entry_id))
    {
      /* Unity might register some "fake" dropdown entries that it might use to
       * to present long menu bars (right now only for appmenu indicator) */
      entry = g_new0 (IndicatorObjectEntry, 1);
      entry->parent_object = self->priv->appmenu_indicator;
      entry->name_hint = g_strdup (entry_id);
      self->priv->dropdown_entries = g_slist_append (self->priv->dropdown_entries, entry);

      /* Now the Hashtable owns the name_hint, so no need to manually free it */
      g_hash_table_insert (self->priv->id2entry_hash, (gpointer)entry->name_hint, entry);
    }

  return entry;
}

static void
ensure_entry_menu_is_closed (PanelService *self,
                             const gchar *panel_id,
                             IndicatorObjectEntry *entry)
{
  PanelServicePrivate  *priv = self->priv;

  /* If the entry has been removed let's make sure that its menu is closed */
  if (GTK_IS_MENU (priv->last_menu) && priv->last_menu == entry->menu)
    {
      if (!priv->last_panel || !panel_id || g_strcmp0 (priv->last_panel, panel_id) == 0)
        gtk_menu_popdown (entry->menu);
    }
}

static void
reinject_key_event_to_root_window (XIDeviceEvent *ev)
{
  XKeyEvent kev;
  kev.display = ev->display;
  kev.window = ev->root;
  kev.root = ev->root;
  kev.subwindow = None;
  kev.time = ev->time;
  kev.x = ev->event_x;
  kev.y = ev->event_x;
  kev.x_root = ev->root_x;
  kev.y_root = ev->root_y;
  kev.same_screen = True;
  kev.keycode = ev->detail;
  kev.state = ev->mods.base;
  kev.type = ev->evtype;

  XSendEvent (kev.display, kev.root, True, KeyPressMask, (XEvent*) &kev);
  XFlush (kev.display);
}

static gboolean
event_matches_keybinding (guint32 modifiers, KeySym key, KeyBinding *kb)
{
  if (modifiers == kb->modifiers && key != NoSymbol)
    {
      if (key == kb->key || key == kb->fallback)
        {
          return TRUE;
        }
    }

  return FALSE;
}

static gboolean
is_special_keysym (KeySym keysym)
{
  /* Multimedia keys, see X11/XF86keysym.h */
  if (keysym >= 0x1008FF00 && keysym <= 0x1008FFFF)
    return TRUE;

  return FALSE;
}

static gboolean
is_control_keysym (KeySym keysym)
{
  if (!is_special_keysym (keysym))
    return FALSE;

  /* Display backlight controls */
  if (keysym >= 0x1008FF01 && keysym <= 0x1008FF0F)
    return TRUE;

  switch (keysym)
    {
      case XF86XK_Battery:
      case XF86XK_Bluetooth:
      case XF86XK_WLAN:
      case XF86XK_UWB:
        return !lockscreen_mode;
      case XF86XK_Suspend:
      case XF86XK_Hibernate:
      case XF86XK_Sleep:
      case XF86XK_PowerOff:
      case XF86XK_ScreenSaver:
        return lockscreen_mode;
    }

  const gchar *keystr = XKeysymToString (keysym);

  if (g_str_has_prefix (keystr, "XF86Audio") ||
      g_str_has_prefix (keystr, "XF86Touchpad"))
    {
      return TRUE;
    }

  return FALSE;
}

static gboolean
is_allowed_keysym (KeySym keysym)
{
  if (keysym == XK_Print)
    return TRUE;

  if (is_special_keysym (keysym))
    return TRUE;

  return FALSE;
}

static GdkFilterReturn
event_filter (GdkXEvent *ev, GdkEvent *gev, PanelService *self)
{
  PanelServicePrivate *priv = self->priv;
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
  if (cookie->type != GenericEvent)
    return ret;

  XIDeviceEvent *event = cookie->data;
  if (!event)
    return ret;

  switch (event->evtype)
    {
      case XI_KeyPress:
        {
          KeySym keysym = XkbKeycodeToKeysym (event->display, event->detail, 0, 0);

          if (lockscreen_mode)
            {
              if (is_control_keysym (keysym))
                {
                  reinject_key_event_to_root_window (event);
                  ret = GDK_FILTER_REMOVE;
                }

              break;
            }

          if (event_matches_keybinding (event->mods.base, keysym, &priv->menu_toggle) ||
              event_matches_keybinding (event->mods.base, keysym, &priv->show_dash) ||
              event_matches_keybinding (event->mods.base, keysym, &priv->show_hud))
            {
              if (GTK_IS_MENU (priv->last_menu))
                gtk_menu_popdown (GTK_MENU (priv->last_menu));

              ret = GDK_FILTER_REMOVE;
            }
          else if (event->mods.base != GDK_CONTROL_MASK)
            {
              if (!IsModifierKey (keysym) && (event->mods.base != 0 || is_allowed_keysym (keysym)))
                {
                  if (GTK_IS_MENU (priv->last_menu) && !is_control_keysym (keysym))
                    gtk_menu_popdown (GTK_MENU (priv->last_menu));

                  reinject_key_event_to_root_window (event);
                  ret = GDK_FILTER_REMOVE;
                }
            }

          break;
        }

      case XI_ButtonPress:
        {
          priv->pressed_entry = get_entry_at_panel (self, priv->last_panel, event->root_x, event->root_y);
          priv->use_event = (priv->pressed_entry == NULL);

          if (priv->pressed_entry)
            ret = GDK_FILTER_REMOVE;

          break;
        }

      case XI_ButtonRelease:
        {
          IndicatorObjectEntry *entry = NULL;
          gboolean event_is_a_click = FALSE;
          entry = get_entry_at_panel (self, priv->last_panel, event->root_x, event->root_y);

          if (event->detail == 1 || event->detail == 3)
            {
              /* Consider only right and left clicks over the indicators entries */
              event_is_a_click = TRUE;
            }
          else if (entry && event->detail == 2)
            {
              /* Middle clicks over an appmenu entry are considered just like
               * all other clicks */
              if (get_entry_parent_indicator (entry) == priv->appmenu_indicator)
                {
                  event_is_a_click = TRUE;
                }
            }

          if (event_is_a_click)
            {
              if (priv->use_event)
                {
                  priv->use_event = FALSE;
                }
              else
                {
                  if (entry)
                    {
                      if (entry != priv->pressed_entry)
                        {
                          ret = GDK_FILTER_REMOVE;
                          priv->use_event = TRUE;
                        }
                      else if (priv->last_entry && entry != priv->last_entry)
                        {
                          /* If we were navigating over indicators using the keyboard
                           * and now we click over the indicator under the mouse, we
                           * must force it to show back again, not make it close */
                          gchar *entry_id = get_indicator_entry_id_by_entry (entry);
                          g_signal_emit (self, _service_signals[ENTRY_ACTIVATE_REQUEST], 0, entry_id);
                          g_free (entry_id);
                        }
                    }
                }
            }
          else if (entry && (event->detail == 2 || event->detail >= 4 || event->detail <= 7))
            {
              /* If we're scrolling or middle-clicking over an indicator
               * (which is not an appmenu entry) then we need to send the
               * event to the indicator itself, and avoid it to close */
              gchar *entry_id = get_indicator_entry_id_by_entry (entry);

              if (event->detail >= 4 || event->detail <= 7)
                {
                  gint32 delta = (event->detail >= 6) ? NUX_HORIZONTAL_SCROLL_DELTA : NUX_VERTICAL_SCROLL_DELTA;
                  delta = (event->detail % 2 == 0) ? delta : delta * -1;
                  panel_service_scroll_entry (self, entry_id, delta);
                }
              else if (event->detail == 2 && entry == priv->pressed_entry)
                {
                  panel_service_secondary_activate_entry (self, entry_id);
                }

              ret = GDK_FILTER_REMOVE;
              g_free (entry_id);
            }

          break;
        }
    }

  return ret;
}

static gboolean
initial_resync (PanelService *self)
{
  g_signal_emit (self, _service_signals[RE_SYNC], 0, "");
  return G_SOURCE_REMOVE;
}

static gboolean
ready_signal (PanelService *self)
{
  if (!lockscreen_mode)
    emit_upstart_event ("indicator-services-start");

  return G_SOURCE_REMOVE;
}

static void
update_keybinding (GSettings *settings, const gchar *key, gpointer data)
{
  KeyBinding *kb = data;
  gchar *binding = g_settings_get_string (settings, key);
  parse_string_keybinding (binding, kb);
  g_free (binding);
}

void
parse_string_keybinding (const char *str, KeyBinding *kb)
{
  kb->key = NoSymbol;
  kb->fallback = NoSymbol;
  kb->modifiers = 0;

  gchar *binding = g_strdup (str);
  gchar *keystart = (binding) ? strrchr (binding, '>') : NULL;

  if (!keystart)
    keystart = binding;

  while (keystart && *keystart != '\0' && !g_ascii_isalnum (*keystart))
    keystart++;

  gchar *keyend = keystart;

  while (keyend && g_ascii_isalnum (*keyend))
    keyend++;

  if (keystart != keyend)
    {
      gchar *keystr = g_strndup (keystart, keyend-keystart);
      kb->key = XStringToKeysym (keystr);
      g_free (keystr);
    }
  else
    {
      /* Parsing the case where we only have meta-keys */
      keyend = (binding) ? strrchr (binding, '>') : NULL;
      keyend = keyend ? keyend - 1 : NULL;
      keystart = keyend;

      while (keystart && keystart > binding && g_ascii_isalnum (*keystart))
        keystart--;

      if (keystart != keyend)
        {
          gchar *keystr = g_strndup (keystart+1, keyend-keystart);
          gchar *left = g_strconcat (keystr, "_L", NULL);
          kb->key = XStringToKeysym (left);

          gchar *right = g_strconcat (keystr, "_R", NULL);
          kb->fallback = XStringToKeysym (right);
          g_free (left);
          g_free (right);
          g_free (keystr);

          keystr = g_strndup (binding, keystart-binding);
          g_free (binding);
          binding = keystr;
        }
    }

  if (kb->key != NoSymbol)
    {
      if (g_strrstr (binding, "<Shift>"))
        {
          kb->modifiers |= ShiftMask;
        }
      if (g_strrstr (binding, "<Control>") || g_strrstr (binding, "<Primary>"))
        {
          kb->modifiers |= ControlMask;
        }
      if (g_strrstr (binding, "<Alt>") || g_strrstr (binding, "<Mod1>"))
        {
          kb->modifiers |= AltMask;
        }
      if (g_strrstr (binding, "<Super>"))
        {
          kb->modifiers |= SuperMask;
        }
    }

  g_free (binding);
}

static void
emit_upstart_event (const gchar *event)
{
  const gchar *upstartsession = g_getenv ("UPSTART_SESSION");

  if (!upstartsession)
    return;

  GError *error = NULL;
  GDBusConnection* conn = g_dbus_connection_new_for_address_sync (upstartsession,
                                                                  G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT,
                                                                  NULL, NULL, &error);

  if (error)
    {
      g_warning ("Unable to connect to Upstart session: %s", error->message);
      g_error_free (error);
      return;
    }

  GVariant *result = g_dbus_connection_call_sync (conn, DBUS_SERVICE_UPSTART,
                                                  DBUS_PATH_UPSTART,
                                                  DBUS_INTERFACE_UPSTART,
                                                  "EmitEvent",
                                                  g_variant_new ("(sasb)", event, NULL, 0),
                                                  NULL, G_DBUS_CALL_FLAGS_NO_AUTO_START, -1,
                                                  NULL, &error);

  if (error)
    {
      g_warning ("Unable to emit Upstart event: %s", error->message);
      g_error_free (error);
    }
  else
    {
      g_variant_unref (result);
    }

  g_object_unref (conn);
}


static void
panel_service_init (PanelService *self)
{
  PanelServicePrivate *priv;
  priv = self->priv = GET_PRIVATE (self);

  gdk_window_add_filter (NULL, (GdkFilterFunc)event_filter, self);

  priv->id2entry_hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  priv->panel2entries_hash = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                    g_free,
                                                    (GDestroyNotify) g_hash_table_destroy);

  priv->gsettings = g_settings_new_with_path (COMPIZ_OPTION_SCHEMA, COMPIZ_OPTION_PATH);
  g_signal_connect (priv->gsettings, "changed::"MENU_TOGGLE_KEYBINDING_KEY,
                    G_CALLBACK (update_keybinding), &priv->menu_toggle);
  g_signal_connect (priv->gsettings, "changed::"SHOW_DASH_KEY,
                    G_CALLBACK (update_keybinding), &priv->show_dash);
  g_signal_connect (priv->gsettings, "changed::"SHOW_HUD_KEY,
                    G_CALLBACK (update_keybinding), &priv->show_hud);

  update_keybinding (priv->gsettings, MENU_TOGGLE_KEYBINDING_KEY, &priv->menu_toggle);
  update_keybinding (priv->gsettings, SHOW_DASH_KEY, &priv->show_dash);
  update_keybinding (priv->gsettings, SHOW_HUD_KEY, &priv->show_hud);
}

static gboolean
panel_service_check_cleared (PanelService *self)
{
  if (self->priv->indicators == NULL)
    {
      g_signal_emit (self, _service_signals[INDICATORS_CLEARED], 0);
      return G_SOURCE_REMOVE;
    }

  return G_SOURCE_CONTINUE;
}

static void
panel_service_actually_remove_indicator (PanelService *self, IndicatorObject *indicator)
{
  g_return_if_fail (PANEL_IS_SERVICE (self));
  g_return_if_fail (INDICATOR_IS_OBJECT (indicator));

  GList *entries, *l;
  gpointer timeout;
  gint position;

  g_signal_handlers_disconnect_by_data (indicator, self);

  position = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (indicator), "position"));
  if (self->priv->timeouts[position] > 0)
    {
      g_source_remove (self->priv->timeouts[position]);
      self->priv->timeouts[position] = SYNC_NEUTRAL;
    }

  timeout = g_object_get_data (G_OBJECT (indicator), "remove-timeout");
  if (timeout)
    {
      g_source_remove (GPOINTER_TO_UINT (timeout));
      g_object_set_data (G_OBJECT (indicator), "remove-timeout", GUINT_TO_POINTER (0));
    }

  entries = indicator_object_get_entries (indicator);

  if (entries)
    {
      for (l = entries; l; l = l->next)
        {
          IndicatorObjectEntry *entry;
          gchar *entry_id;
          GHashTableIter iter;
          gpointer key, value;
          GSList *ll;

          entry = l->data;

          if (entry->label)
            {
              g_signal_handlers_disconnect_by_data (entry->label, indicator);
            }

          if (entry->image)
            {
              g_signal_handlers_disconnect_by_data (entry->image, indicator);
            }

          if ((ll = g_slist_find (self->priv->dropdown_entries, entry)))
            {
              self->priv->dropdown_entries = g_slist_delete_link (self->priv->dropdown_entries, ll);
              g_hash_table_remove (self->priv->id2entry_hash, entry->name_hint);
              g_free (entry);
            }
          else
            {
              entry_id = get_indicator_entry_id_by_entry (entry);
              g_hash_table_remove (self->priv->id2entry_hash, entry_id);
              g_free (entry_id);
            }

          g_hash_table_iter_init (&iter, self->priv->panel2entries_hash);
          while (g_hash_table_iter_next (&iter, &key, &value))
            {
              GHashTable *entry2geometry_hash = value;

              if (g_hash_table_size (entry2geometry_hash) > 1)
                g_hash_table_remove (entry2geometry_hash, entry);
              else
                g_hash_table_iter_remove (&iter);
            }
        }

      g_list_free (entries);
    }

  self->priv->indicators = g_slist_remove (self->priv->indicators, indicator);

  if (g_object_is_floating (G_OBJECT (indicator)))
    {
      g_object_ref_sink (G_OBJECT (indicator));
    }

  if (self->priv->appmenu_indicator == indicator)
    self->priv->appmenu_indicator = NULL;

  g_object_unref (G_OBJECT (indicator));
}

static gboolean
panel_service_indicator_remove_timeout (IndicatorObject *indicator)
{
  PanelService *self = panel_service_get_default ();
  panel_service_actually_remove_indicator (self, indicator);

  return FALSE;
}

static PanelService *
get_or_init_static_service (gboolean* is_new)
{
  if (is_new) *is_new = FALSE;

  if (!PANEL_IS_SERVICE (static_service))
    {
      static_service = g_object_new (PANEL_TYPE_SERVICE, NULL);
      if (is_new) *is_new = TRUE;
    }

  return static_service;
}

static void
initial_load_default_or_custom_indicators (PanelService *self, GList *indicators)
{
  GList *l;

  suppress_signals = TRUE;

  if (!indicators)
    {
      if (!lockscreen_mode)
        {
          load_indicators (self);
        }

      load_indicators_from_indicator_files (self);
      sort_indicators (self);
    }
  else
    {
      for (l = indicators; l; l = l->next)
        {
          IndicatorObject *object = l->data;

          if (INDICATOR_IS_OBJECT (object))
            panel_service_add_indicator (self, object);
        }
    }

  suppress_signals = FALSE;

  g_idle_add ((GSourceFunc)initial_resync, self);
  g_idle_add ((GSourceFunc)ready_signal, self);
}

PanelService *
panel_service_get_default ()
{
  gboolean is_new;
  PanelService *self = get_or_init_static_service (&is_new);

  if (is_new)
    {
      initial_load_default_or_custom_indicators (self, NULL);
    }

  return self;
}

PanelService *
panel_service_get_default_with_indicators (GList *indicators)
{
  gboolean is_new;
  PanelService *self = get_or_init_static_service (&is_new);

  if (is_new && indicators)
    {
      initial_load_default_or_custom_indicators (self, indicators);
    }

  return self;
}

void
panel_service_set_lockscreen_mode (gboolean enable)
{
  lockscreen_mode = enable;
}

guint
panel_service_get_n_indicators (PanelService *self)
{
  g_return_val_if_fail (PANEL_IS_SERVICE (self), 0);

  return g_slist_length (self->priv->indicators);
}

IndicatorObject *
panel_service_get_indicator_nth (PanelService *self, guint position)
{
  g_return_val_if_fail (PANEL_IS_SERVICE (self), NULL);

  return (IndicatorObject *) g_slist_nth_data (self->priv->indicators, position);
}

IndicatorObject *
panel_service_get_indicator (PanelService *self, const gchar *indicator_id)
{
  g_return_val_if_fail (PANEL_IS_SERVICE (self), NULL);
  GSList *l;

  for (l = self->priv->indicators; l; l = l->next)
    {
      if (g_strcmp0 (indicator_id,
                     g_object_get_data (G_OBJECT (l->data), "id")) == 0)
        {
          return (IndicatorObject *) l->data;
        }
    }

  return NULL;
}

void
panel_service_add_indicator (PanelService *self, IndicatorObject *indicator)
{
  g_return_if_fail (PANEL_IS_SERVICE (self));
  g_return_if_fail (INDICATOR_IS_OBJECT (indicator));

  g_object_ref (indicator);
  load_indicator (self, indicator, NULL);
}

static void
panel_service_prepare_indicator_removal (PanelService *self,
                                         IndicatorObject *indicator,
                                         gboolean notify)
{
  g_object_set_data (G_OBJECT (indicator), "remove", GINT_TO_POINTER (TRUE));
  gpointer timeout = g_object_get_data (G_OBJECT (indicator), "remove-timeout");

  if (timeout)
    {
      g_source_remove (GPOINTER_TO_UINT (timeout));
    }

  if (notify)
    {
      notify_object (indicator);
    }

  guint id = g_timeout_add_seconds (1,
                                    (GSourceFunc) panel_service_indicator_remove_timeout,
                                    indicator);
  g_object_set_data (G_OBJECT (indicator), "remove-timeout", GUINT_TO_POINTER (id));
}

void
panel_service_remove_indicator (PanelService *self, IndicatorObject *indicator)
{
  g_return_if_fail (PANEL_IS_SERVICE (self));
  g_return_if_fail (INDICATOR_IS_OBJECT (indicator));

  panel_service_prepare_indicator_removal (self, indicator, TRUE);
}

void
panel_service_clear_indicators (PanelService *self)
{
  g_return_if_fail (PANEL_IS_SERVICE (self));
  GSList *l = self->priv->indicators;

  while (l)
    {
      IndicatorObject *ind = l->data;
      l = l->next;
      panel_service_prepare_indicator_removal (self, ind, FALSE);
    }

  g_signal_emit (self, _service_signals[RE_SYNC], 0, "");
  g_idle_add ((GSourceFunc)panel_service_check_cleared, self);
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

  return G_SOURCE_REMOVE;
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
  g_return_if_fail (PANEL_IS_SERVICE (self));
  g_return_if_fail (entry != NULL);

  self->priv->removed_entries = g_slist_remove (self->priv->removed_entries, entry);
  gchar *entry_id = get_indicator_entry_id_by_entry (entry);
  g_hash_table_insert (self->priv->id2entry_hash, entry_id, entry);

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

static gboolean
on_removed_idle (PanelService *self)
{
  GHashTableIter iter;
  GHashTable *entry2geometry_hash;
  IndicatorObjectEntry *entry;
  gpointer value;
  GSList *l;

  for (l = self->priv->removed_entries; l; l = l->next)
    {
      entry = l->data;
      ensure_entry_menu_is_closed (self, NULL, entry);

      g_hash_table_iter_init (&iter, self->priv->panel2entries_hash);
      while (g_hash_table_iter_next (&iter, NULL, &value))
        {
          if ((entry2geometry_hash = value))
            {
              g_hash_table_remove (entry2geometry_hash, entry);

              if (g_hash_table_size (entry2geometry_hash) == 0)
                {
                  g_hash_table_iter_remove (&iter);
                }
            }
        }

    }

  g_slist_free (self->priv->removed_entries);
  self->priv->removed_entries = NULL;
  self->priv->remove_idle = 0;

  return G_SOURCE_REMOVE;
}

static void
on_entry_removed (IndicatorObject      *object,
                  IndicatorObjectEntry *entry,
                  PanelService         *self)
{
  g_return_if_fail (PANEL_IS_SERVICE (self));
  g_return_if_fail (entry != NULL);

  gchar *entry_id = get_indicator_entry_id_by_entry (entry);
  g_hash_table_remove (self->priv->id2entry_hash, entry_id);
  g_free (entry_id);

  if (entry->label)
    {
      g_signal_handlers_disconnect_by_data (entry->label, object);
    }

  if (entry->image)
    {
      g_signal_handlers_disconnect_by_data (entry->image, object);
    }

  self->priv->removed_entries = g_slist_append (self->priv->removed_entries, entry);

  if (self->priv->remove_idle)
    g_source_remove (self->priv->remove_idle);

  self->priv->remove_idle = g_idle_add ((GSourceFunc) on_removed_idle, self);

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

  if (!entry)
    {
      if (GTK_IS_MENU (self->priv->last_menu))
        gtk_menu_popdown (GTK_MENU (self->priv->last_menu));

      return;
    }

  entry_id = get_indicator_entry_id_by_entry (entry);
  g_signal_emit (self, _service_signals[ENTRY_ACTIVATE_REQUEST], 0, entry_id);
  g_free (entry_id);
}

static const gchar * indicator_environment[] = {
  "unity",
  "unity-3d",
  "unity-panel-service",
  "unity-all-menus",
  NULL
};

static void
load_indicator (PanelService *self, IndicatorObject *object, const gchar *_name)
{
  PanelServicePrivate *priv = self->priv;
  gchar *name;
  GList *entries, *entry;

  indicator_object_set_environment (object, (GStrv)indicator_environment);

  if (_name != NULL)
    name = g_strdup (_name);
  else
    name = g_strdup_printf ("%p", object);

  priv->indicators = g_slist_append (priv->indicators, object);

  if (!priv->appmenu_indicator && g_strcmp0 (name, APPMENU_INDICATOR_NAME) == 0)
    priv->appmenu_indicator = object;

  g_object_set_data_full (G_OBJECT (object), "id", g_strdup (name), g_free);

  g_signal_connect (object, INDICATOR_OBJECT_SIGNAL_ENTRY_ADDED,
                    G_CALLBACK (on_entry_added), self);
  g_signal_connect (object, INDICATOR_OBJECT_SIGNAL_ENTRY_REMOVED,
                    G_CALLBACK (on_entry_removed), self);
  g_signal_connect (object, INDICATOR_OBJECT_SIGNAL_ENTRY_MOVED,
                    G_CALLBACK (on_entry_moved), self);
  g_signal_connect (object, INDICATOR_OBJECT_SIGNAL_MENU_SHOW,
                    G_CALLBACK (on_indicator_menu_show), self);

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

static void
load_indicators_from_indicator_files (PanelService *self)
{
  GDir *dir;
  const gchar *name;
  GError *error = NULL;

  dir = g_dir_open (INDICATOR_SERVICE_DIR, 0, &error);
  if (!dir)
    {
      g_warning ("unable to open indicator service file directory: %s", error->message);
      g_error_free (error);
      return;
    }

  while ((name = g_dir_read_name (dir)))
    {
      gchar *filename;
      IndicatorNg *indicator;

      filename = g_build_filename (INDICATOR_SERVICE_DIR, name, NULL);
      indicator = indicator_ng_new_for_profile (filename, !lockscreen_mode ? "desktop" : "desktop_lockscreen", &error);
      if (indicator)
        {
          load_indicator (self, INDICATOR_OBJECT (indicator), name);
        }
      else
        {
          g_warning ("unable to load '%s': %s", name, error->message);
          g_clear_error (&error);
        }

      g_free (filename);
    }

  g_dir_close (dir);
}

static gint
name2order (const gchar * name, const gchar * hint)
{
  int i;

  for (i = 0; indicator_order[i][0] != NULL; i++)
    {
      if (g_strcmp0(name, indicator_order[i][0]) == 0 &&
          g_strcmp0(hint, indicator_order[i][1]) == 0)
        {
          return i;
        }
    }
  return -1;
}

static gint
name2priority (const gchar * name, const gchar * hint)
{
  gint order = name2order (name, hint);
  if (order > -1)
    return order * MAX_INDICATOR_ENTRIES;

  return order;
}

static void
sort_indicators (PanelService *self)
{
  GSList *i;
  int     k = 0;
  int     floating_indicators = 0;
  int     max_priority = G_N_ELEMENTS(indicator_order) * MAX_INDICATOR_ENTRIES;

  for (i = self->priv->indicators; i; i = i->next)
    {
      IndicatorObject *io = i->data;
      gint pos;

      pos = indicator_object_get_position (io);

      /* Continue using the state ordering as long as there are still
       * plugins statically defined in this file. Give them a much
       * higher position though, so that they appear to the right of the
       * indicators that return a proper position */
      if (pos < 0)
        {
          gint prio = name2priority (g_object_get_data (G_OBJECT (io), "id"), NULL);

          if (prio < 0)
            {
              prio = name2priority ("floating-indicators", NULL) + floating_indicators;
              floating_indicators++;
            }

          pos = max_priority - prio;
        }

      /* unity's concept of priorities is inverse to ours right now */
      g_object_set_data (G_OBJECT (i->data), "priority", GINT_TO_POINTER (max_priority - pos));

      g_object_set_data (G_OBJECT (i->data), "position", GINT_TO_POINTER (k));
      self->priv->timeouts[k] = SYNC_NEUTRAL;
      k++;
    }
}

static gchar *
gtk_image_to_data (GtkImage *image, guint32 *storage_type)
{
  if (!GTK_IS_IMAGE (image))
    return NULL;

  *storage_type = gtk_image_get_storage_type (image);
  gchar *ret = NULL;

  switch (*storage_type)
    {
      case GTK_IMAGE_PIXBUF:
      {
        gchar *file;
        g_object_get (image, "file", &file, NULL);

        if (file)
          {
            if (file[0] != '\0')
              {
                *storage_type = GTK_IMAGE_ICON_NAME;
                ret = file;
                break;
              }

            g_free (file);
          }

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
          }

        break;
      }
      case GTK_IMAGE_STOCK:
      {
        g_object_get (G_OBJECT (image), "stock", &ret, NULL);
        break;
      }
      case GTK_IMAGE_ICON_NAME:
      {
        g_object_get (G_OBJECT (image), "icon-name", &ret, NULL);
        break;
      }
      case GTK_IMAGE_GICON:
      {
        GIcon *icon = NULL;
        gtk_image_get_gicon (image, &icon, NULL);
        if (G_IS_ICON (icon))
          ret = g_icon_to_string (icon);

        break;
      }
      case GTK_IMAGE_EMPTY:
      {
        break;
      }
    default:
      {
        g_warning ("Unable to support GtkImageType: %u", *storage_type);
      }
    }

  return ret;
}

static void
indicator_entry_to_variant (IndicatorObjectEntry *entry,
                            const gchar          *id,
                            const gchar          *indicator_id,
                            GVariantBuilder      *b,
                            gint                  prio)
{
  gboolean is_label = GTK_IS_LABEL (entry->label);
  gboolean is_image = GTK_IS_IMAGE (entry->image);
  guint32 image_type = 0;
  gchar *image_data = gtk_image_to_data (entry->image, &image_type);

  g_variant_builder_add (b, ENTRY_SIGNATURE,
                         indicator_id,
                         id,
                         entry->name_hint ? entry->name_hint : "",
                         entry->parent_window,
                         is_label ? gtk_label_get_label (entry->label) : "",
                         is_label ? gtk_widget_get_sensitive (GTK_WIDGET (entry->label)) : FALSE,
                         is_label ? gtk_widget_get_visible (GTK_WIDGET (entry->label)) : FALSE,
                         is_image ? image_type : 0,
                         image_data ? image_data : "",
                         is_image ? gtk_widget_get_sensitive (GTK_WIDGET (entry->image)) : FALSE,
                         is_image ? gtk_widget_get_visible (GTK_WIDGET (entry->image)) : FALSE,
                         prio);

  g_free (image_data);
}

static void
indicator_entry_null_to_variant (const gchar     *indicator_id,
                                 GVariantBuilder *b)
{
  g_variant_builder_add (b, ENTRY_SIGNATURE,
                         indicator_id,
                         "",
                         "",
                         0,
                         "",
                         FALSE,
                         FALSE,
                         0,
                         "",
                         FALSE,
                         FALSE,
                         -1);
}

static void
indicator_object_full_to_variant (PanelService *self, IndicatorObject *object,
                                  const gchar *indicator_id, GVariantBuilder *b)
{
  GList *entries, *e;
  GHashTable *index_hash = NULL;
  gint parent_prio = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (object), "priority"));
  entries = indicator_object_get_entries (object);
  guint index = 0;

  if (entries)
    {
      if (object == self->priv->appmenu_indicator)
        index_hash = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, NULL);

      for (e = entries; e; e = e->next)
        {
          gint prio = -1;
          IndicatorObjectEntry *entry = e->data;
          gchar *id = get_indicator_entry_id_by_entry (entry);

          if (entry->name_hint)
            {
              prio = name2priority (indicator_id, entry->name_hint);
            }

          if (prio < 0)
            {
              if (index_hash)
                {
                  index = GPOINTER_TO_UINT (g_hash_table_lookup (index_hash,
                                                                 GUINT_TO_POINTER (entry->parent_window)));
                }

              prio = parent_prio + index;
              index++;

              if (index_hash)
                {
                  g_hash_table_insert (index_hash, GUINT_TO_POINTER (entry->parent_window),
                                                   GUINT_TO_POINTER (index));
                }
            }

          indicator_entry_to_variant (entry, id, indicator_id, b, prio);
          g_free (id);
        }

      if (index_hash)
        g_hash_table_destroy (index_hash);

      g_list_free (entries);
    }
  else
    {
      /* Add a null entry to indicate that there is an indicator here, it's just empty */
      indicator_entry_null_to_variant (indicator_id, b);
    }
}

static void
indicator_object_to_variant (PanelService *self, IndicatorObject *object,
                             const gchar *indicator_id, GVariantBuilder *b)
{
  if (!GPOINTER_TO_INT (g_object_get_data (G_OBJECT (object), "remove")))
    {
      indicator_object_full_to_variant (self, object, indicator_id, b);
    }
  else
    {
      indicator_entry_null_to_variant (indicator_id, b);
      panel_service_actually_remove_indicator (self, object);
    }
}

static int
get_monitor_at (gint x, gint y)
{
  gint i;
  gdouble scale;
  GdkScreen *screen = gdk_screen_get_default ();
  gint monitors = gdk_screen_get_n_monitors (screen);

  for (i = 0; i < monitors; ++i)
    {
      GdkRectangle rect = { 0 };
      gdk_screen_get_monitor_geometry (screen, i, &rect);
      scale = gdk_screen_get_monitor_scale_factor (screen, i);

      if (scale != 1.0)
        {
          rect.x *= scale;
          rect.y *= scale;
          rect.width *= scale;
          rect.height *= scale;
        }

      if (rect_contains_point (&rect, x, y))
        {
          return i;
        }
    }

  return gdk_screen_get_monitor_at_point (screen, x, y);
}

static int
get_monitor_scale_at (gint x, gint y)
{
  gint monitor = get_monitor_at (x, y);
  return gdk_screen_get_monitor_scale_factor (gdk_screen_get_default (), monitor);
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

  GdkScreen *screen = gdk_screen_get_default ();
  gint monitor = get_monitor_at (priv->last_x, priv->last_y);
  gtk_menu_set_monitor (menu, monitor);

  gint scale = gdk_screen_get_monitor_scale_factor (screen, monitor);
  *x = priv->last_x / scale;
  *y = priv->last_y / scale;
  *push = TRUE;
}

static void
on_active_menu_hidden (GtkMenu *menu, PanelService *self)
{
  PanelServicePrivate *priv = self->priv;
  g_signal_handlers_disconnect_by_data (priv->last_menu, self);

  priv->last_x = 0;
  priv->last_y = 0;
  priv->last_menu_button = 0;

  priv->last_panel = NULL;
  priv->last_menu = NULL;
  priv->last_entry = NULL;
  priv->last_left = 0;
  priv->last_right = 0;
  priv->last_top = 0;
  priv->last_bottom = 0;

  priv->use_event = FALSE;
  priv->pressed_entry = NULL;

  g_signal_emit (self, _service_signals[ENTRY_ACTIVATED], 0, "", "", 0, 0, 0, 0);
}


/*
 * Public Methods
 */
GVariant *
panel_service_sync (PanelService *self)
{
  GVariantBuilder b;
  IndicatorObject *indicator;
  GSList *i;
  gint position;

  g_variant_builder_init (&b, G_VARIANT_TYPE ("("ENTRY_ARRAY_SIGNATURE")"));
  g_variant_builder_open (&b, G_VARIANT_TYPE (ENTRY_ARRAY_SIGNATURE));

  for (i = self->priv->indicators; i;)
    {
      /* An indicator could be removed during this cycle, so we should be safe */
      indicator = i->data;
      i = i->next;

      const gchar *indicator_id = g_object_get_data (G_OBJECT (indicator), "id");
      position = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (indicator), "position"));
      indicator_object_to_variant (self, indicator, indicator_id, &b);

      /* Set the sync back to neutral */
      self->priv->timeouts[position] = SYNC_NEUTRAL;
    }

  g_variant_builder_close (&b);
  return g_variant_builder_end (&b);
}

GVariant *
panel_service_sync_one (PanelService *self, const gchar *indicator_id)
{
  GVariantBuilder b;
  GSList *i;

  g_variant_builder_init (&b, G_VARIANT_TYPE ("("ENTRY_ARRAY_SIGNATURE")"));
  g_variant_builder_open (&b, G_VARIANT_TYPE (ENTRY_ARRAY_SIGNATURE));

  for (i = self->priv->indicators; i; i = i->next)
    {
      if (g_strcmp0 (indicator_id,
                     g_object_get_data (G_OBJECT (i->data), "id")) == 0)
        {
          gint position = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (i->data), "position"));
          indicator_object_to_variant (self, i->data, indicator_id, &b);

          /* Set the sync back to neutral */
          self->priv->timeouts[position] = SYNC_NEUTRAL;

          break;
        }
    }

  g_variant_builder_close (&b);
  return g_variant_builder_end (&b);
}

void
panel_service_sync_geometry (PanelService *self,
                             const gchar *panel_id,
                             const gchar *entry_id,
                             gint x,
                             gint y,
                             gint width,
                             gint height)
{
  IndicatorObject *object;
  IndicatorObjectEntry *entry;
  GHashTable *entry2geometry_hash;
  gboolean valid_entry = TRUE;
  PanelServicePrivate  *priv = self->priv;

  entry = get_indicator_entry_by_id (self, entry_id);

  /* If the entry we read is not valid, maybe it has already been removed
   * or unparented, so we need to make sure that the related key on the
   * entry2geometry_hash is correctly removed and the value is free'd */
  if (!entry)
    {
      IndicatorObjectEntry *invalid_entry;
      if (sscanf (entry_id, "%p", &invalid_entry) == 1)
        {
          entry = invalid_entry;
          valid_entry = FALSE;
        }
    }

  if (entry)
    {
      entry2geometry_hash = g_hash_table_lookup (priv->panel2entries_hash, panel_id);

      if (width < 0 || height < 0 || !valid_entry)
        {
          if (valid_entry)
            {
              GSList *l;
              ensure_entry_menu_is_closed (self, panel_id, entry);

              if ((l = g_slist_find (self->priv->dropdown_entries, entry)))
                {
                  valid_entry = FALSE;
                  self->priv->dropdown_entries = g_slist_delete_link (self->priv->dropdown_entries, l);
                  g_hash_table_remove (self->priv->id2entry_hash, entry->name_hint);
                  g_free (entry);
                }
            }

          if (entry2geometry_hash)
            {
              g_hash_table_remove (entry2geometry_hash, entry);

              if (g_hash_table_size (entry2geometry_hash) == 0)
                {
                  g_hash_table_remove (priv->panel2entries_hash, panel_id);
                }
            }
        }
      else
        {
          GdkRectangle *geo = NULL;

          if (entry2geometry_hash == NULL)
            {
              entry2geometry_hash = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                                           NULL, g_free);
              g_hash_table_insert (priv->panel2entries_hash, g_strdup (panel_id),
                                   entry2geometry_hash);
            }
          else
            {
              geo = g_hash_table_lookup (entry2geometry_hash, entry);
            }

          if (geo == NULL)
            {
              geo = g_new0 (GdkRectangle, 1);
              g_hash_table_insert (entry2geometry_hash, entry, geo);
            }

          /* If the current entry geometry has changed, we need to move the menu
           * accordingly to the change we recorded! */
          if (GTK_IS_MENU (priv->last_menu) && priv->last_menu == entry->menu &&
              g_strcmp0 (priv->last_panel, panel_id) == 0)
            {
              GtkWidget *top_widget = gtk_widget_get_toplevel (GTK_WIDGET (priv->last_menu));

              if (GTK_IS_WINDOW (top_widget))
                {
                  GtkWindow *top_win = GTK_WINDOW (top_widget);
                  gint old_x, old_y;

                  gint scale = get_monitor_scale_at (x, y);
                  gtk_window_get_position (top_win, &old_x, &old_y);
                  gtk_window_move (top_win, old_x - (geo->x - x) / scale, old_y - (geo->y - y) / scale);
                }
            }

          geo->x = x;
          geo->y = y;
          geo->width = width;
          geo->height = height;
        }

      if (valid_entry)
        {
          object = get_entry_parent_indicator (entry);
          g_signal_emit (self, _service_signals[GEOMETRIES_CHANGED], 0, object, entry, x, y, width, height);
        }
    }
}

static gboolean
panel_service_entry_is_visible (PanelService *self, IndicatorObjectEntry *entry)
{
  g_return_val_if_fail (PANEL_IS_SERVICE (self), FALSE);
  g_return_val_if_fail (entry != NULL, FALSE);

  if (GTK_IS_LABEL (entry->label))
    {
      if (gtk_widget_get_visible (GTK_WIDGET (entry->label)) &&
          gtk_widget_is_sensitive (GTK_WIDGET (entry->label)))
        {
          return TRUE;
        }
    }

  if (GTK_IS_IMAGE (entry->image))
    {
      if (gtk_widget_get_visible (GTK_WIDGET (entry->image)) &&
          gtk_widget_is_sensitive (GTK_WIDGET (entry->image)))
        {
          return TRUE;
        }
    }

  if (g_slist_find (self->priv->dropdown_entries, entry))
    return TRUE;

  return FALSE;
}

static gboolean
panel_service_entry_is_visible_on_panel (PanelService *self,
                                         IndicatorObjectEntry *entry,
                                         const gchar *panel_id)
{
  GHashTable *entry2geometry_hash;
  GHashTableIter panel_iter;
  gpointer key, value;
  gboolean found_geo;

  g_return_val_if_fail (PANEL_IS_SERVICE (self), FALSE);
  g_return_val_if_fail (entry != NULL, FALSE);

  found_geo = FALSE;

  if (panel_id)
    {
      entry2geometry_hash = g_hash_table_lookup (self->priv->panel2entries_hash, panel_id);

      if (entry2geometry_hash && g_hash_table_lookup (entry2geometry_hash, entry))
        {
          found_geo = TRUE;
        }
    }
  else
    {
      g_hash_table_iter_init (&panel_iter, self->priv->panel2entries_hash);

      while (g_hash_table_iter_next (&panel_iter, &key, &value) && !found_geo)
        {
          entry2geometry_hash = value;

          if (g_hash_table_lookup (entry2geometry_hash, entry))
            {
              found_geo = TRUE;
            }
        }
    }

  if (!found_geo)
    return FALSE;

  return panel_service_entry_is_visible (self, entry);
}

static int
indicator_entry_compare_func (gpointer* v1, gpointer* v2)
{
  return (GPOINTER_TO_INT (v1[1]) > GPOINTER_TO_INT (v2[1])) ? 1 : -1;
}

static void
activate_next_prev_menu (PanelService         *self,
                         IndicatorObjectEntry *entry,
                         GtkMenuDirectionType  direction)
{
  IndicatorObjectEntry *new_entry;
  PanelServicePrivate *priv = self->priv;
  GSList *indicators = priv->indicators;
  GList  *ordered_entries = NULL;
  GList  *entries;
  gchar  *id;
  GSList *l, *sl;
  GList *ll;

  for (l = indicators; l; l = l->next)
    {
      IndicatorObject *object = l->data;
      gint parent_priority = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (object), "priority"));
      entries = indicator_object_get_entries (object);
      const gchar *indicator_id = g_object_get_data (G_OBJECT (object), "id");

      if (entries)
        {
          int index = 0;

          if (!priv->last_dropdown_entry)
            {
              for (sl = priv->dropdown_entries; sl; sl = sl->next)
                {
                  IndicatorObjectEntry *dropdown = sl->data;

                  if (dropdown->parent_object == object)
                    entries = g_list_append (entries, dropdown);
                }
            }

          for (ll = entries; ll; ll = ll->next)
            {
              gint prio = -1;
              new_entry = ll->data;

              if (!priv->last_dropdown_entry || new_entry != entry)
                {
                  if (!panel_service_entry_is_visible_on_panel (self, new_entry, priv->last_panel))
                    continue;
                }

              if (new_entry->name_hint)
                {
                  prio = name2priority (indicator_id, new_entry->name_hint);
                }

              if (prio < 0)
                {
                  prio = parent_priority + index;
                  index++;
                }

              gpointer *values = g_new (gpointer, 2);
              values[0] = new_entry;
              values[1] = GINT_TO_POINTER (prio);
              ordered_entries = g_list_insert_sorted (ordered_entries, values,
                                                      (GCompareFunc) indicator_entry_compare_func);
            }

          g_list_free (entries);
        }
    }

  new_entry = NULL;
  for (ll = ordered_entries; ll; ll = ll->next)
    {
      gpointer *values = ll->data;
      if (entry == values[0])
        {
          if (direction == GTK_MENU_DIR_CHILD)
            {
              values = ll->next ? ll->next->data : ordered_entries->data;
            }
          else
            {
              values = ll->prev ? ll->prev->data : g_list_last (ordered_entries)->data;
            }

          new_entry = values[0];
          break;
        }
    }

  if (new_entry)
    {
      id = get_indicator_entry_id_by_entry (new_entry);
      g_signal_emit (self, _service_signals[ENTRY_ACTIVATE_REQUEST], 0, id);
      g_free (id);
    }

  g_list_free_full (ordered_entries, g_free);
}

static void
on_active_menu_move_current (GtkMenu              *menu,
                             GtkMenuDirectionType  direction,
                             PanelService         *self)
{
  PanelServicePrivate *priv;
  IndicatorObjectEntry *entry;

  g_return_if_fail (PANEL_IS_SERVICE (self));
  priv = self->priv;

  /* Not interested in up or down */
  if (direction == GTK_MENU_DIR_NEXT || direction == GTK_MENU_DIR_PREV)
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
              && gtk_widget_get_state_flags (item) & GTK_STATE_FLAG_PRELIGHT
              && gtk_menu_item_get_submenu (GTK_MENU_ITEM (item)))
            {
              /* Skip direction due to there being a submenu,
               * and we don't want to inhibit going into that */
              g_list_free (children);
              return;
            }
        }
      g_list_free (children);

      entry = (priv->last_dropdown_entry) ? priv->last_dropdown_entry : priv->last_entry;
    }
  else
    {
      entry = priv->last_entry;
    }

  /* Find the next/prev indicator */
  activate_next_prev_menu (self, entry, direction);
}

static void
menuitem_activated (GtkWidget *menuitem, IndicatorObjectEntry *entry)
{
  IndicatorObject *object = get_entry_parent_indicator (entry);
  indicator_object_entry_activate (object, entry, CurrentTime);
}

static void
panel_service_show_entry_common (PanelService *self,
                                 IndicatorObject *object,
                                 IndicatorObjectEntry *entry,
                                 GtkMenu      *fake_menu,
                                 guint32       xid,
                                 gint32        x,
                                 gint32        y,
                                 guint32       button)
{
  PanelServicePrivate *priv;
  GtkWidget           *last_menu;

  g_return_if_fail (PANEL_IS_SERVICE (self));
  g_return_if_fail (INDICATOR_IS_OBJECT (object));
  g_return_if_fail (entry);

  priv = self->priv;

  if (priv->last_entry == entry)
    return;

  last_menu = GTK_WIDGET (priv->last_menu);

  if (GTK_IS_MENU (priv->last_menu))
    {
      priv->last_x = 0;
      priv->last_y = 0;

      g_signal_handlers_disconnect_by_data (priv->last_menu, self);

      priv->last_panel = NULL;
      priv->last_entry = NULL;
      priv->last_menu = NULL;
      priv->last_menu_button = 0;
    }

  if (entry != NULL)
    {
      if (GTK_IS_MENU (entry->menu) || fake_menu)
        {
          if (xid > 0)
            {
              indicator_object_entry_activate_window (object, entry, xid, CurrentTime);
            }
          else
            {
              indicator_object_entry_activate (object, entry, CurrentTime);
            }

          priv->last_menu = (fake_menu) ? fake_menu : entry->menu;
        }
      else
        {
          /* For some reason, this entry doesn't have a menu.  To simplify the
             rest of the code and to keep scrubbing fluidly, we'll create a
             stub menu for the duration of this scrub. */
          priv->last_menu = GTK_MENU (gtk_menu_new ());

          GtkWidget *menu_item = gtk_menu_item_new_with_label (_("Activate"));
          gtk_menu_shell_append (GTK_MENU_SHELL (priv->last_menu), menu_item);
          gtk_widget_show (menu_item);

          g_signal_connect (priv->last_menu, "deactivate",
                            G_CALLBACK (gtk_widget_destroy), NULL);
          g_signal_connect (priv->last_menu, "destroy",
                            G_CALLBACK (gtk_widget_destroyed), &priv->last_menu);
          g_signal_connect (menu_item, "activate",
                            G_CALLBACK (menuitem_activated), entry);
        }

      priv->last_entry = entry;
      priv->last_x = x;
      priv->last_y = y;
      priv->last_menu_button = button;
      priv->last_panel = get_panel_for_parent_at (self, xid, x, y);

      g_signal_connect (priv->last_menu, "hide", G_CALLBACK (on_active_menu_hidden), self);
      g_signal_connect_after (priv->last_menu, "move-current",
                              G_CALLBACK (on_active_menu_move_current), self);

      gtk_menu_shell_set_take_focus (GTK_MENU_SHELL (priv->last_menu), TRUE);
      gtk_menu_popup (priv->last_menu, NULL, NULL, positon_menu, self, button, CurrentTime);
      gboolean visible = gtk_widget_is_visible (GTK_WIDGET (priv->last_menu));

      if (!visible)
        {
          /* If the menu is not visible at this point, it's very likely that's
           * due to a keyboard grab, so let's try with a menu with no key-grab */
          gtk_menu_shell_set_take_focus (GTK_MENU_SHELL (priv->last_menu), FALSE);
          gtk_menu_popup (priv->last_menu, NULL, NULL, positon_menu, self, button, CurrentTime);
          visible = gtk_widget_is_visible (GTK_WIDGET (priv->last_menu));
        }

      if (visible)
        {
          gtk_menu_reposition (priv->last_menu);

          GdkWindow *gdkwin = gtk_widget_get_window (GTK_WIDGET (priv->last_menu));
          gint left=0, top=0, width=0, height=0;

          gdk_window_get_geometry (gdkwin, NULL, NULL, &width, &height);
          gdk_window_get_origin (gdkwin, &left, &top);

          gchar *entry_id = get_indicator_entry_id_by_entry (entry);
          g_signal_emit (self, _service_signals[ENTRY_ACTIVATED], 0,
                         priv->last_panel, entry_id, left, top, width, height);
          g_free (entry_id);

          priv->last_left = left;
          priv->last_right = left + width -1;
          priv->last_top = top;
          priv->last_bottom = top + height -1;
        }
      else
        {
          on_active_menu_hidden (priv->last_menu, self);
        }
    }

  /* We popdown the old one last so we don't accidently send key focus back to the
   * active application (which will make it change colour (as state changes), which
   * then looks like flickering to the user.
   */
  if (GTK_IS_MENU (last_menu))
    gtk_menu_popdown (GTK_MENU (last_menu));
}

void
panel_service_show_entry (PanelService *self,
                          const gchar  *entry_id,
                          guint32       xid,
                          gint32        x,
                          gint32        y,
                          guint32       button)
{
  IndicatorObject      *object;
  IndicatorObjectEntry *entry;

  g_return_if_fail (PANEL_IS_SERVICE (self));

  entry = get_indicator_entry_by_id (self, entry_id);
  object = get_entry_parent_indicator (entry);

  panel_service_show_entry_common (self, object, entry, NULL, xid, x, y, button);
}

static void
on_drop_down_menu_hidden (GtkWidget *menu)
{
  GList *l, *children;

  children = gtk_container_get_children (GTK_CONTAINER (menu));

  for (l = children; l; l = l->next)
  {
    if (!GTK_IS_MENU_ITEM (l->data))
      continue;

    GtkMenuItem *menu_item = GTK_MENU_ITEM (l->data);
    GtkWidget *entry_menu = gtk_menu_item_get_submenu (menu_item);
    GtkWidget *old_attached = g_object_get_data (G_OBJECT (entry_menu), "ups-attached-widget");

    gtk_menu_item_set_submenu (menu_item, NULL);

    if (old_attached)
      gtk_menu_attach_to_widget (GTK_MENU (entry_menu), old_attached, NULL);
  }

  g_list_free (children);
  gtk_widget_destroy (menu);
}

void
panel_service_show_entries (PanelService *self,
                            gchar       **entries,
                            const gchar  *selected,
                            guint32       xid,
                            gint32        x,
                            gint32        y)
{
  gint                 i;
  IndicatorObject      *object;
  IndicatorObjectEntry *entry, *selected_entry, *first_entry, *last_entry;
  GtkWidget            *menu;

  g_return_if_fail (PANEL_IS_SERVICE (self));
  g_return_if_fail (entries && entries[0]);

  first_entry = get_indicator_entry_by_id (self, entries[0]);

  if (first_entry == self->priv->last_entry)
    return;

  if (!first_entry)
    {
      g_warning ("Impossible to show entries for an invalid entry or object");
      return;
    }

  last_entry = NULL;
  menu = gtk_menu_new ();
  selected_entry = get_indicator_entry_by_id (self, selected);
  object = get_entry_parent_indicator (first_entry);

  for (i = 0; entries[i]; ++i)
    {
      entry = get_indicator_entry_by_id (self, entries[i]);

      if (entry != first_entry && !last_entry)
        continue;

      GtkWidget *menu_item;
      const char *label = NULL;

      if (GTK_IS_LABEL (entry->label))
        label = gtk_label_get_label (entry->label);

      if (GTK_IS_IMAGE (entry->image))
        {
          G_GNUC_BEGIN_IGNORE_DEPRECATIONS
          if (label)
            {
              menu_item = gtk_image_menu_item_new_with_mnemonic (label);
            }
          else
            {
              menu_item = gtk_image_menu_item_new ();
            }

          gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), GTK_WIDGET (entry->image));
          gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (menu_item), TRUE);
          G_GNUC_END_IGNORE_DEPRECATIONS
        }
      else if (label)
        {
          menu_item = gtk_menu_item_new_with_mnemonic (label);
        }
      else
        {
          continue;
        }

      GtkWidget *old_attached = gtk_menu_get_attach_widget (entry->menu);
      if (old_attached)
        {
          g_object_set_data (G_OBJECT (entry->menu), "ups-attached-widget", old_attached);
          gtk_menu_detach (entry->menu);
        }

      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
      gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), GTK_WIDGET (entry->menu));
      gtk_widget_show (menu_item);
      last_entry = entry;

      if (entry == selected_entry)
        gtk_menu_shell_select_item (GTK_MENU_SHELL (menu), menu_item);
    }

  if (!last_entry)
    {
      g_warning ("No valid entries to show");
      gtk_widget_destroy (menu);
      return;
    }

  g_signal_connect_after (menu, "hide", G_CALLBACK (on_drop_down_menu_hidden), NULL);
  g_signal_connect (menu, "destroy", G_CALLBACK (gtk_widget_destroyed), &self->priv->last_dropdown_entry);

  panel_service_show_entry_common (self, object, first_entry, GTK_MENU (menu), xid, x, y, 1);
  self->priv->last_dropdown_entry = last_entry;
}

void
panel_service_show_app_menu (PanelService *self, guint32 xid, gint32 x, gint32 y)
{
  IndicatorObject      *object;
  IndicatorObjectEntry *entry;
  GList                *entries;

  g_return_if_fail (PANEL_IS_SERVICE (self));
  g_return_if_fail (INDICATOR_IS_OBJECT (self->priv->appmenu_indicator));

  object = self->priv->appmenu_indicator;
  entries = indicator_object_get_entries (object);

  if (entries)
    {
      entry = entries->data;
      g_list_free (entries);

      panel_service_show_entry_common (self, object, entry, NULL, xid, x, y, 1);
    }
}

void
panel_service_secondary_activate_entry (PanelService *self, const gchar *entry_id)
{
  IndicatorObject      *object;
  IndicatorObjectEntry *entry;

  g_return_if_fail (PANEL_IS_SERVICE (self));

  entry = get_indicator_entry_by_id (self, entry_id);
  g_return_if_fail (entry);

  object = get_entry_parent_indicator (entry);
  g_signal_emit_by_name(object, INDICATOR_OBJECT_SIGNAL_SECONDARY_ACTIVATE, entry,
                        CurrentTime);
}

void
panel_service_scroll_entry (PanelService   *self,
                            const gchar    *entry_id,
                            gint32         delta)
{
  IndicatorObject      *object;
  IndicatorObjectEntry *entry;

  g_return_if_fail (PANEL_IS_SERVICE (self));

  entry = get_indicator_entry_by_id (self, entry_id);
  g_return_if_fail (entry);

  GdkScrollDirection direction = G_MAXINT;

  switch (delta)
    {
      case NUX_VERTICAL_SCROLL_DELTA:
        direction = INDICATOR_OBJECT_SCROLL_UP;
        break;
      case -NUX_VERTICAL_SCROLL_DELTA:
        direction = INDICATOR_OBJECT_SCROLL_DOWN;
        break;
      case NUX_HORIZONTAL_SCROLL_DELTA:
        direction = INDICATOR_OBJECT_SCROLL_LEFT;
        break;
      case -NUX_HORIZONTAL_SCROLL_DELTA:
        direction = INDICATOR_OBJECT_SCROLL_RIGHT;
        break;
      }

  if (direction != G_MAXINT)
    {
      object = get_entry_parent_indicator (entry);
      g_signal_emit_by_name(object, INDICATOR_OBJECT_SIGNAL_ENTRY_SCROLLED,
                            entry, 1, direction);
    }
}

void
panel_service_close_active_entry (PanelService *self)
{
  g_return_if_fail (PANEL_IS_SERVICE (self));

  if (GTK_IS_MENU (self->priv->last_menu))
    {
      gtk_menu_popdown (GTK_MENU (self->priv->last_menu));
    }
}
