// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 *
 */

#ifndef TEST_MOCK_SCOPE_H
#define TEST_MOCK_SCOPE_H

#include <gmock/gmock.h>
#include <UnityCore/GSettingsScopes.h>
#include <UnityCore/GLibSource.h>

namespace unity
{
namespace dash
{

// Mock Scopes for use in xless tests. (no dbus!)

class MockScopeProxy : public ScopeProxyInterface
{
public:
  MockScopeProxy(std::string const& _name, std::string const& _icon_hint)
  : connected_(true)
  {
    connected.SetGetterFunction([this] () { return connected_; });
    name.SetGetterFunction([_name] () { return _name; });
    icon_hint.SetGetterFunction([_icon_hint] () { return _icon_hint; });
    visible.SetGetterFunction([]() { return true; });
  }

  void CreateProxy() { connected_ = true; }

  virtual void Search(std::string const& search_hint, SearchCallback const& callback = nullptr, GCancellable* cancellable = nullptr)
  {
    source_manager.AddIdle([callback] () {
      callback(glib::HintsMap(), glib::Error());
      return true;
    }, "Search");
  }

  virtual void Activate(LocalResult const& result, uint activate_type, glib::HintsMap const& hints, ActivateCallback const& callback = nullptr, GCancellable* cancellable = nullptr)
  {
    source_manager.AddIdle([callback, result] (){
      callback(result, ScopeHandledType::GOTO_DASH_URI, glib::HintsMap(), glib::Error());
      return true;
    }, "Activate");
  }

  virtual void UpdatePreviewProperty(LocalResult const& result, glib::HintsMap const& hints, UpdatePreviewPropertyCallback const& callback, GCancellable* cancellable)
  {
    source_manager.AddIdle([callback, result] (){
      callback(glib::HintsMap(), glib::Error());
      return true;
    }, "UpdatePreviewProperty");
  }


  virtual Results::Ptr GetResultsForCategory(unsigned category) const
  {
    return Results::Ptr();
  }

protected:
  glib::SourceManager source_manager;
  bool connected_;
};

class MockScopeData : public ScopeData
{
public:
  MockScopeData(std::string const& scope_id)
  {
    id = scope_id;
  }
};

class MockScope : public Scope
{
public:
  typedef std::shared_ptr<MockScope> Ptr;

  MockScope(ScopeData::Ptr const& scope_data,
            std::string const& name = "",
            std::string const& icon_hint = "")
  : Scope(scope_data)
  {
    proxy_func = [name, icon_hint]()
    {
      return ScopeProxyInterface::Ptr(new MockScopeProxy(name, icon_hint));
    };
    
    Init();
  }

  virtual ScopeProxyInterface::Ptr CreateProxyInterface() const
  {
    return proxy_func();
  }

  std::function<ScopeProxyInterface::Ptr(void)> proxy_func;
};

class MockGSettingsScopes : public GSettingsScopes
{
public:
  MockGSettingsScopes() {}
  ~MockGSettingsScopes() {}

  using GSettingsScopes::InsertScope;
  using GSettingsScopes::RemoveScope;

protected:
  virtual Scope::Ptr CreateScope(ScopeData::Ptr const& scope_data)
  {
    Scope::Ptr scope(new MockScope(scope_data));
    return scope;
  }
};

} // namesapce dash
} // namesapce unity


#endif // TEST_MOCK_SCOPE_H
