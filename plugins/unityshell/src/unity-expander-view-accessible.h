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

#ifndef UNITY_EXPANDER_VIEW_ACCESSIBLE_H
#define UNITY_EXPANDER_VIEW_ACCESSIBLE_H

#include <atk/atk.h>

#include <Nux/Nux.h>
#include <Nux/Layout.h>

#include "nux-view-accessible.h"

G_BEGIN_DECLS

#define UNITY_TYPE_EXPANDER_VIEW_ACCESSIBLE            (unity_expander_view_accessible_get_type ())
#define UNITY_EXPANDER_VIEW_ACCESSIBLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), UNITY_TYPE_EXPANDER_VIEW_ACCESSIBLE, UnityExpanderViewAccessible))
#define UNITY_EXPANDER_VIEW_ACCESSIBLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), UNITY_TYPE_EXPANDER_VIEW_ACCESSIBLE, UnityExpanderViewAccessibleClass))
#define UNITY_IS_EXPANDER_VIEW_ACCESSIBLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UNITY_TYPE_EXPANDER_VIEW_ACCESSIBLE))
#define UNITY_IS_EXPANDER_VIEW_ACCESSIBLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), UNITY_TYPE_EXPANDER_VIEW_ACCESSIBLE))
#define UNITY_EXPANDER_VIEW_ACCESSIBLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), UNITY_TYPE_EXPANDER_VIEW_ACCESSIBLE, UnityExpanderViewAccessibleClass))

typedef struct _UnityExpanderViewAccessible        UnityExpanderViewAccessible;
typedef struct _UnityExpanderViewAccessibleClass   UnityExpanderViewAccessibleClass;
typedef struct _UnityExpanderViewAccessiblePrivate UnityExpanderViewAccessiblePrivate;

struct _UnityExpanderViewAccessible
{
  NuxViewAccessible parent;

  /*< private >*/
  UnityExpanderViewAccessiblePrivate* priv;
};

struct _UnityExpanderViewAccessibleClass
{
  NuxViewAccessibleClass parent_class;
};

GType      unity_expander_view_accessible_get_type(void);
AtkObject* unity_expander_view_accessible_new(nux::Object* object);

G_END_DECLS

#endif /* __UNITY_EXPANDER_VIEW_ACCESSIBLE_H__ */
