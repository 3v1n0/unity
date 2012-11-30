// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2012 Canonical Ltd
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
* Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
*
*/


#include <limits>
#include <gtest/gtest.h>
#include <unity-shared/TimeUtil.h>

using namespace testing;
using namespace std;

TEST(TestTimeUtil, Testin32BufferOverflow)
{
  struct timespec start, end;
  unity::TimeUtil::SetTimeStruct(&start, &end);

  end.tv_sec = 20736000 - start.tv_sec;

  EXPECT_GT(unity::TimeUtil::TimeDelta(&end, &start), 0);
}

