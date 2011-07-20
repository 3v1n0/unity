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

#include "ResultsModel.h"

#include <NuxCore/Logger.h>

#include "config.h"
#include "GLibSignal.h"
#include "GLibWrapper.h"

namespace unity {
namespace dash {

namespace {
nux::logging::Logger logger("unity.dash.resultsmodel");
}

class ResultsModel::Impl
{
public:
  Impl(ResultsModel* owner);
  ~Impl();
  
  void OnRowAdded(DeeModel* model, DeeModelIter* iter);
  void OnRowChanged(DeeModel* model, DeeModelIter* iter);
  void OnRowRemoved(DeeModel* model, DeeModelIter* iter);

  void OnSwarmNameChanged(std::string const& swarm_name);
  unsigned int count();

  const ResultsModelIter IterAtIndex(unsigned int index);

  ResultsModel* owner_;

  glib::Object<DeeModel> model_;
  glib::SignalManager signal_manager_;
  DeeModelTag* renderer_tag_;
};


ResultsModel::Impl::Impl(ResultsModel* owner)
  : owner_(owner)
{}

ResultsModel::Impl::~Impl()
{}

void ResultsModel::Impl::OnRowAdded(DeeModel* model, DeeModelIter* iter)
{
  ResultsModelIter it(model, iter, renderer_tag_);
  owner_->row_added(it);
}

void ResultsModel::Impl::OnRowChanged(DeeModel* model, DeeModelIter* iter)
{
  ResultsModelIter it(model, iter, renderer_tag_);
  owner_->row_changed(it);
}

void ResultsModel::Impl::OnRowRemoved(DeeModel* model, DeeModelIter* iter)
{
  ResultsModelIter it(model, iter, renderer_tag_);
  owner_->row_removed(it);
}

void ResultsModel::Impl::OnSwarmNameChanged(std::string const& swarm_name)
{
  LOG_DEBUG(logger) << "New swarm name: " << swarm_name;

  // Let the views clean up properly
  if (model_)
    dee_model_clear(model_);

  model_ = dee_shared_model_new(swarm_name.c_str());
  renderer_tag_ = dee_model_register_tag(model_, NULL);

  signal_manager_.Add(
    new glib::Signal<void, DeeModel*, DeeModelIter*>(model_,
                                                     "row-added",
                                                      sigc::mem_fun(this, &Impl::OnRowAdded)));

  signal_manager_.Add(
    new glib::Signal<void, DeeModel*, DeeModelIter*>(model_,
                                                     "row-changed",
                                                      sigc::mem_fun(this, &Impl::OnRowChanged)));

  signal_manager_.Add(
    new glib::Signal<void, DeeModel*, DeeModelIter*>(model_,
                                                     "row-removed",
                                                      sigc::mem_fun(this, &Impl::OnRowRemoved)));
}

unsigned int ResultsModel::Impl::count()
{
  if (model_)
    return dee_model_get_n_rows(model_);
  else
    return 0;
}

const ResultsModelIter ResultsModel::Impl::IterAtIndex(unsigned int index)
{
  ResultsModelIter it(model_, dee_model_get_iter_at_row(model_, index), renderer_tag_);
  return it;
}

ResultsModel::ResultsModel()

  : pimpl(new Impl(this))
{
  swarm_name.changed.connect(sigc::mem_fun(pimpl, &Impl::OnSwarmNameChanged));
  count.SetGetterFunction(sigc::mem_fun(pimpl, &Impl::count));
}

ResultsModel::~ResultsModel()
{
  delete pimpl;
}

const ResultsModelIter ResultsModel::IterAtIndex(unsigned int index)
{
  return pimpl->IterAtIndex(index);
}

}
}
