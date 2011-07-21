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

#include "IndicatorEntry.h"

#include <iostream>

namespace unity
{
namespace indicator
{

std::string const Entry::UNUSED_ID("|");

Entry::Entry(std::string const& id,
             std::string const& label,
             bool label_sensitive,
             bool label_visible,
             int  image_type,
             std::string const& image_data,
             bool image_sensitive,
             bool image_visible)
  : id_(id)
  , label_(label)
  , label_visible_(label_visible)
  , label_sensitive_(label_sensitive)
  , image_type_(image_type)
  , image_data_(image_data)
  , image_visible_(image_visible)
  , image_sensitive_(image_sensitive)
  , show_now_(false)
  , active_(false)
{
}

std::string const& Entry::id() const
{
  return id_;
}

std::string const& Entry::label() const
{
  return label_;
}

bool Entry::image_visible() const
{
  return image_visible_;
}

bool Entry::image_sensitive() const
{
  return image_sensitive_;
}

bool Entry::label_visible() const
{
  return label_visible_;
}

bool Entry::label_sensitive() const
{
  return label_sensitive_;
}

int Entry::image_type() const
{
  return image_type_;
}

std::string const& Entry::image_data() const
{
  return image_data_;
}

void Entry::set_active(bool active)
{
  if (active_ == active)
    return;

  active_ = active;
  active_changed.emit(active);
  updated.emit();
}

bool Entry::active() const
{
  return active_;
}

Entry& Entry::operator=(Entry const& rhs)
{
  id_ = rhs.id_;
  label_ = rhs.label_;
  label_sensitive_ = rhs.label_sensitive_;
  label_visible_ = rhs.label_visible_;
  image_type_ = rhs.image_type_;
  image_data_ = rhs.image_data_;
  image_sensitive_ = rhs.image_sensitive_;
  image_visible_ = rhs.image_visible_;

  updated.emit();
  return *this;
}

bool Entry::show_now() const
{
  return show_now_;
}

void Entry::set_show_now(bool show_now)
{
  if (show_now_ == show_now)
    return;

  show_now_ = show_now;
  show_now_changed.emit(show_now);
  updated.emit();
}

void Entry::MarkUnused()
{
  id_ = UNUSED_ID;
  label_ = "";
  label_sensitive_ = false;
  label_visible_ = false;
  image_type_ = 0;
  image_data_ = "";
  image_sensitive_ = false;
  image_visible_ = false;
  updated.emit();
}

bool Entry::IsUnused() const
{
  return id_ == UNUSED_ID;
}

void Entry::ShowMenu(int x, int y, int timestamp, int button)
{
  on_show_menu.emit(id_, x, y, timestamp, button);
}

void Entry::SecondaryActivate(unsigned int timestamp)
{
  on_secondary_activate.emit(id_, timestamp);
}

void Entry::Scroll(int delta)
{
  on_scroll.emit(id_, delta);
}

std::ostream& operator<<(std::ostream& out, Entry const& e)
{
  out << "<indicator::Entry " << e.id()
      << std::boolalpha
      << " \"" << e.label() << "\" ("
      << e.label_sensitive() << ", " << e.label_visible() << ") image ("
      << e.image_sensitive() << ", " << e.image_visible() << ") "
      << (e.active() ? "active" : "not-active") << " "
      << (e.show_now() ? "show" : "dont-show") << " >";
  return out;
}


} // namespace indicator
} // namespace unity
