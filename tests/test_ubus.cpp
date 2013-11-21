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
 * Authored by: Michal Hruby <michal.hruby@canonical.com>
 *
 */

#include <gtest/gtest.h>
#include <time.h>
#include "TimeUtil.h"
#include "test_utils.h"

#include <UnityCore/Variant.h>
#include <UnityCore/GLibSource.h>
#include "UBusServer.h"
#include "UBusWrapper.h"

#define MESSAGE1 "TEST MESSAGE ONE"
#define MESSAGE2 "ՄᕅᏆⲤꙨႧΈ Ϊટ ಗשׁຣ໐ɱË‼‼❢"

using namespace unity;

namespace
{

struct TestUBusServer : public testing::Test
{
  UBusServer ubus_server;
  bool callback_called;
  unsigned callback_call_count;
  glib::Variant last_msg_variant;

  virtual ~TestUBusServer() {}

  virtual void SetUp()
  {
    callback_called = false;
    callback_call_count = 0;
  }

  void Callback(glib::Variant const& message)
  {
    callback_called = true;
    callback_call_count++;

    last_msg_variant = message;
  }

  void ProcessMessages()
  {
    bool expired = false;
    glib::Idle idle([&] { expired = true; return false; },
                    glib::Source::Priority::LOW);
    Utils::WaitUntil(expired);
  }
};

struct TestUBusManager : public testing::Test
{
  UBusManager ubus_manager;
  bool callback_called;
  unsigned callback_call_count;
  glib::Variant last_msg_variant;

  virtual ~TestUBusManager() {}
  virtual void SetUp()
  {
    callback_called = false;
    callback_call_count = 0;
  }

  void Callback(glib::Variant const& message)
  {
    callback_called = true;
    callback_call_count++;

    last_msg_variant = message;
  }

