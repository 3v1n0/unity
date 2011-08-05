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

#include "MultiRangeFilter.h"

#include <NuxCore/Logger.h>

namespace unity
{
namespace dash
{

namespace
{
nux::logging::Logger logger("unity.dash.multirangefilter");
}

MultiRangeFilter::MultiRangeFilter(DeeModel* model, DeeModelIter* iter)
  : Filter(model, iter)
  , left_pos_(-1)
  , right_pos_(-1)
{
  options.SetGetterFunction(sigc::mem_fun(this, &MultiRangeFilter::get_options));
  Refresh();
}

void MultiRangeFilter::Clear()
{
  left_pos_ = -1;
  right_pos_ = -1;

  for(auto option: options_)
  {
    option->active = false;
  }
  UpdateState(false);

  options.EmitChanged(options_);
}

void MultiRangeFilter::Toggle(std::string const& id)
{
  int position = PositionOfId(id);

  if (left_pos_ == -1 && right_pos_ == -1)
  {
    left_pos_ = position;
    right_pos_ = position;
  }
  else if (left_pos_ > position)
  {
    left_pos_ = position;
  }
  else if (right_pos_ < position)
  {
    right_pos_ = position;
  }

  int i = 0;
  for(auto option: options_)
  {
    if (i < left_pos_)
      option->active = false;
    else if (i <= right_pos_)
      option->active = true;
    else
      option->active = false;

    i++;
  }
  UpdateState(true);

  options.EmitChanged(options_);
}

void MultiRangeFilter::Update(Filter::Hints& hints)
{
  GVariant* options_variant = hints["options"];
  GVariantIter* options_iter;

  g_variant_get(options_variant, "a(sssb)", &options_iter);

  char *id = NULL;
  char *name = NULL;
  char *icon_hint = NULL;
  gboolean active = false;

  for (auto option: options_)
    option_removed.emit(option);
  options_.clear();

  while (g_variant_iter_loop(options_iter, "(sssb)", &id, &name, &icon_hint, &active))
  {
    FilterOption::Ptr option(new FilterOption(id, name, icon_hint, active));
    options_.push_back(option);
    option_added.emit(option);
  }

  options.EmitChanged(options_);
}

MultiRangeFilter::Options const& MultiRangeFilter::get_options() const
{
  return options_;
}

void MultiRangeFilter::UpdateState(bool raw_filtering)
{
  if (!IsValid())
    return;

  GVariantBuilder options;
  g_variant_builder_init(&options, G_VARIANT_TYPE("a(sssb)"));

  for(auto option: options_)
  {
    std::string id = option->id;
    std::string name = option->name;
    std::string icon_hint = option->icon_hint;
    bool active = option->active;

    g_variant_builder_add(&options, "sssb",
                          id.c_str(), name.c_str(),
                          icon_hint.c_str(), active ? TRUE : FALSE);
  }
  
  GVariantBuilder hints;
  g_variant_builder_init(&hints, G_VARIANT_TYPE("a{sv}"));
  g_variant_builder_add(&hints, "{sv}", "options", g_variant_builder_end(&options));

  dee_model_set_value(model_,iter_,
                      FilterColumn::RENDERER_STATE,
                      g_variant_builder_end(&hints));
  dee_model_set_value(model_, iter_,
                      FilterColumn::FILTERING,
                      g_variant_new("b", raw_filtering ? TRUE : FALSE));
}

int MultiRangeFilter::PositionOfId(std::string const& id)
{
  int i = 0;
  for (auto option: options_)
  {
    if (option->id == id)
      return i;
    i++;
  }
  return -1;
}


}
}
