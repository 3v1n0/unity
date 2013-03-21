// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2013 Canonical Ltd
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
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

#include <gmock/gmock.h>
#include <UnityCore/GLibWrapper.h>

using namespace unity::glib;

namespace
{

TEST(TestGLibCancellable, Construction)
{
  Cancellable cancellable;
  Error error;
  ASSERT_TRUE(cancellable.Get().IsType(G_TYPE_CANCELLABLE));
  EXPECT_FALSE(cancellable.IsCancelled());
  EXPECT_FALSE(cancellable.IsCancelled(error));
}

TEST(TestGLibCancellable, Destruction)
{
  Object<GCancellable> canc_obj;
  {
    Cancellable cancellable;
    canc_obj = cancellable.Get();
  }

  EXPECT_TRUE(g_cancellable_is_cancelled(canc_obj));
}

TEST(TestGLibCancellable, Cancel)
{
  Cancellable cancellable;

  cancellable.Cancel();
  EXPECT_TRUE(cancellable.IsCancelled());
}

TEST(TestGLibCancellable, CancelWithError)
{
  Cancellable cancellable;
  Error error;

  cancellable.Cancel();
  EXPECT_TRUE(cancellable.IsCancelled(error));
  EXPECT_FALSE(error.Message().empty());
}

TEST(TestGLibCancellable, Reset)
{
  Cancellable cancellable;

  cancellable.Cancel();
  ASSERT_TRUE(cancellable.IsCancelled());

  auto obj = cancellable.Get();
  cancellable.Reset();
  EXPECT_FALSE(cancellable.IsCancelled());
  EXPECT_EQ(obj, cancellable.Get());
}

TEST(TestGLibCancellable, Renew)
{
  Cancellable cancellable;

  cancellable.Cancel();
  ASSERT_TRUE(cancellable.IsCancelled());

  auto obj = cancellable.Get();
  cancellable.Renew();
  EXPECT_FALSE(cancellable.IsCancelled());
  EXPECT_NE(obj, cancellable.Get());
}

TEST(TestGLibCancellable, OperatorGCancellable)
{
  Cancellable cancellable;

  EXPECT_EQ(g_cancellable_is_cancelled(cancellable), cancellable.IsCancelled());

  g_cancellable_cancel(cancellable);
  EXPECT_EQ(g_cancellable_is_cancelled(cancellable), cancellable.IsCancelled());
}

TEST(TestGLibCancellable, Equality)
{
  Cancellable cancellable;

  ASSERT_EQ(cancellable.Get(), cancellable);
  ASSERT_EQ(cancellable.Get().RawPtr(), cancellable);
}

TEST(TestGLibCancellable, NotEquality)
{
  Cancellable cancellable1;
  Cancellable cancellable2;

  ASSERT_NE(cancellable1, cancellable2);
  ASSERT_NE(cancellable1.Get(), cancellable2);
  ASSERT_NE(cancellable1.Get().RawPtr(), cancellable2);
}

} // Namespace
