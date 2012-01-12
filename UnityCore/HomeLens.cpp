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

#include <glib.h>
#include <string>
#include <stdexcept>

#include "GLibSignal.h"
#include "HomeLens.h"
#include "Lens.h"

namespace unity
{
namespace dash
{

namespace
{

nux::logging::Logger logger("unity.dash.homelens");

}

class HomeLens::Impl : public sigc::trackable
{
public:
  Impl(HomeLens* owner);
  ~Impl();

  void OnLensAdded(Lens::Ptr& lens);
  void OnRowAdded(DeeModel* model, DeeModelIter* iter);
  //void OnRowRemoved(DeeModel* model, DeeModelIter* iter);
  //void OnRowChanged(DeeModel* model, DeeModelIter* iter);

  HomeLens* owner_;
  Lenses::LensList lenses_;
  glib::SignalManager sig_manager_;
  GVariant** row_buf_;
  unsigned int n_cols;
};

HomeLens::Impl::Impl(HomeLens *owner)
  : owner_(owner)
{
  n_cols = dee_model_get_n_columns(owner->results()->model());
  row_buf_= g_new0(GVariant*, n_cols);
}

HomeLens::Impl::~Impl()
{
  g_free(row_buf_);
}

// FIXME i18n _("Home") description, searchhint
// FIXME should use home icon

void HomeLens::Impl::OnLensAdded (Lens::Ptr& lens)
{
  typedef glib::Signal<void, DeeModel*, DeeModelIter*> RowSignalType;

  lenses_.push_back (lens);
  owner_->lens_added.emit(lens);

  sig_manager_.Add(new RowSignalType(lens->global_results()->model(),
                                     "row-added",
                                     sigc::mem_fun(this, &HomeLens::Impl::OnRowAdded)));
// FIXME row-removed and row-changed

}

void HomeLens::Impl::OnRowAdded(DeeModel* model, DeeModelIter* iter)
{
  printf ("ROW ADDED\n"); // FIXME

  dee_model_get_row (model, iter, row_buf_);
  dee_model_append_row (owner_->results()->model(), row_buf_);

  for (unsigned int i = 0; i < n_cols; i++) g_variant_unref(row_buf_[i]);
}

HomeLens::HomeLens()
  : Lens("home.lens", "", "", "Home", "iconhint",
         "description", "searchhint", true, "", ModelType::LOCAL)
  , pimpl(new Impl(this))
{
  count.SetGetterFunction(sigc::mem_fun(&pimpl->lenses_, &Lenses::LensList::size));
  search_in_global = false;

  /*global_results = Results::Ptr(new Results(ModelType::LOCAL));
  results = Results::Ptr(new Results(ModelType::LOCAL));
  categories = Categories::Ptr(new Categories(ModelType::LOCAL));
  filters = Filters::Ptr(new Filters(ModelType::LOCAL));*/
}

HomeLens::~HomeLens()
{
  delete pimpl;
}

void HomeLens::AddLenses(Lenses& lenses)
{
  for (auto lens : lenses.GetLenses())
  {
    pimpl->OnLensAdded(lens);
  }

  lenses.lens_added.connect(sigc::mem_fun(pimpl, &HomeLens::Impl::OnLensAdded));
}

Lenses::LensList HomeLens::GetLenses() const
{
  return pimpl->lenses_;
}

Lens::Ptr HomeLens::GetLens(std::string const& lens_id) const
{
  for (auto lens: pimpl->lenses_)
  {
    if (lens->id == lens_id)
    {
      return lens;
    }
  }

  return Lens::Ptr();
}

Lens::Ptr HomeLens::GetLensAtIndex(std::size_t index) const
{
  try
  {
    return pimpl->lenses_.at(index);
  }
  catch (std::out_of_range& error)
  {
    LOG_WARN(logger) << error.what();
  }

  return Lens::Ptr();
}

}
}
