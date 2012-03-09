// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2010-2011 Canonical Ltd
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
*              Marco Trevisan (Treviño) <mail@3v1n0.net>
*/

#ifndef UNITY_INDICATOR_ENTRY_H
#define UNITY_INDICATOR_ENTRY_H

#include <iosfwd>
#include <map>
#include <string>

#include <sigc++/signal.h>
#include <boost/shared_ptr.hpp>

#include <NuxCore/Rect.h>

namespace unity
{
namespace indicator
{

// The EntryLocationMap is used to map the entry name to the
// physical location on the screen.
typedef std::map<std::string, nux::Rect> EntryLocationMap;

class Entry
{
public:
  typedef boost::shared_ptr<Entry> Ptr;

  Entry(std::string const& id,
        std::string const& name_hint,
        std::string const& label,
        bool label_sensitive,
        bool label_visible,
        int  image_type,
        std::string const& image_data,
        bool image_sensitive,
        bool image_visible,
        int  priority);

  // Assignment emits updated event.
  Entry& operator=(Entry const& rhs);

  std::string const& id() const;
  std::string const& name_hint() const;
  std::string const& label() const;
  int image_type() const;
  std::string const& image_data() const;
  bool image_visible() const;
  bool image_sensitive() const;
  bool label_visible() const;
  bool label_sensitive() const;

  void set_active(bool active);
  bool active() const;

  void set_geometry(nux::Rect const& geometry);
  nux::Rect const& geometry() const;

  int priority() const;

  bool visible() const;

  /**
   * Whether this entry should be shown to the user.
   * Example uses: Application menubar items are only shown when the user
   * movoes its mouse over the panel or holds down the Alt key.
   */
  bool show_now() const;
  void set_show_now(bool show_now);

  void ShowMenu(int x, int y, unsigned int button, unsigned int timestamp);
  void ShowMenu(unsigned int xid, int x, int y, unsigned int button, unsigned int timestamp);
  void SecondaryActivate(unsigned int timestamp);
  void Scroll(int delta);

  void setLabel(std::string const& label, bool sensitive, bool visible);
  void setImage(int type, std::string const& data, bool sensitive, bool visible);
  void setPriority(int priority);

  // Signals
  sigc::signal<void> updated;
  sigc::signal<void, bool> active_changed;
  sigc::signal<void, nux::Rect const&> geometry_changed;
  sigc::signal<void, bool> show_now_changed;

  sigc::signal<void, std::string const&, unsigned int, int, int, unsigned int, unsigned int> on_show_menu;
  sigc::signal<void, std::string const&, unsigned int> on_secondary_activate;
  sigc::signal<void, std::string const&, int> on_scroll;

private:
  std::string id_;
  std::string name_hint_;

  std::string label_;
  bool label_visible_;
  bool label_sensitive_;

  int image_type_;
  std::string image_data_;
  bool image_visible_;
  bool image_sensitive_;
  int priority_;

  bool show_now_;
  bool active_;

  nux::Rect geometry_;
};

std::ostream& operator<<(std::ostream& out, Entry const& e);

}
}

#endif
