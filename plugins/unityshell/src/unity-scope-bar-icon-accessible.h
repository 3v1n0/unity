/*
 * Copyright (C) 2015 Canonical Ltd
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
 * Authored by: Luke Yelavich <luke.yelavich@canonical.com>
 */

#ifndef UNITY_SCOPE_BAR_ICON_ACCESSIBLE_H
#define UNITY_SCOPE_BAR_ICON_ACCESSIBLE_H

#include <atk/atk.h>

#include "nux-view-accessible.h"

G_BEGIN_DECLS

#define UNITY_TYPE_SCOPE_BAR_ICON_ACCESSIBLE            (unity_scope_bar_icon_accessible_get_type ())
#define UNITY_SCOPE_BAR_ICON_ACCESSIBLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), UNITY_TYPE_SCOPE_BAR_ICON_ACCESSIBLE, UnityScopeBarIconAccessible))
#define UNITY_SCOPE_BAR_ICON_ACCESSIBLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), UNITY_TYPE_SCOPE_BAR_ICON_ACCESSIBLE, UnityScopeBarIconAccessibleClass))
#define UNITY_IS_SCOPE_BAR_ICON_ACCESSIBLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UNITY_TYPE_SCOPE_BAR_ICON_ACCESSIBLE))
#define UNITY_IS_SCOPE_BAR_ICON_ACCESSIBLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), UNITY_TYPE_SCOPE_BAR_ICON_ACCESSIBLE))
#define UNITY_SCOPE_BAR_ICON_ACCESSIBLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), UNITY_TYPE_SCOPE_BAR_ICON_ACCESSIBLE, UnityScopeBarIconAccessibleClass))

typedef struct _UnityScopeBarIconAccessible        UnityScopeBarIconAccessible;
typedef struct _UnityScopeBarIconAccessibleClass   UnityScopeBarIconAccessibleClass;
typedef struct _UnityScopeBarIconAccessiblePrivate   UnityScopeBarIconAccessiblePrivate;

struct _UnityScopeBarIconAccessible
{
  NuxViewAccessible parent;

  /*< private >*/
  UnityScopeBarIconAccessiblePrivate* priv;
};

struct _UnityScopeBarIconAccessibleClass
{
  NuxViewAccessibleClass parent_class;
};

GType      unity_scope_bar_icon_accessible_get_type(void);
AtkObject* unity_scope_bar_icon_accessible_new(nux::Object* object);

G_END_DECLS

#endif /* __UNITY_SCOPE_BAR_ICON_ACCESSIBLE_H__ */
