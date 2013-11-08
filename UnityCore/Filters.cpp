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

#include "Filters.h"

namespace unity
{
namespace dash
{

FilterAdaptor::FilterAdaptor(DeeModel* model,
                             DeeModelIter* iter,
                             DeeModelTag* renderer_tag)
  : RowAdaptorBase(model, iter, renderer_tag)
{
  renderer_name.SetGetterFunction(sigc::bind(sigc::mem_fun(this, &RowAdaptorBase::GetStringAt), 3));
}

FilterAdaptor::FilterAdaptor(FilterAdaptor const& other)
  : RowAdaptorBase(other.model_, other.iter_, other.tag_)
{
  renderer_name.SetGetterFunction(sigc::bind(sigc::mem_fun(this, &RowAdaptorBase::GetStringAt), 3));
}

std::string FilterAdaptor::get_id() const
{
  if (model_ && iter_)
    return dee_model_get_string(model_, iter_, FilterColumn::ID);
  return "";
}

std::string FilterAdaptor::get_name() const
{
  if (model_ && iter_)
    return dee_model_get_string(model_, iter_, FilterColumn::NAME);
  return "";
}

std::string FilterAdaptor::get_icon_hint() const
{
  if (model_ && iter_)
    return dee_model_get_string(model_, iter_, FilterColumn::ICON_HINT);
  return "";
}

std::string FilterAdaptor::get_renderer_name() const
{
  if (model_ && iter_)
    return dee_model_get_string(model_, iter_, FilterColumn::RENDERER_NAME);
  return "";
}

bool FilterAdaptor::get_visible() const
{
  if (model_ && iter_)
    return dee_model_get_bool(model_, iter_, FilterColumn::VISIBLE);
  return false;
}

bool FilterAdaptor::get_collapsed() const
{
  if (model_ && iter_)
    return dee_model_get_bool(model_, iter_, FilterColumn::COLLAPSED);
  return true;
}

bool FilterAdaptor::get_filtering() const
{
  if (model_ && iter_)
    return dee_model_get_bool(model_, iter_, FilterColumn::FILTERING);
  return false;
}

Filter::Ptr FilterAdaptor::create_filter() const
{
  return Filter::FilterFromIter(model_, iter_);
}

void FilterAdaptor::MergeState(glib::HintsMap const& hints)
{
  glib::HintsMap current_hints;
  glib::Variant row_state_value(dee_model_get_value(model_, iter_, FilterColumn::RENDERER_STATE), glib::StealRef());
  row_state_value.ASVToHints(current_hints);

  for (auto iter = hints.begin(); iter != hints.end(); ++iter)
  {
    current_hints[iter->first] = iter->second;
  }

  dee_model_set_value(model_, iter_, FilterColumn::RENDERER_STATE, glib::Variant(current_hints));
}


Filters::Filters()
: Filters(ModelType::REMOTE)
{}

Filters::Filters(ModelType model_type)
: Model<FilterAdaptor>::Model(model_type)
{
  row_added.connect(sigc::mem_fun(this, &Filters::OnRowAdded));
  row_changed.connect(sigc::mem_fun(this, &Filters::OnRowChanged));
  row_removed.connect(sigc::mem_fun(this, &Filters::OnRowRemoved));
}

Filters::~Filters()
{}

Filter::Ptr Filters::FilterAtIndex(std::size_t index)
{
  FilterAdaptor adaptor = RowAtIndex(index);
  if (filter_map_.find(adaptor.get_id()) == filter_map_.end())
  {
    OnRowAdded(adaptor);
  }
  return filter_map_[adaptor.get_id()];
}

bool Filters::ApplyStateChanges(glib::Variant const& filter_rows)
{
  if (!filter_rows)
    return false;

  if (!g_variant_is_of_type (filter_rows, G_VARIANT_TYPE ("a(ssssa{sv}bbb)")))
  {
    return false;
  }

  gchar* id = nullptr;
  gchar* name = nullptr;
  gchar* icon_hint = nullptr;
  gchar* renderer_name = nullptr;
  GVariant* hints = nullptr;
  gboolean visible;
  gboolean collapsed;
  gboolean filtering;

  GVariantIter iter;
  g_variant_iter_init(&iter, filter_rows);
  while (g_variant_iter_loop(&iter, "(ssss@a{sv}bbb)", &id,
                                                       &name,
                                                       &icon_hint,
                                                       &renderer_name,
                                                       &hints,
                                                       &visible,
                                                       &collapsed,
                                                       &filtering))
  {
    for (FilterAdaptorIterator it(begin()); !it.IsLast(); ++it)
    {
      FilterAdaptor filter_adaptor = *it;

      if (id && filter_adaptor.get_id().compare(id) == 0)
      {
        glib::HintsMap hints_map;
        if (glib::Variant(hints).ASVToHints(hints_map))
        {
          filter_adaptor.MergeState(hints_map);
        }
      }
    }
  }

  return true;
}

FilterAdaptorIterator Filters::begin() const
{
  return FilterAdaptorIterator(model(), dee_model_get_first_iter(model()), GetTag());

}

FilterAdaptorIterator Filters::end() const
{
  return FilterAdaptorIterator(model(), dee_model_get_last_iter(model()), GetTag());
}

void Filters::OnRowAdded(FilterAdaptor& filter)
{
  Filter::Ptr ret = filter.create_filter();

  filter_map_[filter.get_id()] = ret;
  filter_added(ret);
}

void Filters::OnRowChanged(FilterAdaptor& filter)
{
  if (filter_map_.find(filter.get_id()) == filter_map_.end())
  {
    filter_changed(filter.create_filter());
    return;
  }
  filter_changed(filter_map_[filter.get_id()]);
}

void Filters::OnRowRemoved(FilterAdaptor& filter)
{
  if (filter_map_.find(filter.get_id()) == filter_map_.end())
  {
    filter_removed(filter.create_filter());
    return;
  }
  filter_removed(filter_map_[filter.get_id()]);
  filter_map_.erase(filter.get_id());
}


}
}
