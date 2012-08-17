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

/**
 * A special Lens implementation that merges together a set of source Lens
 * instances.
 *
 * NOTE: Changes in the filter models are currently not propagated back to the
 *       the source lenses. If we want to support filters on the dash home
 *       screen this needs to be addressed.
 */
class HomeLens : public Lens, public Lenses
{
public:
  typedef std::shared_ptr<HomeLens> Ptr;

  /* Specifies mode for category merging */
  enum MergeMode
  {
    DISPLAY_NAME,
    OWNER_LENS
  };

  /**
   * Should be constructed with i18n arguments:
   *                         _("Home"), _("Home screen"), _("Search")
   */
  HomeLens(std::string const& name,
           std::string const& description,
           std::string const& search_hint,
           MergeMode merge_mode = MergeMode::OWNER_LENS);
  virtual ~HomeLens();

  void AddLenses(Lenses& lenses);

  Lenses::LensList GetLenses() const;
  Lens::Ptr GetLens(std::string const& lens_id) const;
  Lens::Ptr GetLensAtIndex(std::size_t index) const;

  void GlobalSearch(std::string const& search_string);
  void Search(std::string const& search_string);
  void Activate(std::string const& uri);
  void Preview(std::string const& uri);

  std::vector<unsigned> GetCategoriesOrder();

private:
  class Impl;
  class ModelMerger;
  class ResultsMerger;
  class CategoryMerger;
  class FiltersMerger;
  class CategoryRegistry;
  Impl* pimpl;
};

}
}

#endif
