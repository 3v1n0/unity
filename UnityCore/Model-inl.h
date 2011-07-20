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

namespace unity {
namespace dash {

namespace {
nux::logging::Logger _model_inl_logger("unity.dash.model");
}

template<class ModelIter>
Model<ModelIter>::Model()
{
  swarm_name.changed.connect(sigc::mem_fun(this, &Model<ModelIter>::OnSwarmNameChanged));
  count.SetGetterFunction(sigc::mem_fun(this, &Model<ModelIter>::get_count));
}

template<class ModelIter>
void Model<ModelIter>::OnSwarmNameChanged(std::string const& swarm_name)
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
                                                      sigc::mem_fun(this, &Model<ModelIter>::OnRowAdded)));

  signal_manager_.Add(
    new glib::Signal<void, DeeModel*, DeeModelIter*>(model_,
                                                     "row-changed",
                                                      sigc::mem_fun(this, &Model<ModelIter>::OnRowChanged)));

  signal_manager_.Add(
    new glib::Signal<void, DeeModel*, DeeModelIter*>(model_,
                                                     "row-removed",
                                                      sigc::mem_fun(this, &Model<ModelIter>::OnRowRemoved)));
}

template<class ModelIter>
void Model<ModelIter>::OnRowAdded(DeeModel* model, DeeModelIter* iter)
{
  ModelIter it(model, iter, renderer_tag_);
  row_added.emit(it);
}

template<class ModelIter>
void Model<ModelIter>::OnRowChanged(DeeModel* model, DeeModelIter* iter)
{
  ModelIter it(model, iter, renderer_tag_);
  row_changed.emit(it);
}

template<class ModelIter>
void Model<ModelIter>::OnRowRemoved(DeeModel* model, DeeModelIter* iter)
{
  ModelIter it(model, iter, renderer_tag_);
  row_removed.emit(it);
}

template<class ModelIter>
const ModelIter Model<ModelIter>::IterAtIndex(unsigned int index)
{
  ModelIter it(model_, dee_model_get_iter_at_row(model_, index), renderer_tag_);
  return it;
}

template<class ModelIter>
unsigned int Model<ModelIter>::get_count()
{
  if (model_)
    return dee_model_get_n_rows(model_);
  else
    return 0;
}

}
}

#endif
