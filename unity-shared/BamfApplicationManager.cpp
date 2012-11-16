// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012 Canonical Ltd
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
 * Authored by: Tim Penhey <tim.penhey@canonical.com>
 */

#include "unity-shared/BamfApplicationManager.h"

#include <NuxCore/Logger.h>

#include "unity-shared/WindowManager.h"


DECLARE_LOGGER(logger, "unity.appmanager.bamf");


namespace unity
{
namespace
{
const char* UNSEEN_QUARK = "unity-unseen";
}

// This function is used by the static Default method on the ApplicationManager
// class, and uses link time to make sure there is a function available.
std::shared_ptr<ApplicationManager> create_application_manager()
{
    return std::shared_ptr<ApplicationManager>(new BamfApplicationManager());
}

BamfApplicationWindow::BamfApplicationWindow(BamfView* view)
  : bamf_view_(view, glib::AddRef())
{
}

std::string BamfApplicationWindow::title() const
{
  return glib::String(bamf_view_get_name(bamf_view_)).Str();
}


BamfApplication::BamfApplication(::BamfApplication* app)
  : bamf_app_(app, glib::AddRef())
  , bamf_view_(glib::object_cast<BamfView>(bamf_app_))
{
  // Hook up the property set/get functions
  seen.SetGetterFunction(sigc::mem_fun(this, &BamfApplication::GetSeen));
  seen.SetSetterFunction(sigc::mem_fun(this, &BamfApplication::SetSeen));
  sticky.SetGetterFunction(sigc::mem_fun(this, &BamfApplication::GetSticky));
  sticky.SetSetterFunction(sigc::mem_fun(this, &BamfApplication::SetSticky));
  visible.SetGetterFunction(sigc::mem_fun(this, &BamfApplication::GetVisible));
  active.SetGetterFunction(sigc::mem_fun(this, &BamfApplication::GetActive));
  running.SetGetterFunction(sigc::mem_fun(this, &BamfApplication::GetRunning));
  urgent.SetGetterFunction(sigc::mem_fun(this, &BamfApplication::GetUrgent));

  glib::SignalBase* sig;
  sig = new glib::Signal<void, BamfView*, gboolean>(bamf_view_, "user-visible-changed",
                          [this] (BamfView*, gboolean visible) {
                            this->visible.changed.emit(visible);
                          });
  signals_.Add(sig);
  sig = new glib::Signal<void, BamfView*, gboolean>(bamf_view_, "active-changed",
                          [this] (BamfView*, gboolean active) {
                            this->active.changed.emit(active);
                          });
  signals_.Add(sig);
  sig = new glib::Signal<void, BamfView*, gboolean>(bamf_view_, "running-changed",
                          [this] (BamfView*, gboolean running) {
                            this->running.changed.emit(running);
                          });
  signals_.Add(sig);
  sig = new glib::Signal<void, BamfView*, gboolean>(bamf_view_, "urgent-changed",
                          [this] (BamfView*, gboolean urgent) {
                            this->urgent.changed.emit(urgent);
                          });
  signals_.Add(sig);
  sig = new glib::Signal<void, BamfView*>(bamf_view_, "closed",
                          [this] (BamfView*) {
                            this->closed.emit();
                          });
  signals_.Add(sig);
}

BamfApplication::~BamfApplication()
{
}

std::string BamfApplication::icon() const
{
  glib::String view_icon(bamf_view_get_icon(bamf_view_));
  return view_icon.Str();
}

std::string BamfApplication::title() const
{
  glib::String name(bamf_view_get_name(bamf_view_));
  return name.Str();
}

WindowList BamfApplication::get_windows() const
{
  WindowList result;

  if (!bamf_app_)
    return result;

  WindowManager& wm = WindowManager::Default();
  std::shared_ptr<GList> children(bamf_view_get_children(BAMF_VIEW(bamf_app_.RawPtr())), g_list_free);
  for (GList* l = children.get(); l; l = l->next)
  {
    if (!BAMF_IS_WINDOW(l->data))
      continue;

    auto window = static_cast<BamfWindow*>(l->data);
    auto view = static_cast<BamfView*>(l->data);

    guint32 xid = bamf_window_get_xid(window);

    if (wm.IsWindowMapped(xid))
    {
      result.push_back(ApplicationWindowPtr(new BamfApplicationWindow(view)));
    }
  }
  return result;
}

bool BamfApplication::GetSeen() const
{
  return g_object_get_qdata(G_OBJECT(bamf_app_.RawPtr()),
                            g_quark_from_string(UNSEEN_QUARK));
}

bool BamfApplication::SetSeen(bool const& param)
{
  bool is_seen = GetSeen();
  if (param == is_seen)
    return false; // unchanged

  void* data = param ? reinterpret_cast<void*>(1) : nullptr;
  g_object_set_qdata(G_OBJECT(bamf_app_.RawPtr()),
                     g_quark_from_string(UNSEEN_QUARK),
                     data);
  return true; // value updated

}



bool BamfApplication::GetSticky() const
{
  return bamf_view_is_sticky(bamf_view_);
}

bool BamfApplication::SetSticky(bool const& param)
{
  bool is_sticky = GetSticky();
  if (param == is_sticky)
    return false; // unchanged

  bamf_view_set_sticky(bamf_view_, true);
  return true; // value updated
}

bool BamfApplication::GetVisible() const
{
  return bamf_view_is_user_visible(bamf_view_);
}

bool BamfApplication::GetActive() const
{
  return bamf_view_is_active(bamf_view_);
}

bool BamfApplication::GetRunning() const
{
  return bamf_view_is_running(bamf_view_);
}

bool BamfApplication::GetUrgent() const
{
  return bamf_view_is_urgent(bamf_view_);
}


BamfApplicationManager::BamfApplicationManager()
 : matcher_(bamf_matcher_get_default())
{
  LOG_TRACE(logger) << "Create BamfApplicationManager";
  view_opened_signal_.Connect(matcher_, "view-opened",
                              sigc::mem_fun(this, &BamfApplicationManager::OnViewOpened));
}

BamfApplicationManager::~BamfApplicationManager()
{
}

ApplicationPtr BamfApplicationManager::active_application() const
{
    return ApplicationPtr();
}

ApplicationPtr BamfApplicationManager::GetApplicationForDesktopFile(std::string const& desktop_file) const
{
  ApplicationPtr result;
  ::BamfApplication* app = bamf_matcher_get_application_for_desktop_file(
    matcher_, desktop_file.c_str(), true);

  if (app)
    result.reset(new BamfApplication(app));

  return result;
}


ApplicationList BamfApplicationManager::running_applications() const
{
  ApplicationList result;
  std::shared_ptr<GList> apps(bamf_matcher_get_applications(matcher_), g_list_free);

  for (GList *l = apps.get(); l; l = l->next)
  {
    if (!BAMF_IS_APPLICATION(l->data))
      continue;

    ::BamfApplication* app = static_cast< ::BamfApplication*>(l->data);

    if (g_object_get_qdata(G_OBJECT(app), g_quark_from_static_string("unity-seen")))
      continue;

    result.push_back(ApplicationPtr(new BamfApplication(app)));
  }
  return result;
}


void BamfApplicationManager::OnViewOpened(BamfMatcher* matcher, BamfView* view)
{
  LOG_DEBUG_BLOCK(logger);
  if (!BAMF_IS_APPLICATION(view))
  {
    LOG_DEBUG(logger) << "view is not an app";
    return;
  }

  ::BamfApplication* app = reinterpret_cast< ::BamfApplication*>(view);
  application_started.emit(ApplicationPtr(new BamfApplication(app)));
}

} // namespace unity
