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

#include "StartupNotifyService.h"
#include <stdio.h>

StartupNotifyService* StartupNotifyService::_default = 0;

StartupNotifyService*
StartupNotifyService::Default()
{
  if (!_default)
    _default = new StartupNotifyService();

  return _default;
}

StartupNotifyService::StartupNotifyService()
  : _sn_display(0)
  , _sn_monitor(0)
{
}

StartupNotifyService::~StartupNotifyService()
{
}

void
StartupNotifyService::OnMonitorEvent(SnMonitorEvent* sn_event, void* user_data)
{
  StartupNotifyService* service = (StartupNotifyService*) user_data;
  SnStartupSequence* seq;
  const char* id;

  seq = sn_monitor_event_get_startup_sequence(sn_event);
  id = sn_startup_sequence_get_id(seq);

  switch (sn_monitor_event_get_type(sn_event))
  {
    case SN_MONITOR_EVENT_INITIATED:
      service->StartupInitiated.emit(id);
      break;
    case SN_MONITOR_EVENT_COMPLETED:
      service->StartupCompleted.emit(id);
      break;
    default:
      break;
  }

}

void
StartupNotifyService::SetSnDisplay(SnDisplay* sn_display, int screen)
{
  _sn_display = sn_display;
  _sn_monitor = sn_monitor_context_new(_sn_display, screen, &StartupNotifyService::OnMonitorEvent, this, NULL);
}
