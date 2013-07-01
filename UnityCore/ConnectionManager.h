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
* Authored by: Marco Trevisan <marco.trevisan@canonical.com>
*/

#ifndef UNITY_CONNECTION_MANAGER
#define UNITY_CONNECTION_MANAGER

#include <memory>
#include <unordered_map>
#include <sigc++/sigc++.h>

namespace unity
{
namespace connection
{
struct handle
{
  handle() : handle_(0) {}
  handle(uint64_t val) : handle_(val) {}
  operator uint64_t() const { return handle_; }
  handle& operator++() { ++handle_; return *this; }
  handle operator++(int) { auto tmp = *this; ++handle_; return tmp; }
  handle& operator--() { --handle_; return *this; }
  handle operator--(int) { auto tmp = *this; --handle_; return tmp; }

private:
  uint64_t handle_;
};
} // connection namespace
} // unity namespace

namespace std
{
// Template specialization, needed for unordered_map
template<> struct hash<unity::connection::handle>
{
  std::size_t operator()(unity::connection::handle const& h) const
  {
    return std::hash<uint64_t>()(h);
  }
};
}

namespace unity
{
namespace connection
{

class Wrapper
{
public:
  typedef std::shared_ptr<Wrapper> Ptr;

  Wrapper() {};
  Wrapper(sigc::connection const& conn) : conn_(conn) {}
  ~Wrapper() { conn_.disconnect(); };

  operator sigc::connection() const { return conn_; }
  operator bool() const { return conn_.connected(); }
  Wrapper& operator=(sigc::connection const& conn) { conn_.disconnect(); conn_ = conn; return *this; }
  const sigc::connection* operator->() const { return &conn_; }
  sigc::connection* operator->() { return &conn_; }
  sigc::connection const& operator*() const { return conn_; }
  sigc::connection& operator*() { return conn_; }
  sigc::connection const& Get() const { return conn_; }

private:
  Wrapper(Wrapper const&) = delete;
  Wrapper& operator=(Wrapper const&) = delete;

  sigc::connection conn_;
};

class Manager
{
public:
  Manager() = default;
  ~Manager();

  handle Add(sigc::connection const&);
  bool Remove(handle);
  bool RemoveAndClear(handle*);
  handle Replace(handle const&, sigc::connection const&);
  sigc::connection Get(handle const&) const;

  void Clear();
  bool Empty() const;
  size_t Size() const;

private:
  Manager(Manager const&) = delete;
  Manager& operator=(Manager const&) = delete;

  std::unordered_map<handle, sigc::connection> connections_;
};

} // connection namespace
} // unity namespace

#endif
