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
#include "panel-indicator-entry-accessible.h"

G_DEFINE_TYPE(PanelIndicatorAccessible, panel_indicator_accessible, ATK_TYPE_OBJECT)

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), PANEL_TYPE_INDICATOR_ACCESSIBLE, PanelIndicatorAccessiblePrivate))

/* AtkObject methods */
static void         panel_indicator_accessible_initialize     (AtkObject *accessible, gpointer data);
static gint         panel_indicator_accessible_get_n_children (AtkObject *accessible);
static AtkObject   *panel_indicator_accessible_ref_child      (AtkObject *accessible, gint i);
static AtkStateSet *panel_indicator_accessible_ref_state_set  (AtkObject *accessible);

struct _PanelIndicatorAccessiblePrivate
{
  IndicatorObject *indicator;
  GSList *a11y_children;
};

/* Indicator callbacks */

static void
on_indicator_entry_added (IndicatorObject *io, IndicatorObjectEntry *entry, gpointer user_data)
{
  AtkObject *accessible;
  PanelIndicatorAccessible *pia = PANEL_INDICATOR_ACCESSIBLE (user_data);

  accessible = panel_indicator_entry_accessible_new (entry);
  if (accessible != NULL)
    {
      pia->priv->a11y_children = g_slist_append (pia->priv->a11y_children, accessible);
      g_signal_emit_by_name (ATK_OBJECT (pia), "children-changed",
			     g_slist_length (pia->priv->a11y_children) - 1,
			     accessible);
    }
}

static void
on_indicator_entry_removed (IndicatorObject *io, IndicatorObjectEntry *entry, gpointer user_data)
{
  GSList *l;
  guint count = 0;
  PanelIndicatorAccessible *pia = PANEL_INDICATOR_ACCESSIBLE (user_data);

  for (l = pia->priv->a11y_children; l != NULL; l = l->next, count++)
    {
      AtkObject *accessible = ATK_OBJECT (l->data);

      if (entry == panel_indicator_entry_accessible_get_entry (PANEL_INDICATOR_ENTRY_ACCESSIBLE (accessible)))
        {
	  pia->priv->a11y_children = g_slist_remove (pia->priv->a11y_children, accessible);
	  g_signal_emit_by_name (ATK_OBJECT (pia), "children-changed",
				 count, accessible);

	  g_object_unref (accessible);
	}
    }
}

static void
on_accessible_desc_updated (IndicatorObject *io, IndicatorObjectEntry *entry, gpointer user_data)
{
  GSList *l;
  PanelIndicatorAccessible *pia = PANEL_INDICATOR_ACCESSIBLE (user_data);

  for (l = pia->priv->a11y_children; l != NULL; l = l->next)
    {
      AtkObject *accessible = ATK_OBJECT (l->data);

      if (entry == panel_indicator_entry_accessible_get_entry (PANEL_INDICATOR_ENTRY_ACCESSIBLE (accessible)))
        {
          atk_object_set_name (accessible, entry->accessible_desc);
          atk_object_set_description (accessible, entry->accessible_desc);
          break;
        }
    }
}

static void
panel_indicator_accessible_finalize (GObject *object)
{
  PanelIndicatorAccessible *pia = PANEL_INDICATOR_ACCESSIBLE (object);

  if (pia->priv != NULL)
    {
      if (pia->priv->indicator != NULL)
        {
          g_signal_handlers_disconnect_by_func (pia->priv->indicator, on_indicator_entry_added, pia);
          g_signal_handlers_disconnect_by_func (pia->priv->indicator, on_indicator_entry_removed, pia);
          g_signal_handlers_disconnect_by_func (pia->priv->indicator, on_accessible_desc_updated, pia);

          g_object_unref (G_OBJECT (pia->priv->indicator));
        }

      while (pia->priv->a11y_children != NULL)
        {
	  AtkObject *accessible = ATK_OBJECT (pia->priv->a11y_children->data);

	  pia->priv->a11y_children = g_slist_remove (pia->priv->a11y_children, accessible);
	  g_object_unref (accessible);
	}
    }

  G_OBJECT_CLASS (panel_indicator_accessible_parent_class)->finalize (object);
}

