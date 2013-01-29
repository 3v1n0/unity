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

#ifndef _UNITY_MT_GRAB_HANDLE_WINDOW_H
#define _UNITY_MT_GRAB_HANDLE_WINDOW_H

#include <Nux/Nux.h>

namespace unity
{
namespace MT
{

class GrabHandle;

class GrabHandleWindow
{
  public:

    virtual ~GrabHandleWindow () {};
    virtual void requestMovement (int x,
                                  int y,
                                  unsigned int direction,
				  unsigned int button) = 0;
    virtual void raiseGrabHandle (const std::shared_ptr <const unity::MT::GrabHandle> &) = 0;
};

};
};

#endif
