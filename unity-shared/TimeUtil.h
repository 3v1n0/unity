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
 * Authored by: Alex Launi <alex.launi@gmail.com>
 */

#ifndef TIME_UTIL_H
#define TIME_UTIL_H

#include <time.h>
#include <cstdint>

typedef int64_t DeltaTime;

namespace unity
{
class TimeUtil
{
public:
  static DeltaTime TimeDelta (struct timespec const* x, struct timespec const* y)
  {
    DeltaTime d_sec = ((x->tv_sec - y->tv_sec));
    DeltaTime d_nsec = ((x->tv_nsec - y->tv_nsec));
    return (d_sec * 1000) + (d_nsec / 1000000);
  }

  static void SetTimeStruct(struct timespec* timer, struct timespec* sister = 0, DeltaTime sister_relation = 0)
  {
    struct timespec current;
    clock_gettime(CLOCK_MONOTONIC, &current);

    if (sister)
    {
      DeltaTime diff = TimeDelta(&current, sister);

      if (diff < sister_relation)
      {
        DeltaTime remove = sister_relation - diff;
        SetTimeBack(&current, remove);
      }
    }

    timer->tv_sec = current.tv_sec;
    timer->tv_nsec = current.tv_nsec;
  }

  static void SetTimeBack(struct timespec* timeref, DeltaTime remove)
  {
    timeref->tv_sec -= remove / 1000;
    remove = remove % 1000;

    if (remove > timeref->tv_nsec / 1000000)
    {
      timeref->tv_sec--;
      timeref->tv_nsec += 1000000000;
    }
    timeref->tv_nsec -= remove * 1000000;
  }
};

namespace time
{
  struct Spec
  {
    Spec()
      : ts({0, 0})
    {}

    void Reset()
    {
      ts.tv_sec = 0;
      ts.tv_nsec = 0;
    }

    void SetToNow()
    {
      clock_gettime(CLOCK_MONOTONIC, &ts);
    }

    DeltaTime TimeDelta(Spec const& other)
    {
      return TimeUtil::TimeDelta(&ts, &other.ts);
    }

    operator struct timespec() const
    {
      return ts;
    }

  private:
    struct timespec ts;
  };
}

}

#endif // TIME_UTIL_H