static void
panel_indicator_accessible_class_init (PanelIndicatorAccessibleClass *klass)
{
  GObjectClass *object_class;
  AtkObjectClass *atk_class;

  /* GObject */
  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = panel_indicator_accessible_finalize;

  /* AtkObject */
  atk_class = ATK_OBJECT_CLASS (klass);
  atk_class->initialize = panel_indicator_accessible_initialize;
  atk_class->get_n_children = panel_indicator_accessible_get_n_children;
  atk_class->ref_child = panel_indicator_accessible_ref_child;
  atk_class->ref_state_set = panel_indicator_accessible_ref_state_set;

  g_type_class_add_private (object_class, sizeof (PanelIndicatorAccessiblePrivate));
}

static void
panel_indicator_accessible_init (PanelIndicatorAccessible *pia)
{
  pia->priv = GET_PRIVATE (pia);
  pia->priv->a11y_children = NULL;
}

AtkObject *
panel_indicator_accessible_new (IndicatorObject *indicator)
{
  PanelIndicatorAccessible *pia;

  pia = g_object_new (PANEL_TYPE_INDICATOR_ACCESSIBLE, NULL);
  atk_object_initialize (ATK_OBJECT (pia), indicator);

  return ATK_OBJECT (pia);
}

/* Implementation of AtkObject methods */

static void
panel_indicator_accessible_initialize (AtkObject *accessible, gpointer data)
{
  PanelIndicatorAccessible *pia;
  GList *entries, *l;

  g_return_if_fail (PANEL_IS_INDICATOR_ACCESSIBLE (accessible));

  ATK_OBJECT_CLASS (panel_indicator_accessible_parent_class)->initialize (accessible, data);

  pia = PANEL_INDICATOR_ACCESSIBLE (accessible);
  atk_object_set_role (accessible, ATK_ROLE_PANEL);

  /* Setup the indicator object */
  pia->priv->indicator = g_object_ref (data);
  g_signal_connect (G_OBJECT (pia->priv->indicator), "entry-added",
		    G_CALLBACK (on_indicator_entry_added), pia);
  g_signal_connect (G_OBJECT (pia->priv->indicator), "entry-removed",
		    G_CALLBACK (on_indicator_entry_removed), pia);
  g_signal_connect (G_OBJECT (pia->priv->indicator), "accessible_desc_update",
		    G_CALLBACK (on_accessible_desc_updated), pia);

  /* Retrieve all entries and create their accessible objects */
  entries = indicator_object_get_entries (pia->priv->indicator);
  for (l = entries; l != NULL; l = l->next)
    {
      AtkObject *accessible;
      IndicatorObjectEntry *entry = (IndicatorObjectEntry *) l->data;

      accessible = panel_indicator_entry_accessible_new (entry);
      pia->priv->a11y_children = g_slist_append (pia->priv->a11y_children, accessible);
    }

  g_list_free (entries);
}

static gint
panel_indicator_accessible_get_n_children (AtkObject *accessible)
{
  PanelIndicatorAccessible *pia = PANEL_INDICATOR_ACCESSIBLE (accessible);

  g_return_val_if_fail (PANEL_IS_INDICATOR_ACCESSIBLE (pia), 0);

  return g_slist_length (pia->priv->a11y_children);
}

static AtkObject *
panel_indicator_accessible_ref_child (AtkObject *accessible, gint i)
{
  PanelIndicatorAccessible *pia = PANEL_INDICATOR_ACCESSIBLE (accessible);

  g_return_val_if_fail (PANEL_IS_INDICATOR_ACCESSIBLE (pia), NULL);

  return g_object_ref (g_slist_nth_data (pia->priv->a11y_children, i));
}

static AtkStateSet *
panel_indicator_accessible_ref_state_set (AtkObject *accessible)
{
  AtkStateSet *state_set;

  g_return_val_if_fail (PANEL_IS_INDICATOR_ACCESSIBLE (accessible), NULL);

  /* Retrieve state_set from parent_class */
  state_set = ATK_OBJECT_CLASS (panel_indicator_accessible_parent_class)->ref_state_set (accessible);

  atk_state_set_add_state (state_set, ATK_STATE_VISIBLE);

  return state_set;
}
