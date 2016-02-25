// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2016 Canonical Ltd
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
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 */

#include "AppStreamApplication.h"

#include <appstream-glib.h>

#include <iostream>

namespace unity
{
namespace appstream
{

Application::Application(std::string const& appstream_id)
  : appstream_id_(appstream_id)
{
  desktop_file.SetGetterFunction([this](){ return appstream_id_; });
  title.SetGetterFunction([this](){ return title_; });
  icon_pixbuf.SetGetterFunction([this](){ return icon_pixbuf_; });

  glib::Object<AsStore> as_store(as_store_new());
  g_return_if_fail(as_store);

  as_store_load(as_store, AS_STORE_LOAD_FLAG_APP_INFO_SYSTEM, nullptr, nullptr);

  AsApp *as_app = as_store_get_app_by_id(as_store, appstream_id_.c_str());
  g_return_if_fail(as_app);

  title_ = glib::gchar_to_string(as_app_get_name(as_app, nullptr));

  AsIcon *as_icon = as_app_get_icon_default(as_app);
  g_return_if_fail(as_icon);

  as_icon_load(as_icon, AS_ICON_LOAD_FLAG_SEARCH_SIZE, nullptr);
  icon_pixbuf_ = glib::Object<GdkPixbuf>(as_icon_get_pixbuf(as_icon), glib::AddRef());
}

AppType Application::type() const
{
  return AppType::NORMAL;
}

std::string Application::repr() const
{
  std::ostringstream sout;
  sout << "<AppStream::Application " << appstream_id_ << " >";
  return sout.str();
}

WindowList const& Application::GetWindows() const
{
  return window_list_;
}

bool Application::OwnsWindow(Window window_id) const
{
  return false;
}

std::vector<std::string> Application::GetSupportedMimeTypes() const
{
  return std::vector<std::string>();
}

ApplicationWindowPtr Application::GetFocusableWindow() const
{
  return nullptr;
}

void Application::Focus(bool show_on_visible, int monitor) const
{}

void Application::Quit() const
{}

bool Application::CreateLocalDesktopFile() const
{
  return false;
}

std::string Application::desktop_id() const
{
  return "";
}

}
}
