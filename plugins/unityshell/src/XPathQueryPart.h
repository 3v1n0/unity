// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010 Canonical Ltd
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
 * Authored by: Thomi Richards <thomi.richards@canonical.com>
 */

#include <string>

#ifndef _XPATH_QUERY_PART
#define _XPATH_QUERY_PART

namespace unity
{
namespace debug
{
  class Introspectable;
  // Stores a part of an XPath query.
  class XPathQueryPart
  {
  public:
    XPathQueryPart(std::string const& query_part);
    bool Matches(Introspectable* node) const;
  private:
    std::string node_name_;
    std::string param_name_;
    std::string param_value_;
  };

}
}

#endif
