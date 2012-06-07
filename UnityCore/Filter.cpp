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

#include "Filter.h"

#include <NuxCore/Logger.h>

#include "CheckOptionFilter.h"
#include "MultiRangeFilter.h"
#include "RadioOptionFilter.h"
#include "RatingsFilter.h"

namespace unity
{
namespace dash
{

namespace
{
nux::logging::Logger logger("unity.dash.filter");
}

using unity::glib::Signal;

Filter::Filter(DeeModel* model, DeeModelIter* iter)
  : model_(model)
  , iter_(iter)
  , ignore_changes_(false)
{
  typedef Signal<void, DeeModel*, DeeModelIter*> RowSignalType;

  // If the model is destroyed (say if the lens restarts) then we should handle
  // that gracefully
  g_object_weak_ref(reinterpret_cast<GObject*>(model_),
                    (GWeakNotify)Filter::OnModelDestroyed, this);

  // Add some filters to handle updates and removed
  signal_manager_.Add(new RowSignalType(model_,
                                        "row-changed",
                                        sigc::mem_fun(this, &Filter::OnRowChanged)));
  signal_manager_.Add(new RowSignalType(model_,
                                        "row-removed",
                                        sigc::mem_fun(this, &Filter::OnRowRemoved)));

  SetupGetters();
}

Filter::~Filter()
{
  if (model_)
    g_object_weak_unref(reinterpret_cast<GObject*>(model_),
                        (GWeakNotify)Filter::OnModelDestroyed, this);
}

void Filter::SetupGetters()
{
  id.SetGetterFunction(sigc::mem_fun(this, &Filter::get_id));
  name.SetGetterFunction(sigc::mem_fun(this, &Filter::get_name));
  icon_hint.SetGetterFunction(sigc::mem_fun(this, &Filter::get_icon_hint));
  renderer_name.SetGetterFunction(sigc::mem_fun(this, &Filter::get_renderer_name));
  visible.SetGetterFunction(sigc::mem_fun(this, &Filter::get_visible));
  collapsed.SetGetterFunction(sigc::mem_fun(this, &Filter::get_collapsed));
  filtering.SetGetterFunction(sigc::mem_fun(this, &Filter::get_filtering));
}

Filter::Ptr Filter::FilterFromIter(DeeModel* model, DeeModelIter* iter)
{
  std::string renderer = dee_model_get_string(model, iter, 3);

  if (renderer == "filter-ratings")
    return std::make_shared<RatingsFilter>(model, iter);
  else if (renderer == "filter-radiooption")
    return std::make_shared<RadioOptionFilter>(model, iter);
  else if (renderer == "filter-checkoption")
    return std::make_shared<CheckOptionFilter>(model, iter);
  else if (renderer == "filter-checkoption-compact")
    return std::make_shared<CheckOptionFilter>(model, iter);
  else if (renderer == "filter-multirange")
    return std::make_shared<MultiRangeFilter>(model, iter);
  else
    return std::make_shared<RatingsFilter>(model, iter);
}

bool Filter::IsValid() const
{
  return model_ && iter_;
}

void Filter::Refresh()
{
  if (model_ && iter_)
    OnRowChanged(model_, iter_);
}

void Filter::IgnoreChanges(bool ignore)
{
  ignore_changes_ = ignore;
}

void Filter::OnModelDestroyed(Filter* self, DeeModel* old_location)
{
  self->OnRowRemoved(self->model_, self->iter_);
}

void Filter::OnRowChanged(DeeModel* model, DeeModelIter* iter)
{
  if (iter_ != iter || ignore_changes_)
    return;

  // Ask our sub-classes to update their state
  Hints hints;
  HintsToMap(hints);
  Update(hints);

  visible.EmitChanged(get_visible());
  filtering.EmitChanged(get_filtering());
}

void Filter::OnRowRemoved(DeeModel* model, DeeModelIter* iter)
{
  if (iter_ != iter)
    return;

  model_ = 0;
  iter_ = 0;
  removed.emit();
}

void Filter::HintsToMap(Hints& map)
{
  GVariant* row_value = dee_model_get_value(model_, iter_, FilterColumn::RENDERER_STATE);

  GVariantIter iter;
  g_variant_iter_init(&iter, row_value);

  char* key = NULL;
  GVariant* value = NULL;
  while (g_variant_iter_loop(&iter, "{sv}", &key, &value))
  {
    map[key] = value;
  }
  g_variant_unref(row_value);
}

std::string Filter::get_id() const
{
  if (IsValid())
    return dee_model_get_string(model_, iter_, FilterColumn::ID);
  return "";
}

std::string Filter::get_name() const
{
  if (IsValid())
    return dee_model_get_string(model_, iter_, FilterColumn::NAME);
  return "";
}

std::string Filter::get_icon_hint() const
{
  if (IsValid())
    return dee_model_get_string(model_, iter_, FilterColumn::ICON_HINT);
  return "";
}

std::string Filter::get_renderer_name() const
{
  if (IsValid())
    return dee_model_get_string(model_, iter_, FilterColumn::RENDERER_NAME);
  return "";
}

bool Filter::get_visible() const
{
  if (IsValid())
    return dee_model_get_bool(model_, iter_, FilterColumn::VISIBLE);
  return false;
}

bool Filter::get_collapsed() const
{
  if (IsValid())
    return dee_model_get_bool(model_, iter_, FilterColumn::COLLAPSED);
  return true;
}

bool Filter::get_filtering() const
{
  if (IsValid())
    return dee_model_get_bool(model_, iter_, FilterColumn::FILTERING);
  return false;
}

}
}
