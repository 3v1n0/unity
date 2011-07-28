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

#ifndef UNITY_MODEL_INL_H
#define UNITY_MODEL_INL_H

#include <NuxCore/Logger.h>

namespace unity
{
namespace dash
{

namespace
{
nux::logging::Logger _model_inl_logger("unity.dash.model");
}

template<class RowAdaptor>
Model<RowAdaptor>::Model()
{
  swarm_name.changed.connect(sigc::mem_fun(this, &Model<RowAdaptor>::OnSwarmNameChanged));
  count.SetGetterFunction(sigc::mem_fun(this, &Model<RowAdaptor>::get_count));
}

template<class RowAdaptor>
void Model<RowAdaptor>::OnSwarmNameChanged(std::string const& swarm_name)
{
  LOG_DEBUG(_model_inl_logger) << "New swarm name: " << swarm_name;

  // Let the views clean up properly
  if (model_)
    dee_model_clear(model_);

  model_ = dee_shared_model_new(swarm_name.c_str());
  renderer_tag_ = dee_model_register_tag(model_, NULL);

  signal_manager_.Add(
    new glib::Signal<void, DeeModel*, DeeModelIter*>(model_,
                                                     "row-added",
                                                     sigc::mem_fun(this, &Model<RowAdaptor>::OnRowAdded)));

  signal_manager_.Add(
    new glib::Signal<void, DeeModel*, DeeModelIter*>(model_,
                                                     "row-changed",
                                                     sigc::mem_fun(this, &Model<RowAdaptor>::OnRowChanged)));

  signal_manager_.Add(
    new glib::Signal<void, DeeModel*, DeeModelIter*>(model_,
                                                     "row-removed",
                                                     sigc::mem_fun(this, &Model<RowAdaptor>::OnRowRemoved)));
}

template<class RowAdaptor>
Model<RowAdaptor>::~Model()
{}

template<class RowAdaptor>
void Model<RowAdaptor>::OnRowAdded(DeeModel* model, DeeModelIter* iter)
{
  RowAdaptor it(model, iter, renderer_tag_);
  row_added.emit(it);
}

template<class RowAdaptor>
void Model<RowAdaptor>::OnRowChanged(DeeModel* model, DeeModelIter* iter)
{
  RowAdaptor it(model, iter, renderer_tag_);
  row_changed.emit(it);
}

template<class RowAdaptor>
void Model<RowAdaptor>::OnRowRemoved(DeeModel* model, DeeModelIter* iter)
{
  RowAdaptor it(model, iter, renderer_tag_);
  row_removed.emit(it);
}

template<class RowAdaptor>
const RowAdaptor Model<RowAdaptor>::RowAtIndex(std::size_t index)
{
  RowAdaptor it(model_,
                dee_model_get_iter_at_row(model_, index),
                renderer_tag_);
  return it;
}

template<class RowAdaptor>
std::size_t Model<RowAdaptor>::get_count()
{
  if (model_)
    return dee_model_get_n_rows(model_);
  else
    return 0;
}

}
}

#endif
