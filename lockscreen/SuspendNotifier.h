/*
 * Copyright (C) 2014 Canonical Ltd
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
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 */

#ifndef UNITY_LOCKSCREEN_SUSPEND_NOTIFIER
#define UNITY_LOCKSCREEN_SHUTDOWN_NOTIFIER

#include <memory>
#include <functional>

namespace unity
{
namespace lockscreen
{

typedef std::function<void()> SuspendCallback;

class SuspendNotifier
{
public:
  typedef std::shared_ptr<SuspendNotifier> Ptr;

  SuspendNotifier();
  ~SuspendNotifier();

  bool RegisterInterest(SuspendCallback const&);
  void UnregisterInterest();

private:
  class Impl;
  std::unique_ptr<Impl> pimpl_;
};

}
}

#endif