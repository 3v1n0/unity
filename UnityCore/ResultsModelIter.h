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

#ifndef UNITY_RESULTS_MODEL_ITER_H
#define UNITY_RESULTS_MODEL_ITER_H

#include <boost/noncopyable.hpp>
#include <dee.h>
#include <NuxCore/Property.h>
#include <sigc++/trackable.h>

namespace unity {
namespace dash {

// This class represents a DeeModelIter for a ResultsModel
// It's slightly chunky, but that is because it's optimized to be stack-allocated
// as it is not expected to be kept by the views, rather views can easily attach
// a "renderer" to the iter, so when the changed or removed signals are called,
// the view can easily find which widget/view belongs to this iter.
class ResultsModelIter
{
public:
  ResultsModelIter(DeeModel* model, DeeModelIter* iter, DeeModelTag* tag);

  ResultsModelIter(ResultsModelIter const& other);

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

private:
  std::string get_uri() const;
  std::string get_icon_hint() const;
  unsigned int get_category() const;
  std::string get_mimetype() const;
  std::string get_name() const;
  std::string get_comment() const;
  std::string get_dnd_uri() const;

private:
  DeeModel* model_;
  DeeModelIter* iter_;
  DeeModelTag* tag_;
};

}
}

#endif
