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

#include <boost/utility.hpp>

namespace unity {
namespace indicator {

class Entry : boost::noncopyable
{
public:
  Entry();

  std::string const& id() const;
  std::string const& label() const;

  // Call g_object_unref on the returned pixbuf
  GdkPixbuf* GetPixbuf();

  void set_active(bool active);
  bool is_active() const;

  void ShowMenu(int x, int y, int timestamp, int button);
  void Scroll(int delta);

  void Refresh (const char *__id,
                const char *__label,
                bool        __label_sensitive,
                bool        __label_visible,
                guint32     __image_type,
                const char *__image_data,
                bool        __image_sensitive,
                bool        __image_visible);

  void OnShowNowChanged (bool show_now_state);

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
