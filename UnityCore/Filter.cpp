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

Filter::Filter()
  : id("")
  , name("")
  , icon_hint("")
  , renderer_name("")
  , visible(false)
  , collapsed(false)
  , filtering(false)
  , model_(0)
  , iter_(0)
{
}

Filter::~Filter()
{
  if (model_)
    g_object_weak_unref(reinterpret_cast<GObject*>(model_),
                        (GWeakNotify)Filter::OnModelDestroyed, this);
}

Filter::Ptr Filter::FilterFromIter(DeeModel* model, DeeModelIter* iter)
{
  std::string renderer = dee_model_get_string(model, iter, 3);

  if (renderer == "ratings")
    return Filter::Ptr(new RatingsFilter(model, iter));

  return Filter::Ptr(new RatingsFilter(model, iter));
}

void Filter::Connect()
{
  if (model_ && iter_)
  {
    // If the model is destroyed (say if the lens restarts) then we should handle
    // that gracefully
    g_object_weak_ref(reinterpret_cast<GObject*>(model_),
                      (GWeakNotify)Filter::OnModelDestroyed, this);

    // Add some filters to handle updates and removed
    signal_manager_.Add(
      new Signal<void, DeeModel*, DeeModelIter*>(model_,
                                                 "row-changed",
                                                 sigc::mem_fun(this, &Filter::OnRowChanged)));
    signal_manager_.Add(
      new Signal<void, DeeModel*, DeeModelIter*>(model_,
                                                 "row-removed",
                                                 sigc::mem_fun(this, &Filter::OnRowRemoved)));

    // Now we have a valid model_ and iter_, let's update the properties
    OnRowChanged(model_, iter_);
  }
  else
  {
    LOG_WARNING(logger) << "Cannot connect if model_ or iter_ are invalid";
  }
}

void Filter::OnModelDestroyed(Filter* self, DeeModel* old_location)
{
  self->OnRowRemoved(self->model_, self->iter_);
}

void Filter::OnRowChanged(DeeModel* model, DeeModelIter* iter)
{
  if (iter_ != iter)
    return;

  UpdateProperties();

  // Ask our sub-classes to update their state
  Hints hints;
  HintsToMap(hints);
  Update(hints);
}

void Filter::OnRowRemoved(DeeModel* model, DeeModelIter* iter)
{
  if (iter_ != iter)
    return;

  model_ = 0;
  iter_ = 0;
  removed.emit();
}

void Filter::UpdateProperties()
{
  id = dee_model_get_string(model_, iter_, FilterColumn::ID);
  name = dee_model_get_string(model_, iter_, FilterColumn::NAME);
  icon_hint = dee_model_get_string(model_, iter_, FilterColumn::ICON_HINT);
  renderer_name = dee_model_get_string(model_, iter_, FilterColumn::RENDERER_NAME);
  visible = dee_model_get_bool(model_, iter_, FilterColumn::VISIBLE) != FALSE;
  collapsed = dee_model_get_bool(model_, iter_, FilterColumn::COLLAPSED) != FALSE;
  filtering = dee_model_get_bool(model_, iter_, FilterColumn::FILTERING) != FALSE;
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

}
}
