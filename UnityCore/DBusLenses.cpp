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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#include "DBusLenses.h"

namespace unity {
namespace dash {

class DBusLenses::Impl
{
public:
  Impl(DBusLenses* owner);
  ~Impl();

  LensList GetLenses() const;
  Lens::Ptr GetLens(std::string const& lens_id) const;
  Lens::Ptr GetLensAtIndex(unsigned int index) const;
  unsigned int TotalLenses() const;

  DBusLenses* owner_;
};

DBusLenses::Impl::Impl(DBusLenses* owner)
  : owner_(owner)
{}

DBusLenses::Impl::~Impl()
{}

Lenses::LensList DBusLenses::Impl::GetLenses() const
{
  LensList list;
  return list;
}

Lens::Ptr DBusLenses::Impl::GetLens(std::string const& lens_id) const
{
  Lens::Ptr p;
  return p;
}

Lens::Ptr DBusLenses::Impl::GetLensAtIndex(unsigned int index) const
{
  Lens::Ptr p;
  return p;
}

unsigned int DBusLenses::Impl::TotalLenses() const
{
  return 0;
}


DBusLenses::DBusLenses()
  : pimpl(new Impl(this))
{}

DBusLenses::~DBusLenses()
{
  delete pimpl;
}

Lenses::LensList DBusLenses::GetLenses() const
{
  return pimpl->GetLenses();
}

Lens::Ptr DBusLenses::GetLens(std::string const& lens_id) const
{
  return pimpl->GetLens(lens_id);
}

Lens::Ptr DBusLenses::GetLensAtIndex(unsigned int index) const
{
  return pimpl->GetLensAtIndex(index);
}

unsigned int DBusLenses::TotalLenses() const
{
  return pimpl->TotalLenses();
}


}
}
