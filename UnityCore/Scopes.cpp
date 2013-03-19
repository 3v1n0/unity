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
 * Authored by: Nick Dedekind <nick.dedeknd@canoncial.com>
 */

#include "Scopes.h"
#include <stdexcept>

namespace unity
{
namespace dash
{
DECLARE_LOGGER(logger, "unity.dash.scopes");

class Scopes::Impl
{
public:
  Impl(Scopes* owner, ScopesReader::Ptr scopes_reader);
  ~Impl();

  void LoadScopes();
  void InsertScope(ScopeData::Ptr const& data, unsigned index);
  void RemoveScope(std::string const& scope);

  ScopeList const& GetScopes() const;
  Scope::Ptr GetScope(std::string const& scope_id, int* index = nullptr) const;
  Scope::Ptr GetScopeAtIndex(std::size_t index) const;
  Scope::Ptr GetScopeForShortcut(std::string const& scope_shortcut) const;

  Scopes* owner_;
  ScopesReader::Ptr scopes_reader_;
  ScopeList scopes_;
  sigc::connection scope_changed_signal;

  std::size_t get_count() const { return scopes_.size(); }

  void UpdateScopes(ScopeDataList const& scopes_list);
};

Scopes::Impl::Impl(Scopes* owner, ScopesReader::Ptr scopes_reader)
: owner_(owner)
, scopes_reader_(scopes_reader)
{
}

Scopes::Impl::~Impl()
{
  scope_changed_signal.disconnect();
}

void Scopes::Impl::UpdateScopes(ScopeDataList const& scopes_list)
{
  // insert new.
  int index = 0;
  for (ScopeData::Ptr const& scope_data: scopes_list)  
  {
    InsertScope(scope_data, index++);
  }

  // remove old.
  std::vector<std::string> remove_scopes;
  for (Scope::Ptr const& scope: scopes_)
  {
    auto scope_data_position = std::find_if(scopes_list.begin(),
                                            scopes_list.end(),
                                            [scope](ScopeData::Ptr const& data) { return data->id() == scope->id(); });
    if (scope_data_position == scopes_list.end())
    {
      remove_scopes.push_back(scope->id());
    }
  }
  for (std::string const& scope: remove_scopes)
  {
    RemoveScope(scope);
  }
}

void Scopes::Impl::LoadScopes()
{
  if (!scopes_reader_)
    return;

  scope_changed_signal.disconnect();
  scope_changed_signal = scopes_reader_->scopes_changed.connect(sigc::mem_fun(this, &Impl::UpdateScopes));
  UpdateScopes(scopes_reader_->GetScopesData());
}

void Scopes::Impl::InsertScope(ScopeData::Ptr const& scope_data, unsigned index)
{
  if (!scope_data)
    return;

  index = std::min(index, static_cast<unsigned>(scopes_.size()));

  auto start = scopes_.begin();
  auto scope_position = std::find_if(scopes_.begin(),
                                     scopes_.end(),
                                     [scope_data](Scope::Ptr const& value) { return value->id() == scope_data->id(); });
  if (scope_position == scopes_.end())
  {
    Scope::Ptr scope(owner_->CreateScope(scope_data));
    if (scope)
    {
      scopes_.insert(start+index, scope);

      owner_->scope_added.emit(scope, index);
    }
  }
  else
  {
    Scope::Ptr scope = *scope_position;
    if (scope_position > (start + index))
    {
      // move before
      scopes_.erase(scope_position);
      scopes_.insert(start + index, scope);

      owner_->scopes_reordered.emit(GetScopes());
    }
    else if (index > 0 &&
             scope_position < (start + index) &&
             scopes_.size() > index)
    {
      // move after
      scopes_.erase(scope_position);

      scopes_.insert(start + index, scope);

      owner_->scopes_reordered.emit(GetScopes());
    }
  }
}

void Scopes::Impl::RemoveScope(std::string const& scope_id)
{
  auto scope_position = std::find_if(scopes_.begin(),
                                     scopes_.end(),
                                     [scope_id](Scope::Ptr const& value) { return value->id() == scope_id; });

  if (scope_position != scopes_.end())
  {
    Scope::Ptr scope = *scope_position;
    scopes_.erase(scope_position);

    owner_->scope_removed.emit(scope);
  }
}

Scopes::ScopeList const& Scopes::Impl::GetScopes() const
{
  return scopes_;
}

Scope::Ptr Scopes::Impl::GetScope(std::string const& scope_id, int* index) const
{
  int tmp_index = 0;
  for (auto scope: scopes_)
  {
    if (scope->id == scope_id)
    {
      if (index)
        *index = tmp_index;
      return scope;
    }
    tmp_index++;
  }
  if (index)
    *index = -1;
  return Scope::Ptr();
}

Scope::Ptr Scopes::Impl::GetScopeAtIndex(std::size_t index) const
{
  try
  {
    return scopes_.at(index);
  }
  catch (std::out_of_range& error)
  {
    LOG_WARN(logger) << error.what();
  }
  return Scope::Ptr();
}

Scope::Ptr Scopes::Impl::GetScopeForShortcut(std::string const& scope_shortcut) const
{
  for (auto scope: scopes_)
  {
    if (scope->shortcut == scope_shortcut)
    {
      return scope;
    }
  }

  return Scope::Ptr();
}

Scopes::Scopes(ScopesReader::Ptr scopes_reader)
: pimpl(new Impl(this, scopes_reader))
{
  count.SetGetterFunction(sigc::mem_fun(pimpl.get(), &Impl::get_count));
}

Scopes::~Scopes()
{
}

void Scopes::LoadScopes()
{
  pimpl->LoadScopes();
}

Scopes::ScopeList const& Scopes::GetScopes() const
{
  return pimpl->GetScopes();
}

Scope::Ptr Scopes::GetScope(std::string const& scope_id, int* position) const
{
  return pimpl->GetScope(scope_id, position);
}

Scope::Ptr Scopes::GetScopeAtIndex(std::size_t index) const
{
  return pimpl->GetScopeAtIndex(index);
}

Scope::Ptr Scopes::GetScopeForShortcut(std::string const& scope_shortcut) const
{
  return pimpl->GetScopeForShortcut(scope_shortcut);
}

void Scopes::AppendScope(std::string const& scope_id)
{
  InsertScope(scope_id, pimpl->scopes_.size());
}

void Scopes::InsertScope(std::string const& scope_id, unsigned index)
{
  if (pimpl->scopes_reader_)
  {
    ScopeData::Ptr scope_data(pimpl->scopes_reader_->GetScopeDataById(scope_id));
    if (scope_data)
    {
      pimpl->InsertScope(scope_data, index);
      return;
    }
  }

  // Fallback - manually create and insert the scope.
  glib::Error error;
  ScopeData::Ptr scope_data(ScopeData::ReadProtocolDataForId(scope_id, error));
  scope_data->visible = false;
  if (scope_data && !error)
  {
    pimpl->InsertScope(scope_data, index);
  }
}

void Scopes::RemoveScope(std::string const& scope_id)
{
  pimpl->RemoveScope(scope_id);
}

Scope::Ptr Scopes::CreateScope(ScopeData::Ptr const& scope_data)
{
  if (!scope_data)
    return Scope::Ptr();

  LOG_DEBUG(logger) << "Creating scope: " << scope_data->id() << " (" << scope_data->dbus_path()  << " @ " << scope_data->dbus_name() << ")";
  Scope::Ptr scope(new Scope(scope_data));
  scope->Init();
  return scope;
}


} // namespace dash
} // namespace unity
