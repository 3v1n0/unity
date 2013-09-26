/*
 * Copyright 2012-2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 *
 */

#include "test_im_text_entry.h"
#include "test_utils.h"

TestEvent::TestEvent(nux::KeyModifier keymod, unsigned long keysym)
{
  type = nux::NUX_KEYDOWN;
  key_modifiers = keymod;
  x11_keysym = keysym;
}

TestEvent::TestEvent(unsigned long keysym)
{
  type = nux::NUX_KEYDOWN;
  x11_keysym = keysym;
}

bool MockTextEntry::InspectKeyEvent(nux::Event const& event)
{
  bool ret;
  key_down.emit(event.type, event.GetKeySym(), event.GetKeyState(), nullptr, 0);
  ret = IMTextEntry::InspectKeyEvent(event);

  if (im_running())
    Utils::WaitForTimeoutMSec(100);

  return ret;
}
