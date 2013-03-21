// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright 2013 Canonical Ltd.
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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 *
 */

#ifndef TEST_MOCK_SCOPE_H
#define TEST_MOCK_SCOPE_H

#include <gmock/gmock.h>
#include <UnityCore/GSettingsScopes.h>
#include <UnityCore/GLibSource.h>

#include <unity-protocol.h>

namespace unity
{
namespace dash
{

namespace
{
const gchar* SETTINGS_NAME = "com.canonical.Unity.Dash";
const gchar* SCOPES_SETTINGS_KEY = "scopes";
}

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

  virtual void ConnectProxy() { connected_ = true; }
  virtual void DisconnectProxy() { connected_ = false; }

  virtual void Search(std::string const& search_hint, SearchCallback const& callback = nullptr, GCancellable* cancellable = nullptr)
  {
    source_manager.AddIdle([search_hint, callback] () {
      callback(search_hint, glib::HintsMap(), glib::Error());
      return true;
    }, "Search");
  }

  virtual void Activate(LocalResult const& result, uint activate_type, glib::HintsMap const& hints, ActivateCallback const& callback = nullptr, GCancellable* cancellable = nullptr)
  {
    source_manager.AddIdle([activate_type, callback, result] ()
    {
      switch (activate_type)
      {
        case UNITY_PROTOCOL_ACTION_TYPE_ACTIVATE_RESULT:
          callback(result, ScopeHandledType::NOT_HANDLED, glib::HintsMap(), glib::Error());
          break;

        case UNITY_PROTOCOL_ACTION_TYPE_PREVIEW_RESULT:
          callback(result, ScopeHandledType::SHOW_PREVIEW, glib::HintsMap(), glib::Error());
          break;

        case UNITY_PROTOCOL_ACTION_TYPE_PREVIEW_ACTION:
          callback(result, ScopeHandledType::HIDE_DASH, glib::HintsMap(), glib::Error());
          break;

        default:
        {
          glib::Error error;
          GError** real_err = &error;
          *real_err = g_error_new_literal(G_SCOPE_ERROR, G_SCOPE_ERROR_NO_ACTIVATION_HANDLER, "Invalid scope activation typehandler");
          callback(result, ScopeHandledType::NOT_HANDLED, glib::HintsMap(), error);
        } break;
      }
      return true;
    }, "Activate");
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
  MockScopeData(std::string const& scope_id,
              std::string const& _dbus_name = "",
              std::string const& _dbus_path = "")
  {
    id = scope_id;
    dbus_name = _dbus_name;
    dbus_path = _dbus_path;
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
  MockGSettingsScopes(const char* scopes_settings[])
  {
    gsettings_client = g_settings_new(SETTINGS_NAME);
    UpdateScopes(scopes_settings);
  }

  using GSettingsScopes::InsertScope;
  using GSettingsScopes::RemoveScope;

  void UpdateScopes(const char* scopes_settings[])
  {
    g_settings_set_strv(gsettings_client, SCOPES_SETTINGS_KEY, scopes_settings);
  }

protected:
  virtual Scope::Ptr CreateScope(ScopeData::Ptr const& scope_data)
  {
    Scope::Ptr scope(new MockScope(scope_data));
    return scope;
  }

private:
  glib::Object<GSettings> gsettings_client;
};

} // namesapce dash
} // namesapce unity


#endif // TEST_MOCK_SCOPE_H
