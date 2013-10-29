// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2010, 2011 Canonical Ltd
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
*/

#ifndef UNITY_ICON_LOADER_H
#define UNITY_ICON_LOADER_H

#include <boost/utility.hpp>

#include <memory>
#include <gtk/gtk.h>
#include <UnityCore/ActionHandle.h>
#include <UnityCore/GLibWrapper.h>

namespace unity
{

class IconLoader : public boost::noncopyable
{
public:
  typedef action::handle Handle;
  typedef std::function<void(std::string const&, int, int, glib::Object<GdkPixbuf> const&)> IconLoaderCallback;

  IconLoader();
  ~IconLoader();

  static IconLoader& GetDefault();

  /**
   * Each of the Load functions return an opaque handle.  The sole use for
   * this is to disconnect the callback slot.
   */

  Handle LoadFromIconName(std::string const&, int max_width, int max_height, IconLoaderCallback const& slot);
  Handle LoadFromGIconString(std::string const&, int max_width, int max_height, IconLoaderCallback const& slot);
  Handle LoadFromFilename(std::string const&, int max_width, int max_height, IconLoaderCallback const& slot);
  Handle LoadFromURI(std::string const&, int max_width, int max_height, IconLoaderCallback const& slot);

  void DisconnectHandle(Handle handle);

private:
  class Impl;
  std::unique_ptr<Impl> pimpl;
};

}

#endif // UNITY_ICON_LOADER_H
