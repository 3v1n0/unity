// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012 Canonical Ltd
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
 * Authored by: Mikkel Kamstrup Erlandsen <mikkel.kamstrup@canonical.com>
 */

#ifndef UNITY_HOME_LENS_H
#define UNITY_HOME_LENS_H

#include <vector>
#include <memory>
#include <sigc++/signal.h>
#include <sigc++/trackable.h>

#include "Lenses.h"
#include "Lens.h"

namespace unity
{
namespace dash
{

class HomeLens : public Lens, public Lenses
{
public:
  typedef std::shared_ptr<HomeLens> Ptr;

  HomeLens();
  virtual ~HomeLens();

  void AddLenses(Lenses& lenses);

  Lenses::LensList GetLenses() const;
  Lens::Ptr GetLens(std::string const& lens_id) const;
  Lens::Ptr GetLensAtIndex(std::size_t index) const;

private:
  class Impl;
  class ModelMerger;
  Impl* pimpl;
};

}
}

#endif
