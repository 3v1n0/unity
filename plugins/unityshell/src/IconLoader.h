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

#include <string>
#include <map>
#include <vector>
#include <sigc++/sigc++.h>
#include <gtk/gtk.h>

namespace unity
{

class IconLoader : public boost::noncopyable
{
public:
  typedef sigc::slot<void, std::string const&, unsigned, GdkPixbuf*> IconLoaderCallback;

  IconLoader();
  ~IconLoader();

  static IconLoader& GetDefault();

  void LoadFromIconName(const char*        icon_name,
                        guint              size,
                        IconLoaderCallback slot);

  void LoadFromGIconString(const char*        gicon_string,
                           guint              size,
                           IconLoaderCallback slot);

  void LoadFromFilename(const char*        filename,
                        guint              size,
                        IconLoaderCallback slot);

  void LoadFromURI(const char*        uri,
                   guint              size,
                   IconLoaderCallback slot);
private:
  class Impl;
  Impl* pimpl;
};

}

#endif // UNITY_ICON_LOADER_H
