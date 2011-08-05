/*
 * Copyright 2010 Canonical Ltd.
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
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 *
 */


#include "config.h"
#include "ubus-server.h"

#define MESSAGE1 "TEST MESSAGE ONE"
#define MESSAGE2 "ՄᕅᏆⲤꙨႧΈ Ϊટ ಗשׁຣ໐ɱË‼‼❢"

static void TestAllocation(void);
static void TestMainLoop(void);
static void TestPropagation(void);

void
TestUBusCreateSuite()
{
#define _DOMAIN "/Unit/UBus"
  g_test_add_func(_DOMAIN"/Allocation", TestAllocation);
  g_test_add_func(_DOMAIN"/Propagation", TestPropagation);
  g_test_add_func(_DOMAIN"/MainLoop", TestMainLoop);
}

static void
TestAllocation()
{
  UBusServer* serv_1 = ubus_server_get_default();
  UBusServer* serv_2 = ubus_server_get_default();

  g_assert(serv_1 != NULL);
  g_assert(serv_2 != NULL);

  // i used a different way of making a singleton than i am used to
  // so i'm not 100% confident in it yet
  g_assert(serv_1 == serv_2);
}

void
test_handler_inc_counter(GVariant* data, gpointer val)
{
  // inc a counter when we get called
  gint* counter = (gint*)val;
  *counter = *counter + 1;
}

void
test_handler_inc_counter_2(GVariant* data, gpointer val)
{
  // inc a counter by two when called
  gint* counter = (gint*)val;
  *counter = *counter + 2;
}

static void
TestPropagation()
{
  UBusServer* ubus;
  gint        counter, i;
  guint       handler1, handler2;

  ubus = ubus_server_get_default();
  handler1 = ubus_server_register_interest(ubus, MESSAGE1,
                                           test_handler_inc_counter,
                                           &counter);

  handler2 = ubus_server_register_interest(ubus, MESSAGE2,  // tests UNICODE
                                           test_handler_inc_counter_2,
                                           &counter);

  counter = 0;
  for (i = 0; i < 1000; i++)
  {
    ubus_server_send_message(ubus, MESSAGE1, NULL);
  }

  ubus_server_force_message_pump(ubus);

  counter = 0;
  for (i = 0; i < 1000; i++)
  {
    ubus_server_send_message(ubus, MESSAGE1, NULL);
    ubus_server_send_message(ubus, MESSAGE2, NULL);
  }
  ubus_server_force_message_pump(ubus);

  g_assert(counter == 3000);

  ubus_server_unregister_interest(ubus, handler1);
  ubus_server_unregister_interest(ubus, handler2);

  counter = 0;
  ubus_server_send_message(ubus, MESSAGE1, NULL);
  ubus_server_send_message(ubus, MESSAGE2, NULL);

  ubus_server_force_message_pump(ubus);

  g_assert(counter == 0);
}

gboolean
main_loop_bailout(gpointer data)
{
  GMainLoop* mainloop = (GMainLoop*)data;
  g_main_quit(mainloop);
  return FALSE;
}

void
test_handler_mainloop(GVariant* data, gpointer val)
{
  // inc a counter when we get called
  gint* counter = (gint*)val;
  *counter = *counter + 1;

}

static void
TestMainLoop()
{
  GMainLoop*  mainloop;
  UBusServer* ubus;
  gint        counter = 0;

  ubus = ubus_server_get_default();
  mainloop = g_main_loop_new(NULL, TRUE);

  g_timeout_add_seconds(1, main_loop_bailout, mainloop);

  ubus_server_register_interest(ubus, MESSAGE1,
                                test_handler_mainloop,
                                &counter);

  ubus_server_send_message(ubus, MESSAGE1, NULL);
  g_main_loop_run(mainloop);

  g_assert(counter == 1);

}


