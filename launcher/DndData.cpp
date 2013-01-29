// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2011 Canonical Ltd
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
* Authored by: Andrea Azzarone <azzaronea@gmail.com>
*/

#include "DndData.h"

#include <cstring>
#include <vector>

#include <gio/gio.h>

#include <UnityCore/GLibWrapper.h>

namespace unity {

void DndData::Fill(const char* uris)
{
  Reset();

  const char* pch = strtok (const_cast<char*>(uris), "\r\n");
  while (pch)
  {
    glib::String content_type(g_content_type_guess(pch,
                                                   nullptr,
                                                   0,
                                                   nullptr));

    if (content_type)
    {
      types_.insert(content_type.Str());
      uris_to_types_[pch] = content_type.Str();
      types_to_uris_[content_type.Str()].insert(pch);
    }

    uris_.insert(pch);

    pch = strtok (NULL, "\r\n");
  }
}

void DndData::Reset()
{
  uris_.clear();
  types_.clear();
  uris_to_types_.clear();
  types_to_uris_.clear();
}

} // namespace unity
