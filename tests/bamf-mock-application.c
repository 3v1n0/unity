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
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 *
 */

#include "bamf-mock-application.h"

G_DEFINE_TYPE (BamfMockApplication, bamf_mock_application, BAMF_TYPE_APPLICATION);

#define BAMF_MOCK_APPLICATION_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BAMF_TYPE_MOCK_APPLICATION, BamfMockApplicationPrivate))

struct _BamfMockApplicationPrivate
{
  gboolean active;
  gboolean running;
  gboolean urgent;
  gchar * name;
  gchar * icon;
  GList * children;
};

void
bamf_mock_application_set_active (BamfMockApplication * self, gboolean active)
{
  g_return_if_fail (BAMF_IS_MOCK_APPLICATION (self));

  if (self->priv->active != active)
  {
    self->priv->active = active;
    g_signal_emit_by_name (G_OBJECT (self), "active-changed", active, NULL);
  }
}

void
bamf_mock_application_set_running (BamfMockApplication * self, gboolean running)
{
  g_return_if_fail (BAMF_IS_MOCK_APPLICATION (self));

  if (self->priv->running != running)
  {
    self->priv->running = running;
    g_signal_emit_by_name (G_OBJECT (self), "running-changed", running, NULL);
  }
}

void
bamf_mock_application_set_urgent (BamfMockApplication * self, gboolean urgent)
{
  g_return_if_fail (BAMF_IS_MOCK_APPLICATION (self));

  if (self->priv->urgent != urgent)
  {
    self->priv->urgent = urgent;
    g_signal_emit_by_name (G_OBJECT (self), "urgent-changed", urgent, NULL);
  }
}

void
bamf_mock_application_set_name (BamfMockApplication * self, const gchar * name)
{
  g_return_if_fail (BAMF_IS_MOCK_APPLICATION (self));

  if (g_strcmp0 (self->priv->name, name) != 0)
  {
    char *old = self->priv->name;
    self->priv->name = g_strdup (name);
    g_signal_emit_by_name (G_OBJECT (self), "name-changed", old, self->priv->name, NULL);
    g_free (old);
  }
}

void
bamf_mock_application_set_icon (BamfMockApplication * self, const gchar * icon)
{
  g_return_if_fail (BAMF_IS_MOCK_APPLICATION (self));

  g_free (self->priv->icon);
  self->priv->icon = g_strdup (icon);
}

void
bamf_mock_application_set_children (BamfMockApplication * self, GList * children)
{
  g_return_if_fail (BAMF_IS_MOCK_APPLICATION (self));

  g_list_free (self->priv->children);
  self->priv->children = g_list_copy (children);
}

static void
bamf_mock_application_finalize (GObject *object)
{
  BamfMockApplication *self = BAMF_MOCK_APPLICATION (object);

  g_free (self->priv->name);
  g_free (self->priv->icon);
  g_list_free (self->priv->children);
}

static GList *
bamf_mock_application_get_children (BamfView *view)
{
  g_return_val_if_fail (BAMF_IS_MOCK_APPLICATION (view), NULL);
  BamfMockApplication *self = BAMF_MOCK_APPLICATION (view);
  return g_list_copy (self->priv->children);
}

static gboolean
bamf_mock_application_is_active (BamfView *view)
{
  g_return_val_if_fail (BAMF_IS_MOCK_APPLICATION (view), FALSE);
  BamfMockApplication *self = BAMF_MOCK_APPLICATION (view);
  return self->priv->active;
}

static gboolean
bamf_mock_application_is_running (BamfView *view)
{
  g_return_val_if_fail (BAMF_IS_MOCK_APPLICATION (view), FALSE);
  BamfMockApplication *self = BAMF_MOCK_APPLICATION (view);
  return self->priv->running;
}

static gboolean
bamf_mock_application_is_urgent (BamfView *view)
{
  g_return_val_if_fail (BAMF_IS_MOCK_APPLICATION (view), FALSE);
  BamfMockApplication *self = BAMF_MOCK_APPLICATION (view);
  return self->priv->urgent;
}

static char *
bamf_mock_application_get_name (BamfView *view)
{
  g_return_val_if_fail (BAMF_IS_MOCK_APPLICATION (view), NULL);
  BamfMockApplication *self = BAMF_MOCK_APPLICATION (view);
  return g_strdup (self->priv->name);
}

static char *
bamf_mock_application_get_icon (BamfView *view)
{
  g_return_val_if_fail (BAMF_IS_MOCK_APPLICATION (view), NULL);
  BamfMockApplication *self = BAMF_MOCK_APPLICATION (view);
  return g_strdup (self->priv->icon);
}

static const char *
bamf_mock_application_view_type (BamfView *view)
{
  g_return_val_if_fail (BAMF_IS_MOCK_APPLICATION (view), NULL);
  return "mock-application";
}

static void
bamf_mock_application_class_init (BamfMockApplicationClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  BamfViewClass *view_class = BAMF_VIEW_CLASS (klass);

  obj_class->finalize = bamf_mock_application_finalize;
  view_class->get_children = bamf_mock_application_get_children;
  view_class->is_active = bamf_mock_application_is_active;
  view_class->is_running = bamf_mock_application_is_running;
  view_class->is_urgent = bamf_mock_application_is_urgent;
  view_class->get_name = bamf_mock_application_get_name;
  view_class->get_icon = bamf_mock_application_get_icon;
  view_class->view_type = bamf_mock_application_view_type;

  g_type_class_add_private (obj_class, sizeof (BamfMockApplicationPrivate));
}

static void
bamf_mock_application_init (BamfMockApplication *self)
{
  self->priv = BAMF_MOCK_APPLICATION_GET_PRIVATE (self);
}

BamfMockApplication *
bamf_mock_application_new ()
{
  return g_object_new (BAMF_TYPE_MOCK_APPLICATION, NULL);
}