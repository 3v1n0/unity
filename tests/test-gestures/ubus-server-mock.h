/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Daniel d'Andrada <daniel.dandrada@canonical.com>
 *
 */

#ifndef UBUS_SERVER_MOCK_H
#define UBUS_SERVER_MOCK_H

#include <glib-object.h>
#include <glib.h>

class UBusServer {
};

UBusServer* ubus_server_get_default();

void ubus_server_send_message(UBusServer*   server,
                              const gchar*  message,
                              GVariant*     data);
#endif // UBUS_SERVER_MOCK_H
