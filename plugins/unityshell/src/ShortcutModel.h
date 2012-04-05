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

#ifndef UNITYSHELL_SHORTCUSMODEL_H
#define UNITYSHELL_SHORTCUSMODEL_H

#include <boost/noncopyable.hpp>
#include <map>
#include <memory>
#include <list>
#include <string>
#include <vector>

#include "AbstractShortcutHint.h"

namespace unity
{
namespace shortcut
{

class Model : boost::noncopyable
{
public:
  typedef std::shared_ptr<Model> Ptr;

  // Ctor and dtor
  Model(std::list<AbstractHint::Ptr>& hints);
  ~Model();

  // Accessors
  std::vector<std::string>& categories() { return categories_; }
  std::map<std::string, std::list<AbstractHint::Ptr>>& hints() { return hints_; }

  void Fill();

private:
  // Private functions
  void AddHint(AbstractHint::Ptr hint);

  // Private members
  std::vector<std::string> categories_;
  std::map<std::string, std::list<AbstractHint::Ptr>> hints_;
};

} // shortcut
} // unity

#endif // UNITYSHELL_SHORTCUTS_H
