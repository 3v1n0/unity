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

#ifndef PLUGINADAPTER_MOCK_H
#define PLUGINADAPTER_MOCK_H

#include <compiz_mock/core/core.h>


class PluginAdapterMock {
public:
  static PluginAdapterMock& Default();

  void ShowGrabHandles(CompWindowMock* window, bool use_timer);

private:
  PluginAdapterMock() {}
  static PluginAdapterMock* _default;
};

#endif
