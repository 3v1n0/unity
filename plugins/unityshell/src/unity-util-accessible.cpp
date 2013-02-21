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

#include <gdk/gdk.h>
#include <stdlib.h>
#include <string.h>

#include "unity-util-accessible.h"
#include "unity-root-accessible.h"

#include "nux-base-window-accessible.h"

static void unity_util_accessible_class_init(UnityUtilAccessibleClass*  klass);
static void unity_util_accessible_init(UnityUtilAccessible* unity_util_accessible);

/* atkutil.h */

static guint         unity_util_accessible_add_global_event_listener(GSignalEmissionHook listener,
                                                                     const gchar*        event_type);
static void          unity_util_accessible_remove_global_event_listener(guint remove_listener);
static AtkObject*    unity_util_accessible_get_root(void);
static const gchar* unity_util_accessible_get_toolkit_name(void);
static const gchar* unity_util_accessible_get_toolkit_version(void);
static guint         unity_util_accessible_add_key_event_listener(AtkKeySnoopFunc listener,
                                                                  gpointer data);
static void          unity_util_accessible_remove_key_event_listener(guint remove_listener);

typedef struct
{
  guint idx;
  gulong hook_id;
  guint signal_id;
} UnityUtilListenerInfo;

typedef struct
{
  AtkKeySnoopFunc func;
  gpointer        data;
  guint           key;
} UnityKeyEventListener;

/* FIXME: move this to a private structure on UnityUtilAccessible? */
static GHashTable* listener_list = NULL;
static AtkObject* root = NULL;
static GSList* key_listener_list = NULL;
static guint event_inspector_id = 0;
static nux::WindowThread* unity_window_thread = NULL;

G_DEFINE_TYPE(UnityUtilAccessible, unity_util_accessible, ATK_TYPE_UTIL);

static void
unity_util_accessible_class_init(UnityUtilAccessibleClass* klass)
{
  AtkUtilClass* atk_class;
  gpointer data;

  data = g_type_class_peek(ATK_TYPE_UTIL);
  atk_class = ATK_UTIL_CLASS(data);

  atk_class->add_global_event_listener    = unity_util_accessible_add_global_event_listener;
  atk_class->remove_global_event_listener = unity_util_accessible_remove_global_event_listener;
  atk_class->add_key_event_listener       = unity_util_accessible_add_key_event_listener;
  atk_class->remove_key_event_listener    = unity_util_accessible_remove_key_event_listener;
  atk_class->get_root                     = unity_util_accessible_get_root;
  atk_class->get_toolkit_name             = unity_util_accessible_get_toolkit_name;
  atk_class->get_toolkit_version          = unity_util_accessible_get_toolkit_version;
}

static void
unity_util_accessible_init(UnityUtilAccessible* unity_util_accessible)
{
}

static AtkObject*
unity_util_accessible_get_root(void)
{
  if (!root)
    root = unity_root_accessible_new();

  return root;
}

static const gchar*
unity_util_accessible_get_toolkit_name(void)
{
  return "UNITY";
}

static const gchar*
unity_util_accessible_get_toolkit_version(void)
{
  /*
   * FIXME:
   * Version is passed in as a -D flag when this file is
   * compiled.
   */
  return "0.1";
}

static guint
add_listener(GSignalEmissionHook listener,
             const gchar*        object_type,
             const gchar*        signal_name,
             const gchar*        hook_data)
{
  GType type;
  guint signal_id;
  guint rc = 0;
  static guint listener_idx = 1;

  if (!listener_list)
    listener_list =  g_hash_table_new_full(g_int_hash, g_int_equal, NULL, g_free);

  type = g_type_from_name(object_type);
  if (type)
  {
    signal_id = g_signal_lookup(signal_name, type);
    if (signal_id > 0)
    {
      UnityUtilListenerInfo* listener_info;

      rc = listener_idx;
      listener_info = g_new0(UnityUtilListenerInfo, 1);
      listener_info->idx = listener_idx;
      listener_info->hook_id = g_signal_add_emission_hook(signal_id, 0, listener,
                                                          g_strdup(hook_data),
                                                          (GDestroyNotify) g_free);
      listener_info->signal_id = signal_id;

      g_hash_table_insert(listener_list, &(listener_info->idx), listener_info);

      listener_idx++;
    }
    else
    {
      /* Mainly becase some "window::xxx" methods not implemented
         on NuxBaseWindowAccessible */
      g_debug("Signal type %s not supported\n", signal_name);
    }
  }
  else
    g_warning("Invalid object type %s\n", object_type);

  return rc;
}

