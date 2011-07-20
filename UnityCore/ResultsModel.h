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

#ifndef UNITY_RESULTS_MODEL_H
#define UNITY_RESULTS_MODEL_H

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <NuxCore/Property.h>
#include <sigc++/trackable.h>

namespace unity {
namespace dash {

class ResultsModelIter : boost::noncopyable
{
public:
  template<typename T>
  void set_renderer(T renderer);
  template<typename T>
  T renderer();

  nux::ROProperty<std::string> uri;
  nux::ROProperty<std::string> icon_hint;
  nux::ROProperty<unsigned int> category; //FIXME:this should be actual category
  nux::ROProperty<std::string> mimetype;
  nux::ROProperty<std::string> name;
  nux::ROProperty<std::string> comment;
  nux::ROProperty<std::string> dnd_uri;

protected:
  virtual void set_renderer_real(void* renderer) = 0;
  virtual void* get_renderer_real() const = 0;
};

class ResultsModel : public sigc::trackable, boost::noncopyable
{
public:
  typedef boost::shared_ptr<ResultsModel> Ptr;

  ResultsModel();
  ~ResultsModel();

  nux::Property<std::string> swarm_name;
  nux::ROProperty<unsigned int> count;

  sigc::signal<void, ResultsModelIter&> row_added;
  sigc::signal<void, ResultsModelIter&> row_changed;
  sigc::signal<void, ResultsModelIter&> row_removed;

  class Impl;
private:
  Impl *pimpl;
};

}
}

#endif
