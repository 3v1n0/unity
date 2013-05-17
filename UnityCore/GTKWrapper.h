// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2013 Canonical Ltd
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
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

#ifndef GTK_WRAPPER

#include <gtk/gtk.h>
#include <GLibWrapper.h>

namespace unity
{
namespace gtk
{

#if GTK_CHECK_VERSION(3, 8, 0)

class IconInfo : public glib::Object<GtkIconInfo>
{
public:
  IconInfo(GtkIconInfo *info = nullptr)
    : glib::Object<GtkIconInfo>(info)
  {}
};

#else

class IconInfo : boost::noncopyable
{
public:
  IconInfo(GtkIconInfo *info = nullptr)
    : icon_info_(info)
  {}

  ~IconInfo()
  {
    if (icon_info_)
      gtk_icon_info_free(icon_info_);
  }

  IconInfo& operator=(GtkIconInfo* val)
  {
    if (icon_info_ == val)
      return *this;

    if (icon_info_)
      gtk_icon_info_free(icon_info_);

    icon_info_ = val;
    return *this;
  }

  operator GtkIconInfo*() const { return icon_info_; }
  operator bool() const { return icon_info_; }
  GtkIconInfo* RawPtr() const { return icon_info_; }

private:
  GtkIconInfo *icon_info_;
};
#endif

}
}

#endif // GTK_WRAPPER