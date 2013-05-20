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
 * Authored by: Marco Biscaro <marcobiscaro2112@gmail.com>
 */

#include "DashViewPrivate.h"

#include <vector>
#include <boost/algorithm/string.hpp>

namespace unity
{
namespace dash
{
namespace impl
{

ScopeFilter parse_scope_uri(std::string const& uri)
{
  ScopeFilter filter;

  filter.id = uri;
  std::size_t pos = uri.find("?");

  // it's a real URI (with parameters)
  if (pos != std::string::npos)
  {
    // id is the uri from begining to the '?' position
    filter.id = uri.substr(0, pos);

    // the components are from '?' position to the end
    std::string components = uri.substr(++pos);

    // split components in tokens
    std::vector<std::string> tokens;
    boost::split(tokens, components, boost::is_any_of("&"));

    for (std::string const& token : tokens)
    {
      // split each token in a pair
      std::size_t equals_pos = token.find("=");

      if (equals_pos != std::string::npos)
      {
        std::string key = token.substr(0, equals_pos);
        std::string value = token.substr(equals_pos + 1);

        // check if it's a filter
        if (boost::starts_with(key, "filter_"))
        {
          filter.filters[key.substr(7)] = value;
        }
      }

    }
  }

  return filter;
}

} // namespace impl
} // namespace dash
} // namespace unity
