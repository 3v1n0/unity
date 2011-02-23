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
#include "panel-service.h"

G_DEFINE_TYPE(PanelIndicatorEntryAccessible, panel_indicator_entry_accessible, ATK_TYPE_OBJECT)

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), PANEL_TYPE_INDICATOR_ENTRY_ACCESSIBLE, PanelIndicatorEntryAccessiblePrivate))

/* AtkObject methods */
static void         panel_indicator_entry_accessible_initialize     (AtkObject *accessible, gpointer data);
static gint         panel_indicator_entry_accessible_get_n_children (AtkObject *accessible);
static AtkObject   *panel_indicator_entry_accessible_ref_child      (AtkObject *accessible, gint i);
static AtkStateSet *panel_indicator_entry_accessible_ref_state_set  (AtkObject *accessible);

struct _PanelIndicatorEntryAccessiblePrivate
{
  IndicatorObjectEntry *entry;
  PanelService         *service;
  AtkStateSet          *state_set;
};

static void
panel_indicator_entry_accessible_finalize (GObject *object)
{
  PanelIndicatorEntryAccessible *piea;

  g_return_if_fail (PANEL_IS_INDICATOR_ENTRY_ACCESSIBLE (object));

  piea = PANEL_INDICATOR_ENTRY_ACCESSIBLE (object);

  if (piea->priv != NULL)
    {
      if (piea->priv->state_set != NULL)
        g_object_unref (piea->priv->state_set);
    }

  G_OBJECT_CLASS (panel_indicator_entry_accessible_parent_class)->finalize (object);
}

static void
panel_indicator_entry_accessible_class_init (PanelIndicatorEntryAccessibleClass *klass)
{
  GObjectClass *object_class;
  AtkObjectClass *atk_class;

  /* GObject */
  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = panel_indicator_entry_accessible_finalize;

  /* AtkObject */
  atk_class = ATK_OBJECT_CLASS (klass);
  atk_class->initialize = panel_indicator_entry_accessible_initialize;
  atk_class->get_n_children = panel_indicator_entry_accessible_get_n_children;
  atk_class->ref_child = panel_indicator_entry_accessible_ref_child;
  atk_class->ref_state_set = panel_indicator_entry_accessible_ref_state_set;

  g_type_class_add_private (object_class, sizeof (PanelIndicatorEntryAccessiblePrivate));
}

static void
on_entry_activated_cb (PanelService *service, const gchar *entry_id, gpointer user_data)
{
  gchar *s;
  PanelIndicatorEntryAccessible *piea = PANEL_INDICATOR_ENTRY_ACCESSIBLE (user_data);

  /* The PanelService sends us a string containing the pointer to the IndicatorObjectEntry */
  s = g_strdup_printf ("%p", piea->priv->entry);
  if (g_str_equal (s, entry_id))
    {
      atk_state_set_add_state (piea->priv->state_set, ATK_STATE_ACTIVE);
      atk_state_set_add_state (piea->priv->state_set, ATK_STATE_FOCUSED);
      atk_state_set_add_state (piea->priv->state_set, ATK_STATE_SHOWING);
    }
  else
    {
      atk_state_set_remove_state (piea->priv->state_set, ATK_STATE_ACTIVE);
      atk_state_set_remove_state (piea->priv->state_set, ATK_STATE_FOCUSED);
      atk_state_set_remove_state (piea->priv->state_set, ATK_STATE_SHOWING);
    }

  g_free (s);
}

static void
panel_indicator_entry_accessible_init (PanelIndicatorEntryAccessible *piea)
{
  piea->priv = GET_PRIVATE (piea);

  piea->priv->state_set = atk_state_set_new ();
  atk_state_set_add_state (piea->priv->state_set, ATK_STATE_ENABLED);
  atk_state_set_add_state (piea->priv->state_set, ATK_STATE_FOCUSABLE);
  atk_state_set_add_state (piea->priv->state_set, ATK_STATE_HORIZONTAL);
  atk_state_set_add_state (piea->priv->state_set, ATK_STATE_SENSITIVE);
  atk_state_set_add_state (piea->priv->state_set, ATK_STATE_VISIBLE);

  /* Set up signals for listening to service changes */
  piea->priv->service = panel_service_get_default ();
  g_signal_connect (piea->priv->service, "entry-activated",
		    G_CALLBACK (on_entry_activated_cb), piea);
}

AtkObject *
panel_indicator_entry_accessible_new (IndicatorObjectEntry *entry)
{
  PanelIndicatorEntryAccessible *piea;

  piea = g_object_new (PANEL_TYPE_INDICATOR_ENTRY_ACCESSIBLE, NULL);
  atk_object_initialize (ATK_OBJECT (piea), entry);

  return ATK_OBJECT (piea);
}

IndicatorObjectEntry *
panel_indicator_entry_accessible_get_entry (PanelIndicatorEntryAccessible *piea)
{
  g_return_val_if_fail (PANEL_IS_INDICATOR_ENTRY_ACCESSIBLE (piea), NULL);

  return piea->priv->entry;
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
  if (GTK_IS_IMAGE (piea->priv->entry->image))
    {
      atk_object_set_role (accessible, ATK_ROLE_IMAGE);
    }
}

static gint
panel_indicator_entry_accessible_get_n_children (AtkObject *accessible)
{
  PanelIndicatorEntryAccessible *piea;
  gint n_children = 0;

  g_return_val_if_fail (PANEL_IS_INDICATOR_ENTRY_ACCESSIBLE (accessible), 0);

  piea = PANEL_INDICATOR_ENTRY_ACCESSIBLE (accessible);
  if (GTK_IS_MENU (piea->priv->entry->menu))
    n_children = 1;

  return n_children;
}

static AtkObject *
panel_indicator_entry_accessible_ref_child (AtkObject *accessible, gint i)
{
  PanelIndicatorEntryAccessible *piea;
  AtkObject *child = NULL;

  g_return_val_if_fail (PANEL_IS_INDICATOR_ENTRY_ACCESSIBLE (accessible), NULL);

  piea = PANEL_INDICATOR_ENTRY_ACCESSIBLE (accessible);
  if (GTK_IS_MENU (piea->priv->entry->menu))
    child = gtk_widget_get_accessible (GTK_WIDGET (piea->priv->entry->menu));

  return child;
}

static AtkStateSet *
panel_indicator_entry_accessible_ref_state_set  (AtkObject *accessible)
{
  PanelIndicatorEntryAccessible *piea;

  g_return_val_if_fail (PANEL_IS_INDICATOR_ENTRY_ACCESSIBLE (accessible), NULL);

  piea = PANEL_INDICATOR_ENTRY_ACCESSIBLE (accessible);

  return ATK_STATE_SET (g_object_ref (piea->priv->state_set));
}
