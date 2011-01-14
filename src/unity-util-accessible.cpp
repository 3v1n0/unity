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
static guint         unity_util_accessible_add_key_event_listener	(AtkKeySnoopFunc listener,
                                                                         gpointer        data);
static void          unity_util_accessible_remove_key_event_listener    (guint remove_listener);
static AtkObject *   unity_util_accessible_get_root			(void);
static const gchar * unity_util_accessible_get_toolkit_name		(void);
static const gchar * unity_util_accessible_get_toolkit_version          (void);


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
  atk_class->add_key_event_listener       = unity_util_accessible_add_key_event_listener;
  atk_class->remove_key_event_listener    = unity_util_accessible_remove_key_event_listener;
  atk_class->get_root                     = unity_util_accessible_get_root;
  atk_class->get_toolkit_name             = unity_util_accessible_get_toolkit_name;
  atk_class->get_toolkit_version          = unity_util_accessible_get_toolkit_version;
}

static void
unity_util_accessible_init (UnityUtilAccessible *unity_util_accessible)
{
  /* instance init: usually not required */
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
unity_util_accessible_add_global_event_listener (GSignalEmissionHook listener,
                                                 const gchar*        event_type)
{
  /* FIXME: implement me!! */
  return 0;
}

static void
unity_util_accessible_remove_global_event_listener (guint remove_listener)
{
  /* FIXME: implement me!! */
}

static guint
unity_util_accessible_add_key_event_listener (AtkKeySnoopFunc listener,
                                              gpointer        data)
{
  /* FIXME: implement me!! */
  return 0;
}

static void
unity_util_accessible_remove_key_event_listener (guint remove_listener)
{
  /* FIXME: implement me!! */
}


void
unity_util_accessible_add_window (nux::BaseWindow *window)
{
  unity_root_accessible_add_window (UNITY_ROOT_ACCESSIBLE (root),
                                    window);
}
