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
// This function is used by the static Default method on the ApplicationManager
// class, and uses link time to make sure there is a function available.
ApplicationManagerPtr create_application_manager()
{
    return ApplicationManagerPtr(new BamfApplicationManager());
}

BamfApplicationWindow::BamfApplicationWindow(BamfView* view)
  : bamf_view_(view, glib::AddRef())
{
}

std::string BamfApplicationWindow::title() const
{
  return glib::String(bamf_view_get_name(bamf_view_)).Str();
}


BamfApplication::BamfApplication(BamfApplication* app)
  : bamf_app_(app, glib::AddRef())
{
}

BamfApplication::~BamfApplication()
{
}

std::string BamfApplication::icon() const
{
    return "TODO";
}

std::string BamfApplication::title() const
{
  glib::String name(bamf_view_get_name(BAMF_VIEW(bamf_app_.RawPtr())));
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




BamfApplicationManager::BamfApplicationManager()
 : matcher_(bamf_matcher_get_default())
{
    LOG_TRACE(logger) << "Create BamfApplicationManager";
}

BamfApplicationManager::~BamfApplicationManager()
{
}

ApplicationPtr BamfApplicationManager::active_application() const
{
    return ApplicationPtr();
}

ApplicationList BamfApplicationManager::running_applications() const
{
  ApplicationList result;
  std::shared_ptr<GList> apps(bamf_matcher_get_applications(matcher_), g_list_free);

  for (GList *l = apps.get(); l; l = l->next)
  {
    if (!BAMF_IS_APPLICATION(l->data))
      continue;

    BamfApplication* app = BAMF_APPLICATION(l->data);

    if (g_object_get_qdata(G_OBJECT(app), g_quark_from_static_string("unity-seen")))
      continue;

    result.push_back(ApplicationPtr(new BamfApplication(app)));
  }
  return result;
}

} // namespace unity
