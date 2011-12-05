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

#ifndef UNITYSHELL_ABSTRACT_SHORTCUT_ICON_H
#define UNITYSHELL_ABSTRACT_SHORTCUT_ICON_H

#include <string>

#include <Nux/Nux.h>
#include <NuxCore/Property.h>

namespace unity
{
namespace shortcut
{

enum OptionType
{
  COMPIZ_OPTION = 0,
  HARDCODED_OPTION
  /* GSETTINGS_OPTION,
   * GCONF_OPTION */
};

class AbstractHint
{
public:
  // Dtor
  virtual ~AbstractHint(){};
  
  // Public Methods
  virtual bool Fill() = 0;
  
  // Properties
  nux::Property<std::string> Category;
  nux::Property<std::string> Prefix;
  nux::Property<std::string> Postfix;
  nux::Property<std::string> Description;
  nux::Property<OptionType> Type;
  nux::Property<std::string> Arg1;
  nux::Property<std::string> Arg2;
  nux::Property<std::string> Arg3;
  nux::Property<std::string> Value;
};

} // namespace shortcut
} // namespace unity

#endif // UNITYSHELL_ABSTRACT_SHORTCUT_ICON_H
