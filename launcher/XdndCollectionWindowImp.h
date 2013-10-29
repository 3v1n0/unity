// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2011-2012 Canonical Ltd
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

#ifndef UNITYSHELL_XDND_COLLECTION_WINDOW_IMP_H
#define UNITYSHELL_XDND_COLLECTION_WINDOW_IMP_H

#include "XdndCollectionWindow.h"

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>

namespace unity {

class XdndCollectionWindowImp : public XdndCollectionWindow
{
public:
  XdndCollectionWindowImp();

  void Collect();
  void Deactivate();
  std::string GetData(std::string const& type);

private:
  nux::ObjectPtr<nux::BaseWindow> window_;
};

}

#endif
