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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#ifndef UNITY_UTILS_H
#define UNITY_UTILS_H

#include <map>
#include <string>

#include <glib.h>

namespace unity
{
namespace dash
{

class Utils
{
public:
  typedef std::map<std::string, GVariant*> HintsMap;

  static void ASVToHints(HintsMap& hints, GVariantIter *iter)
  {
    char* key = NULL;
    GVariant* value = NULL;
    while (g_variant_iter_loop(iter, "{sv}", &key, &value))
    {
      hints[key] = value;
    }
  }
};

}
}

#endif
