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

/**
 * SECTION:unity-panel-accessible
 * @Title: UnityPanelAccessible
 * @short_description: Implementation of the ATK interfaces for #Panel
 * @see_also: Panel
 *
 * #UnityPanelAccessible implements the required ATK interfaces for
 * #Panel, ie: exposing the different items contained in the panel
 * as children.
 *
 */

#include <Nux/Nux.h>
#include "PanelView.h"
#include "unity-panel-accessible.h"

#include "unitya11y.h"

/* GObject */
static void unity_panel_accessible_class_init (UnityPanelAccessibleClass *klass);
static void unity_panel_accessible_init       (UnityPanelAccessible *self);

/* AtkObject */
static const gchar *unity_panel_accessible_get_name       (AtkObject *accessible);
static void         unity_panel_accessible_initialize     (AtkObject *accessible, gpointer data);
static gint         unity_panel_accessible_get_n_children (AtkObject *accessible);
static AtkObject   *unity_panel_accessible_ref_child      (AtkObject *accessible, gint i);

#define UNITY_PANEL_ACCESSIBLE_GET_PRIVATE(obj)                      \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), UNITY_TYPE_PANEL_ACCESSIBLE, UnityPanelAccessiblePrivate))

G_DEFINE_TYPE (UnityPanelAccessible, unity_panel_accessible,  NUX_TYPE_VIEW_ACCESSIBLE)

struct _UnityPanelAccessiblePrivate
{
};

static void
unity_panel_accessible_class_init (UnityPanelAccessibleClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  AtkObjectClass *atk_class = ATK_OBJECT_CLASS (klass);

  /* AtkObject */
  atk_class->get_name = unity_panel_accessible_get_name;
  atk_class->initialize = unity_panel_accessible_initialize;
  atk_class->get_n_children = unity_panel_accessible_get_n_children;
  atk_class->ref_child = unity_panel_accessible_ref_child;

  g_type_class_add_private (gobject_class, sizeof (UnityPanelAccessiblePrivate));
}

static void
unity_panel_accessible_init (UnityPanelAccessible *self)
{
  self->priv = UNITY_PANEL_ACCESSIBLE_GET_PRIVATE (self);
}

AtkObject *
unity_panel_accessible_new (nux::Object *object)
{
  AtkObject *accessible;

  g_return_val_if_fail (dynamic_cast<PanelView *>(object), NULL);

  accessible = ATK_OBJECT (g_object_new (UNITY_TYPE_PANEL_ACCESSIBLE, NULL));

  atk_object_initialize (accessible, object);

  return accessible;
}

static const gchar *
unity_panel_accessible_get_name (AtkObject *accessible)
{
  return "UnityPanelAccessible";
}

static void
unity_panel_accessible_initialize (AtkObject *accessible, gpointer data)
{
  ATK_OBJECT_CLASS (unity_panel_accessible_parent_class)->initialize (accessible, data);

  accessible->role = ATK_ROLE_PANEL;
}

static gint
unity_panel_accessible_get_n_children (AtkObject *accessible)
{
  nux::Object *nux_object = NULL;
  PanelView *panel;
  gint rc = 0;
  std::list<nux::Object *> *children;

  g_return_val_if_fail (UNITY_IS_PANEL_ACCESSIBLE (accessible), 0);

  nux_object = nux_object_accessible_get_object (NUX_OBJECT_ACCESSIBLE (accessible));
  if (!nux_object) /* state is defunct */
    return 0;

  panel = dynamic_cast<PanelView *>(nux_object);

  if ((children = panel->GetChildren ()))
    rc = children->size ();

  return rc;
}

static AtkObject *
unity_panel_accessible_ref_child (AtkObject *accessible, gint i)
{
  nux::Object *nux_object = NULL;
  PanelView *panel;
  AtkObject *child_accessible = NULL;
  std::list<nux::Object *> *children;

  g_return_val_if_fail (UNITY_IS_PANEL_ACCESSIBLE (accessible), NULL);

  nux_object = nux_object_accessible_get_object (NUX_OBJECT_ACCESSIBLE (accessible));
  if (!nux_object) /* state is defunct */
    return NULL;

  panel = dynamic_cast<PanelView *>(nux_object);

  if ((children = panel->GetChildren ()))
    {
      nux::Object *child;

      std::list<nux::Object *>::iterator it;

      it = children->begin ();
      std::advance (it, i);

      child = dynamic_cast<nux::Object *> (*it);
      child_accessible = unity_a11y_get_accessible (child);
      if (child_accessible != NULL)
        g_object_ref (child_accessible);
    }

  return child_accessible;
}
