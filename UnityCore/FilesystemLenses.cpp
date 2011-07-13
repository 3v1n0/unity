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

#include "FilesystemLenses.h"

namespace unity {
namespace dash {

class FilesystemLenses::Impl
{
public:
  Impl(FilesystemLenses* owner);
  Impl(FilesystemLenses* owner, std::string const& lens_directory);

  ~Impl();

  LensList GetLenses() const;
  Lens::Ptr GetLens(std::string const& lens_id) const;
  Lens::Ptr GetLensAtIndex(unsigned int index) const;
  unsigned int LensCount() const;

  FilesystemLenses* owner_;
};

FilesystemLenses::Impl::Impl(FilesystemLenses* owner)
  : owner_(owner)
{}

FilesystemLenses::Impl::Impl(FilesystemLenses* owner, std::string const& lens_directory)
{}

FilesystemLenses::Impl::~Impl()
{}

Lenses::LensList FilesystemLenses::Impl::GetLenses() const
{
  LensList list;
  return list;
}

Lens::Ptr FilesystemLenses::Impl::GetLens(std::string const& lens_id) const
{
  Lens::Ptr p;
  return p;
}

Lens::Ptr FilesystemLenses::Impl::GetLensAtIndex(unsigned int index) const
{
  Lens::Ptr p;
  return p;
}

unsigned int FilesystemLenses::Impl::LensCount() const
{
  return 0;
}


FilesystemLenses::FilesystemLenses()
  : pimpl(new Impl(this))
{}

FilesystemLenses::FilesystemLenses(std::string const& lens_directory)
  :pimpl(new Impl(this, lens_directory))
{}

FilesystemLenses::~FilesystemLenses()
{
  delete pimpl;
}

Lenses::LensList FilesystemLenses::GetLenses() const
{
  return pimpl->GetLenses();
}

Lens::Ptr FilesystemLenses::GetLens(std::string const& lens_id) const
{
  return pimpl->GetLens(lens_id);
}

Lens::Ptr FilesystemLenses::GetLensAtIndex(unsigned int index) const
{
  return pimpl->GetLensAtIndex(index);
}

unsigned int FilesystemLenses::LensCount() const
{
  return pimpl->LensCount();
}


}
}
