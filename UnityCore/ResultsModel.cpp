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

#include <dee.h>
#include <NuxCore/Logger.h>

#include "config.h"
#include "GLibSignal.h"
#include "GLibWrapper.h"

namespace unity {
namespace dash {

namespace {
nux::logging::Logger logger("unity.dash.resultsmodel");
}

class ResultsModelIterImpl : public ResultsModelIter
{
public:
  ResultsModelIterImpl(DeeModel* model,
                       DeeModelIter* iter,
                       DeeModelTag* renderer_tag)
    : model_(model)
    , iter_(iter)
    , tag_(renderer_tag)
  {
    uri.SetGetterFunction(sigc::mem_fun(this, &ResultsModelIterImpl::get_uri));
    icon_hint.SetGetterFunction(sigc::mem_fun(this, &ResultsModelIterImpl::get_icon_hint));
    category.SetGetterFunction(sigc::mem_fun(this, &ResultsModelIterImpl::get_category));
    mimetype.SetGetterFunction(sigc::mem_fun(this, &ResultsModelIterImpl::get_mimetype));
    name.SetGetterFunction(sigc::mem_fun(this, &ResultsModelIterImpl::get_name));
    comment.SetGetterFunction(sigc::mem_fun(this, &ResultsModelIterImpl::get_comment));
    dnd_uri.SetGetterFunction(sigc::mem_fun(this, &ResultsModelIterImpl::get_dnd_uri));
  }

  std::string get_uri() const
  {
    return dee_model_get_string(model_, iter_, 0);
  }

  std::string get_icon_hint() const
  {
    return dee_model_get_string(model_, iter_, 1);
  }

  unsigned int get_category() const
  {
    return dee_model_get_uint32(model_, iter_, 2);
  }

  std::string get_mimetype() const
  {
    return dee_model_get_string(model_, iter_, 3);
  }

  std::string get_name() const
  {
    return dee_model_get_string(model_, iter_, 4);
  }

  std::string get_comment() const
  {
    return dee_model_get_string(model_, iter_, 5);
  }

  std::string get_dnd_uri() const
  {
    return dee_model_get_string(model_, iter_, 6);
  }

  // Work around non-virtual template functions
  void set_renderer_real(void* renderer)
  {
    dee_model_set_tag(model_, iter_, tag_, renderer);
  }

  void* get_renderer_real()
  {
    return dee_model_get_tag(model_, iter_, tag_);
  }

  DeeModel* model_;
  DeeModelIter* iter_;
  DeeModelTag* tag_;
};


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
  ResultsModelIterImpl it(model, iter, renderer_tag_);
  owner_->row_added(it);
}

void ResultsModel::Impl::OnRowChanged(DeeModel* model, DeeModelIter* iter)
{
  ResultsModelIterImpl it(model, iter, renderer_tag_);
  owner_->row_changed(it);
}

void ResultsModel::Impl::OnRowRemoved(DeeModel* model, DeeModelIter* iter)
{
  ResultsModelIterImpl it(model, iter, renderer_tag_);
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

}
}
