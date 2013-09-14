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

#ifndef UNITY_TRACK_H
#define UNITY_TRACK_H

#include <NuxCore/Property.h>

#include "ModelRowAdaptor.h"

namespace unity
{
namespace dash
{

/* This class represents a DeeModelIter for a TracksModel
 * It's slightly chunky, but that is because it's optimized to be stack-allocated
 * as it is not expected to be kept by the views, rather views can easily attach
 * a "renderer" to the iter, so when the changed or removed signals are called,
 * the view can easily find which widget/view belongs to this iter.
 */
class Track : public RowAdaptorBase
{
public:
  Track(DeeModel* model, DeeModelIter* iter, DeeModelTag* tag);

  Track(Track const& other);
  Track& operator=(Track const& other);

  nux::ROProperty<std::string> uri;
  nux::ROProperty<int> track_number;
  nux::ROProperty<std::string> title;
  nux::ROProperty<unsigned> length;
  nux::ROProperty<std::size_t> index;

private:
  void SetupGetters();
  std::size_t get_index() const;
};

}
}

#endif
