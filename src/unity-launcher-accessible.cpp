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

/**
 * SECTION:nux-view-accessible
 * @Title: NuxViewAccessible
 * @short_description: Implementation of the ATK interfaces for #Launcher
 * @see_also: Launcher
 *
 * #NuxViewAccessible implements the required ATK interfaces for
 * #Launcher, ie: exposing the different LauncherIcon on the model as
 * #child of the object.
 *
 */

#include "unity-launcher-accessible.h"

#include "unitya11y.h"
#include "Launcher.h"
#include "LauncherModel.h"

/* GObject */
static void unity_launcher_accessible_class_init (UnityLauncherAccessibleClass *klass);
static void unity_launcher_accessible_init       (UnityLauncherAccessible *self);

/* AtkObject.h */
static void       unity_launcher_accessible_initialize     (AtkObject *accessible,
                                                            gpointer   data);
static gint       unity_launcher_accessible_get_n_children (AtkObject *obj);
static AtkObject *unity_launcher_accessible_ref_child      (AtkObject *obj,
                                                            gint i);

#define UNITY_LAUNCHER_ACCESSIBLE_GET_PRIVATE(obj)                      \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), UNITY_TYPE_LAUNCHER_ACCESSIBLE, UnityLauncherAccessiblePrivate))

G_DEFINE_TYPE (UnityLauncherAccessible, unity_launcher_accessible,  NUX_TYPE_VIEW_ACCESSIBLE)

struct _UnityLauncherAccessiblePrivate
{
};

static void
unity_launcher_accessible_class_init (UnityLauncherAccessibleClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  AtkObjectClass *atk_class = ATK_OBJECT_CLASS (klass);

  /* AtkObject */
  atk_class->get_n_children = unity_launcher_accessible_get_n_children;
  atk_class->ref_child = unity_launcher_accessible_ref_child;
  atk_class->initialize = unity_launcher_accessible_initialize;

  g_type_class_add_private (gobject_class, sizeof (UnityLauncherAccessiblePrivate));
}

static void
unity_launcher_accessible_init (UnityLauncherAccessible *self)
{
  self->priv = UNITY_LAUNCHER_ACCESSIBLE_GET_PRIVATE (self);
}

AtkObject*
unity_launcher_accessible_new (nux::Object *object)
{
  AtkObject *accessible = NULL;

  g_return_val_if_fail (dynamic_cast<Launcher *>(object), NULL);

  accessible = ATK_OBJECT (g_object_new (UNITY_TYPE_LAUNCHER_ACCESSIBLE, NULL));

  atk_object_initialize (accessible, object);

  return accessible;
}

/* AtkObject.h */
static void
unity_launcher_accessible_initialize (AtkObject *accessible,
                                      gpointer data)
{
  ATK_OBJECT_CLASS (unity_launcher_accessible_parent_class)->initialize (accessible, data);

  accessible->role = ATK_ROLE_TOOL_BAR;
}

static gint
unity_launcher_accessible_get_n_children (AtkObject *obj)
{
  nux::Object *object = NULL;
  Launcher *launcher = NULL;
  LauncherModel *launcher_model = NULL;

  g_return_val_if_fail (UNITY_IS_LAUNCHER_ACCESSIBLE (obj), 0);

  object = nux_object_accessible_get_object (NUX_OBJECT_ACCESSIBLE (obj));
  if (!object) /* state is defunct */
    return 0;

  launcher = dynamic_cast<Launcher *>(object);

  launcher_model = launcher->GetModel ();

  if (launcher_model)
    return launcher_model->Size ();
  else
    return 0;
}

static AtkObject*
unity_launcher_accessible_ref_child (AtkObject *obj,
                                     gint i)
{
  gint num = 0;
  nux::Object *nux_object = NULL;
  Launcher *launcher = NULL;
  LauncherModel *launcher_model = NULL;
  LauncherModel::iterator it;
  nux::Object *child = NULL;
  AtkObject *child_accessible = NULL;

  num = atk_object_get_n_accessible_children (obj);
  g_return_val_if_fail ((i < num)&&(i >= 0), NULL);

  nux_object = nux_object_accessible_get_object (NUX_OBJECT_ACCESSIBLE (obj));
  if (!nux_object) /* state is defunct */
    return 0;

  launcher = dynamic_cast<Launcher *>(nux_object);
  launcher_model = launcher->GetModel ();

  it = launcher_model->begin ();
  std::advance (it, i);

  child = dynamic_cast<nux::Object *>(*it);
  child_accessible = unity_a11y_get_accessible (child);

  g_object_ref (child_accessible);

  return child_accessible;
}
