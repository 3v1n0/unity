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
#include <boost/shared_ptr.hpp>
#include <dee.h>
#include <NuxCore/Property.h>
#include <sigc++/trackable.h>

#include "GLibSignal.h"
#include "GLibWrapper.h"
#include "ModelRowAdaptor.h"

namespace unity {
namespace dash {

/* This template class encapsulates the basics of talking to a DeeSharedModel,
 * however it is a template as you can choose your own RowAdaptor (see
 * ResultsRowAdaptor.h for an example) which then presents the data in the rows
 * in a instance-specific way.
 */
template<class RowAdaptor>
class Model : public sigc::trackable, boost::noncopyable
{
public:
  typedef boost::shared_ptr<Model> Ptr;

  Model();
  virtual ~Model();

  const RowAdaptor RowAtIndex(unsigned int index);

  nux::Property<std::string> swarm_name;
  nux::ROProperty<unsigned int> count;
  nux::ROProperty<bool> synchronized;

  sigc::signal<void, RowAdaptor&> row_added;
  sigc::signal<void, RowAdaptor&> row_changed;
  sigc::signal<void, RowAdaptor&> row_removed;

private:
  void OnRowAdded(DeeModel* model, DeeModelIter* iter);
  void OnRowChanged(DeeModel* model, DeeModelIter* iter);
  void OnRowRemoved(DeeModel* model, DeeModelIter* iter);
  void OnSwarmNameChanged(std::string const& swarm_name);
  unsigned int get_count();
  bool is_synchronized();

private:
  glib::Object<DeeModel> model_;
  glib::SignalManager signal_manager_;
  DeeModelTag* renderer_tag_;
};

}
}

#include "Model-inl.h"

#endif
