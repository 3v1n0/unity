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
  COMPIZ_KEY_OPTION = 0,
  COMPIZ_METAKEY_OPTION,
  COMPIZ_MOUSE_OPTION,
  HARDCODED_OPTION
  /* GSETTINGS_OPTION,
   * GCONF_OPTION */
};

class AbstractHint
{
public:
  typedef std::shared_ptr<AbstractHint> Ptr;
  // Ctor
  AbstractHint(std::string const& category,
               std::string const& prefix,
               std::string const& postfix,
               std::string const& description,
               OptionType const type,
               std::string const& arg1,
               std::string const& arg2 = "",
               std::string const& arg3 = "")
    : category(category)
    , prefix(prefix)
    , postfix(postfix)
    , description(description)
    , type(type)
    , arg1(arg1)
    , arg2(arg2)
    , arg3(arg3)
  {
  }

  // Copy ctor
  AbstractHint(unity::shortcut::AbstractHint const& obj)
    : category(obj.category())
    , prefix(obj.prefix())
    , postfix(obj.postfix())
    , description(obj.description())
    , type(obj.type())
    , arg1(obj.arg1())
    , arg2(obj.arg2())
    , arg3(obj.arg3())
    , value(obj.value())
    , shortkey(obj.shortkey())
  {
  }

  // Dtor
  virtual ~AbstractHint(){};

  // Public Methods
  virtual bool Fill() = 0;

  // Properties
  nux::Property<std::string> category;
  nux::Property<std::string> prefix;
  nux::Property<std::string> postfix;
  nux::Property<std::string> description;
  nux::Property<OptionType> type;
  nux::Property<std::string> arg1;
  nux::Property<std::string> arg2;
  nux::Property<std::string> arg3;
  nux::Property<std::string> value;
  nux::Property<std::string> shortkey;
};

} // namespace shortcut
} // namespace unity

#endif // UNITYSHELL_ABSTRACT_SHORTCUT_ICON_H
