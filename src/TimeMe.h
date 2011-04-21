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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#ifndef _TIME_ME_H_
#define _TIME_ME_H_

#include <glib.h>

extern gint64 _____last_time_____;
extern gint64 _____start_time_____;

#define TIME_ME_START(foo) { _____last_time_____ = g_get_monotonic_time (); _____start_time_____ = _____last_time_____; g_print ("\nSTARTED (%s)\n", foo);}

#define TIME_ME_LOG(foo) { \
  gint64 ____now_time___ = g_get_monotonic_time ();  \
  g_print ("%f: %s\n", (____now_time___ - _____last_time_____)/1000.0f, foo); \
  _____last_time_____ = ____now_time___; \
}

#define TIME_ME_FINSH(foo) { \
  gint64 ____now_time___ = g_get_monotonic_time ();  \
  g_print ("\n%f: FINISHED (%s)\n", (____now_time___ - _____start_time_____)/1000.0f, foo); \
}

#define TIME_ME_FUNC() {_____last_time_____ = g_get_monotonic_time (); _____start_time_____ = _____last_time_____;}
#define TIME_ME_ENDFUNC() TIME_ME_FINSH(G_STRFUNC)

#endif //_TIME_ME_H_
