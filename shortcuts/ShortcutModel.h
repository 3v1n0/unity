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
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 */

#ifndef UNITYSHELL_SHORTCUS_MODEL_H
#define UNITYSHELL_SHORTCUS_MODEL_H

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

  Model(std::list<AbstractHint::Ptr> const& hints);

  nux::Property<int> categories_per_column;
  std::vector<std::string> const& categories() const { return categories_; }
  std::map<std::string, std::list<AbstractHint::Ptr>> const& hints() const { return hints_; }

  void Fill();

private:
  void AddHint(AbstractHint::Ptr const& hint);

  std::vector<std::string> categories_;
  std::map<std::string, std::list<AbstractHint::Ptr>> hints_;
};

}
}

#endif
