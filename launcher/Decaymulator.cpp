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

#include "Decaymulator.h"

namespace unity {
namespace ui {

Decaymulator::Decaymulator()
{
  value.changed.connect(sigc::mem_fun(this, &Decaymulator::OnValueChanged));
}

void Decaymulator::OnValueChanged(int value)
{
  if (!decay_timer_ && value > 0)
  {
    decay_timer_.reset(new glib::Timeout(10, sigc::mem_fun(this, &Decaymulator::OnDecayTimeout)));
  }
}

bool Decaymulator::OnDecayTimeout()
{
  int partial_decay = rate_of_decay / 100;

  if (value <= partial_decay)
  {
    value = 0;
    decay_timer_.reset();
    return false;
  }

  value = value - partial_decay;
  return true;
}

}
}
