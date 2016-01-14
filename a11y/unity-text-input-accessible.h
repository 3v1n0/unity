/*
 * Copyright (C) 2014 Canonical Ltd
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
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 */

#ifndef UNITY_TEXT_INPUT_ACCESSIBLE_H
#define UNITY_TEXT_INPUT_ACCESSIBLE_H

#include <atk/atk.h>

#include "nux-view-accessible.h"

G_BEGIN_DECLS

#define UNITY_TYPE_TEXT_INPUT_ACCESSIBLE            (unity_text_input_accessible_get_type ())
#define UNITY_TEXT_INPUT_ACCESSIBLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), UNITY_TYPE_TEXT_INPUT_ACCESSIBLE, UnityTextInputAccessible))
#define UNITY_TEXT_INPUT_ACCESSIBLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), UNITY_TYPE_TEXT_INPUT_ACCESSIBLE, UnityTextInputAccessibleClass))
#define UNITY_IS_TEXT_INPUT_ACCESSIBLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UNITY_TYPE_TEXT_INPUT_ACCESSIBLE))
#define UNITY_IS_TEXT_INPUT_ACCESSIBLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), UNITY_TYPE_TEXT_INPUT_ACCESSIBLE))
#define UNITY_TEXT_INPUT_ACCESSIBLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), UNITY_TYPE_TEXT_INPUT_ACCESSIBLE, UnityTextInputAccessibleClass))

typedef struct _UnityTextInputAccessible          UnityTextInputAccessible;
typedef struct _UnityTextInputAccessibleClass     UnityTextInputAccessibleClass;
typedef struct _UnityTextInputAccessiblePrivate   UnityTextInputAccessiblePrivate;

struct _UnityTextInputAccessible
{
  NuxViewAccessible parent;

  /*< private >*/
  UnityTextInputAccessiblePrivate* priv;
};

struct _UnityTextInputAccessibleClass
{
  NuxViewAccessibleClass parent_class;
};

GType      unity_text_input_accessible_get_type(void);
AtkObject* unity_text_input_accessible_new(nux::Object* object);

G_END_DECLS

#endif
