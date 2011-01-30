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

#ifndef _PANEL_UTIL_ACCESSIBLE_H_
#define _PANEL_UTIL_ACCESSIBLE_H_

#include <atk/atk.h>

G_BEGIN_DECLS

#define PANEL_TYPE_UTIL_ACCESSIBLE (panel_util_accessible_get_type ())

#define PANEL_UTIL_ACCESSIBLE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PANEL_TYPE_UTIL_ACCESSIBLE, PanelUtilAccessible))
#define PANEL_UTIL_ACCESSIBLE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), PANEL_TYPE_UTIL_ACCESSIBLE, PanelUtilAccessibleClass))
#define PANEL_IS_UTIL_ACCESSIBLE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PANEL_TYPE_UTIL_ACCESSIBLE))
#define PANEL_IS_UTIL_ACCESSIBLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PANEL_TYPE_UTIL_ACCESSIBLE))
#define PANEL_UTIL_ACCESSIBLE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), PANEL_TYPE_UTIL_ACCESSIBLE, PanelUtilAccessibleClass))

typedef struct _PanelUtilAccessible      PanelUtilAccessible;
typedef struct _PanelUtilAccessibleClass PanelUtilAccessibleClass;

struct _PanelUtilAccessible
{
  AtkUtil parent;
};

struct _PanelUtilAccessibleClass
{
  AtkUtilClass parent_class;
};

GType panel_util_accessible_get_type (void);

G_END_DECLS

#endif
