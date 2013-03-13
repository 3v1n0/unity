// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2013 Canonical Ltd
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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 */

 #ifndef _SERVICE_SCOPE_H_
#define _SERVICE_SCOPE_H_

#include "test_scope_impl.h"
#include <UnityCore/GLibWrapper.h>

namespace unity
{
namespace service
{

class Scope
{
public:
  Scope(std::string const& scope_id);
  ~Scope();

private:
  void AddCategories();
  void AddFilters();

private:
  glib::Object<TestScope> scope_;
};

}
}

#endif /* _SERVICE_SCOPE_H_ */
