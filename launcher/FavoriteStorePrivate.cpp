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
* Authored by: Andrea Azzaronea <azzaronea@gmail.com>
*/

#include <algorithm>
#include <boost/utility.hpp>

#include "FavoriteStorePrivate.h"

namespace unity
{
namespace internal
{
namespace impl
{

std::vector<std::string> GetNewbies(std::list<std::string> const& old, std::list<std::string> const& fresh)
{
  auto sorted_old(old);
  auto sorted_fresh(fresh);

  sorted_old.sort();
  sorted_fresh.sort();

  std::vector<std::string> result;
  std::set_difference(sorted_fresh.begin(), sorted_fresh.end(), sorted_old.begin(), sorted_old.end(),
                      std::inserter(result, result.end()));

  return result;
}

void GetSignalAddedInfo(std::list<std::string> const& favs, std::vector<std::string> const& newbies,
                        std::string const& path, std::string& position, bool& before)
{
  auto it = std::find(favs.begin(), favs.end(), path);
  before = (it == favs.begin());
  position = "";

  if (before and favs.size() > 1)
  {
    while (it != favs.end() && std::find(newbies.begin(), newbies.end(), *it) != newbies.end())
      ++it;

    if (it != favs.end())
      position = *it;
  }
  else if (!before)
  {
    position = *(boost::prior(it));
  }

}

std::vector<std::string> GetRemoved(std::list<std::string> const& old, std::list<std::string> const& fresh)
{
  auto sorted_old(old);
  auto sorted_fresh(fresh);

  sorted_old.sort();
  sorted_fresh.sort();

  std::vector<std::string> result;
  std::set_difference(sorted_old.begin(), sorted_old.end(), sorted_fresh.begin(), sorted_fresh.end(),
                      std::inserter(result, result.end()));

  return result;
}


bool NeedToBeReordered(std::list<std::string> const& old, std::list<std::string> const& fresh)
{
  auto sorted_old(old);
  auto sorted_fresh(fresh);

  sorted_old.sort();
  sorted_fresh.sort();

  std::vector<std::string> ignore_old, ignore_fresh;

  std::set_difference(sorted_old.begin(), sorted_old.end(), sorted_fresh.begin(), sorted_fresh.end(),
                      std::inserter(ignore_old, ignore_old.end()));
  std::set_difference(sorted_fresh.begin(), sorted_fresh.end(), sorted_old.begin(), sorted_old.end(),
                      std::inserter(ignore_fresh, ignore_fresh.end()));

  auto it_old = old.begin();
  auto it_fresh = fresh.begin();

  while (it_old != old.end() && it_fresh != fresh.end())
  {

    while (it_old != old.end() && std::find(ignore_old.begin(), ignore_old.end(), *it_old) != ignore_old.end())
      ++it_old;

    while (it_fresh != fresh.end() && std::find(ignore_fresh.begin(), ignore_fresh.end(), *it_fresh) != ignore_fresh.end())
      ++it_fresh;

    if (it_old == old.end() || it_fresh == fresh.end())
      break;

    if (*it_old != *it_fresh)
    {
      return true;
    }

    ++it_old;
    ++it_fresh;
  }

  return false;
}

bool IsDesktopFilePath(std::string const& path)
{
  static const std::string desktop_ext = ".desktop";
  auto path_len = path.length();
  auto desktop_length = desktop_ext.length();

  if (path_len > desktop_length)
  {
    return path.compare(path_len - desktop_length, desktop_length, desktop_ext) == 0;
  }

  return false;
}


} // namespace impl
} // namespace internal
} // namespace unity
