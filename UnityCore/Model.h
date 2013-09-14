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

#ifndef UNITY_MODEL_H
#define UNITY_MODEL_H

#include <boost/noncopyable.hpp>
#include <dee.h>
#include <memory>
#include <NuxCore/Property.h>
#include <sigc++/trackable.h>

#include "GLibSignal.h"
#include "GLibWrapper.h"
#include "ModelRowAdaptor.h"

namespace unity
{
namespace dash
{

enum ModelType
{
  REMOTE,
  REMOTE_SHARED,
  LOCAL,
  UNATTACHED
};

/* This template class encapsulates the basics of talking to a DeeSharedModel,
 * however it is a template as you can choose your own RowAdaptor (see
 * ResultsRowAdaptor.h for an example) which then presents the data in the rows
 * in a instance-specific way.
 */
template<class RowAdaptor>
class Model : public sigc::trackable, boost::noncopyable
{
public:
  typedef std::shared_ptr<Model> Ptr;

  Model (ModelType model_type = ModelType::REMOTE_SHARED);
  virtual ~Model();

  virtual const RowAdaptor RowAtIndex(std::size_t index) const;
  DeeModelTag* GetTag() const;

  nux::Property<std::string> swarm_name;

  nux::ROProperty<std::size_t> count;
  nux::ROProperty<uint64_t> seqnum;
  nux::ROProperty<glib::Object<DeeModel>> model;

  sigc::signal<void, RowAdaptor&> row_added;
  sigc::signal<void, RowAdaptor&> row_changed;
  sigc::signal<void, RowAdaptor&> row_removed;

  sigc::signal<void, uint64_t, uint64_t> begin_transaction;
  sigc::signal<void, uint64_t, uint64_t> end_transaction;

  typedef std::function<DeeModelTag*(glib::Object<DeeModel> const& model)> GetDeeTagFunc;

  void SetModel(glib::Object<DeeModel> const& model);
  void SetModel(glib::Object<DeeModel> const& model, GetDeeTagFunc const& func);

private:
  void Init();
  void OnRowAdded(DeeModel* model, DeeModelIter* iter);
  void OnRowChanged(DeeModel* model, DeeModelIter* iter);
  void OnRowRemoved(DeeModel* model, DeeModelIter* iter);
  void OnTransactionBegin(DeeModel* model, guint64 begin_seq, guint64 end_seq);
  void OnTransactionEnd(DeeModel* model, guint64 begin_seq, guint64 end_seq);
  void OnSwarmNameChanged(std::string const& swarm_name);
  std::size_t get_count() const;
  uint64_t get_seqnum() const;
  glib::Object<DeeModel> get_model() const;

private:
  glib::Object<DeeModel> model_;
  glib::SignalManager sig_manager_;
  DeeModelTag* renderer_tag_;
  ModelType model_type_;

  RowAdaptor cached_adaptor1_;
  RowAdaptor cached_adaptor2_;
  RowAdaptor cached_adaptor3_;
};

} // namespace dash
} // namespace unity

#include "Model-inl.h"

#endif
