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
  : model_type_(ModelType::REMOTE)
  , cached_adaptor1_(nullptr, nullptr, nullptr)
  , cached_adaptor2_(nullptr, nullptr, nullptr)
  , cached_adaptor3_(nullptr, nullptr, nullptr)
{
  Init();
}

template<class RowAdaptor>
Model<RowAdaptor>::Model (ModelType model_type)
  : model_type_(model_type)
  , cached_adaptor1_(nullptr, nullptr, nullptr)
  , cached_adaptor2_(nullptr, nullptr, nullptr)
  , cached_adaptor3_(nullptr, nullptr, nullptr)
{
  Init();

  if (model_type == ModelType::LOCAL)
    swarm_name = ":local";
}

template<class RowAdaptor>
void Model<RowAdaptor>::Init ()
{
  swarm_name.changed.connect(sigc::mem_fun(this, &Model<RowAdaptor>::OnSwarmNameChanged));
  count.SetGetterFunction(sigc::mem_fun(this, &Model<RowAdaptor>::get_count));
  seqnum.SetGetterFunction(sigc::mem_fun(this, &Model<RowAdaptor>::get_seqnum));
  model.SetGetterFunction(sigc::mem_fun(this, &Model<RowAdaptor>::get_model));
}

template<class RowAdaptor>
void Model<RowAdaptor>::OnSwarmNameChanged(std::string const& swarm_name)
{
  typedef glib::Signal<void, DeeModel*, DeeModelIter*> RowSignalType;
  typedef glib::Signal<void, DeeModel*, guint64, guint64> TransactionSignalType;

  LOG_DEBUG(_model_inl_logger) << "New swarm name: " << swarm_name;

  // Let the views clean up properly
  if (model_)
  {
    dee_model_clear(model_);
    sig_manager_.Disconnect(model_);
  }

  switch(model_type_)
  {
    case ModelType::LOCAL:
      model_ = dee_sequence_model_new();
      break;
    case ModelType::REMOTE:
      model_ = dee_shared_model_new(swarm_name.c_str());
      sig_manager_.Add(new TransactionSignalType(model_,
                                                 "begin-transaction",
                                                 sigc::mem_fun(this, &Model<RowAdaptor>::OnTransactionBegin)));

      sig_manager_.Add(new TransactionSignalType(model_,
                                                 "end-transaction",
                                                 sigc::mem_fun(this, &Model<RowAdaptor>::OnTransactionEnd)));
      break;
    default:
      LOG_ERROR(_model_inl_logger) <<  "Unexpected ModelType " << model_type_;
      break;
  }

  model.EmitChanged(model_);


  renderer_tag_ = dee_model_register_tag(model_, NULL);

  sig_manager_.Add(new RowSignalType(model_,
                                     "row-added",
                                     sigc::mem_fun(this, &Model<RowAdaptor>::OnRowAdded)));

  sig_manager_.Add(new RowSignalType(model_,
                                     "row-changed",
                                     sigc::mem_fun(this, &Model<RowAdaptor>::OnRowChanged)));

  sig_manager_.Add(new RowSignalType(model_,
                                     "row-removed",
                                     sigc::mem_fun(this, &Model<RowAdaptor>::OnRowRemoved)));
}

template<class RowAdaptor>
Model<RowAdaptor>::~Model()
{}

template<class RowAdaptor>
void Model<RowAdaptor>::OnRowAdded(DeeModel* model, DeeModelIter* iter)
{
  // careful here - adding rows to the model inside the callback
  // will invalidate the cached_adaptor!
  // This needs to be used as a listener only!
  cached_adaptor1_.SetTarget(model, iter, renderer_tag_);
  row_added.emit(cached_adaptor1_);
}

template<class RowAdaptor>
void Model<RowAdaptor>::OnRowChanged(DeeModel* model, DeeModelIter* iter)
{
  // careful here - changing rows inside the callback will invalidate
  // the cached_adaptor!
  // This needs to be used as a listener only!
  cached_adaptor2_.SetTarget(model, iter, renderer_tag_);
  row_changed.emit(cached_adaptor2_);
}

template<class RowAdaptor>
void Model<RowAdaptor>::OnRowRemoved(DeeModel* model, DeeModelIter* iter)
{
  // careful here - removing rows from the model inside the callback
  // will invalidate the cached_adaptor!
  // This needs to be used as a listener only!
  cached_adaptor3_.SetTarget(model, iter, renderer_tag_);
  row_removed.emit(cached_adaptor3_);
}

template<class RowAdaptor>
void Model<RowAdaptor>::OnTransactionBegin(DeeModel* model, guint64 begin_seqnum64, guint64 end_seqnum64)
{
  unsigned long long begin_seqnum, end_seqnum;

  begin_seqnum = static_cast<unsigned long long> (begin_seqnum64);
  end_seqnum = static_cast<unsigned long long> (end_seqnum64);
  begin_transaction.emit(begin_seqnum, end_seqnum);
}

template<class RowAdaptor>
void Model<RowAdaptor>::OnTransactionEnd(DeeModel* model, guint64 begin_seqnum64, guint64 end_seqnum64)
{
  unsigned long long begin_seqnum, end_seqnum;

  begin_seqnum = static_cast<unsigned long long> (begin_seqnum64);
  end_seqnum = static_cast<unsigned long long> (end_seqnum64);
  end_transaction.emit(begin_seqnum, end_seqnum);
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
DeeModelTag* Model<RowAdaptor>::GetTag()
{
  return renderer_tag_;
}

template<class RowAdaptor>
std::size_t Model<RowAdaptor>::get_count()
{
  if (model_)
    return dee_model_get_n_rows(model_);
  else
    return 0;
}

template<class RowAdaptor>
unsigned long long Model<RowAdaptor>::get_seqnum()
{
  if (model_ && DEE_IS_SERIALIZABLE_MODEL ((DeeModel*) model_))
    return dee_serializable_model_get_seqnum(model_);
  else
    return 0;
}

template<class RowAdaptor>
glib::Object<DeeModel> Model<RowAdaptor>::get_model()
{
  return model_;
}

}
}

#endif
