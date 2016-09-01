// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010-2012 Canonical Ltd
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
 *              Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 */

#include <NuxCore/Logger.h>

#include "config.h"

#include "GLibWrapper.h"
#include "GLibDBusProxy.h"
#include "GLibSource.h"
#include "Variant.h"
#include "DBusIndicators.h"
#include "services/panel-service-private.h"

namespace unity
{
namespace indicator
{

namespace
{
DECLARE_LOGGER(logger, "unity.indicator.dbus");

inline bool verify_variant_type(GVariant* value, const gchar* type)
{
  if (!g_variant_is_of_type (value, G_VARIANT_TYPE(type)))
  {
    LOG_ERROR(logger) << "Got invalid variant type: '"
                      << g_variant_get_type_string(value) << "' ('"
                      << type << "' was expected)";
    return false;
  }

  return true;
}
} // anonymous namespace


/* Connects to the remote panel service (unity-panel-service) and translates
 * that into something that the panel can show */
struct DBusIndicators::Impl
{
  Impl(std::string const& dbus_name, DBusIndicators* owner);

  void CheckLocalService();
  void RequestSyncAll();
  void RequestSyncIndicator(std::string const& name);
  void Sync(GVariant* args, glib::Error const&);
  void SyncGeometries(std::string const& name, EntryLocationMap const& locations);
  void ShowEntriesDropdown(Indicator::Entries const&, Entry::Ptr const&, unsigned xid, int x, int y);
  void CloseActiveEntry();

  void OnConnected();
  void OnDisconnected();

  void OnReSync(GVariant* parameters);
  void OnIconsPathChanged(GVariant* parameters);
  void OnEntryActivated(GVariant* parameters);
  void OnEntryActivatedRequest(GVariant* parameters);

  void OnEntryScroll(std::string const& entry_id, int delta);
  void OnEntryShowMenu(std::string const& entry_id, unsigned int xid, int x, int y, unsigned int button);
  void OnEntrySecondaryActivate(std::string const& entry_id);
  void OnShowAppMenu(unsigned int xid, int x, int y);

  DBusIndicators* owner_;

