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
  on_decay_handle = 0;
  value.changed.connect(sigc::mem_fun(this, &Decaymulator::OnValueChanged));
}

void Decaymulator::OnValueChanged(int value)
{
  if (!on_decay_handle && value > 0)
  {
    on_decay_handle = g_timeout_add(10, &Decaymulator::OnDecayTimeout, this);
  }
}

gboolean Decaymulator::OnDecayTimeout(gpointer data)
{
  Decaymulator* self = (Decaymulator*) data;

  int partial_decay = self->rate_of_decay / 100;

  if (self->value <= partial_decay)
  {
    self->value = 0;
    self->on_decay_handle = 0;
    return FALSE;
  }


  self->value = self->value - partial_decay;
  return TRUE;
}

}
}