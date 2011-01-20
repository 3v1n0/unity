/*
 * Copyright (C) 2009-2010 Canonical Ltd
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
#ifndef _PERF_LOGGER_H_
#define _PERF_LOGGER_H_

//#include "perf-logger.h"

#ifdef ENABLE_LOGGER
  #define START_FUNCTION() G_STMT_START { \
                            perf_timeline_logger_start_process (perf_timeline_logger_get_default(), G_STRFUNC);\
                         } G_STMT_END
  #define LOGGER_START_PROCESS(process) { perf_timeline_logger_start_process (perf_timeline_logger_get_default(), process);}

  #define END_FUNCTION() G_STMT_START { \
                            perf_timeline_logger_end_process (perf_timeline_logger_get_default(), G_STRFUNC);\
                         } G_STMT_END
  #define LOGGER_END_PROCESS(process) { perf_timeline_logger_end_process (perf_timeline_logger_get_default(), process);}
  #define LOGGER_WRITE_LOG(file) { perf_timeline_logger_write_log (perf_timeline_logger_get_default(), file);}
#else
  #define START_FUNCTION()
  #define LOGGER_START_PROCESS(process)
  #define END_FUNCTION()
  #define LOGGER_END_PROCESS(process)
  #define LOGGER_WRITE_LOG(file)
#endif /* ENABLE_LOGGER */

#endif /* PERF_LOGGER_H */
