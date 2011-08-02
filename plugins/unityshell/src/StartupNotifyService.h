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

#ifndef STARTUPNOTIFYSERVICE_H
#define STARTUPNOTIFYSERVICE_H

#ifndef SN_API_NOT_YET_FROZEN
#define SN_API_NOT_YET_FROZEN
#endif
#include <libsn/sn.h>

#include <sigc++/sigc++.h>

class StartupNotifyService : public sigc::trackable
{

public:
  static StartupNotifyService* Default();

  void SetSnDisplay(SnDisplay* sn_display, int screen);

  sigc::signal<void, const char*> StartupInitiated;
  sigc::signal<void, const char*> StartupCompleted;

protected:
  StartupNotifyService();
  ~StartupNotifyService();

private:
  static void OnMonitorEvent(SnMonitorEvent* sn_event, void* user_data);

  static StartupNotifyService* _default;

  SnDisplay*                   _sn_display;
  SnMonitorContext*            _sn_monitor;
};

#endif // STARTUPNOTIFYSERVICE_H
