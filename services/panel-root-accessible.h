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

#ifndef _PANEL_ROOT_ACCESSIBLE_H_
#define _PANEL_ROOT_ACCESSIBLE_H_

#include <atk/atk.h>

G_BEGIN_DECLS

#define PANEL_TYPE_ROOT_ACCESSIBLE (panel_root_accessible_get_type ())
#define PANEL_ROOT_ACCESSIBLE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PANEL_TYPE_ROOT_ACCESSIBLE, PanelRootAccessible))
#define PANEL_ROOT_ACCESSIBLE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), PANEL_TYPE_ROOT_ACCESSIBLE, PanelRootAccessibleClass))
#define PANEL_IS_ROOT_ACCESSIBLE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PANEL_TYPE_ROOT_ACCESSIBLE))
#define PANEL_IS_ROOT_ACCESSIBLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PANEL_TYPE_ROOT_ACCESSIBLE))
#define PANEL_ROOT_ACCESSIBLE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), PANEL_TYPE_ROOT_ACCESSIBLE, PanelRootAccessibleClass))

typedef struct _PanelRootAccessible        PanelRootAccessible;
typedef struct _PanelRootAccessibleClass   PanelRootAccessibleClass;
typedef struct _PanelRootAccessiblePrivate PanelRootAccessiblePrivate;

struct _PanelRootAccessible
{
	AtkObject                   parent;
	PanelRootAccessiblePrivate *priv;
};

struct _PanelRootAccessibleClass
{
	AtkObjectClass parent_class;
};

GType      panel_root_accessible_get_type (void);
AtkObject *panel_root_accessible_new      (void);

#endif
