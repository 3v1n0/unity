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

#ifndef NUX_OBJECT_ACCESSIBLE_H
#define NUX_OBJECT_ACCESSIBLE_H

#include <atk/atk.h>

#include <Nux/Nux.h>
#include <NuxCore/Object.h>

G_BEGIN_DECLS

#define NUX_TYPE_OBJECT_ACCESSIBLE            (nux_object_accessible_get_type ())
#define NUX_OBJECT_ACCESSIBLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NUX_TYPE_OBJECT_ACCESSIBLE, NuxObjectAccessible))
#define NUX_OBJECT_ACCESSIBLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NUX_TYPE_OBJECT_ACCESSIBLE, NuxObjectAccessibleClass))
#define NUX_IS_OBJECT_ACCESSIBLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NUX_TYPE_OBJECT_ACCESSIBLE))
#define NUX_IS_OBJECT_ACCESSIBLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), NUX_TYPE_OBJECT_ACCESSIBLE))
#define NUX_OBJECT_ACCESSIBLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NUX_TYPE_OBJECT_ACCESSIBLE, NuxObjectAccessibleClass))

typedef struct _NuxObjectAccessible        NuxObjectAccessible;
typedef struct _NuxObjectAccessibleClass   NuxObjectAccessibleClass;
typedef struct _NuxObjectAccessiblePrivate NuxObjectAccessiblePrivate;

struct _NuxObjectAccessible
{
  AtkObject parent;

  /* < private > */
  NuxObjectAccessiblePrivate* priv;
};

struct _NuxObjectAccessibleClass
{
  AtkObjectClass parent_class;
};

GType      nux_object_accessible_get_type(void);
AtkObject* nux_object_accessible_new(nux::Object* object);

nux::Object* nux_object_accessible_get_object(NuxObjectAccessible* self);

G_END_DECLS

#endif /* __NUX_OBJECT_ACCESSIBLE_H__ */
