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
*              Marco Trevisan (Treviño) <mail@3v1n0.net>
*/

#include "IndicatorEntry.h"

#include <iostream>
#include <algorithm>

namespace unity
{
namespace indicator
{

Entry::Entry(std::string const& id,
             std::string const& name_hint,
             std::string const& label,
             bool label_sensitive,
             bool label_visible,
             int  image_type,
             std::string const& image_data,
             bool image_sensitive,
             bool image_visible,
             int priority)
  : id_(id)
  , name_hint_(name_hint)
  , label_(label)
  , label_visible_(label_visible)
  , label_sensitive_(label_sensitive)
  , image_type_(image_type)
  , image_data_(image_data)
  , image_visible_(image_visible)
  , image_sensitive_(image_sensitive)
  , priority_(priority)
  , show_now_(false)
  , active_(false)
{}

Entry::Entry(std::string const& id, std::string const& name_hint)
  : id_(id)
  , name_hint_(name_hint)
  , label_visible_(false)
  , label_sensitive_(false)
  , image_type_(0)
  , image_visible_(false)
  , image_sensitive_(false)
  , priority_(-1)
  , show_now_(false)
  , active_(false)
{}

std::string const& Entry::name_hint() const
{
  return name_hint_;
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

int Entry::priority() const
{
  return priority_;
}

bool Entry::visible() const
{
  return ((label_visible_ && !label_.empty()) ||
          (image_type_ != 0 && image_visible_ && !image_data_.empty()));
}

void Entry::set_active(bool active)
{
  if (active_ == active)
    return;

  active_ = active;
  active_changed.emit(active);
  updated.emit();

  for (auto const& parent : parents_)
    parent->set_active(active_);
}

void Entry::set_geometry(nux::Rect const& geometry)
{
  if (geometry_ == geometry)
    return;

  geometry_ = geometry;
  geometry_changed.emit(geometry);
  updated.emit();

  for (auto const& parent : parents_)
    parent->set_geometry(geometry_);
}

void Entry::set_label(std::string const& label, bool sensitive, bool visible)
{
  if (label_ == label && sensitive == label_sensitive_ && visible == label_visible_)
    return;

  label_ = label;
  label_sensitive_ = sensitive;
  label_visible_ = visible;
  updated.emit();
}

void Entry::set_image(int type, std::string const& data, bool sensitive, bool visible)
{
  if (image_type_ == type && image_data_ == data &&
      image_sensitive_ == sensitive && image_visible_ == visible)
  {
    return;
  }

  image_type_ = type;
  image_data_ = data;
  image_sensitive_ = sensitive;
  image_visible_ = visible;
  updated.emit();
}

void Entry::set_priority(int priority)
{
  if (priority_ == priority)
    return;

  priority_ = priority;
  updated.emit();
}

bool Entry::active() const
{
  return active_;
}

nux::Rect const& Entry::geometry() const
{
  return geometry_;
}

Entry& Entry::operator=(Entry const& rhs)
{
  if (&rhs == this)
    return *this;

  id_ = rhs.id_;
  name_hint_ = rhs.name_hint_;
  label_ = rhs.label_;
  label_sensitive_ = rhs.label_sensitive_;
  label_visible_ = rhs.label_visible_;
  image_type_ = rhs.image_type_;
  image_data_ = rhs.image_data_;
  image_sensitive_ = rhs.image_sensitive_;
  image_visible_ = rhs.image_visible_;
  priority_ = rhs.priority_;
  parents_ = rhs.parents_;

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

  for (auto const& parent : parents_)
    parent->set_show_now(show_now_);
}

void Entry::add_parent(Entry::Ptr const& parent)
{
  if (!parent || std::find(parents_.begin(), parents_.end(), parent) != parents_.end())
    return;

  bool was_empty = parents_.empty();
  parents_.push_back(parent);
  parent->set_geometry(geometry_);
  parent->set_show_now(was_empty ? show_now_ : (parent->show_now() || show_now_));
  parent->set_active(was_empty ? active_ : (parent->active() || active_));
}

void Entry::rm_parent(Entry::Ptr const& parent)
{
  auto it = std::find(parents_.begin(), parents_.end(), parent);

  if (it != parents_.end())
    parents_.erase(it);
}

std::vector<Entry::Ptr> const& Entry::parents() const
{
  return parents_;
}

void Entry::ShowMenu(int x, int y, unsigned button)
{
  ShowMenu(0, x, y, button);
}

void Entry::ShowMenu(unsigned int xid, int x, int y, unsigned button)
{
  on_show_menu.emit(id_, xid, x, y, button);
}

void Entry::SecondaryActivate()
{
  on_secondary_activate.emit(id_);
}

void Entry::Scroll(int delta)
{
  on_scroll.emit(id_, delta);
}

std::ostream& operator<<(std::ostream& out, Entry const& e)
{
  out << "<indicator::Entry " << e.id() << " hint: '" << e.name_hint() << "' "
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
