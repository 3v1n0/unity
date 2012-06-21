// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011 Canonical Ltd
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

#include <unity-protocol.h>

#include "SeriesPreview.h"

namespace unity
{
namespace dash
{

class SeriesPreview::Impl
{
public:
  Impl(SeriesPreview* owner, glib::Object<GObject> const& proto_obj);

  void SetupGetters();
  int get_selected_item_index() const { return selected_item_index_; };
  SeriesItemPtrList get_items() const { return items_list_; };
  Preview::Ptr get_child_preview() const { return child_preview_; };

  SeriesPreview* owner_;

  Preview::Ptr child_preview_;
  SeriesItemPtrList items_list_;
  int selected_item_index_;
};

SeriesPreview::Impl::Impl(SeriesPreview* owner,
                          glib::Object<GObject> const& proto_obj)
  : owner_(owner)
  , selected_item_index_(-1)
{
  auto preview = glib::object_cast<UnityProtocolSeriesPreview>(proto_obj);

  int items_len;
  auto items = unity_protocol_series_preview_get_items(preview, &items_len);
  for (int i = 0; i < items_len; i++)
  {
    UnityProtocolSeriesItemRaw* raw_item = &items[i];
    items_list_.push_back(std::make_shared<SeriesItem>(
          raw_item->uri, raw_item->title, raw_item->icon_hint));
  }

  glib::Object<UnityProtocolPreview> child_preview(
      unity_protocol_series_preview_get_child_preview(preview),
      glib::AddRef());
  child_preview_ = Preview::PreviewForProtocolObject(glib::object_cast<GObject>(child_preview));

  selected_item_index_ = unity_protocol_series_preview_get_selected_item(preview);

  SetupGetters();
}

void SeriesPreview::Impl::SetupGetters()
{
  owner_->selected_item_index.SetGetterFunction(
      sigc::mem_fun(this, &SeriesPreview::Impl::get_selected_item_index));
}

SeriesPreview::SeriesPreview(unity::glib::Object<GObject> const& proto_obj)
  : Preview(proto_obj)
  , pimpl(new Impl(this, proto_obj))
{
}

SeriesPreview::~SeriesPreview()
{
  delete pimpl;
}

Preview::Ptr SeriesPreview::GetChildPreview() const
{
  return pimpl->get_child_preview();
}

SeriesPreview::SeriesItemPtrList SeriesPreview::GetItems() const
{
  return pimpl->get_items();
}

}
}
