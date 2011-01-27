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

#include "panel-root-accessible.h"
#include "panel-util-accessible.h"

G_DEFINE_TYPE(PanelUtilAccessible, panel_util_accessible, ATK_TYPE_UTIL)

/* AtkUtil methods */
static AtkObject   *panel_util_accessible_get_root (void);

static AtkObject *root = NULL;

/* GObject methods implementation */

static void
panel_util_accessible_class_init (PanelUtilAccessibleClass *klass)
{
	AtkUtilClass *atk_class;

	/* AtkUtil */
	atk_class = ATK_UTIL_CLASS (klass);
	atk_class->get_root = panel_util_accessible_get_root;
}

static void
panel_util_accessible_init (PanelUtilAccessible *panel_util)
{
}

/* AtkUtil methods implementation */

static AtkObject *
panel_util_accessible_get_root (void)
{
	if (!root)
		root = panel_root_accessible_new ();

	return root;
}
