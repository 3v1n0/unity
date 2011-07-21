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

#ifndef NUX_AREA_ACCESSIBLE_H
#define NUX_AREA_ACCESSIBLE_H

#include <atk/atk.h>

#include "nux-object-accessible.h"

G_BEGIN_DECLS

#define NUX_TYPE_AREA_ACCESSIBLE            (nux_area_accessible_get_type ())
#define NUX_AREA_ACCESSIBLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NUX_TYPE_AREA_ACCESSIBLE, NuxAreaAccessible))
#define NUX_AREA_ACCESSIBLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NUX_TYPE_AREA_ACCESSIBLE, NuxAreaAccessibleClass))
#define NUX_IS_AREA_ACCESSIBLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NUX_TYPE_AREA_ACCESSIBLE))
#define NUX_IS_AREA_ACCESSIBLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), NUX_TYPE_AREA_ACCESSIBLE))
#define NUX_AREA_ACCESSIBLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NUX_TYPE_AREA_ACCESSIBLE, NuxAreaAccessibleClass))

typedef struct _NuxAreaAccessible        NuxAreaAccessible;
typedef struct _NuxAreaAccessibleClass   NuxAreaAccessibleClass;
typedef struct _NuxAreaAccessiblePrivate NuxAreaAccessiblePrivate;

struct _NuxAreaAccessible
{
  NuxObjectAccessible parent;

  /*< private >*/
  NuxAreaAccessiblePrivate* priv;
};

struct _NuxAreaAccessibleClass
{
  NuxObjectAccessibleClass parent_class;
};

GType      nux_area_accessible_get_type(void);
AtkObject* nux_area_accessible_new(nux::Object* object);

G_END_DECLS

#endif /* __NUX_AREA_ACCESSIBLE_H__ */
