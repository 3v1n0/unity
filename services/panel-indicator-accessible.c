// -*- Mode: C; tab-width:2; indent-tabs-mode: t; c-basic-offset: 2 -*-
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

#include "panel-indicator-accessible.h"

G_DEFINE_TYPE(PanelIndicatorAccessible, panel_indicator_accessible, ATK_TYPE_OBJECT)

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), PANEL_TYPE_INDICATOR_ACCESSIBLE, PanelIndicatorAccessiblePrivate))

/* AtkObject methods */
static void       panel_indicator_accessible_initialize     (AtkObject *accessible, gpointer data);
static gint       panel_indicator_accessible_get_n_children (AtkObject *accessible);
static AtkObject *panel_indicator_accessible_ref_child      (AtkObject *accessible, gint i);

struct _PanelIndicatorAccessiblePrivate
{
};

static void
panel_indicator_accessible_class_init (PanelIndicatorAccessibleClass *klass)
{
	GObjectClass *object_class;
	AtkObjectClass *atk_class;

	/* GObject */
	object_class = G_OBJECT_CLASS (klass);

	/* AtkObject */
	atk_class = ATK_OBJECT_CLASS (klass);
	atk_class->initialize = panel_indicator_accessible_initialize;
	atk_class->get_n_children = panel_indicator_accessible_get_n_children;
	atk_class->ref_child = panel_indicator_accessible_ref_child;

	g_type_class_add_private (object_class, sizeof (PanelIndicatorAccessiblePrivate));
}

static void
panel_indicator_accessible_init (PanelIndicatorAccessible *pia)
{
	pia->priv = GET_PRIVATE (pia);
}

AtkObject *
panel_indicator_accessible_new (void)
{
	AtkObject *accessible;

	accessible = ATK_OBJECT (g_object_new (PANEL_TYPE_INDICATOR_ACCESSIBLE, NULL));

	atk_object_initialize (accessible, NULL);

	return accessible;
}

/* Implementation of AtkObject methods */

static void
panel_indicator_accessible_initialize (AtkObject *accessible, gpointer data)
{
	g_return_if_fail (PANEL_IS_INDICATOR_ACCESSIBLE (accessible));

	atk_object_set_role (accessible, ATK_ROLE_LABEL);

	ATK_OBJECT_CLASS (panel_indicator_accessible_parent_class)->initialize (accessible, data);
}

static gint
panel_indicator_accessible_get_n_children (AtkObject *accessible)
{
	g_return_val_if_fail (PANEL_IS_INDICATOR_ACCESSIBLE (accessible), 0);

	return 0;
}

static AtkObject *
panel_indicator_accessible_ref_child (AtkObject *accessible, gint i)
{
	g_return_val_if_fail (PANEL_IS_INDICATOR_ACCESSIBLE (accessible), NULL);

	return NULL;
}
