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
#include "ActionHandle.h"

namespace unity
{
namespace connection
{
typedef unity::action::handle handle;

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

  std::unordered_map<handle, Wrapper::Ptr> connections_;
};

} // connection namespace
} // unity namespace

#endif