  glib::DBusProxy gproxy_;
  glib::Source::UniquePtr reconnect_timeout_;
  glib::Source::UniquePtr show_entry_idle_;
  glib::Source::UniquePtr show_appmenu_idle_;
  std::vector<std::string> icon_paths_;
  std::unordered_map<std::string, EntryLocationMap> cached_locations_;
};


// Public Methods
DBusIndicators::Impl::Impl(std::string const& dbus_name, DBusIndicators* owner)
  : owner_(owner)
  , gproxy_(dbus_name, UPS_PATH, UPS_IFACE, G_BUS_TYPE_SESSION,
            G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES)
{
  gproxy_.Connect("ReSync", sigc::mem_fun(this, &DBusIndicators::Impl::OnReSync));
  gproxy_.Connect("IconPathsChanged", sigc::mem_fun(this, &DBusIndicators::Impl::OnIconsPathChanged));
  gproxy_.Connect("EntryActivated", sigc::mem_fun(this, &DBusIndicators::Impl::OnEntryActivated));
  gproxy_.Connect("EntryActivateRequest", sigc::mem_fun(this, &DBusIndicators::Impl::OnEntryActivatedRequest));

  gproxy_.connected.connect(sigc::mem_fun(this, &DBusIndicators::Impl::OnConnected));
  gproxy_.disconnected.connect(sigc::mem_fun(this, &DBusIndicators::Impl::OnDisconnected));

  CheckLocalService();

  if (gproxy_.IsConnected()) {
    RequestSyncAll();
  }
}

void DBusIndicators::Impl::CheckLocalService()
{
  if (g_getenv("PANEL_USE_LOCAL_SERVICE"))
  {
    glib::Error error;

    g_spawn_command_line_sync("killall -9 unity-panel-service",
                              nullptr, nullptr, nullptr, nullptr);

    // This is obviously hackish, but this part of the code is mostly hackish...
    // Let's attempt to run it from where we expect it to be
    std::string cmd = UNITY_LIBDIR"/" + std::string("unity-panel-service");
    LOG_WARN(logger) << "Couldn't load panel from installed services, "
                     << "so trying to load panel from known location: " << cmd;

    g_spawn_command_line_async(cmd.c_str(), &error);

    if (error)
    {
      LOG_ERROR(logger) << "Unable to launch remote service manually: "
                        << error.Message();
    }
  }
}

void DBusIndicators::Impl::OnConnected()
{
  OnIconsPathChanged(nullptr);
  RequestSyncAll();
}

void DBusIndicators::Impl::OnDisconnected()
{
  for (auto indicator : owner_->GetIndicators())
  {
    owner_->RemoveIndicator(indicator->name());
  }

  cached_locations_.clear();

  CheckLocalService();
}

void DBusIndicators::Impl::OnReSync(GVariant* parameters)
{
  glib::String indicator_name;
  g_variant_get(parameters, "(s)", &indicator_name);

  if (indicator_name && !indicator_name.Str().empty())
  {
    RequestSyncIndicator(indicator_name);
  }
  else
  {
    RequestSyncAll();
  }
}

void DBusIndicators::Impl::OnIconsPathChanged(GVariant*)
{
  gproxy_.CallBegin("GetIconPaths", nullptr, [this] (GVariant* paths, glib::Error const& e) {
    if (e || !paths)
    {
      LOG_ERROR(logger) << "Something went wrong on GetIconPaths: " << e;
      return;
    }

    bool changed = false;
    gsize length;
    glib::Variant array(g_variant_get_child_value(paths, 0));
    const gchar** icon_paths = g_variant_get_strv(array, &length);

    if (icon_paths_.size() != length)
    {
      changed = true;
    }
    else
    {
      for (unsigned i = 0; i < length; ++i)
      {
        if (icon_paths_[i] != glib::gchar_to_string(icon_paths[i]))
        {
          changed = true;
          break;
        }
      }
    }

    if (changed)
    {
      icon_paths_.resize(length);

      for (unsigned i = 0; i < length; ++i)
        icon_paths_[i] = glib::gchar_to_string(icon_paths[i]);

      owner_->icon_paths_changed.emit();
    }

    g_free(icon_paths);
  });
}

void DBusIndicators::Impl::OnEntryActivated(GVariant* parameters)
{
  if (!verify_variant_type(parameters, "(ss(iiuu))"))
    return;

  glib::String panel;
  glib::String entry_id;
  nux::Rect geo;
  g_variant_get(parameters, "(ss(iiuu))", &panel, &entry_id, &geo.x, &geo.y, &geo.width, &geo.height);

  if (entry_id)
    owner_->ActivateEntry(panel.Str(), entry_id.Str(), geo);
}

void DBusIndicators::Impl::OnEntryActivatedRequest(GVariant* parameters)
{
  if (!verify_variant_type(parameters, "(s)"))
    return;

  glib::String entry_name;
  g_variant_get(parameters, "(s)", &entry_name);

  if (entry_name)
    owner_->on_entry_activate_request.emit(entry_name);
}

void DBusIndicators::Impl::RequestSyncAll()
{
  gproxy_.CallBegin("Sync", nullptr, sigc::mem_fun(this, &DBusIndicators::Impl::Sync));
}

void DBusIndicators::Impl::RequestSyncIndicator(std::string const& name)
{
  GVariant* parameter = g_variant_new("(s)", name.c_str());

  gproxy_.CallBegin("SyncOne", parameter, sigc::mem_fun(this, &DBusIndicators::Impl::Sync));
}


void DBusIndicators::Impl::OnEntryShowMenu(std::string const& entry_id,
                                           unsigned int xid, int x, int y,
                                           unsigned int button)
{
  owner_->on_entry_show_menu.emit(entry_id, xid, x, y, button);

  // We have to do this because on certain systems X won't have time to
  // respond to our request for XUngrabPointer and this will cause the
  // menu not to show

  show_entry_idle_.reset(new glib::Idle(glib::Source::Priority::DEFAULT));
  show_entry_idle_->Run([this, entry_id, xid, x, y, button] {
    gproxy_.Call("ShowEntry", g_variant_new("(suiiu)", entry_id.c_str(), xid,
                                                       x, y, button));
    return false;
  });
}

void DBusIndicators::Impl::ShowEntriesDropdown(Indicator::Entries const& entries, Entry::Ptr const& selected, unsigned xid, int x, int y)
{
  if (entries.empty())
    return;

  auto const& selected_id = (selected) ? selected->id() : "";
  owner_->on_entry_show_menu.emit(selected ? selected_id : "dropdown", xid, x, y, 0);

  GVariantBuilder builder;
  g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);
  for (auto const& entry : entries)
    g_variant_builder_add(&builder, "s", entry->id().c_str());

