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
#include "panel-service.h"

/* AtkObject methods */
static void         pia_component_interface_init              (AtkComponentIface *iface);

static void         panel_indicator_accessible_initialize     (AtkObject *accessible, gpointer data);
static gint         panel_indicator_accessible_get_n_children (AtkObject *accessible);
static AtkObject   *panel_indicator_accessible_ref_child      (AtkObject *accessible, gint i);
static AtkStateSet *panel_indicator_accessible_ref_state_set  (AtkObject *accessible);

struct _PanelIndicatorAccessiblePrivate
{
  IndicatorObject *indicator;
  PanelService *service;
  GSList *a11y_children;
  gint x;
  gint y;
  gint width;
  gint height;
};

G_DEFINE_TYPE_WITH_CODE(PanelIndicatorAccessible,
			panel_indicator_accessible,
			ATK_TYPE_OBJECT,
			G_IMPLEMENT_INTERFACE (ATK_TYPE_COMPONENT, pia_component_interface_init))

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), PANEL_TYPE_INDICATOR_ACCESSIBLE, PanelIndicatorAccessiblePrivate))

/* Indicator callbacks */

static void
on_indicator_entry_added (IndicatorObject *io, IndicatorObjectEntry *entry, gpointer user_data)
{
  AtkObject *accessible;
  PanelIndicatorAccessible *pia = PANEL_INDICATOR_ACCESSIBLE (user_data);

  accessible = panel_indicator_entry_accessible_new (entry);
  if (accessible != NULL)
    {
      atk_object_set_parent (accessible, ATK_OBJECT (pia));
      pia->priv->a11y_children = g_slist_append (pia->priv->a11y_children, accessible);
      g_signal_emit_by_name (ATK_OBJECT (pia), "children-changed::add",
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
  gboolean found = FALSE;
  AtkObject *accessible = NULL;

  for (l = pia->priv->a11y_children; l != NULL; l = g_slist_next (l))
    {
      accessible = ATK_OBJECT (l->data);

      if (entry == panel_indicator_entry_accessible_get_entry (PANEL_INDICATOR_ENTRY_ACCESSIBLE (accessible)))
        {
          found = TRUE;
          break;
	}
      else
        count++;
    }


  if (found)
    {
      pia->priv->a11y_children = g_slist_remove (pia->priv->a11y_children, accessible);
      g_signal_emit_by_name (ATK_OBJECT (pia), "children-changed::remove",
                             count, accessible);

      g_object_unref (accessible);
    }
}

static void
on_accessible_desc_updated (IndicatorObject *io, IndicatorObjectEntry *entry, gpointer user_data)
{
  GSList *l;
  PanelIndicatorAccessible *pia = PANEL_INDICATOR_ACCESSIBLE (user_data);
  gboolean found = FALSE;
  AtkObject *entry_accessible = NULL;
  AtkObject *widget_accessible = NULL;

  for (l = pia->priv->a11y_children; l != NULL; l = l->next)
    {
      entry_accessible = ATK_OBJECT (l->data);

      if (entry == panel_indicator_entry_accessible_get_entry (PANEL_INDICATOR_ENTRY_ACCESSIBLE (entry_accessible)))
        {
          found = TRUE;
          break;
        }
    }

  if (!found)
    return;

  if (GTK_IS_LABEL (entry->label))
    {
      widget_accessible = gtk_widget_get_accessible (GTK_WIDGET (entry->label));
    }
  else if (GTK_IS_IMAGE (entry->image))
    {
      widget_accessible = gtk_widget_get_accessible (GTK_WIDGET (entry->image));
    }
  else
    {
      g_warning ("a11y: Current entry is not a label or a image.");
    }

  if (ATK_IS_OBJECT (widget_accessible))
    {
      gchar *name = (gchar*) atk_object_get_name (widget_accessible);
      gchar *description = (gchar*) entry->accessible_desc;

      if (name == NULL)
        name = "";

      if (description == NULL)
        description = "";

      atk_object_set_name (entry_accessible, name);
      atk_object_set_description (entry_accessible, description);
    }
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
  PanelIndicatorAccessible *pia;
  AtkRectangle rect;
  GSList *l;
  gboolean minimum_set = FALSE;

  pia = PANEL_INDICATOR_ACCESSIBLE (user_data);

  g_return_if_fail (PANEL_IS_INDICATOR_ACCESSIBLE (pia));

  if (object != pia->priv->indicator)
    return;

  /* Iterate over all children to get width and height */
  pia->priv->width = pia->priv->height = 0;

  for (l = pia->priv->a11y_children; l != NULL; l = l->next)
    {
      gint e_x, e_y, e_width, e_height;
      AtkObject *accessible = ATK_OBJECT (l->data);

      atk_component_get_extents (ATK_COMPONENT (accessible), &e_x, &e_y, &e_width, &e_height, ATK_XY_SCREEN);
      if (minimum_set)
        {
          if (e_x < pia->priv->x)
            pia->priv->x = e_x;
          if (e_y < pia->priv->y)
            pia->priv->y = e_y;
	}
      else
        {
	  pia->priv->x = e_x;
	  pia->priv->y = e_y;
	  minimum_set = TRUE;
	}

      pia->priv->width += e_width;
      if (e_height > pia->priv->height)
	pia->priv->height = e_height;
    }

  /* Notify ATK objects of change of coordinates */
  rect.x = pia->priv->x;
  rect.y = pia->priv->y;
  rect.width = pia->priv->width;
  rect.height = pia->priv->height;
  g_signal_emit_by_name (ATK_COMPONENT (pia), "bounds-changed", &rect);
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

      if (pia->priv->a11y_children != NULL)
        {
          g_slist_free_full(pia->priv->a11y_children, g_object_unref);
          pia->priv->a11y_children = NULL;
	}

      g_signal_handlers_disconnect_by_func (pia->priv->service, on_geometries_changed_cb, pia);
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
  pia->priv->x = pia->priv->y = pia->priv->width = pia->priv->height = 0;

  /* Set up signals for listening to service changes */
  pia->priv->service = panel_service_get_default ();
  g_signal_connect (pia->priv->service, "geometries-changed",
		    G_CALLBACK (on_geometries_changed_cb), pia);
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
panel_indicator_accessible_get_extents (AtkComponent *component,
					gint *x,
					gint *y,
					gint *width,
					gint *height,
					AtkCoordType coord_type)
{
  PanelIndicatorAccessible *pia;

  g_return_if_fail (PANEL_IS_INDICATOR_ACCESSIBLE (component));

  pia = PANEL_INDICATOR_ACCESSIBLE (component);

  /* We ignore AtkCoordType for now, as the panel is always at the top left
     corner and so relative and absolute coordinates are the same */
  *x = pia->priv->x;
  *y = pia->priv->y;
  *width = pia->priv->width;
  *height = pia->priv->height;
}

static void
pia_component_interface_init (AtkComponentIface *iface)
{
  g_return_if_fail (iface != NULL);

  iface->get_extents = panel_indicator_accessible_get_extents;
}

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
      AtkObject *child;
      IndicatorObjectEntry *entry = (IndicatorObjectEntry *) l->data;

      child = panel_indicator_entry_accessible_new (entry);
      atk_object_set_parent (child, accessible);
      pia->priv->a11y_children = g_slist_append (pia->priv->a11y_children, child);
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
