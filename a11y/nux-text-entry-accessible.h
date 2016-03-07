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

#ifndef NUX_TEXT_ENTRY_ACCESSIBLE_H
#define NUX_TEXT_ENTRY_ACCESSIBLE_H

#include <atk/atk.h>

#include "nux-view-accessible.h"

G_BEGIN_DECLS

#define NUX_TYPE_TEXT_ENTRY_ACCESSIBLE            (nux_text_entry_accessible_get_type ())
#define NUX_TEXT_ENTRY_ACCESSIBLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NUX_TYPE_TEXT_ENTRY_ACCESSIBLE, NuxTextEntryAccessible))
#define NUX_TEXT_ENTRY_ACCESSIBLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NUX_TYPE_TEXT_ENTRY_ACCESSIBLE, NuxTextEntryAccessibleClass))
#define NUX_IS_TEXT_ENTRY_ACCESSIBLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NUX_TYPE_TEXT_ENTRY_ACCESSIBLE))
#define NUX_IS_TEXT_ENTRY_ACCESSIBLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), NUX_TYPE_TEXT_ENTRY_ACCESSIBLE))
#define NUX_TEXT_ENTRY_ACCESSIBLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NUX_TYPE_TEXT_ENTRY_ACCESSIBLE, NuxTextEntryAccessibleClass))

typedef struct _NuxTextEntryAccessible        NuxTextEntryAccessible;
typedef struct _NuxTextEntryAccessibleClass   NuxTextEntryAccessibleClass;
typedef struct _NuxTextEntryAccessiblePrivate   NuxTextEntryAccessiblePrivate;

struct _NuxTextEntryAccessible
{
  NuxViewAccessible parent;

  /*< private >*/
  NuxTextEntryAccessiblePrivate* priv;
};

struct _NuxTextEntryAccessibleClass
{
  NuxViewAccessibleClass parent_class;
};

GType      nux_text_entry_accessible_get_type(void);
AtkObject* nux_text_entry_accessible_new(nux::Object* object);

G_END_DECLS

#endif /* __NUX_TEXT_ENTRY_ACCESSIBLE_H__ */
