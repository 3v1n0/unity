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
 * Authored by: Sam Spilsbury <sam.spilsbury@canonical.com>
 */

#include "unity-mt-grab-handle-layout.h"
#include "unity-mt-grab-handle.h"

unsigned int
unity::MT::getLayoutForMask (unsigned int state,
                             unsigned int actions)
{
  unsigned int allHandles = 0;
  for (unsigned int i = 0; i < NUM_HANDLES; i++)
  {
    allHandles |= (1 << i);
  }

  struct _skipInfo
  {
    /* All must match in order for skipping to apply */
    unsigned int state; /* Match if in state */
    unsigned int notstate; /* Match if not in state */
    unsigned int actions; /* Match if in actions */
    unsigned int notactions; /* Match if not in actions */
    unsigned int allowOnly;
  };

  const unsigned int numSkipInfo = 5;
  const struct _skipInfo skip[5] =
  {
    /* Vertically maximized, don't care
     * about left or right handles, or
     * the movement handle */
    {
      MaximizedVertMask,
      MaximizedHorzMask,
      0, static_cast<unsigned int>(~0),
      LeftHandle | RightHandle | MiddleHandle
    },
    /* Horizontally maximized, don't care
     * about top or bottom handles, or
     * the movement handle */
    {
      MaximizedHorzMask,
      MaximizedVertMask,
      0, static_cast<unsigned int>(~0),
      TopHandle | BottomHandle | MiddleHandle
    },
    /* Maximized, don't care about the movement
    * handle */
    {
      MaximizedVertMask | MaximizedHorzMask,
      0, 0, static_cast<unsigned int>(~0),
      MiddleHandle
    },
    /* Immovable, don't show move handle */
    {
      0,
      static_cast<unsigned int>(~0),
      static_cast<unsigned int>(~0), MoveMask,
      TopLeftHandle | TopHandle | TopRightHandle |
      LeftHandle | RightHandle |
      BottomLeftHandle | BottomHandle | BottomRightHandle
    },
    /* Not resizable, don't show resize handle */
    {
      0,
      static_cast<unsigned int>(~0),
      static_cast<unsigned int>(~0), ResizeMask,
      MiddleHandle
    },
  };

  /* Set the high bit if it was zero */
  if (!state)
    state |= 0x8000;

  /* Set the high bit if it was zero */
  if (!actions)
    actions |= 0x8000;

  for (unsigned int j = 0; j < numSkipInfo; j++)
  {
    const bool exactState = skip[j].state && skip[j].state != static_cast <unsigned int> (~0);
    const bool exactActions = skip[j].actions && skip[j].actions != static_cast <unsigned int> (~0);

    bool stateMatch = false;
    bool actionMatch = false;

    if (exactState)
      stateMatch = (skip[j].state & state) == skip[j].state;
    else
      stateMatch = skip[j].state & state;

    stateMatch &= !(state & skip[j].notstate);

    if (exactActions)
      actionMatch = (skip[j].actions & actions) == skip[j].actions;
    else
      actionMatch = skip[j].actions & actions;

    actionMatch &= !(actions & skip[j].notactions);

    if (stateMatch || actionMatch)
      allHandles &= skip[j].allowOnly;  
  }

  return allHandles;
}
