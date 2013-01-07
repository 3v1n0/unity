// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Andrea Azzaronea <andrea.azzarone@canonical>
 *
 */

#include <glib.h>

#include "bamf-mock-window.h"

G_DEFINE_TYPE (BamfMockWindow, bamf_mock_window, BAMF_TYPE_WINDOW);

#define BAMF_MOCK_WINDOW_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BAMF_TYPE_MOCK_WINDOW, BamfMockWindowPrivate))

struct _BamfMockWindowPrivate
{
  BamfWindow* transient;
  BamfWindowType window_type;
  guint32 xid;
  guint32 pid;
  gint monitor;
  GHashTable* props;
  BamfWindowMaximizationType maximized;
  time_t last_active;
};

void
bamf_mock_window_set_transient (BamfMockWindow *self, BamfWindow* transient)
{
  g_return_if_fail (BAMF_IS_MOCK_WINDOW (self));

  self->priv->transient = transient;
  g_object_add_weak_pointer (G_OBJECT (self->priv->transient), (gpointer *) &self->priv->transient);
}

void
bamf_mock_window_set_window_type (BamfMockWindow *self, BamfWindowType window_type)
{
  g_return_if_fail (BAMF_IS_MOCK_WINDOW (self));
  self->priv->window_type = window_type;
}

void
bamf_mock_window_set_xid (BamfMockWindow *self, guint32 xid)
{
  g_return_if_fail (BAMF_IS_MOCK_WINDOW (self));
  self->priv->xid = xid;
}

void
bamf_mock_window_set_pid (BamfMockWindow *self, guint32 pid)
{
  g_return_if_fail (BAMF_IS_MOCK_WINDOW (self));
  self->priv->pid = pid;
}

void
bamf_mock_window_set_monitor (BamfMockWindow *self, gint monitor)
{
  g_return_if_fail (BAMF_IS_MOCK_WINDOW (self));

  if (self->priv->monitor != monitor)
  {
    gint old_value = self->priv->monitor;
    self->priv->monitor = monitor;
    g_signal_emit_by_name (G_OBJECT (self), "monitor-changed", old_value, monitor, NULL);
  }
}

void 
bamf_mock_window_set_utf8_prop (BamfMockWindow *self, const char* prop, const char* value)
{
  g_return_if_fail (BAMF_IS_MOCK_WINDOW (self));

  g_hash_table_insert(self->priv->props, g_strdup(prop), g_strdup(value));
}

void
bamf_mock_window_set_maximized (BamfMockWindow *self, BamfWindowMaximizationType maximized)
{
  g_return_if_fail (BAMF_IS_MOCK_WINDOW (self));

  if (self->priv->maximized != maximized)
  {
    BamfWindowMaximizationType old_value = self->priv->maximized;
    self->priv->maximized = maximized;
    g_signal_emit_by_name (G_OBJECT (self), "maximized-changed", old_value, maximized, NULL);
  }
}

void bamf_mock_window_set_last_active (BamfMockWindow *self, time_t last_active)
{
  g_return_if_fail (BAMF_IS_MOCK_WINDOW (self));
  self->priv->last_active = last_active;
}

static void
bamf_mock_window_finalize (GObject *object)
{
  BamfMockWindow *self = BAMF_MOCK_WINDOW (object);

  if (self->priv->transient)
  {
    g_object_remove_weak_pointer(G_OBJECT (self->priv->transient), (gpointer *) &self->priv->transient);
    self->priv->transient = NULL;
  }


  if (self->priv->props)
  {
    g_hash_table_unref (self->priv->props);
    self->priv->props = NULL;
  }
}

static BamfWindow *
bamf_mock_window_get_transient (BamfWindow *window)
{
  g_return_val_if_fail (BAMF_IS_MOCK_WINDOW (window), NULL);
  BamfMockWindow *self = BAMF_MOCK_WINDOW (window);
  return self->priv->transient;
}

static BamfWindowType
bamf_mock_window_get_window_type (BamfWindow *window)
{
  g_return_val_if_fail (BAMF_IS_MOCK_WINDOW (window), BAMF_WINDOW_NORMAL);
  BamfMockWindow *self = BAMF_MOCK_WINDOW (window);
  return self->priv->window_type;
}

static guint32
bamf_mock_window_get_xid (BamfWindow *window)
{
  g_return_val_if_fail (BAMF_IS_MOCK_WINDOW (window), 0);
  BamfMockWindow *self = BAMF_MOCK_WINDOW (window);
  return self->priv->xid;
}

static guint32
bamf_mock_window_get_pid (BamfWindow *window)
{
  g_return_val_if_fail (BAMF_IS_MOCK_WINDOW (window), 0);
  BamfMockWindow *self = BAMF_MOCK_WINDOW (window);
  return self->priv->pid;
}

static gint
bamf_mock_window_get_monitor (BamfWindow *window)
{
  g_return_val_if_fail (BAMF_IS_MOCK_WINDOW (window), BAMF_WINDOW_NORMAL);
  BamfMockWindow *self = BAMF_MOCK_WINDOW (window);
  return self->priv->monitor;
}

static gchar *
bamf_mock_window_get_utf8_prop (BamfWindow *window, const char* prop)
{
  g_return_val_if_fail (BAMF_IS_MOCK_WINDOW (window), BAMF_WINDOW_NORMAL);
  BamfMockWindow *self = BAMF_MOCK_WINDOW (window);

  if (g_hash_table_lookup(self->priv->props, prop))
    return g_strdup(g_hash_table_lookup(self->priv->props, prop));

  return NULL;
}

static BamfWindowMaximizationType
bamf_mock_window_maximized (BamfWindow *window)
{
  g_return_val_if_fail (BAMF_IS_MOCK_WINDOW (window), BAMF_WINDOW_FLOATING);
  BamfMockWindow *self = BAMF_MOCK_WINDOW (window);
  return self->priv->maximized;
}

static time_t
bamf_mock_window_last_active (BamfWindow *window)
{
  g_return_val_if_fail (BAMF_IS_MOCK_WINDOW (window), 0);
  BamfMockWindow *self = BAMF_MOCK_WINDOW (window);
  return self->priv->last_active;
} 

static void
bamf_mock_window_class_init (BamfMockWindowClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  BamfWindowClass *window_class = BAMF_WINDOW_CLASS (klass);

  obj_class->finalize = bamf_mock_window_finalize;
  window_class->get_transient = bamf_mock_window_get_transient;
  window_class->get_window_type = bamf_mock_window_get_window_type;
  window_class->get_xid = bamf_mock_window_get_xid;
  window_class->get_pid = bamf_mock_window_get_pid;
  window_class->get_monitor = bamf_mock_window_get_monitor;
  window_class->get_utf8_prop = bamf_mock_window_get_utf8_prop;
  window_class->maximized = bamf_mock_window_maximized;
  window_class->last_active = bamf_mock_window_last_active;

  g_type_class_add_private (obj_class, sizeof (BamfMockWindowPrivate));
}

static void
bamf_mock_window_init (BamfMockWindow *self)
{
  self->priv = BAMF_MOCK_WINDOW_GET_PRIVATE (self);

  self->priv->transient = NULL;
  self->priv->window_type = BAMF_WINDOW_NORMAL;
  self->priv->xid = 0;
  self->priv->pid = 0;
  self->priv->monitor = 0;
  self->priv->props = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
  self->priv->maximized = BAMF_WINDOW_FLOATING;
  self->priv->last_active = 0;
}

BamfMockWindow *
bamf_mock_window_new ()
{
  return g_object_new (BAMF_TYPE_MOCK_WINDOW, NULL);
}
