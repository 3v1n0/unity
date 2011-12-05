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
 
#ifndef UNITYSHELL_SHORTCUTHINT_H
#define UNITYSHELL_SHORTCUTHINT_H

#include "AbstractShortcutHint.h"

namespace unity
{
namespace shortcut
{

class Hint : public AbstractHint
{
public:
  // Ctor and dtor
  // TODO: Can i move the costructor into the abstract hint class?
  Hint(std::string category, std::string prefix,
       std::string postfix, std::string description,
       OptionType type, std::string arg1, 
       std::string arg2 = "", std::string arg3 = "");
  ~Hint();

  // Copy ctor
  Hint(unity::shortcut::Hint const& obj);

  // Public methods
  bool Fill();
};

} // namespace shortcut
} // namespace unity

 #endif // UNITYSHELL_SHORTCUTHINT_H
