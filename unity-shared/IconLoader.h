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
#include <UnityCore/GLibWrapper.h>

namespace unity
{

class IconLoader : public boost::noncopyable
{
public:
  typedef std::function<void(std::string const&, int, int, glib::Object<GdkPixbuf> const&)> IconLoaderCallback;

  IconLoader();
  ~IconLoader();

  static IconLoader& GetDefault();

  /**
   * Each of the Load functions return an opaque handle.  The sole use for
   * this is to disconnect the callback slot.
   */

  int LoadFromIconName(std::string const& icon_name,
                       int max_width,
                       int max_height,
                       IconLoaderCallback slot);

  int LoadFromGIconString(std::string const& gicon_string,
                          int max_width,
                          int max_height,
                          IconLoaderCallback slot);

  int LoadFromFilename(std::string const& filename,
                       int max_width,
                       int max_height,
                       IconLoaderCallback slot);

  int LoadFromURI(std::string const& uri,
                  int max_width,
                  int max_height,
                  IconLoaderCallback slot);

  void DisconnectHandle(int handle);

private:
  class Impl;
  std::unique_ptr<Impl> pimpl;
};

}

#endif // UNITY_ICON_LOADER_H
