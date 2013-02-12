// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
                Nick Dedekind <nick.dedeknd@canoncial.com>
 */

#ifndef UNITY_SCOPES_H
#define UNITY_SCOPES_H

#include <sigc++/trackable.h>
#include <sigc++/signal.h>

#include "Scope.h"

namespace unity
{
namespace dash
{

typedef std::vector<ScopeData::Ptr> ScopeDataList;

class ScopesReader : public sigc::trackable, boost::noncopyable
{
public:
  typedef std::shared_ptr<ScopesReader> Ptr;
  virtual ~ScopesReader() {}

  virtual void LoadScopes() = 0;
  virtual ScopeDataList const& GetScopesData() const = 0;

  virtual ScopeData::Ptr GetScopeDataById(std::string const& scope_id) const = 0;

  sigc::signal<void, ScopeDataList const&> scopes_changed;
};

class Scopes : public sigc::trackable, boost::noncopyable
{
public:
  typedef std::shared_ptr<Scopes> Ptr;
  typedef std::vector<Scope::Ptr> ScopeList;
 
  Scopes(ScopesReader::Ptr scope_reader);
  virtual ~Scopes();

  virtual void LoadScopes();

  /**
   * Get the currently loaded Scopess. This is necessary as some of the consumers
   * of this object employ a lazy-loading technique to reduce the overhead of
   * starting Unity. Therefore, the Scopes may already have been loaded by the time
   * the objects have been initiated (and so just connecting to the signals is not
   * enough)
   */
  virtual ScopeList const& GetScopes() const;
  virtual Scope::Ptr GetScope(std::string const& scope_id, int* position = nullptr) const;
  virtual Scope::Ptr GetScopeAtIndex(std::size_t index) const;
  virtual Scope::Ptr GetScopeForShortcut(std::string const& scope_shortcut) const;

  nux::ROProperty<std::size_t> count;

  sigc::signal<void, Scope::Ptr const&, int> scope_added;
  sigc::signal<void, Scope::Ptr const&> scope_removed;

  sigc::signal<void, ScopeList const&> scopes_reordered;

  virtual void AppendScope(std::string const& scope_id);
  virtual void InsertScope(std::string const& scope_id, unsigned index);
  virtual void RemoveScope(std::string const& scope_id);

protected:
  virtual Scope::Ptr CreateScope(ScopeData::Ptr const& scope_data);

private:
  class Impl;
  std::unique_ptr<Impl> pimpl;

  friend class TestScope;
};

} // namespace dash
} // namespace unity

#endif // UNITY_SCOPES_H
