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
* Authored by: William Hua <william.hua@canonical.com>
*/

#ifndef __GNOME_KEY_GRABBER_H__
#define __GNOME_KEY_GRABBER_H__

#include "KeyGrabber.h"

namespace unity
{
namespace key
{
class GnomeGrabber : public Grabber
{
public:
  GnomeGrabber();
  virtual ~GnomeGrabber();

  uint32_t AddAction(CompAction const&) override;
  bool RemoveAction(CompAction const&) override;
  bool RemoveAction(uint32_t action_id) override;

  CompAction::Vector& GetActions() override;

protected:
  struct TestMode {};
  GnomeGrabber(TestMode const& dummy);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace key
} // namespace unity

#endif // __GNOME_KEY_GRABBER_H__
