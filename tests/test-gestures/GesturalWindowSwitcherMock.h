/*
 * Copyright (C) 2012 Canonical Ltd
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
 * Authored by: Daniel d'Andrada <daniel.dandrada@canonical.com>
 */

#ifndef GESTURAL_WINDOW_SWITCHER_MOCK_H
#define GESTURAL_WINDOW_SWITCHER_MOCK_H

#include <Nux/Gesture.h>

namespace unity
{

class GesturalWindowSwitcherMock : public nux::GestureTarget
{
  public:
    GesturalWindowSwitcherMock() {}
    virtual ~GesturalWindowSwitcherMock() {}

    virtual nux::GestureDeliveryRequest GestureEvent(const nux::GestureEvent &event)
    {
      return nux::GestureDeliveryRequest::NONE;
    }
};
typedef std::shared_ptr<GesturalWindowSwitcherMock> ShPtGesturalWindowSwitcherMock;

} //namespace unity
#endif // GESTURAL_WINDOW_SWITCHER_MOCK_H
