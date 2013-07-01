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
 * Authored by: Marco Trevisan (Treviño) <marco.trevisan@canonical.com>
 */

#include <gmock/gmock.h>
#include <UnityCore/ConnectionManager.h>
#include <glib.h>

using namespace unity;

namespace
{

TEST(TestConnectionHandle, Initialization)
{
  connection::handle handle;
  EXPECT_EQ(handle, 0);

  uint64_t val = g_random_int();
  connection::handle random = val;
  EXPECT_EQ(random, val);
}

TEST(TestConnectionHandle, Assignment)
{
  connection::handle handle;
  ASSERT_EQ(handle, 0);

  uint64_t val = g_random_int();
  handle = val;
  EXPECT_EQ(handle, val);
}

TEST(TestConnectionHandle, CastToScalarType)
{
  connection::handle handle = 5;
  ASSERT_EQ(handle, 5);

  int int_handle = handle;
  EXPECT_EQ(int_handle, 5);

  unsigned uint_handle = handle;
  EXPECT_EQ(uint_handle, 5);
}

TEST(TestConnectionHandle, PrefixIncrementOperator)
{
  connection::handle handle;
  ASSERT_EQ(handle, 0);

  for (auto i = 1; i <= 10; ++i)
  {
    ASSERT_EQ(++handle, i);
    ASSERT_EQ(handle, i);
  }
}

TEST(TestConnectionHandle, PostfixIncrementOperator)
{
  connection::handle handle;
  ASSERT_EQ(handle, 0);

  for (auto i = 1; i <= 10; ++i)
  {
    ASSERT_EQ(handle++, i-1);
    ASSERT_EQ(handle, i);
  }
}

TEST(TestConnectionHandle, PrefixDecrementOperator)
{
  connection::handle handle(10);
  ASSERT_EQ(handle, 10);

  for (auto i = 10; i > 0; --i)
  {
    ASSERT_EQ(--handle, i-1);
    ASSERT_EQ(handle, i-1);
  }
}

TEST(TestConnectionHandle, PostfixDecrementOperator)
{
  connection::handle handle(10);
  ASSERT_EQ(handle, 10);

  for (auto i = 10; i > 0; --i)
  {
    ASSERT_EQ(handle--, i);
    ASSERT_EQ(handle, i-1);
  }
}

connection::handle global_handle = 0;

struct SignalerObject
{
  sigc::signal<void> awesome_signal;
};

// connection::Wrapper tests

TEST(TestConnectionWrapper, InitializationEmpty)
{
  connection::Wrapper wrapper;
  EXPECT_FALSE(wrapper.Get().connected());
}

TEST(TestConnectionWrapper, InitializationFromConnection)
{
  SignalerObject signaler;
  sigc::connection conn = signaler.awesome_signal.connect([] {/* Awesome callback! */});

  connection::Wrapper wrapper(conn);
  EXPECT_TRUE(conn.connected());
}

TEST(TestConnectionWrapper, Get)
{
  SignalerObject signaler;
  sigc::connection conn = signaler.awesome_signal.connect([] {/* Awesome callback! */});

  connection::Wrapper wrapper(conn);
  EXPECT_TRUE(wrapper.Get().connected());
}

TEST(TestConnectionWrapper, DisconnectOnDestruction)
{
  SignalerObject signaler;
  sigc::connection conn = signaler.awesome_signal.connect([] {/* Awesome callback! */});

  {
    connection::Wrapper wrapper(conn);
    ASSERT_TRUE(conn.connected());
  }

  EXPECT_FALSE(conn.connected());
}

TEST(TestConnectionWrapper, CastToConnection)
{
  SignalerObject signaler;
  sigc::connection conn = signaler.awesome_signal.connect([] {/* Awesome callback! */});

  connection::Wrapper wrapper(conn);
  conn.block(true);

  sigc::connection conn2 = wrapper;
  EXPECT_TRUE(conn2.blocked());
}

TEST(TestConnectionWrapper, CastToBool)
{
  SignalerObject signaler;

  connection::Wrapper wrapper;
  EXPECT_FALSE(wrapper);

  sigc::connection conn = signaler.awesome_signal.connect([] {/* Awesome callback! */});
  wrapper = conn;
  EXPECT_TRUE(wrapper);
}

TEST(TestConnectionWrapper, PointerConstOperator)
{
  SignalerObject signaler;
  sigc::connection conn = signaler.awesome_signal.connect([] {/* Awesome callback! */});

  connection::Wrapper wrapper(conn);
  conn.block(true);

  EXPECT_TRUE(wrapper->blocked());
  EXPECT_TRUE(wrapper->connected());
}

TEST(TestConnectionWrapper, PointerOperator)
{
  SignalerObject signaler;
  sigc::connection conn = signaler.awesome_signal.connect([] {/* Awesome callback! */});
  connection::Wrapper wrapper(conn);

  wrapper->disconnect();
  EXPECT_FALSE(conn.connected());
}

TEST(TestConnectionWrapper, StarConstOperator)
{
  SignalerObject signaler;
  sigc::connection conn = signaler.awesome_signal.connect([] {/* Awesome callback! */});

  connection::Wrapper wrapper(conn);
  conn.block(true);

  EXPECT_TRUE((*wrapper).blocked());
  EXPECT_TRUE((*wrapper).connected());
}

TEST(TestConnectionWrapper, StarOperator)
{
  SignalerObject signaler;
  sigc::connection conn = signaler.awesome_signal.connect([] {/* Awesome callback! */});

  connection::Wrapper wrapper(conn);

  (*wrapper).disconnect();
  EXPECT_FALSE(conn.connected());
}

TEST(TestConnectionWrapper, AssignmentOperator)
{
  SignalerObject signaler;
  sigc::connection conn1 = signaler.awesome_signal.connect([] {/* Awesome callback! */});

  connection::Wrapper wrapper(conn1);
  conn1.block(true);

  ASSERT_TRUE(conn1.connected());

  sigc::connection conn2 = signaler.awesome_signal.connect([] {/* Awesome callback! */});
  wrapper = conn2;

  EXPECT_FALSE(conn1.connected());
  EXPECT_TRUE(conn2.connected());
}

// connection::Manager tests

TEST(TestConnectionManager, Initialization)
{
  connection::Manager manager;
  EXPECT_TRUE(manager.Empty());
  EXPECT_EQ(manager.Size(), 0);
}

TEST(TestConnectionManager, AddEmpty)
{
  connection::Manager manager;
  sigc::connection empty_connection;
  ASSERT_TRUE(empty_connection.empty());

  connection::handle handle = manager.Add(empty_connection);

  EXPECT_EQ(handle, global_handle);
  EXPECT_TRUE(manager.Empty());
  EXPECT_EQ(manager.Size(), 0);
}

TEST(TestConnectionManager, AddSignal)
{
  connection::Manager manager;
  SignalerObject signaler;

  sigc::connection conn = signaler.awesome_signal.connect([] {/* Awesome callback! */});
  ASSERT_TRUE(conn.connected());

  connection::handle handle = manager.Add(conn);

  ++global_handle;
  EXPECT_EQ(handle, global_handle);
  EXPECT_FALSE(manager.Empty());
  EXPECT_EQ(manager.Size(), 1);
}

TEST(TestConnectionManager, AddMultipleSignals)
{
  connection::Manager manager;
  SignalerObject signaler;

  for (int i = 1; i <= 10; ++i)
  {
    auto const& conn = signaler.awesome_signal.connect([] {/* Awesome callback! */});
    auto handle = manager.Add(conn);
    EXPECT_EQ(handle, ++global_handle);
    EXPECT_EQ(manager.Size(), i);
  }
}

TEST(TestConnectionManager, RemoveAvailable)
{
  connection::Manager manager;
  SignalerObject signaler;

  sigc::connection conn = signaler.awesome_signal.connect([] {/* Awesome callback! */});
  auto handle = manager.Add(conn);
  ASSERT_TRUE(conn.connected());
  ASSERT_FALSE(manager.Empty());

  EXPECT_TRUE(manager.Remove(handle));
  EXPECT_FALSE(conn.connected());
  EXPECT_TRUE(manager.Empty());
}

TEST(TestConnectionManager, RemoveAndClearAvailable)
{
  connection::Manager manager;
  SignalerObject signaler;

  sigc::connection conn = signaler.awesome_signal.connect([] {/* Awesome callback! */});
  auto handle = manager.Add(conn);
  ASSERT_TRUE(conn.connected());
  ASSERT_FALSE(manager.Empty());

  EXPECT_TRUE(manager.RemoveAndClear(&handle));
  EXPECT_FALSE(conn.connected());
  EXPECT_TRUE(manager.Empty());
  EXPECT_EQ(handle, 0);
}

TEST(TestConnectionManager, RemoveUnavailable)
{
  connection::Manager manager;

  connection::handle handle = 5;
  EXPECT_FALSE(manager.RemoveAndClear(&handle));
  EXPECT_TRUE(manager.Empty());
  EXPECT_EQ(handle, 5);
}

TEST(TestConnectionManager, ReplaceOnEmpty)
{
  connection::Manager manager;
  SignalerObject signaler;

  auto const& conn = signaler.awesome_signal.connect([] {/* Awesome callback! */});
  EXPECT_GT(manager.Replace(0, conn), 0);
  EXPECT_FALSE(manager.Empty());
}

TEST(TestConnectionManager, ReplaceUnavailable)
{
  connection::Manager manager;
  SignalerObject signaler;

  auto const& conn = signaler.awesome_signal.connect([] {/* Awesome callback! */});
  manager.Add(conn);
  ASSERT_FALSE(manager.Empty());

  EXPECT_GT(manager.Replace(0, conn), 0);
  EXPECT_EQ(manager.Size(), 2);
}

TEST(TestConnectionManager, ReplaceAvailable)
{
  connection::Manager manager;
  SignalerObject signaler;

  sigc::connection first_conn = signaler.awesome_signal.connect([] {/* Awesome callback! */});
  auto first_handle = manager.Add(first_conn);
  ASSERT_FALSE(manager.Empty());

  sigc::connection second_conn = signaler.awesome_signal.connect([] {/* Awesome callback! */});
  auto second_handle = manager.Replace(first_handle, second_conn);
  EXPECT_EQ(manager.Size(), 1);
  EXPECT_EQ(first_handle, second_handle);

  EXPECT_FALSE(first_conn.connected());
  EXPECT_TRUE(second_conn.connected());
}

TEST(TestConnectionManager, GetAvailable)
{
  connection::Manager manager;
  SignalerObject signaler;

  sigc::connection first_conn = signaler.awesome_signal.connect([] {/* Awesome callback! */});
  auto handle = manager.Add(first_conn);
  ASSERT_FALSE(manager.Empty());

  auto second_conn = manager.Get(handle);
  EXPECT_TRUE(second_conn.connected());

  second_conn.disconnect();
  EXPECT_FALSE(first_conn.connected());
}

TEST(TestConnectionManager, GetUnavailable)
{
  connection::Manager manager;

  auto conn = manager.Get(0);
  EXPECT_FALSE(conn.connected());
  EXPECT_TRUE(conn.empty());

  conn = manager.Get(g_random_int());
  EXPECT_FALSE(conn.connected());
  EXPECT_TRUE(conn.empty());
}

TEST(TestConnectionManager, Clear)
{
  SignalerObject signaler;
  std::vector<sigc::connection> connections;
  connection::Manager manager;

  for (int i = 1; i <= 10; ++i)
  {
    auto const& conn = signaler.awesome_signal.connect([] {/* Awesome callback! */});
    connections.push_back(conn);
    ASSERT_TRUE(connections.back().connected());
    manager.Add(conn);
  }

  ASSERT_FALSE(manager.Empty());
  ASSERT_EQ(manager.Size(), connections.size());

  manager.Clear();

  for (auto const& conn : connections)
    ASSERT_FALSE(conn.connected());

  EXPECT_TRUE(manager.Empty());
}

TEST(TestConnectionManager, DisconnectOnDestruction)
{
  SignalerObject signaler;
  std::vector<sigc::connection> connections;

  for (int i = 1; i <= 10; ++i)
  {
    auto const& conn = signaler.awesome_signal.connect([] {/* Awesome callback! */});
    connections.push_back(conn);
    ASSERT_TRUE(connections.back().connected());
  }

  {
    connection::Manager manager;

    for (auto const& conn : connections)
      manager.Add(conn);

    ASSERT_FALSE(manager.Empty());
    ASSERT_EQ(manager.Size(), connections.size());
  }

  for (auto const& conn : connections)
    ASSERT_FALSE(conn.connected());
}

TEST(TestConnectionManager, DestructWithDisconnected)
{
  SignalerObject signaler;
  std::vector<sigc::connection> connections;

  {
    connection::Manager manager;

    for (int i = 1; i <= 10; ++i)
    {
      auto const& conn = signaler.awesome_signal.connect([] {/* Awesome callback! */});
      connections.push_back(conn);
      manager.Add(conn);
    }

    ASSERT_FALSE(manager.Empty());
    ASSERT_EQ(manager.Size(), connections.size());

    for (auto& conn : connections)
      conn.disconnect();

    EXPECT_EQ(manager.Size(), connections.size());
  }

  // At this point the manager has been destructed
}

} // Namespace
