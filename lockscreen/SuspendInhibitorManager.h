/*
 * Copyright (C) 2016 Canonical Ltd
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

#ifndef UNITY_LOCKSCREEN_SUSPEND_INHIBITOR_MANAGER
#define UNITY_LOCKSCREEN_SUSPEND_INHIBITOR_MANAGER

#include <memory>
#include <string>
#include <sigc++/signal.h>

namespace unity
{
namespace lockscreen
{

class SuspendInhibitorManager
{
public:
  typedef std::shared_ptr<SuspendInhibitorManager> Ptr;

  SuspendInhibitorManager();
  ~SuspendInhibitorManager();

  void Inhibit(std::string const&);
  void Uninhibit();
  bool IsInhibited();

  sigc::signal<void> connected;
  sigc::signal<void> about_to_suspend;

private:
  class Impl;
  std::unique_ptr<Impl> pimpl_;
};

}
}

#endif
