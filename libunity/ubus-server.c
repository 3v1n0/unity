/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * u-bus-server.c
 * Copyright (C) 2010 Canonical, Ltd.
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License
 * version 3.0 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License version 3.0 for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Authored by Gordon Allott <gord.allott@canonical.com>
 */

#include "ubus-server.h"
#include <string.h>
#include <stdlib.h>
#include <glib.h>

#define UBUS_SERVER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
  UBUS_TYPE_SERVER, \
  UBusServerPrivate))

struct _UBusServerPrivate
{
  GHashTable *message_interest_table;
  GHashTable *dispatch_table;

  GQueue     *message_queue;

  gulong      id_sequencial_number;
  gboolean    message_pump_queued;
};


G_DEFINE_TYPE (UBusServer, ubus_server, G_TYPE_INITIALLY_UNOWNED);

struct _UBusDispatchInfo
{
  gulong    id;
  UBusCallback callback;
  gchar       *message;
  gpointer    *user_data;
};
typedef struct _UBusDispatchInfo UBusDispatchInfo;

UBusDispatchInfo *
ubus_dispatch_info_new (UBusServer* server, const gchar *message,
                        UBusCallback callback, gpointer user_data)
{
  g_return_val_if_fail (UBUS_IS_SERVER (server), NULL);
  UBusServerPrivate *priv = server->priv;
  UBusDispatchInfo *info;

  if (priv->id_sequencial_number < 1)
    {
      g_critical (G_STRLOC ": ID's are overflowing");
    }

  info = g_new (UBusDispatchInfo, 1);
  info->id = priv->id_sequencial_number++;
  info->callback = callback;
  info->message = g_strdup (message);
  info->user_data = user_data;

  return info;
}

void
ubus_dispatch_info_free (UBusDispatchInfo *info)
{
  g_free (info->message);
  g_free (info);
}

struct _UBusMessageInfo
{
  gchar     *message;
  GVariant  *data;
};

typedef struct _UBusMessageInfo UBusMessageInfo;

/*
 * If @data is floating the constructed message info will
 * assume ownership of the ref
 */
static UBusMessageInfo *
ubus_message_info_new (const gchar *message, GVariant *data)
{
  UBusMessageInfo *info = g_slice_new (UBusMessageInfo);
  info->message = g_strdup (message);
  info->data = data;

  if (data != NULL)
    g_variant_ref_sink (data);
  return info;
}

static void
ubus_message_info_free (UBusMessageInfo *info)
{
  if (info->data != NULL)
    g_variant_unref (info->data);
  g_free (info->message);
  g_slice_free (UBusMessageInfo, info);
}

static gboolean
ulong_equal (gconstpointer v1,
             gconstpointer v2)
{
  return *((const gulong*) v1) == *((const gulong*) v2);
}

static guint
ulong_hash (gconstpointer v)
{
  return (guint) *(const gulong*) v;
}

static void
ubus_server_init (UBusServer *server)
{
  UBusServerPrivate *priv;
  priv = server->priv = UBUS_SERVER_GET_PRIVATE (server);

  // message_interest_table holds the message/DispatchInfo relationship
  priv->message_interest_table = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                        g_free,
                                                        (GDestroyNotify)g_sequence_free);
  // dispatch table holds the individial id/DispatchInfo pairs
  priv->dispatch_table = g_hash_table_new_full (ulong_hash, ulong_equal,
                                                g_free,
                                                (GDestroyNotify)ubus_dispatch_info_free);

  // for anyone thats wondering (hi kamstrup!), there are two hash tables so
  // that lookups are fast when sending messages and removing handlers

  priv->message_queue = g_queue_new ();
  priv->id_sequencial_number = 1;
}

static void
ubus_server_finalize (GObject *object)
{
  UBusServer *server = UBUS_SERVER (object);
  UBusServerPrivate *priv = server->priv;
  g_hash_table_destroy (priv->message_interest_table);
  g_hash_table_destroy (priv->dispatch_table);

  UBusMessageInfo *info = g_queue_pop_tail (priv->message_queue);
  for (; info != NULL; info = g_queue_pop_tail (priv->message_queue))
  {
    if (info->data != NULL)
      g_variant_unref (info->data);
    g_free (info);
  }

  g_queue_free (priv->message_queue);

  G_OBJECT_CLASS (ubus_server_parent_class)->finalize (object);
}

static void
ubus_server_class_init (UBusServerClass *klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS (klass);
  g_type_class_add_private (klass, sizeof (UBusServerPrivate));
  object_class->finalize = ubus_server_finalize;
}

UBusServer *
ubus_server_get_default ()
{
  static gsize singleton;
  if (g_once_init_enter (&singleton))
    {
      UBusServer *server;
      server = g_object_new (UBUS_TYPE_SERVER, NULL);
      g_object_ref_sink (server);
      g_once_init_leave (&singleton, (gsize) server);
    }

  // we actually just want to hold our own reference and not let anything
  // else reference us, because we never want to lose that reference, we are
  // only allowed to initalise once
  return (UBusServer *)singleton;
}

