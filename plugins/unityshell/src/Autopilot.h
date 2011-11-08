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
 * Authored by: Alex Launi <alex.launi@canonical.com>
 */

#ifndef _AUTOPILOT_H
#define _AUTOPILOT_H 1

#include <sys/time.h>

#include <glib.h>
#include <gio/gio.h>

#include <core/core.h>
#include <Nux/Nux.h>
#include <Nux/TimerProc.h>

#include "Monitor.h"
#include "ubus-server.h"

#define TEST_TIMEOUT 6000

namespace unity {

typedef struct
{
  gboolean passed;
  std::string name;
  guint ubus_handle;
  nux::TimerHandle expiration_handle;
  unity::performance::Monitor* monitor;
} TestArgs;

class Autopilot
{
public:
  Autopilot(CompScreen* screen, GDBusConnection* connection);
  ~Autopilot();

  void StartTest(const std::string name);

  UBusServer* GetUBusConnection();
  GDBusConnection* GetDBusConnection();

  void RegisterUBusInterest(const std::string signal, TestArgs* args);

private:
  static void TestFinished(void* val);
  static void OnTestPassed(GVariant* payload, TestArgs* args);
};
}

#endif /* _AUTOPILOT_H */
