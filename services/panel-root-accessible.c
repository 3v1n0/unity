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
 * Authored by: Rodrigo Moya <rodrigo.moya@canonical.com>
 */

#include <glib/gi18n.h>
#include "panel-indicator-accessible.h"
#include "panel-root-accessible.h"
#include "panel-service.h"

G_DEFINE_TYPE(PanelRootAccessible, panel_root_accessible, ATK_TYPE_OBJECT)

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), PANEL_TYPE_ROOT_ACCESSIBLE, PanelRootAccessiblePrivate))

/* AtkObject methods */
static void       panel_root_accessible_initialize     (AtkObject *accessible, gpointer data);
static gint       panel_root_accessible_get_n_children (AtkObject *accessible);
static AtkObject *panel_root_accessible_ref_child      (AtkObject *accessible, gint i);
static AtkObject *panel_root_accessible_get_parent     (AtkObject *accessible);

struct _PanelRootAccessiblePrivate
{
  PanelService *service;
  GSList *a11y_children;
};

static void
panel_root_accessible_finalize (GObject *object)
{
  PanelRootAccessible *root = PANEL_ROOT_ACCESSIBLE (object);

  if ((root->priv != NULL) && (root->priv->a11y_children != NULL))
    {
      g_slist_free_full(root->priv->a11y_children, g_object_unref);
      root->priv->a11y_children = NULL;
    }
}

static void
panel_root_accessible_class_init (PanelRootAccessibleClass *klass)
{
  GObjectClass *object_class;
  AtkObjectClass *atk_class;

  /* GObject */
  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = panel_root_accessible_finalize;

  /* AtkObject */
  atk_class = ATK_OBJECT_CLASS (klass);
  atk_class->initialize = panel_root_accessible_initialize;
  atk_class->get_n_children = panel_root_accessible_get_n_children;
  atk_class->ref_child = panel_root_accessible_ref_child;
  atk_class->get_parent = panel_root_accessible_get_parent;

  g_type_class_add_private (object_class, sizeof (PanelRootAccessiblePrivate));
}

static void
panel_root_accessible_init (PanelRootAccessible *root)
{
  root->priv = GET_PRIVATE (root);
  root->priv->a11y_children = NULL;
  root->priv->service = panel_service_get_default ();
}

AtkObject *
panel_root_accessible_new (void)
{
  AtkObject *accessible;

  accessible = ATK_OBJECT (g_object_new (PANEL_TYPE_ROOT_ACCESSIBLE, NULL));
  atk_object_initialize (accessible, NULL);

  return accessible;
}

static void
on_indicator_removed (gpointer *data, GObject *removed_indicator)
{
  PanelRootAccessible *root = data[0];
  AtkObject *child = data[1];

  g_return_if_fail (PANEL_IS_ROOT_ACCESSIBLE (root));
  g_return_if_fail (ATK_IS_OBJECT (child));

  gint index = g_slist_index (root->priv->a11y_children, child);
  if (index >= 0)
    {
      root->priv->a11y_children = g_slist_remove (root->priv->a11y_children, child);
      g_signal_emit_by_name (root, "children-changed::remove", index, child);
      g_object_unref (child);
    }

  g_free (data);
}

/* Implementation of AtkObject methods */

static void
panel_root_accessible_initialize (AtkObject *accessible, gpointer data)
{
  gint n_children, i;
  PanelRootAccessible *root = PANEL_ROOT_ACCESSIBLE (accessible);

  g_return_if_fail (PANEL_IS_ROOT_ACCESSIBLE (accessible));

  ATK_OBJECT_CLASS (panel_root_accessible_parent_class)->initialize (accessible, data);

  accessible->role = ATK_ROLE_APPLICATION;
  atk_object_set_name (accessible, g_get_prgname ());
  atk_object_set_parent (accessible, NULL);

  /* Retrieve all indicators and create their accessible objects */
  n_children = panel_service_get_n_indicators (root->priv->service);
  for (i = 0; i < n_children; i++)
    {
      IndicatorObject *indicator;

      indicator = panel_service_get_indicator_nth (root->priv->service, i);
      if (INDICATOR_IS_OBJECT (indicator))
        {
          AtkObject *child;
          gpointer *weak_data;

          child = panel_indicator_accessible_new (indicator);
          /* FIXME: use proper signals once we support dynamic adding/removing
           * of indicators */
          weak_data = g_new0 (gpointer, 2);
          weak_data[0] = root;
          weak_data[1] = child;
          g_object_weak_ref (G_OBJECT (indicator), 
              (GWeakNotify) on_indicator_removed, weak_data);

          atk_object_set_parent (child, accessible);
          root->priv->a11y_children = g_slist_append (root->priv->a11y_children, child);
        }
    }
}

static gint
panel_root_accessible_get_n_children (AtkObject *accessible)
{
  PanelRootAccessible *root = PANEL_ROOT_ACCESSIBLE (accessible);

  g_return_val_if_fail (PANEL_IS_ROOT_ACCESSIBLE (accessible), 0);

  return g_slist_length (root->priv->a11y_children);
}

static AtkObject *
panel_root_accessible_ref_child (AtkObject *accessible, gint i)
{
  PanelRootAccessible *root = PANEL_ROOT_ACCESSIBLE (accessible);

  g_return_val_if_fail (PANEL_IS_ROOT_ACCESSIBLE (accessible), NULL);

  return g_object_ref (g_slist_nth_data (root->priv->a11y_children, i));
}

static AtkObject *
panel_root_accessible_get_parent (AtkObject *accessible)
{
  g_return_val_if_fail (PANEL_IS_ROOT_ACCESSIBLE (accessible), NULL);

  return NULL;
}
