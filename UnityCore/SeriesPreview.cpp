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
  void selected_item_reply(GVariant* reply);
  int get_selected_item_index() const { return selected_item_index_; };
  bool set_selected_item_index(int index);
  SeriesItemPtrList get_items() const { return items_list_; };
  Preview::Ptr get_child_preview() const
  {
    if (!child_preview_->parent_lens)
      child_preview_->parent_lens = owner_->parent_lens();
    if (child_preview_->preview_uri().empty())
      child_preview_->preview_uri = owner_->preview_uri();
    return child_preview_;
  };

  SeriesPreview* owner_;
  glib::Object<UnityProtocolSeriesPreview> raw_preview_;

  Preview::Ptr child_preview_;
  SeriesItemPtrList items_list_;
  int selected_item_index_;
};

SeriesPreview::Impl::Impl(SeriesPreview* owner,
                          glib::Object<GObject> const& proto_obj)
  : owner_(owner)
  , selected_item_index_(-1)
{
  raw_preview_ = glib::object_cast<UnityProtocolSeriesPreview>(proto_obj);

  int items_len;
  auto items = unity_protocol_series_preview_get_items(raw_preview_,
                                                       &items_len);
  for (int i = 0; i < items_len; i++)
  {
    UnityProtocolSeriesItemRaw* raw_item = &items[i];
    items_list_.push_back(std::make_shared<SeriesItem>(
          raw_item->uri, raw_item->title, raw_item->icon_hint));
  }

  glib::Object<UnityProtocolPreview> child_preview(
      unity_protocol_series_preview_get_child_preview(raw_preview_),
      glib::AddRef());
  child_preview_ = Preview::PreviewForProtocolObject(glib::object_cast<GObject>(child_preview));

  selected_item_index_ =
    unity_protocol_series_preview_get_selected_item(raw_preview_);

  SetupGetters();
}

void SeriesPreview::Impl::SetupGetters()
{
  owner_->selected_item_index.SetGetterFunction(
      sigc::mem_fun(this, &SeriesPreview::Impl::get_selected_item_index));
  owner_->selected_item_index.SetSetterFunction(
      sigc::mem_fun(this, &SeriesPreview::Impl::set_selected_item_index));
}

bool SeriesPreview::Impl::set_selected_item_index(int index)
{
  if (index != selected_item_index_)
  {
    selected_item_index_ = index;
    
    UnityProtocolPreview *preview = UNITY_PROTOCOL_PREVIEW(raw_preview_.RawPtr());
    unity_protocol_preview_begin_updates(preview);
    unity_protocol_series_preview_set_selected_item(raw_preview_, index);
    glib::Variant properties(unity_protocol_preview_end_updates(preview),
                             glib::StealRef());
    owner_->Update(properties, sigc::mem_fun(this, &SeriesPreview::Impl::selected_item_reply));
    return true;
  }

  return false;
}

void SeriesPreview::Impl::selected_item_reply(GVariant *reply)
{
  glib::Variant dict(reply);
  glib::HintsMap hints;
  dict.ASVToHints(hints);

  auto iter = hints.find("preview");
  if (iter != hints.end())
  {
    Preview::Ptr new_child = Preview::PreviewForVariant(iter->second);
    new_child->parent_lens = owner_->parent_lens();
    new_child->preview_uri = owner_->preview_uri(); // FIXME: really?
    child_preview_ = new_child;
    owner_->child_preview_changed.emit(new_child);
  }
}

SeriesPreview::SeriesPreview(unity::glib::Object<GObject> const& proto_obj)
  : Preview(proto_obj)
  , pimpl(new Impl(this, proto_obj))
{
}

SeriesPreview::~SeriesPreview()
{
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
