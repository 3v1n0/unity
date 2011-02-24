/*
 * Copyright (C) 2011 Canonical Ltd
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
 * Authored by: Alejandro Piñeiro Iglesias <apinheiro@igalia.com>
 *              Rodrigo Moya <rodrigo.moya@canonical.com>
 */

#include <stdlib.h>
#include <string.h>

#include "unity-util-accessible.h"
#include "unity-root-accessible.h"

static void unity_util_accessible_class_init (UnityUtilAccessibleClass	*klass);
static void unity_util_accessible_init       (UnityUtilAccessible *unity_util_accessible);

/* atkutil.h */

static guint         unity_util_accessible_add_global_event_listener    (GSignalEmissionHook listener,
                                                                         const gchar*        event_type);
static void          unity_util_accessible_remove_global_event_listener (guint remove_listener);
static AtkObject *   unity_util_accessible_get_root			(void);
static const gchar * unity_util_accessible_get_toolkit_name		(void);
static const gchar * unity_util_accessible_get_toolkit_version          (void);

typedef struct {
  guint idx;
  gulong hook_id;
  guint signal_id;
} UnityUtilListenerInfo;

static GHashTable *listener_list = NULL;
static AtkObject* root = NULL;

G_DEFINE_TYPE (UnityUtilAccessible, unity_util_accessible, ATK_TYPE_UTIL);

static void
unity_util_accessible_class_init (UnityUtilAccessibleClass *klass)
{
  AtkUtilClass *atk_class;
  gpointer data;

  data = g_type_class_peek (ATK_TYPE_UTIL);
  atk_class = ATK_UTIL_CLASS (data);

  atk_class->add_global_event_listener    = unity_util_accessible_add_global_event_listener;
  atk_class->remove_global_event_listener = unity_util_accessible_remove_global_event_listener;
  atk_class->get_root                     = unity_util_accessible_get_root;
  atk_class->get_toolkit_name             = unity_util_accessible_get_toolkit_name;
  atk_class->get_toolkit_version          = unity_util_accessible_get_toolkit_version;
}

static void
unity_util_accessible_init (UnityUtilAccessible *unity_util_accessible)
{
}

static AtkObject*
unity_util_accessible_get_root (void)
{
  if (!root)
    root = unity_root_accessible_new ();

  return root;
}

static const gchar *
unity_util_accessible_get_toolkit_name (void)
{
  return "UNITY";
}

static const gchar *
unity_util_accessible_get_toolkit_version (void)
{
 /*
  * FIXME:
  * Version is passed in as a -D flag when this file is
  * compiled.
  */
  return "0.1";
}

static guint
add_listener (GSignalEmissionHook listener,
	      const gchar        *object_type,
	      const gchar        *signal_name,
	      const gchar        *hook_data)
{
  GType type;
  guint signal_id;
  guint rc = 0;
  static guint listener_idx = 1;

  if (!listener_list)
    listener_list =  g_hash_table_new_full (g_int_hash, g_int_equal, NULL, g_free);

  type = g_type_from_name (object_type);
  if (type)
    {
      signal_id = g_signal_lookup (signal_name, type);
      if (signal_id > 0)
        {
          UnityUtilListenerInfo *listener_info;

	  rc = listener_idx;
	  listener_info = g_new0 (UnityUtilListenerInfo, 1);
	  listener_info->idx = listener_idx;
	  listener_info->hook_id = g_signal_add_emission_hook (signal_id, 0, listener,
							       g_strdup (hook_data),
							       (GDestroyNotify) g_free);
	  listener_info->signal_id = signal_id;

	  g_hash_table_insert (listener_list, &(listener_info->idx), listener_info);

	  listener_idx++;
	}
      else
        g_debug ("Signal type %s not supported\n", signal_name);
    }
  else
    g_warning ("Invalid object type %s\n", object_type);

  return rc;
}

static guint
unity_util_accessible_add_global_event_listener (GSignalEmissionHook listener,
                                                 const gchar*        event_type)
{
  gchar **split_string;
  guint rc = 0;

  split_string = g_strsplit (event_type, ":", 3);
  if (split_string)
    {
      if (g_str_equal ("window", split_string[0]))
        {
	  /* FIXME: need to specifically process window: events (create, destroy,
	     minimize, maximize, restore, activate, deactivate) */
	}
      else
        {
          rc = add_listener (listener, split_string[1], split_string[2], event_type);
	}

      g_strfreev (split_string);
    }

  return rc;
}

static void
unity_util_accessible_remove_global_event_listener (guint remove_listener)
{
  if (remove_listener > 0)
    {
      UnityUtilListenerInfo *listener_info;

      listener_info = (UnityUtilListenerInfo *) g_hash_table_lookup (listener_list, &remove_listener);
      if (listener_info != NULL)
        {
	  if (listener_info->hook_id != 0 && listener_info->signal_id != 0)
            {
	      g_signal_remove_emission_hook (listener_info->signal_id,
					     listener_info->hook_id);
	      g_hash_table_remove (listener_list, &remove_listener);
	    }
	  else
	    {
              g_warning ("Invalid listener hook_id %ld or signal_id %d",
			 listener_info->hook_id, listener_info->signal_id);
	    }
	}
      else
        g_warning ("No listener with the specified ID: %d", remove_listener);
    }
  else
    g_warning ("Invalid listener_id: %d", remove_listener);
}

void
unity_util_accessible_add_window (nux::BaseWindow *window)
{
  unity_root_accessible_add_window
    (UNITY_ROOT_ACCESSIBLE (unity_util_accessible_get_root ()), window);
}