gulong
ubus_server_register_interest (UBusServer* server, const gchar *message,
                               UBusCallback callback, gpointer user_data)
{
  g_return_val_if_fail (UBUS_IS_SERVER (server), 0);
  g_return_val_if_fail (message != NULL, 0);

  UBusServerPrivate *priv = server->priv;
  GSequence *dispatch_list = g_hash_table_lookup (priv->message_interest_table,
                                                 message);
  UBusDispatchInfo *info;

  if (dispatch_list == NULL)
    {
      // not had this message before so add a new entry to the message_interest table
      gchar *key = g_strdup (message);
      dispatch_list = g_sequence_new (NULL); // we use a sequence because its a stable pointer
      g_hash_table_insert (priv->message_interest_table, key, dispatch_list);
    }

  // add the callback to the dispatch table
  info = ubus_dispatch_info_new (server, message, callback, user_data);
  gulong *id = g_malloc (sizeof (gulong));
  *id = info->id;
  g_hash_table_insert (priv->dispatch_table, id, info);

  // add the dispatch info to the dispatch list in the message interest table
  g_sequence_append (dispatch_list, info);

  return info->id;
}

static gboolean
ubus_server_pump_message_queue (UBusServer *server)
{
  g_return_val_if_fail (UBUS_IS_SERVER (server), FALSE);
  UBusServerPrivate *priv = server->priv;
  UBusMessageInfo *info;

  priv->message_pump_queued = TRUE;

  // loop through each message queued and call the dispatch functions associated
  // with it. something in the back of my mind says it would be quicker in some
  // situations to sort the queue first so that duplicate messages can re-use
  // the same dispatch_list lookups.. but thats a specific case.


  info = g_queue_pop_tail (priv->message_queue);
  for (; info != NULL; info = g_queue_pop_tail (priv->message_queue))
    {
      GSequence *dispatch_list;
      dispatch_list = g_hash_table_lookup (priv->message_interest_table,
                                           info->message);

      if (dispatch_list == NULL)
        continue; // no handlers for this message

      GSequenceIter *iter = g_sequence_get_begin_iter (dispatch_list);
      GSequenceIter *end = g_sequence_get_end_iter (dispatch_list);

      while (iter != end)
        {
          GSequenceIter *next = g_sequence_iter_next (iter);
          UBusDispatchInfo *dispatch_info = g_sequence_get (iter);
          UBusCallback callback = dispatch_info->callback;

          (*callback) (info->data, dispatch_info->user_data);

          iter = next;
        }

      ubus_message_info_free (info);
    }

  return FALSE;
}

static void
ubus_server_queue_message_pump (UBusServer *server)
{
  g_return_if_fail (UBUS_IS_SERVER (server));
  UBusServerPrivate *priv = server->priv;

  if (priv->message_pump_queued)
    return;

  g_idle_add ((GSourceFunc)ubus_server_pump_message_queue, server);
  priv->message_pump_queued = TRUE;
}

void
ubus_server_send_message (UBusServer *server, const gchar *message,
                          GVariant *data)
{
  g_return_if_fail (UBUS_IS_SERVER (server));
  g_return_if_fail (message != NULL);
  UBusServerPrivate *priv = server->priv;

  UBusMessageInfo *message_info = ubus_message_info_new (message, data);
  g_queue_push_head (priv->message_queue, message_info);

  ubus_server_queue_message_pump (server);
}

void
ubus_server_unregister_interest (UBusServer* server, gulong handle)
{
  g_return_if_fail (UBUS_IS_SERVER (server));
  g_return_if_fail (handle > 0);
  UBusServerPrivate *priv = server->priv;
  UBusDispatchInfo *info;
  GSequence *dispatch_list;

  // get our info
  info = g_hash_table_lookup (priv->dispatch_table, &handle);

  if (info == NULL)
    {
      g_warning (G_STRLOC ": Handle %lu does not exist", handle);
      return;
    }

  // now the slightly sucky bit, we have to remove from our message-interest
  // table, but we can only find it by itterating through a sequence
  // but this is not so bad because we know *which* sequence its in

  dispatch_list = g_hash_table_lookup (priv->message_interest_table,
                                       info->message);

  if (dispatch_list == NULL)
  {
    g_critical (G_STRLOC ": Handle exists but not dispatch list, ubus has "\
                         "become unstable");
    return;
  }

  GSequenceIter *iter = g_sequence_get_begin_iter (dispatch_list);
  GSequenceIter *end = g_sequence_get_end_iter (dispatch_list);
  while (iter != end)
    {
      GSequenceIter *next = g_sequence_iter_next (iter);
      UBusDispatchInfo *info_test = g_sequence_get (iter);

      if (info_test->id == handle)
      {
        g_sequence_remove (iter);
      }

      iter = next;
    }

  if (g_sequence_get_length (dispatch_list) == 0)
  {
    // free the key/value pair
    g_hash_table_remove (priv->message_interest_table, info->message);
  }

  // finally remove the dispatch_table hash table.
  g_hash_table_remove (priv->dispatch_table, &handle);

}

void
ubus_server_force_message_pump (UBusServer* server)
{
  ubus_server_pump_message_queue (server);
}
