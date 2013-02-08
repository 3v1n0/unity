// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2013 Canonical Ltd
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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 */

 #ifndef _SERVICE_SCOPE_H_
#define _SERVICE_SCOPE_H_

#include <dee.h>

G_BEGIN_DECLS

#define SERVICE_TYPE_SCOPE (service_scope_get_type ())

#define SERVICE_SCOPE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
  SERVICE_TYPE_SCOPE, ServiceScope))

#define SERVICE_SCOPE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass),\
  SERVICE_TYPE_SCOPE, ServiceScopeClass))

#define SERVICE_IS_SCOPE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
  SERVICE_TYPE_SCOPE))

#define SERVICE_IS_SCOPE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
  SERVICE_TYPE_SCOPE))

#define SERVICE_SCOPE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),\
  SERVICE_TYPE_SCOPE, ServiceScopeClass))

typedef struct _ServiceScope        ServiceScope;
typedef struct _ServiceScopeClass   ServiceScopeClass;
typedef struct _ServiceScopePrivate ServiceScopePrivate;

struct _ServiceScope
{
  GObject parent;

  ServiceScopePrivate *priv;
};

struct _ServiceScopeClass
{
  GObjectClass parent_class;
};

GType service_scope_get_type(void) G_GNUC_CONST;

// ServiceScope* service_scope_new(void);

G_END_DECLS

#endif /* _SERVICE_SCOPE_H_ */
