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

#include "CheckOptionFilter.h"

#include <NuxCore/Logger.h>

namespace unity
{
namespace dash
{
DECLARE_LOGGER(logger, "unity.dash.filter.checkoption");

CheckOptionFilter::CheckOptionFilter(DeeModel* model, DeeModelIter* iter)
: Filter(model, iter)
, show_all_button_(true)
{
  options.SetGetterFunction(sigc::mem_fun(this, &CheckOptionFilter::get_options));
  show_all_button.SetGetterFunction(sigc::mem_fun(this, &CheckOptionFilter::get_show_all_button));
  Refresh();
}

void CheckOptionFilter::Clear()
{
  for(auto option: options_)
    option->active = false;
}

void CheckOptionFilter::Update(Filter::Hints& hints)
{  
  GVariant* show_all_button_variant = hints["show-all-button"];
  if (show_all_button_variant)
  {
    bool tmp_show = show_all_button_;
    g_variant_get(show_all_button_variant, "b", &show_all_button_);
    if (tmp_show != show_all_button_)
      show_all_button.EmitChanged(show_all_button_);
  }

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

    std::string data(id);
    option->active.changed.connect(sigc::bind(sigc::mem_fun(this, &CheckOptionFilter::OptionChanged), data));
    options_.push_back(option);
    option_added.emit(option);
  }

  g_variant_iter_free(options_iter);
}

void CheckOptionFilter::OptionChanged(bool is_active, std::string const& id)
{
  UpdateState();
}

CheckOptionFilter::CheckOptions const& CheckOptionFilter::get_options() const
{
  return options_;
}

bool CheckOptionFilter::get_show_all_button() const
{
  return show_all_button_;
}

void CheckOptionFilter::UpdateState()
{
  if (!IsValid())
    return;
  gboolean raw_filtering = FALSE;

  GVariantBuilder options;
  g_variant_builder_init(&options, G_VARIANT_TYPE("a(sssb)"));

  for(auto option: options_)
  {
    std::string id = option->id;
    std::string name = option->name;
    std::string icon_hint = option->icon_hint;
    bool active = option->active;

    raw_filtering = raw_filtering ? TRUE : active;

    g_variant_builder_add(&options, "(sssb)",
                          id.c_str(), name.c_str(),
                          icon_hint.c_str(), active ? TRUE : FALSE);
  }
  
  GVariantBuilder hints;
  g_variant_builder_init(&hints, G_VARIANT_TYPE("a{sv}"));
  g_variant_builder_add(&hints, "{sv}", "options", g_variant_builder_end(&options));

  IgnoreChanges(true);
  dee_model_set_value(model_,iter_,
                      FilterColumn::RENDERER_STATE,
                      g_variant_builder_end(&hints));
  dee_model_set_value(model_, iter_,
                      FilterColumn::FILTERING,
                      g_variant_new("b", raw_filtering));
  IgnoreChanges(false);

  filtering.EmitChanged(filtering);
}


}
}
