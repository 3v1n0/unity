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


IndicatorEntry::IndicatorEntry ()
  : label_sensitive_(false)
  , label_visible_(false)
  , image_type_(0)
  , image_visible_(false)
  , image_sensitive_(false)
  , show_now_(false)
  , dirty_(false)
  , active_(false)
{
}

std::string const& IndicatorEntry::id() const
{
  return id_;
}

std::string const& IndicatorEntry::label() const;
{
  return label_;
}

bool IndicatorEntry::image_sensitive() const
{
  return image_sensitive_;
}

bool IndicatorEntry::label_sensitive() const
{
  return label_sensitive_;
}

GdkPixbuf* IndicatorEntry::GetPixbuf()
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

void IndicatorEntry::set_active(bool active)
{
  if (active_ == active)
    return;

  active_ = active;
  active_changed.emit(active);
  updated.emit();
}

bool IndicatorEntry::active()
{
  return active_;
}

void IndicatorEntry::Refresh(std::string const& id,
                             std::string const& label,
                             bool label_sensitive,
                             bool label_visible,
                             int  image_type,
                             std::string const& image_data,
                             bool image_sensitive,
                             bool image_visible)
{
  id_ = id;
  label_ = label;
  label_sensitive_ = label_sensitive;
  label_visible_ = label_visible;
  image_type_ = image_type;
  image_data_ = image_data;
  image_sensitive_ = image_sensitive;
  image_visible_ = image_visible;

  updated.emit ();
}

bool IndicatorEntry::show_now() const
{
  return show_now_;
}

void IndicatorEntry::set_show_now(bool show_now)
{
  // TODO: check to see if we need to emit for every setting, or only
  // if the value actually changes.
  show_now_ = show_now;
  show_now_changed.emit(show_now);
  updated.emit();
}

void IndicatorEntry::ShowMenu(int x, int y, int timestamp, int button)
{
  on_show_menu.emit(id_, x, y, timestamp, button);
}

void IndicatorEntry::Scroll(int delta)
{
  on_scroll.emit(id_, delta);
}


} // namespace indicator
} // namespace unity
