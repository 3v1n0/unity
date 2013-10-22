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

#include <Nux/Nux.h>
#include <NuxCore/Logger.h>

#include "DecorationsManager.h"
#include "unityshell.h"

namespace
{
DECLARE_LOGGER(logger, "unity.decoration.manager");

namespace atom
{
Atom _NET_REQUEST_FRAME_EXTENTS = 0;
Atom _NET_FRAME_EXTENTS = 0;
}
}

namespace unity
{
namespace decoration
{

struct Manager::Impl
{
  Impl(UnityScreen *uscreen)
    : uscreen_(uscreen)
    , cscreen_(uscreen_->screen)
  {
    atom::_NET_REQUEST_FRAME_EXTENTS = XInternAtom(cscreen_->dpy(), "_NET_REQUEST_FRAME_EXTENTS", False);
    atom::_NET_FRAME_EXTENTS = XInternAtom(cscreen_->dpy(), "_NET_FRAME_EXTENTS", False);
    cscreen_->updateSupportedWmHints();
  }

  ~Impl()
  {
    cscreen_->addSupportedAtomsSetEnabled(uscreen_, false);
    cscreen_->updateSupportedWmHints();
  }

  UnityScreen *uscreen_;
  CompScreen *cscreen_;
};

Manager::Manager(UnityScreen *screen)
  : impl_(new Manager::Impl(screen))
{}

Manager::~Manager()
{}

void Manager::AddSupportedAtoms(std::vector<Atom>& atoms) const
{
  atoms.push_back(atom::_NET_REQUEST_FRAME_EXTENTS);
}

bool Manager::HandleEvent(XEvent* xevent)
{
  return false;
}

} // decoration namespace
} // unity namespace
