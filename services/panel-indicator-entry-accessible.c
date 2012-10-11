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

/* AtkObject methods */
static void         piea_component_interface_init                   (AtkComponentIface *iface);

static void         panel_indicator_entry_accessible_initialize     (AtkObject *accessible, gpointer data);
static gint         panel_indicator_entry_accessible_get_n_children (AtkObject *accessible);
static AtkObject   *panel_indicator_entry_accessible_ref_child      (AtkObject *accessible, gint i);
static AtkStateSet *panel_indicator_entry_accessible_ref_state_set  (AtkObject *accessible);

struct _PanelIndicatorEntryAccessiblePrivate
{
  IndicatorObjectEntry *entry;
  GtkMenu *             menu;
  PanelService         *service;
  gint                  x;
  gint                  y;
  gint                  width;
  gint                  height;
  gboolean              active;
};

G_DEFINE_TYPE_WITH_CODE(PanelIndicatorEntryAccessible,
			panel_indicator_entry_accessible,
			ATK_TYPE_OBJECT,
			G_IMPLEMENT_INTERFACE (ATK_TYPE_COMPONENT, piea_component_interface_init))

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), PANEL_TYPE_INDICATOR_ENTRY_ACCESSIBLE, PanelIndicatorEntryAccessiblePrivate))

static void
on_entry_activated_cb (PanelService *service, const gchar *entry_id,
                       gint x, gint y, guint w, guint h, gpointer user_data)
{
  gchar *s;
  gboolean adding = FALSE;
  PanelIndicatorEntryAccessible *piea;

  g_return_if_fail (PANEL_IS_INDICATOR_ENTRY_ACCESSIBLE (user_data));

  piea = PANEL_INDICATOR_ENTRY_ACCESSIBLE (user_data);

  /* The PanelService sends us a string containing the pointer to the IndicatorObjectEntry */
  s = g_strdup_printf ("%p", piea->priv->entry);
  if (g_str_equal (s, entry_id))
    {
      adding = TRUE;
      piea->priv->active = TRUE;
    }
  else
    {
      piea->priv->active = FALSE;
    }

  /* Notify AT's about the states' changes */
  atk_object_notify_state_change (ATK_OBJECT (piea), ATK_STATE_ACTIVE, adding);
  atk_object_notify_state_change (ATK_OBJECT (piea), ATK_STATE_FOCUSED, adding);
  atk_object_notify_state_change (ATK_OBJECT (piea), ATK_STATE_SHOWING, adding);

  g_free (s);
}

static void
on_geometries_changed_cb (PanelService *service,
			  IndicatorObject *object,
			  IndicatorObjectEntry *entry,
			  gint x,
			  gint y,
			  gint width,
			  gint height,
			  gpointer user_data)
{
  PanelIndicatorEntryAccessible *piea;
  AtkRectangle rect;

  piea = PANEL_INDICATOR_ENTRY_ACCESSIBLE (user_data);

  g_return_if_fail (PANEL_IS_INDICATOR_ENTRY_ACCESSIBLE (piea));

  if (entry != piea->priv->entry)
    return;

  piea->priv->x = x;
  piea->priv->y = y;
  piea->priv->width = width;
  piea->priv->height = height;

  /* Notify ATK objects of change of coordinates */
  rect.x = piea->priv->x;
  rect.y = piea->priv->y;
  rect.width = piea->priv->width;
  rect.height = piea->priv->height;
  g_signal_emit_by_name (ATK_COMPONENT (piea), "bounds-changed", &rect);
}

static void
panel_indicator_entry_accessible_dispose (GObject *object)
{
  PanelIndicatorEntryAccessible *piea;

  g_return_if_fail (PANEL_IS_INDICATOR_ENTRY_ACCESSIBLE (object));

  piea = PANEL_INDICATOR_ENTRY_ACCESSIBLE (object);

  if (piea->priv != NULL)
    {
      piea->priv->entry = NULL;
      g_clear_object(&piea->priv->menu);
    }

  G_OBJECT_CLASS (panel_indicator_entry_accessible_parent_class)->dispose (object);
  return;
}

