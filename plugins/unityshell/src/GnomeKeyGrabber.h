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

#include <core/core.h>

namespace unity
{
namespace grabber
{

class GnomeKeyGrabber
{
public:

  explicit GnomeKeyGrabber(CompScreen* screen);
  ~GnomeKeyGrabber();

  CompAction::Vector& getActions();
  void addAction(const CompAction& action);
  void removeAction(const CompAction& action);

  struct Impl;

protected:

  struct TestMode {};
  GnomeKeyGrabber(CompScreen* screen, TestMode const&);

private:

  std::unique_ptr<Impl> impl_;
};

} // namespace grabber
} // namespace unity

#endif // __GNOME_KEY_GRABBER_H__
