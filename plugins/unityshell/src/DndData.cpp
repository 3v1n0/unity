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
  
void DndData::Fill(char* uris)
{ 
  Reset();
  
  char* pch = strtok (uris, "\r\n");
  while (pch != NULL)
  {
    glib::Object<GFile> file(g_file_new_for_uri(pch));
    glib::Object<GFileInfo> info(g_file_query_info(file, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, G_FILE_QUERY_INFO_NONE, NULL, NULL));
    const char* content_type = g_file_info_get_content_type(info);
    
    uris_.insert(pch);
    
    if (content_type != NULL)
    {
      types_.insert(content_type);
      uris_to_types_[pch] = content_type;
      types_to_uris_[content_type].insert(pch);
    }
       
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