  void ProcessMessages()
  {
    bool expired = false;
    glib::Idle idle([&] { expired = true; return false; },
                    glib::Source::Priority::LOW);
    Utils::WaitUntil(expired);
  }
};

// UBus tests

TEST_F(TestUBusServer, Contruct)
{
  EXPECT_FALSE(callback_called);
}

TEST_F(TestUBusServer, SingleDispatch)
{
  ubus_server.RegisterInterest(MESSAGE1, sigc::mem_fun(this, &TestUBusServer::Callback));
  ubus_server.SendMessage(MESSAGE1);

  ProcessMessages();

  EXPECT_TRUE(callback_called);
  EXPECT_EQ(callback_call_count, 1);
}

TEST_F(TestUBusServer, SingleDispatchWithData)
{
  ubus_server.RegisterInterest(MESSAGE1, sigc::mem_fun(this, &TestUBusServer::Callback));
  ubus_server.SendMessage(MESSAGE1, g_variant_new_string("UserData"));

  ProcessMessages();

  EXPECT_TRUE(callback_called);
  EXPECT_EQ(callback_call_count, 1);
  EXPECT_EQ(last_msg_variant.GetString(), "UserData");
}

TEST_F(TestUBusServer, SingleDispatchUnicode)
{
  ubus_server.RegisterInterest(MESSAGE2, sigc::mem_fun(this, &TestUBusServer::Callback));
  ubus_server.SendMessage(MESSAGE2);

  ProcessMessages();

  EXPECT_TRUE(callback_called);
  EXPECT_EQ(callback_call_count, 1);
}

TEST_F(TestUBusServer, SendUnregisteredMessage)
{
  ubus_server.SendMessage(MESSAGE1);

  ProcessMessages();
  EXPECT_FALSE(callback_called);
}

TEST_F(TestUBusServer, SendUninterestedMessage)
{
  ubus_server.RegisterInterest(MESSAGE1, sigc::mem_fun(this, &TestUBusServer::Callback));
  ubus_server.SendMessage(MESSAGE2);

  ProcessMessages();
  EXPECT_FALSE(callback_called);
}

TEST_F(TestUBusServer, MultipleDispatches)
{
  ubus_server.RegisterInterest(MESSAGE1, sigc::mem_fun(this, &TestUBusServer::Callback));
  ubus_server.RegisterInterest(MESSAGE1, sigc::mem_fun(this, &TestUBusServer::Callback));
  ubus_server.RegisterInterest(MESSAGE1, sigc::mem_fun(this, &TestUBusServer::Callback));
  ubus_server.SendMessage(MESSAGE1);

  ProcessMessages();
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(callback_call_count, 3);
}

TEST_F(TestUBusServer, MultipleDispatchesWithData)
{
  int cb_count = 0;
  ubus_server.RegisterInterest(MESSAGE1, [&cb_count] (glib::Variant const& data)
  {
    cb_count++;
    EXPECT_EQ(data.GetString(), "foo");
  });
  ubus_server.RegisterInterest(MESSAGE1, [&cb_count] (glib::Variant const& data)
  {
    cb_count++;
    EXPECT_EQ(data.GetString(), "foo");
  });
  ubus_server.SendMessage(MESSAGE2);
  ubus_server.SendMessage(MESSAGE1, g_variant_new_string("foo"));

  ProcessMessages();
  EXPECT_EQ(cb_count, 2);
}

TEST_F(TestUBusServer, DispatchesInOrder)
{
  std::string order;

  ubus_server.RegisterInterest(MESSAGE1, [&order] (glib::Variant const&)
      { order += "1"; });
  ubus_server.RegisterInterest(MESSAGE1, [&order] (glib::Variant const&)
      { order += "2"; });
  ubus_server.RegisterInterest(MESSAGE1, [&order] (glib::Variant const&)
      { order += "3"; });
  ubus_server.RegisterInterest(MESSAGE2, [&order] (glib::Variant const&)
      { order += "4"; });

  ubus_server.SendMessage(MESSAGE1);
  ubus_server.SendMessage(MESSAGE2);

  ProcessMessages();
  EXPECT_EQ(order, "1234");

  // prepare for second round of dispatches
  order = "";

  ubus_server.RegisterInterest(MESSAGE2, [&order] (glib::Variant const&)
      { order += "5"; });

  // swap order of the messages
  ubus_server.SendMessage(MESSAGE2);
  ubus_server.SendMessage(MESSAGE1);

  ProcessMessages();
  EXPECT_EQ(order, "45123");
}

TEST_F(TestUBusServer, DispatchesWithPriority)
{
  std::string order;

  ubus_server.RegisterInterest(MESSAGE1, [&order] (glib::Variant const&)
      { order += "1"; });
  ubus_server.RegisterInterest(MESSAGE1, [&order] (glib::Variant const&)
      { order += "2"; });
  ubus_server.RegisterInterest(MESSAGE1, [&order] (glib::Variant const&)
      { order += "3"; });
  ubus_server.RegisterInterest(MESSAGE2, [&order] (glib::Variant const&)
      { order += "4"; });
  ubus_server.RegisterInterest(MESSAGE2, [&order] (glib::Variant const&)
      { order += "5"; });

  glib::Variant data;
  ubus_server.SendMessage(MESSAGE1, data);
  ubus_server.SendMessageFull(MESSAGE2, data, glib::Source::Priority::HIGH);

  ProcessMessages();
  EXPECT_EQ(order, "45123");
}

TEST_F(TestUBusManager, RegisterAndSend)
{
  ubus_manager.RegisterInterest(MESSAGE1, sigc::mem_fun(this, &TestUBusManager::Callback));

  UBusManager::SendMessage(MESSAGE1);

  ProcessMessages();
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(callback_call_count, 1);
}

TEST_F(TestUBusManager, Unregister)
{
  auto interest_id = ubus_manager.RegisterInterest(MESSAGE1, 
      sigc::mem_fun(this, &TestUBusManager::Callback));

  UBusManager::SendMessage(MESSAGE1);

  ubus_manager.UnregisterInterest(interest_id);

  ProcessMessages();
  EXPECT_FALSE(callback_called);
}

TEST_F(TestUBusManager, AutoUnregister)
{
  if (true)
  {
    // want this to go out of scope
    UBusManager manager;

    manager.RegisterInterest(MESSAGE1,
                             sigc::mem_fun(this, &TestUBusManager::Callback));
  }

  UBusManager::SendMessage(MESSAGE1);

  ProcessMessages();
  EXPECT_FALSE(callback_called);
}

TEST_F(TestUBusManager, UnregisterInsideCallback)
{
  unsigned interest_id;
  interest_id = ubus_manager.RegisterInterest(MESSAGE1, [&] (glib::Variant const& data)
  {
    callback_called = true;
    callback_call_count++;
    ubus_manager.UnregisterInterest(interest_id);
  });

  UBusManager::SendMessage(MESSAGE1);
  UBusManager::SendMessage(MESSAGE1);

  ProcessMessages();
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(callback_call_count, 1);
}

TEST_F(TestUBusManager, DispatchWithPriority)
{
  std::string order;

  ubus_manager.RegisterInterest(MESSAGE1, [&order] (glib::Variant const&)
      { order += "1"; });
  ubus_manager.RegisterInterest(MESSAGE1, [&order] (glib::Variant const&)
      { order += "2"; });
  ubus_manager.RegisterInterest(MESSAGE1, [&order] (glib::Variant const&)
      { order += "3"; });
  ubus_manager.RegisterInterest(MESSAGE2, [&order] (glib::Variant const&)
      { order += "4"; });
  ubus_manager.RegisterInterest(MESSAGE2, [&order] (glib::Variant const&)
      { order += "5"; });

  glib::Variant data;
  UBusManager::SendMessage(MESSAGE1, data);
  UBusManager::SendMessage(MESSAGE2, data, glib::Source::Priority::HIGH);

  ProcessMessages();
  EXPECT_EQ(order, "45123");
}

}