static void
do_window_event_initialization(void)
{
  /*
   * Ensure that NuxBaseWindowClass exists
   */
  g_type_class_unref(g_type_class_ref(NUX_TYPE_BASE_WINDOW_ACCESSIBLE));
}

static guint
unity_util_accessible_add_global_event_listener(GSignalEmissionHook listener,
                                                const gchar*        event_type)
{
  gchar** split_string;
  guint rc = 0;

  split_string = g_strsplit(event_type, ":", 3);
  if (split_string)
  {
    if (g_str_equal("window", split_string[0]))
    {
      /* Using NuxBaseWindow as the toplevelwindow */
      static gboolean initialized = FALSE;

      if (initialized == FALSE)
      {
        do_window_event_initialization();
        initialized = TRUE;
      }

      rc = add_listener(listener, "NuxBaseWindowAccessible", split_string [1], event_type);
    }
    else
    {
      rc = add_listener(listener, split_string[1], split_string[2], event_type);
    }

    g_strfreev(split_string);
  }

  return rc;
}

static void
unity_util_accessible_remove_global_event_listener(guint remove_listener)
{
  if (remove_listener > 0)
  {
    UnityUtilListenerInfo* listener_info;

    listener_info = (UnityUtilListenerInfo*) g_hash_table_lookup(listener_list, &remove_listener);
    if (listener_info != NULL)
    {
      if (listener_info->hook_id != 0 && listener_info->signal_id != 0)
      {
        g_signal_remove_emission_hook(listener_info->signal_id,
                                      listener_info->hook_id);
        g_hash_table_remove(listener_list, &remove_listener);
      }
      else
      {
        g_warning("Invalid listener hook_id %ld or signal_id %d",
                  listener_info->hook_id, listener_info->signal_id);
      }
    }
    else
      g_warning("No listener with the specified ID: %d", remove_listener);
  }
  else
    g_warning("Invalid listener_id: %d", remove_listener);
}

static guint
translate_nux_modifiers_to_gdk_state(unsigned long nux_modifier)
{
  guint result = 0;

  if (nux_modifier & nux::NUX_STATE_SHIFT)
    result |= GDK_SHIFT_MASK;

  if (nux_modifier & nux::NUX_STATE_CAPS_LOCK)
    result |= GDK_LOCK_MASK;

  if (nux_modifier & nux::NUX_STATE_CTRL)
    result |= GDK_CONTROL_MASK;

  /* From gdk documentation

     GDK_MOD1_MASK : the fourth modifier key (it depends on the
     modifier mapping of the X server which key is interpreted as this
     modifier, but normally it is the Alt key).
   */

  if (nux_modifier & nux::NUX_STATE_ALT)
    result |= GDK_MOD1_MASK;

  /* FIXME: not sure how to translate this ones */
  // if (nux_modifier & NUX_STATE_SCROLLLOCK)
  // if (nux_modifier & NUX_STATE_NUMLOCK)

  return result;
}

