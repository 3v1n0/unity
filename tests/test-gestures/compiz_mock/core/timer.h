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

#ifndef COMPIZ_TIMER_MOCK_H
#define COMPIZ_TIMER_MOCK_H

#include <boost/function.hpp>

class CompTimerMock
{
  public:
	typedef boost::function<bool ()> CallBack;

  CompTimerMock()
  {
    is_running = false;
    callback = NULL;

    // OBS: no support for more than one simultaneous timer
    instance = this;
  }

  virtual ~CompTimerMock()
  {
    instance = nullptr;
  }

	void setCallback (CallBack callback)
  {
    this->callback = callback;
  }

	void setTimes(unsigned int min, unsigned int max = 0)
  {
  }

	void start()
  {
    is_running = true;
  }

	void stop()
  {
    is_running = false;
  }

  void ForceTimeout()
  {
    if (is_running && callback)
    {
      callback();
      is_running = false;
    }
  }

  CallBack callback;
  bool is_running;

  static CompTimerMock *instance;
};

#endif // COMPIZ_TIMER_MOCK_H
