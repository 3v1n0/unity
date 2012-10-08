/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Daniel d'Andrada <daniel.dandrada@canonical.com>
 *
 */

#include "PluginAdapterMock.h"

PluginAdapterMock *PluginAdapterMock::_default = 0;

PluginAdapterMock& PluginAdapterMock::Default() {
  if (!_default) {
    _default = new PluginAdapterMock;
  }
  return *_default;
}

void PluginAdapterMock::ShowGrabHandles(CompWindowMock* window, bool use_timer) {
}
