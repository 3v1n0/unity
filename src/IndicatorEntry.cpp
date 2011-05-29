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

#include <gtk/gtk.h>

namespace unity {
namespace indicator {

std::string const Entry::UNUSED_ID("|");

Entry::Entry()
  : label_visible_(false)
  , label_sensitive_(false)
  , image_type_(0)
  , image_visible_(false)
  , image_sensitive_(false)
  , show_now_(false)
  , dirty_(false)
  , active_(false)
{
}

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
  , dirty_(false)
  , active_(false)
{
}

std::string const& Entry::id() const
{
  return id_;
}

std::string const& Entry::label() const;
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

GdkPixbuf* Entry::GetPixbuf() const
{
  GdkPixbuf* ret = NULL;

  if (image_type_ == GTK_IMAGE_PIXBUF)
  {
    gsize len = 0;
    guchar* decoded = g_base64_decode(image_data_.c_str(), &len);

    GInputStream* stream = g_memory_input_stream_new_from_data(decoded,
                                                               len, NULL);

    ret = gdk_pixbuf_new_from_stream(stream, NULL, NULL);

    g_free(decoded);
    g_input_stream_close(stream, NULL, NULL);
  }
  else if (image_type_ == GTK_IMAGE_STOCK ||
           image_type_ == GTK_IMAGE_ICON_NAME)
  {
    ret = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
                                   image_data_.c_str(),
                                   22,
                                   (GtkIconLookupFlags)0,
                                   NULL);
  }
  else if (image_type_ == GTK_IMAGE_GICON)
  {
    GIcon* icon = g_icon_new_for_string(image_data_.c_str(), NULL);
    GtkIconInfo* info = gtk_icon_theme_lookup_by_gicon(
        gtk_icon_theme_get_default(), icon, 22, (GtkIconLookupFlags)0);
    if (info)
    {
      ret = gtk_icon_info_load_icon(info, NULL);
      gtk_icon_info_free(info);
    }
    g_object_unref (icon);
  }

  return ret;
}

void Entry::set_active(bool active)
{
  if (active_ == active)
    return;

  active_ = active;
  active_changed.emit(active);
  updated.emit();
}

bool Entry::active()
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

  updated.emit ();
}

bool Entry::show_now() const
{
  return show_now_;
}

void Entry::set_show_now(bool show_now)
{
  // TODO: check to see if we need to emit for every setting, or only
  // if the value actually changes.
  show_now_ = show_now;
  show_now_changed.emit(show_now);
  updated.emit();
}

void Entry::MarkUnused()
{
  id_ = "|";;
  label_ = "";
  label_sensitive_ = false;
  label_visible_ = false;
  image_type_ = 0;
  image_data_ = "";
  image_sensitive_ = false;
  image_visible_ = false;
}

void Entry::ShowMenu(int x, int y, int timestamp, int button)
{
  on_show_menu.emit(id_, x, y, timestamp, button);
}

void Entry::Scroll(int delta)
{
  on_scroll.emit(id_, delta);
}


} // namespace indicator
} // namespace unity
