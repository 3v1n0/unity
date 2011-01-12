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
 * SECTION:unity-accessible-root
 * @short_description: Root object for the UNITY accessible support
 * @see_also: #ClutterStage
 *
 * #UnityRootAccessible is the root object of the accessibility
 * tree-like hierarchy, exposing the application level.
 *
 * Implementation notes: FIXME, RIGHT NOW IS JUST A DUMMY IMPLEMENTATION
 */

#include "unity-root-accessible.h"

/* GObject */
static void unity_root_accessible_class_init (UnityRootAccessibleClass *klass);
static void unity_root_accessible_init       (UnityRootAccessible *root);
static void unity_root_accessible_finalize   (GObject *object);

/* AtkObject.h */
static void       unity_root_accessible_initialize     (AtkObject *accessible,
                                                        gpointer   data);
static gint       unity_root_accessible_get_n_children (AtkObject *obj);
static AtkObject *unity_root_accessible_ref_child      (AtkObject *obj,
                                                        gint i);
static AtkObject *unity_root_accessible_get_parent     (AtkObject *obj);


#define UNITY_ROOT_ACCESSIBLE_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), UNITY_TYPE_ROOT_ACCESSIBLE, UnityRootAccessiblePrivate))

G_DEFINE_TYPE (UnityRootAccessible, unity_root_accessible,  ATK_TYPE_OBJECT)

struct _UnityRootAccessiblePrivate
{
  GList *window_list;
};

static void
unity_root_accessible_class_init (UnityRootAccessibleClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  AtkObjectClass *atk_class = ATK_OBJECT_CLASS (klass);

  gobject_class->finalize = unity_root_accessible_finalize;

  /* AtkObject */
  atk_class->get_n_children = unity_root_accessible_get_n_children;
  atk_class->ref_child = unity_root_accessible_ref_child;
  atk_class->get_parent = unity_root_accessible_get_parent;
  atk_class->initialize = unity_root_accessible_initialize;

  g_type_class_add_private (gobject_class, sizeof (UnityRootAccessiblePrivate));
}

static void
unity_root_accessible_init (UnityRootAccessible      *root)
{
  root->priv = UNITY_ROOT_ACCESSIBLE_GET_PRIVATE (root);

  root->priv->window_list = NULL;
}

AtkObject*
unity_root_accessible_new (void)
{
  AtkObject *accessible = NULL;

  accessible = ATK_OBJECT (g_object_new (UNITY_TYPE_ROOT_ACCESSIBLE, NULL));

  atk_object_initialize (accessible, NULL);

  return accessible;
}

static void
unity_root_accessible_finalize (GObject *object)
{
  UnityRootAccessible *root = UNITY_ROOT_ACCESSIBLE (object);

  g_return_if_fail (UNITY_IS_ROOT_ACCESSIBLE (object));

  if (root->priv->window_list)
    {
      g_list_free (root->priv->window_list);
      root->priv->window_list = NULL;
    }

  G_OBJECT_CLASS (unity_root_accessible_parent_class)->finalize (object);
}

/* AtkObject.h */
static void
unity_root_accessible_initialize (AtkObject *accessible,
                                  gpointer data)
{
  accessible->role = ATK_ROLE_APPLICATION;

  // FIXME: compiz doesn't set the program name using g_set_prgname,
  // and AFAIK, there isn't a way to get it. Requires further investigation.
  // accessible->name = g_get_prgname();
  atk_object_set_name (accessible, "Unity");
  atk_object_set_parent (accessible, NULL);

  ATK_OBJECT_CLASS (unity_root_accessible_parent_class)->initialize (accessible, data);
}

static gint
unity_root_accessible_get_n_children (AtkObject *obj)
{
  return 0;
}

static AtkObject*
unity_root_accessible_ref_child (AtkObject *obj,
                                 gint i)
{

  return NULL;
}

static AtkObject*
unity_root_accessible_get_parent (AtkObject *obj)
{
  return NULL;
}
