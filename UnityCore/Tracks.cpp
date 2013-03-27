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

#include "Tracks.h"

namespace unity
{
namespace dash
{

Tracks::Tracks()
: Model<Track>::Model(ModelType::REMOTE)
{
  row_added.connect(sigc::mem_fun(this, &Tracks::OnRowAdded));
  row_changed.connect(sigc::mem_fun(this, &Tracks::OnRowChanged));
  row_removed.connect(sigc::mem_fun(this, &Tracks::OnRowRemoved));
}

Tracks::Tracks(ModelType model_type)
: Model<Track>::Model(model_type)
{
  row_added.connect(sigc::mem_fun(this, &Tracks::OnRowAdded));
  row_changed.connect(sigc::mem_fun(this, &Tracks::OnRowChanged));
  row_removed.connect(sigc::mem_fun(this, &Tracks::OnRowRemoved));
}

void Tracks::OnRowAdded(Track& result)
{
  track_added.emit(result);
}

void Tracks::OnRowChanged(Track& result)
{
  track_changed.emit(result);
}

void Tracks::OnRowRemoved(Track& result)
{
  track_removed.emit(result);
}

}
}
