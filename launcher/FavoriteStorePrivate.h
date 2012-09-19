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

#ifndef UNITYSHELL_FAVORITESTOREPRIVATE_H
#define UNITYSHELL_FAVORITESTOREPRIVATE_H

#include <list>
#include <string>

namespace unity
{
namespace internal
{
namespace impl
{

std::vector<std::string> GetNewbies(std::list<std::string> const& old, std::list<std::string> const& fresh);

void GetSignalAddedInfo(std::list<std::string> const& favs, std::vector<std::string> const& newbies,
                        std::string const& path, std::string& position, bool& before);

std::vector<std::string> GetRemoved(std::list<std::string> const& old, std::list<std::string> const& fresh);

bool NeedToBeReordered(std::list<std::string> const& old, std::list<std::string> const& fresh);

bool IsDesktopFilePath(std::string const& path);


} // namespace impl
} // namespace internal
} // namespace unity

#endif

