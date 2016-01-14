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
 * Authored by: Alejandro Piñeiro Iglesias <apinheiro@igalia.com>
 */

#ifndef NUX_VIEW_ACCESSIBLE_H
#define NUX_VIEW_ACCESSIBLE_H

#include <atk/atk.h>

#include "nux-area-accessible.h"

#include <Nux/Nux.h>
#include <Nux/View.h>

G_BEGIN_DECLS

#define NUX_TYPE_VIEW_ACCESSIBLE            (nux_view_accessible_get_type ())
#define NUX_VIEW_ACCESSIBLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NUX_TYPE_VIEW_ACCESSIBLE, NuxViewAccessible))
#define NUX_VIEW_ACCESSIBLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NUX_TYPE_VIEW_ACCESSIBLE, NuxViewAccessibleClass))
#define NUX_IS_VIEW_ACCESSIBLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NUX_TYPE_VIEW_ACCESSIBLE))
#define NUX_IS_VIEW_ACCESSIBLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), NUX_TYPE_VIEW_ACCESSIBLE))
#define NUX_VIEW_ACCESSIBLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NUX_TYPE_VIEW_ACCESSIBLE, NuxViewAccessibleClass))

typedef struct _NuxViewAccessible        NuxViewAccessible;
typedef struct _NuxViewAccessibleClass   NuxViewAccessibleClass;
typedef struct _NuxViewAccessiblePrivate NuxViewAccessiblePrivate;

struct _NuxViewAccessible
{
  NuxAreaAccessible parent;

  /*< private >*/
  NuxViewAccessiblePrivate* priv;
};

struct _NuxViewAccessibleClass
{
  NuxAreaAccessibleClass parent_class;
};

GType      nux_view_accessible_get_type(void);
AtkObject* nux_view_accessible_new(nux::Object* object);

G_END_DECLS

#endif /* __NUX_VIEW_ACCESSIBLE_H__ */
