// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2010 Canonical Ltd
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

#ifndef UNITY_INDICATOR_ENTRY_H
#define UNITY_INDICATOR_ENTRY_H

#include <string>
#include <gio/gio.h>

#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

namespace unity {
namespace indicator {

class Entry : boost::noncopyable
{
public:
  typedef boost::shared_ptr<Entry> Ptr;

  Entry();

  std::string const& id() const;
  std::string const& label() const;
  bool image_sensitive() const;
  bool label_sensitive() const;

  // Call g_object_unref on the returned pixbuf
  GdkPixbuf* GetPixbuf();

  void set_active(bool active);
  bool active() const;

  void ShowMenu(int x, int y, int timestamp, int button);
  void Scroll(int delta);

  void Refresh(std::string const& id,
               std::string const& label,
               bool label_sensitive,
               bool label_visible,
               int  image_type,
               std::string const& image_data,
               bool image_sensitive,
               bool image_visible);

  bool show_now() const;
  void set_show_now(bool show_now);

  // Signals
  sigc::signal<void> updated;
  sigc::signal<void, bool> active_changed;
  sigc::signal<void, bool> show_now_changed;

  sigc::signal<void, std::string const&, int, int, int, int> on_show_menu;
  sigc::signal<void, std::string const&, int> on_scroll;

private:
  std::string id_;

  std::string label_;
  bool label_sensitive_;
  bool label_visible_;

  int image_type_;
  std::string image_data_;
  bool image_visible_;
  bool image_sensitive_;

  bool show_now_;
  bool dirty_;
  bool active_;
};

}
}

#endif
