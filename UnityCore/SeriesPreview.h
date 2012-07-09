// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012 Canonical Ltd
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
 *              Michal Hruby <michal.hruby@canonical.com>
 */

#ifndef UNITY_SERIES_PREVIEW_H
#define UNITY_SERIES_PREVIEW_H

#include <memory>

#include <sigc++/trackable.h>

#include "Preview.h"

namespace unity
{
namespace dash
{

class SeriesPreview : public Preview
{
public:
  struct SeriesItem
  {
    std::string uri;
    std::string title;
    std::string icon_hint;

    SeriesItem() {};
    SeriesItem(const gchar* uri_, const gchar* title_, const gchar* icon_hint_)
      : uri(uri_ != NULL ? uri_ : "")
      , title(title_ != NULL ? title_ : "")
      , icon_hint(icon_hint_ != NULL ? icon_hint_ : "") {};
  };

  typedef std::shared_ptr<SeriesPreview> Ptr;
  typedef std::shared_ptr<SeriesItem> SeriesItemPtr;
  typedef std::vector<SeriesItemPtr> SeriesItemPtrList;
  
  SeriesPreview(unity::glib::Object<GObject> const& proto_obj);
  ~SeriesPreview();

  nux::RWProperty<int> selected_item_index;

  SeriesItemPtrList GetItems() const;
  Preview::Ptr GetChildPreview() const;

  sigc::signal<void, Preview::Ptr const&> child_preview_changed;

private:
  class Impl;
  std::unique_ptr<Impl> pimpl;
};

}
}

#endif
