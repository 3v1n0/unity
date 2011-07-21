/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * u-bus-server.h
 * Copyright (C) 2010 Canonical, Ltd.
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License
 * version 3.0 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License version 3.0 for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Authored by Gordon Allott <gord.allott@canonical.com>
 */

#ifndef _U_BUS_SERVER_H_
#define _U_BUS_SERVER_H_

#include <glib-object.h>
#include <glib.h>
G_BEGIN_DECLS

#define UBUS_TYPE_SERVER             (ubus_server_get_type ())
#define UBUS_SERVER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), UBUS_TYPE_SERVER, UBusServer))
#define UBUS_SERVER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), UBUS_TYPE_SERVER, UBusServerClass))
#define UBUS_IS_SERVER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UBUS_TYPE_SERVER))
#define UBUS_IS_SERVER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), UBUS_TYPE_SERVER))
#define UBUS_SERVER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), UBUS_TYPE_SERVER, UBusServerClass))

typedef struct _UBusServerClass UBusServerClass;
typedef struct _UBusServer UBusServer;
typedef struct _UBusServerPrivate  UBusServerPrivate;

struct _UBusServerClass
{
  GObjectClass parent_class;

  /* padding */
  void (*_unity_padding1)(void);
  void (*_unity_padding2)(void);
  void (*_unity_padding3)(void);
  void (*_unity_padding4)(void);
  void (*_unity_padding5)(void);
  void (*_unity_padding6)(void);
};

struct _UBusServer
{
  GObject parent_instance;

  UBusServerPrivate* priv;
};

typedef void (*UBusCallback)(GVariant*     data,
                             gpointer      user_data);

GType        ubus_server_get_type(void) G_GNUC_CONST;
UBusServer*  ubus_server_get_default();
void         ubus_server_prime_context(UBusServer*   server,
                                       GMainContext* context);
guint        ubus_server_register_interest(UBusServer*   server,
                                           const gchar*  message,
                                           UBusCallback  callback,
                                           gpointer      user_data);
void         ubus_server_send_message(UBusServer*   server,
                                      const gchar*  message,
                                      GVariant*     data);
void         ubus_server_unregister_interest(UBusServer*   server,
                                             guint handle);
void         ubus_server_force_message_pump(UBusServer*   server);

G_END_DECLS

#endif /* _U_BUS_SERVER_H_ */
