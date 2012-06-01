/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Andrea Azzarone <azzaronea@gmail.com>
 *              Tim Penhey <tim.penhey@canonical.com>
 *
 */

#include "HudPrivate.h"

namespace unity
{
namespace hud
{
namespace impl
{

std::vector<std::pair<std::string, bool>> RefactorText(std::string const& text)
{
  std::vector<std::pair<std::string, bool>> ret;

  static const std::string bold_start("<b>");
  static const std::string bold_end("</b>");

  std::string::size_type last = 0;
  std::string::size_type len = text.length();
  std::string::size_type pos = text.find(bold_start);

  while (pos != std::string::npos)
  {
    if (pos != last)
    {
      ret.push_back(std::pair<std::string, bool>(text.substr(last, pos - last), false));
    }

    // Look for the end
    pos += 3; // // to skip the "<b>"
    std::string::size_type end_pos = text.find(bold_end, pos);
    // We hope we find it, if we don't, just output everything...
    if (end_pos != std::string::npos)
    {
      ret.push_back(std::pair<std::string, bool>(text.substr(pos, end_pos - pos), true));
      last = end_pos + 4; // the length of "</b>"
      pos = text.find(bold_start, last);
    }
    else
    {
      ret.push_back(std::pair<std::string, bool>(text.substr(pos), true));
      pos = std::string::npos;
      last = len;
    }
  }

  if (last < len)
  {
    ret.push_back(std::pair<std::string, bool>(text.substr(last), false));
  }

  return ret;
}

} // namespace unity
} // namespace hud
} // namespace impl
