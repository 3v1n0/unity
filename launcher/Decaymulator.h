// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010 Canonical Ltd
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
 * Authored by: Jason Smith <jason.smith@canonical.com>
 */

#ifndef UNITY_DECAYMULATOR_H
#define UNITY_DECAYMULATOR_H

#include <NuxCore/Property.h>
#include <UnityCore/GLibSource.h>

namespace unity
{
namespace ui
{

class Decaymulator
{
public:
  Decaymulator();

  nux::Property<int> rate_of_decay;
  nux::Property<int> value;

private:
  void OnValueChanged(int value);
  bool OnDecayTimeout();

  glib::Source::UniquePtr decay_timer_;
};

}
}

#endif
