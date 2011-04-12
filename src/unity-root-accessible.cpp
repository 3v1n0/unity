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
 *
 * #UnityRootAccessible is the root object of the accessibility
 * tree-like hierarchy, exposing the application level.
 *
 */

#include "unity-root-accessible.h"
#include "unitya11y.h"

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
/* private */
static void       explore_children                     (AtkObject *obj);


#define UNITY_ROOT_ACCESSIBLE_GET_PRIVATE(obj)                          \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), UNITY_TYPE_ROOT_ACCESSIBLE, UnityRootAccessiblePrivate))

G_DEFINE_TYPE (UnityRootAccessible, unity_root_accessible,  ATK_TYPE_OBJECT)

struct _UnityRootAccessiblePrivate
{
  /* we save on window_list the accessible object for the windows
     registered */
  GSList *window_list;
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
      g_slist_free (root->priv->window_list);
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
  atk_object_set_name (accessible, "unity");
  atk_object_set_parent (accessible, NULL);

  ATK_OBJECT_CLASS (unity_root_accessible_parent_class)->initialize (accessible, data);
}

static gint
unity_root_accessible_get_n_children (AtkObject *obj)
{
  UnityRootAccessible *root = UNITY_ROOT_ACCESSIBLE (obj);

  return g_slist_length (root->priv->window_list);
}

static AtkObject*
unity_root_accessible_ref_child (AtkObject *obj,
                                 gint i)
{
  UnityRootAccessible *root = NULL;
  gint num = 0;
  AtkObject *item = NULL;

  root = UNITY_ROOT_ACCESSIBLE (obj);
  num = atk_object_get_n_accessible_children (obj);
  g_return_val_if_fail ((i < num)&&(i >= 0), NULL);

  item = ATK_OBJECT (g_slist_nth_data (root->priv->window_list, i));

  if (!item)
    return NULL;

  g_object_ref (item);

  return item;
}

static AtkObject*
unity_root_accessible_get_parent (AtkObject *obj)
{
  return NULL;
}



/*
 * It adds a window to the internal window_list managed by the
 *
 *
 * FIXME: third parties manually using this method should be
 * temporal. This method should be a internal root method, as part of
 * a basewindow::show callback, as in the case of gail
 */
AtkObject *
unity_root_accessible_add_window (UnityRootAccessible *self,
                                  nux::BaseWindow *window)
{
  AtkObject *window_accessible = NULL;
  gint index = 0;

  g_return_val_if_fail (UNITY_IS_ROOT_ACCESSIBLE (self), NULL);

  window_accessible =
    unity_a11y_get_accessible (window);

  self->priv->window_list =
    g_slist_append (self->priv->window_list, window_accessible);

  index = g_slist_index (self->priv->window_list, window_accessible);

  explore_children (window_accessible);

  g_signal_emit_by_name (self, "children-changed::add",
                         index, window_accessible, NULL);

  return window_accessible;
}

/* private */
/*
 * FIXME: temporal solution
 *
 * Normally not all the accessible objects on the hierarchy are
 * available from the beginning, and they are being created by demand
 * due the request on the AT (ie: orca) side
 *
 * It usually follows a top-down approach. Top objects emits a signal
 * of interest, so AT apps get interest on it, and request their
 * children. One example is the signal "window::activate". AT receives
 * a signal meaning that a top level object is activated, so request
 * their children (and gran-children).
 *
 * Due technical reasons, right now it is hard to find a suitable way
 * to emit the signal "activate" on the BaseWindow. That means that
 * objects on the bottom of the hierarchy are not created, so Orca
 * doesn't react to changes on sections like the Launcher.
 *
 * So in order to prevent that, we make a manual exploration of the
 * hierarchy in order to ensure that those objects are there.
 *
 * NOTE: this manual exploration is not required with at-spi2, just
 * with at-spi.
 *
 */
static void
explore_children (AtkObject *obj)
{
  gint num = 0;
  gint i = 0;
  AtkObject *atk_child = NULL;

  g_return_if_fail (ATK_IS_OBJECT (obj));

  num = atk_object_get_n_accessible_children (obj);

  for (i = 0; i < num; i++)
    {
      atk_child = atk_object_ref_accessible_child (obj, i);
      explore_children (atk_child);
      g_object_unref (atk_child);
    }
}
