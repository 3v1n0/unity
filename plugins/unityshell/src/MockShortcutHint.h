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

#ifndef UNITYSHELL_MOCK_SHORTCUT_HINT_H
#define UNITYSHELL_MOCK_SHORTCUT_HINT_H

#include "AbstractShortcutHint.h"

namespace unity
{
namespace shortcut
{

class MockHint : public AbstractHint
{
public:
   // Ctor and dtor
  MockHint(std::string const& category,
           std::string const& prefix,
           std::string const& postfix,
           std::string const& description,
           OptionType const type,
           std::string const& arg1, 
           std::string const& arg2 = "",
           std::string const& arg3 = "")
    : AbstractHint(category, prefix, postfix, description, type, arg1, arg2, arg3)
  {
  }
  
  ~MockHint() {};
  
  // Methods...
  bool Fill()
  {
    switch (type())
    {
      case COMPIZ_MOUSE_OPTION:
      case COMPIZ_KEY_OPTION:
      case COMPIZ_METAKEY_OPTION:
        value = arg1() + "-" + arg2();
        shortkey = prefix() + value() + postfix();
        return true;
      
      case HARDCODED_OPTION:
        value = arg1();
        shortkey = prefix() + value() + postfix();
        return true;
    }
    
    return false;
  }
};
  
} // shortcut hint
} // namespace unity

#endif // UNITYSHELL_MOCK_SHORTCUT_HINT_H