static void
panel_indicator_entry_accessible_finalize (GObject *object)
{
  PanelIndicatorEntryAccessible *piea;

  g_return_if_fail (PANEL_IS_INDICATOR_ENTRY_ACCESSIBLE (object));

  piea = PANEL_INDICATOR_ENTRY_ACCESSIBLE (object);

  if (piea->priv != NULL)
    {
      g_signal_handlers_disconnect_by_func (piea->priv->service, on_entry_activated_cb, piea);
      g_signal_handlers_disconnect_by_func (piea->priv->service, on_geometries_changed_cb, piea);
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
  object_class->dispose = panel_indicator_entry_accessible_dispose;
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
panel_indicator_entry_accessible_init (PanelIndicatorEntryAccessible *piea)
{
  piea->priv = GET_PRIVATE (piea);
  piea->priv->x = piea->priv->y = piea->priv->width = piea->priv->height = 0;

  /* Set up signals for listening to service changes */
  piea->priv->active = FALSE;
  piea->priv->service = panel_service_get_default ();
  g_signal_connect (piea->priv->service, "geometries-changed",
		    G_CALLBACK (on_geometries_changed_cb), piea);
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
panel_indicator_entry_accessible_get_extents (AtkComponent *component,
					      gint *x,
					      gint *y,
					      gint *width,
					      gint *height,
					      AtkCoordType coord_type)
{
  PanelIndicatorEntryAccessible *piea;

  g_return_if_fail (PANEL_IS_INDICATOR_ENTRY_ACCESSIBLE (component));

  piea = PANEL_INDICATOR_ENTRY_ACCESSIBLE (component);

  /* We ignore AtkCoordType for now, as the panel is always at the top left
     corner and so relative and absolute coordinates are the same */
  *x = piea->priv->x;
  *y = piea->priv->y;
  *width = piea->priv->width;
  *height = piea->priv->height;
}

static void
piea_component_interface_init (AtkComponentIface *iface)
{
  g_return_if_fail (iface != NULL);

  iface->get_extents = panel_indicator_entry_accessible_get_extents;
}

static void
panel_indicator_entry_accessible_initialize (AtkObject *accessible, gpointer data)
{
  PanelIndicatorEntryAccessible *piea;

  g_return_if_fail (PANEL_IS_INDICATOR_ENTRY_ACCESSIBLE (accessible));

  ATK_OBJECT_CLASS (panel_indicator_entry_accessible_parent_class)->initialize (accessible, data);

  piea = PANEL_INDICATOR_ENTRY_ACCESSIBLE (accessible);
  piea->priv->entry = (IndicatorObjectEntry *) data;
  if (piea->priv->entry->menu != NULL)
    {
      piea->priv->menu = g_object_ref(piea->priv->entry->menu);
    }

  if (GTK_IS_LABEL (piea->priv->entry->label))
    {
      atk_object_set_role (accessible, ATK_ROLE_LABEL);
      atk_object_set_name (accessible,
			   gtk_label_get_text (piea->priv->entry->label));
    }
  if (GTK_IS_IMAGE (piea->priv->entry->image))
    {
      atk_object_set_role (accessible, ATK_ROLE_IMAGE);
      if (piea->priv->entry->accessible_desc != NULL)
        atk_object_set_name (accessible, piea->priv->entry->accessible_desc);
    }

  if (piea->priv->entry->accessible_desc != NULL)
    atk_object_set_description (accessible, piea->priv->entry->accessible_desc);
}

static gint
panel_indicator_entry_accessible_get_n_children (AtkObject *accessible)
{
  PanelIndicatorEntryAccessible *piea;
  gint n_children = 0;

  g_return_val_if_fail (PANEL_IS_INDICATOR_ENTRY_ACCESSIBLE (accessible), 0);

  piea = PANEL_INDICATOR_ENTRY_ACCESSIBLE (accessible);

  if (piea->priv->entry != NULL && piea->priv->entry->parent_object && piea->priv->menu != NULL && GTK_IS_MENU (piea->priv->menu))
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
  if (piea->priv->entry->parent_object && GTK_IS_MENU (piea->priv->entry->menu))
    {
      child = gtk_widget_get_accessible (GTK_WIDGET (piea->priv->entry->menu));
      atk_object_set_parent (child, accessible);
    }

  if (child != NULL)
    return g_object_ref (child);
  else
    return NULL;
}

static AtkStateSet *
panel_indicator_entry_accessible_ref_state_set  (AtkObject *accessible)
{
  AtkStateSet *state_set;
  PanelIndicatorEntryAccessible *piea;

  g_return_val_if_fail (PANEL_IS_INDICATOR_ENTRY_ACCESSIBLE (accessible), NULL);

  piea = PANEL_INDICATOR_ENTRY_ACCESSIBLE (accessible);

  /* Retrieve state_set from parent_class */
  state_set = ATK_OBJECT_CLASS (panel_indicator_entry_accessible_parent_class)->ref_state_set (accessible);

  atk_state_set_add_state (state_set, ATK_STATE_ENABLED);
  atk_state_set_add_state (state_set, ATK_STATE_FOCUSABLE);
  atk_state_set_add_state (state_set, ATK_STATE_HORIZONTAL);
  atk_state_set_add_state (state_set, ATK_STATE_SENSITIVE);
  atk_state_set_add_state (state_set, ATK_STATE_VISIBLE);

  if (piea->priv->active)
    {
      atk_state_set_add_state (state_set, ATK_STATE_ACTIVE);
      atk_state_set_add_state (state_set, ATK_STATE_FOCUSED);
      atk_state_set_add_state (state_set, ATK_STATE_SHOWING);
    }

  return state_set;
}
