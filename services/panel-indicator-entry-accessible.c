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

#include "panel-indicator-entry-accessible.h"

G_DEFINE_TYPE(PanelIndicatorEntryAccessible, panel_indicator_entry_accessible, ATK_TYPE_OBJECT)

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), PANEL_TYPE_INDICATOR_ENTRY_ACCESSIBLE, PanelIndicatorEntryAccessiblePrivate))

/* AtkObject methods */
static void       panel_indicator_entry_accessible_initialize     (AtkObject *accessible, gpointer data);
static gint       panel_indicator_entry_accessible_get_n_children (AtkObject *accessible);
static AtkObject *panel_indicator_entry_accessible_ref_child      (AtkObject *accessible, gint i);

struct _PanelIndicatorEntryAccessiblePrivate
{
  IndicatorObjectEntry *entry;
};

static void
panel_indicator_entry_accessible_class_init (PanelIndicatorEntryAccessibleClass *klass)
{
  GObjectClass *object_class;
  AtkObjectClass *atk_class;

  /* GObject */
  object_class = G_OBJECT_CLASS (klass);

  /* AtkObject */
  atk_class = ATK_OBJECT_CLASS (klass);
  atk_class->initialize = panel_indicator_entry_accessible_initialize;
  atk_class->get_n_children = panel_indicator_entry_accessible_get_n_children;
  atk_class->ref_child = panel_indicator_entry_accessible_ref_child;

  g_type_class_add_private (object_class, sizeof (PanelIndicatorEntryAccessiblePrivate));
}

static void
panel_indicator_entry_accessible_init (PanelIndicatorEntryAccessible *piea)
{
  piea->priv = GET_PRIVATE (piea);
}

AtkObject *
panel_indicator_entry_accessible_new (IndicatorObjectEntry *entry)
{
  PanelIndicatorEntryAccessible *piea;

  piea = g_object_new (PANEL_TYPE_INDICATOR_ENTRY_ACCESSIBLE, NULL);
  atk_object_initialize (ATK_OBJECT (piea), entry);

  return (AtkObject *) piea;
}

/* Implementation of AtkObject methods */

static void
panel_indicator_entry_accessible_initialize (AtkObject *accessible, gpointer data)
{
  PanelIndicatorEntryAccessible *piea;

  g_return_if_fail (PANEL_IS_INDICATOR_ENTRY_ACCESSIBLE (accessible));

  ATK_OBJECT_CLASS (panel_indicator_entry_accessible_parent_class)->initialize (accessible, data);

  piea = PANEL_INDICATOR_ENTRY_ACCESSIBLE (accessible);
  piea->priv->entry = (IndicatorObjectEntry *) data;

  if (GTK_IS_LABEL (piea->priv->entry->label))
    {
      atk_object_set_role (accessible, ATK_ROLE_LABEL);
      atk_object_set_name (accessible, gtk_label_get_text (piea->priv->entry->label));
    }
  else if (GTK_IS_IMAGE (piea->priv->entry->image))
    {
      atk_object_set_role (accessible, ATK_ROLE_IMAGE);
    }
}

static gint
panel_indicator_entry_accessible_get_n_children (AtkObject *accessible)
{
  return 0;
}

static AtkObject *
panel_indicator_entry_accessible_ref_child (AtkObject *accessible, gint i)
{
  return NULL;
}