static AtkKeyEventStruct*
atk_key_event_from_nux_event_key(nux::Event* event)
{
  AtkKeyEventStruct* atk_event = g_new0(AtkKeyEventStruct, 1);
  gunichar key_unichar;
  static GdkDisplay* display = gdk_display_get_default();
  static GdkKeymap* keymap = gdk_keymap_get_for_display(display);
  GdkKeymapKey* keys = NULL;
  gint n_keys = 0;
  gboolean success = FALSE;

  switch (event->type)
  {
    case nux::NUX_KEYDOWN:
      atk_event->type = ATK_KEY_EVENT_PRESS;
      break;
    case nux::NUX_KEYUP:
      atk_event->type = ATK_KEY_EVENT_RELEASE;
      break;
    default:
      /* we don't call atk_key_event_from_nux_event_key if the event
         is different to keydown or keyup */
      g_assert_not_reached();
      g_free(atk_event);
      return NULL;
  }

  atk_event->state = translate_nux_modifiers_to_gdk_state(event->key_modifiers);

  atk_event->keyval = event->x11_keysym;
  atk_event->keycode = event->x11_keycode;

  /* GDK applies the modifiers to the keyval, and ATK expects that, so
   * we need to do this also here, as it is not done on the release  */

  success = gdk_keymap_get_entries_for_keyval(keymap, atk_event->keyval,
                                              &keys, &n_keys);
  success &= n_keys > 0;

  if (success)
  {
    gint group;
    guint new_keyval;
    gint effective_group;
    gint level;
    GdkModifierType consumed;

    group = keys [0].group;

    success = gdk_keymap_translate_keyboard_state(keymap,
                                                  atk_event->keycode,
                                                  (GdkModifierType) atk_event->state,
                                                  group,
                                                  &new_keyval,
                                                  &effective_group,
                                                  &level,
                                                  &consumed);
    if (success)
      atk_event->keyval = new_keyval;
  }

  atk_event->string = NULL;
  if (event->text && event->text[0])
  {
    key_unichar = g_utf8_get_char(event->text);

    if (g_unichar_validate(key_unichar) && g_unichar_isgraph(key_unichar))
    {
      GString* new_string = NULL;

      new_string = g_string_new("");
      new_string = g_string_insert_unichar(new_string, 0, key_unichar);
      atk_event->string = new_string->str;
      g_string_free(new_string, FALSE);
    }
  }

  /* If ->string is still NULL we compute it from the keyval*/
  if (atk_event->string == NULL)
    atk_event->string = g_strdup(gdk_keyval_name(atk_event->keyval));

  /* e_x11_timestamp is zero, see bug LB#735645*/
  atk_event->timestamp = g_get_real_time() / 1000;

#ifdef DEBUG_ANY_KEY_EVENT
  g_debug("[a11y] AtkKeyEvent:\n\t\tsym 0x%x\n\t\tmods %x\n\t\tcode %u\n\t\ttime %lx \n\t\tstring %s\n",
          (unsigned int) atk_event->keyval,
          (unsigned int) atk_event->state,
          (unsigned int) atk_event->keycode,
          (unsigned long int) atk_event->timestamp,
          atk_event->string);
#endif

  return atk_event;
}

static int
unity_util_event_inspector(nux::Area* area,
                           nux::Event* event,
                           void* data)
{
  GSList* list = NULL;
  AtkKeyEventStruct* atk_key_event = NULL;
  gint result = 0;

  if ((event->type != nux::NUX_KEYDOWN) && (event->type != nux::NUX_KEYUP))
    return 0;

  atk_key_event = atk_key_event_from_nux_event_key(event);

  for (list = key_listener_list; list; list = list->next)
  {
    UnityKeyEventListener* listener = (UnityKeyEventListener*) list->data;

    result |= listener->func(atk_key_event, listener->data);
  }

  g_free(atk_key_event->string);
  g_free(atk_key_event);

  return result;
}

static guint
unity_util_accessible_add_key_event_listener(AtkKeySnoopFunc listener_func,
                                             gpointer data)
{
  static guint key = 0;
  UnityKeyEventListener* listener;

  if (event_inspector_id == 0)
  {
    if (unity_window_thread == NULL)
      return 0;

    event_inspector_id = unity_window_thread->InstallEventInspector(unity_util_event_inspector, NULL);
  }

  key++;

  listener = g_slice_new0(UnityKeyEventListener);
  listener->func = listener_func;
  listener->data = data;
  listener->key = key;

  key_listener_list = g_slist_append(key_listener_list, listener);

  return key;
}

static void
unity_util_accessible_remove_key_event_listener(guint remove_listener)
{
  GSList* l;

  for (l = key_listener_list; l; l = l->next)
  {
    UnityKeyEventListener* listener = (UnityKeyEventListener*) l->data;

    if (listener->key == remove_listener)
    {
      g_slice_free(UnityKeyEventListener, listener);
      key_listener_list = g_slist_delete_link(key_listener_list, l);

      break;
    }
  }

  if (key_listener_list == NULL)
  {
    if (unity_window_thread == NULL)
      return;

    unity_window_thread->RemoveEventInspector(event_inspector_id);
    event_inspector_id = 0;
  }
}

/* Public */
void
unity_util_accessible_set_window_thread(nux::WindowThread* wt)
{
  unity_window_thread = wt;
}
