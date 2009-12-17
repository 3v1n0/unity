/*
 * Copyright (C) 2009 Canonical Ltd
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
 * Authored by Gordon Allott <gord.allott@canonical.com>
 * 
 */
#include <unity.h>

#define LOGGER_START_PROCESS(process) if (unity_is_logging) { unity_timeline_logger_start_process (unity_timeline_logger_get_default(), process);}
#define LOGGER_END_PROCESS(process) if (unity_is_logging) { unity_timeline_logger_end_process (unity_timeline_logger_get_default(), process);}