  glib::Variant parameters(g_variant_new("(assuii)", &builder, selected_id.c_str(), xid, x, y));

  show_entry_idle_.reset(new glib::Idle(glib::Source::Priority::DEFAULT));
  show_entry_idle_->Run([this, parameters] {
    gproxy_.Call("ShowEntriesDropdown", parameters);
    return false;
  });
}

void DBusIndicators::Impl::OnShowAppMenu(unsigned int xid, int x, int y)
{
  owner_->on_entry_show_menu.emit("appmenu", xid, x, y, 0);

  // We have to do this because on certain systems X won't have time to
  // respond to our request for XUngrabPointer and this will cause the
  // menu not to show

  show_entry_idle_.reset(new glib::Idle(glib::Source::Priority::DEFAULT));
  show_entry_idle_->Run([this, xid, x, y] {
    gproxy_.Call("ShowAppMenu", g_variant_new("(uii)", xid, x, y));
    return false;
  });
}

void DBusIndicators::Impl::OnEntrySecondaryActivate(std::string const& entry_id)
{
  gproxy_.Call("SecondaryActivateEntry", g_variant_new("(s)", entry_id.c_str()));
}

void DBusIndicators::Impl::OnEntryScroll(std::string const& entry_id, int delta)
{
  gproxy_.Call("ScrollEntry", g_variant_new("(si)", entry_id.c_str(), delta));
}

void DBusIndicators::Impl::CloseActiveEntry()
{
  gproxy_.Call("CloseActiveEntry");
}

void DBusIndicators::Impl::Sync(GVariant* args, glib::Error const& error)
{
  if (!args || error)
    return;

  GVariantIter* iter            = nullptr;
  gchar*        indicator_id    = nullptr;
  gchar*        entry_id        = nullptr;
  gchar*        name_hint       = nullptr;
  guint32       parent_window   = 0;
  gchar*        label           = nullptr;
  gboolean      label_sensitive = false;
  gboolean      label_visible   = false;
  guint32       image_type      = 0;
  gchar*        image_data      = nullptr;
  gboolean      image_sensitive = false;
  gboolean      image_visible   = false;
  gint32        priority        = -1;

  if (!verify_variant_type(args, "(" ENTRY_ARRAY_SIGNATURE ")"))
    return;

  std::unordered_map<Indicator::Ptr, Indicator::Entries> indicators;

  g_variant_get(args, "(" ENTRY_ARRAY_SIGNATURE ")", &iter);
  while (g_variant_iter_loop(iter, ENTRY_SIGNATURE,
                             &indicator_id,
                             &entry_id,
                             &name_hint,
                             &parent_window,
                             &label,
                             &label_sensitive,
                             &label_visible,
                             &image_type,
                             &image_data,
                             &image_sensitive,
                             &image_visible,
                             &priority))
  {
    std::string entry(G_LIKELY(entry_id) ? entry_id : "");
    std::string indicator_name(G_LIKELY(indicator_id) ? indicator_id : "");

    Indicator::Ptr indicator = owner_->GetIndicator(indicator_name);
    if (!indicator)
    {
      indicator = owner_->AddIndicator(indicator_name);
    }

    Indicator::Entries& entries = indicators[indicator];

    // Empty entries are empty indicators.
    if (!entry.empty())
    {
      Entry::Ptr e = indicator->GetEntry(entry_id);
      int old_priority = e ? e->priority() : -1;

      // Indicators can only add or remove entries, so if
      // there is a priority change we can't reuse the existing ones
      if (old_priority != priority)
        e.reset();

      if (!e)
      {
        e = std::make_shared<Entry>(entry, name_hint, parent_window,
                                    label, label_sensitive, label_visible,
                                    image_type, image_data, image_sensitive, image_visible,
                                    priority);
      }
      else
      {
        e->set_label(label, label_sensitive, label_visible);
        e->set_image(image_type, image_data, image_sensitive, image_visible);
        e->set_priority(priority);
      }

      entries.push_back(e);
    }
  }
  g_variant_iter_free(iter);

  for (auto const& i : indicators)
    i.first->Sync(i.second);
}

void DBusIndicators::Impl::SyncGeometries(std::string const& name,
                                          EntryLocationMap const& locations)
{
  if (!gproxy_.IsConnected() || G_UNLIKELY(name.empty()))
    return;

  bool found_changed_locations = false;
  EntryLocationMap& cached_locations = cached_locations_[name];

  GVariantBuilder b;
  g_variant_builder_init(&b, G_VARIANT_TYPE("(sa(siiii))"));
  g_variant_builder_add(&b, "s", name.c_str());

  g_variant_builder_open(&b, G_VARIANT_TYPE("a(siiii)"));

  // Only send to panel service the geometries of items that have changed
  for (auto const& location : locations)
  {
    auto const& id = location.first;
    auto const& rect = location.second;

    if (cached_locations[id] != rect)
    {
      g_variant_builder_add(&b, "(siiii)", id.c_str(), rect.x, rect.y, rect.width, rect.height);
      found_changed_locations = true;
    }
  }

  // Inform panel service of the entries that have been removed sending invalid values
  for (auto const& location : cached_locations)
  {
    auto const& id = location.first;

    if (locations.find(id) == locations.end())
    {
      g_variant_builder_add(&b, "(siiii)", id.c_str(), 0, 0, -1, -1);
      found_changed_locations = true;
    }
  }

  if (!found_changed_locations)
  {
    g_variant_builder_clear(&b);
    return;
  }

  g_variant_builder_close(&b);

  gproxy_.CallBegin("SyncGeometries", g_variant_builder_end(&b), [] (GVariant*, glib::Error const& e) {});
  cached_locations = locations;
}

DBusIndicators::DBusIndicators()
  : pimpl(new Impl(UPS_NAME_DESKTOP, this))
{}

DBusIndicators::DBusIndicators(std::string const& dbus_name)
  : pimpl(new Impl(dbus_name, this))
{}

LockScreenDBusIndicators::LockScreenDBusIndicators()
  : DBusIndicators(UPS_NAME_LOCKSCREEN)
{}

DBusIndicators::~DBusIndicators()
{}

bool DBusIndicators::IsConnected() const
{
  return pimpl->gproxy_.IsConnected();
}

void DBusIndicators::SyncGeometries(std::string const& name,
                                    EntryLocationMap const& locations)
{
  pimpl->SyncGeometries(name, locations);
}

void DBusIndicators::ShowEntriesDropdown(Indicator::Entries const& entries,
                                         Entry::Ptr const& selected,
                                         unsigned xid, int x, int y)
{
  pimpl->ShowEntriesDropdown(entries, selected, xid, x, y);
}

void DBusIndicators::CloseActiveEntry()
{
  pimpl->CloseActiveEntry();
}

void DBusIndicators::OnEntryScroll(std::string const& entry_id, int delta)
{
  pimpl->OnEntryScroll(entry_id, delta);
}

void DBusIndicators::OnEntryShowMenu(std::string const& entry_id,
                                     unsigned int xid, int x, int y,
                                     unsigned int button)
{
  pimpl->OnEntryShowMenu(entry_id, xid, x, y, button);
}

void DBusIndicators::OnEntrySecondaryActivate(std::string const& entry_id)
{
  pimpl->OnEntrySecondaryActivate(entry_id);
}

void DBusIndicators::OnShowAppMenu(unsigned int xid, int x, int y)
{
  pimpl->OnShowAppMenu(xid, x, y);
}

std::vector<std::string> const& DBusIndicators::IconPaths() const
{
  return pimpl->icon_paths_;
}

} // namespace indicator
} // namespace unity
