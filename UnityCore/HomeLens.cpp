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
#include "Model.h"

namespace unity
{
namespace dash
{

namespace
{

nux::logging::Logger logger("unity.dash.homelens");

}

class HomeLens::ModelMerger : public sigc::trackable
{
public:
  ModelMerger(glib::Object<DeeModel> target);
  ~ModelMerger();

  void AddSource(glib::Object<DeeModel> source);

private:
  void OnSourceRowAdded(DeeModel *model, DeeModelIter *iter);
  //void OnSourceRowRemoved(DeeModel* model, DeeModelIter* iter);
  //void OnSourceRowChanged(DeeModel* model, DeeModelIter* iter);
  void EnsureRowBuf(DeeModel *source);

private:
  glib::SignalManager sig_manager_;
  GVariant** row_buf_;
  unsigned int n_cols_;
  glib::Object<DeeModel> target_;
};

class HomeLens::Impl : public sigc::trackable
{
public:
  Impl(HomeLens* owner);
  ~Impl();

  void OnLensAdded(Lens::Ptr& lens);
  void OnGlobalResultsModelChanged(glib::Object<DeeModel> model);
  //HomeLens::Impl::OnGlobalResultsModelChanged
  //HomeLens::Impl::OnGlobalResultsModelChanged

  HomeLens* owner_;
  Lenses::LensList lenses_;
  HomeLens::ModelMerger results_merger_;
  HomeLens::ModelMerger categories_merger_;
  HomeLens::ModelMerger filters_merger_;
};

HomeLens::ModelMerger::ModelMerger(glib::Object<DeeModel> target)
  : target_(target)
{}

HomeLens::ModelMerger::~ModelMerger()
{
  if (row_buf_)
    delete row_buf_;
}

void HomeLens::ModelMerger::AddSource(glib::Object<DeeModel> source)
{
  typedef glib::Signal<void, DeeModel*, DeeModelIter*> RowSignalType;

  if (source.RawPtr() == NULL)
  {
    LOG_ERROR(logger) << "Trying to add NULL source to ModelMerger";
    return;
  }

  // FIXME row-removed and row-changed
  sig_manager_.Add(new RowSignalType(source.RawPtr(),
                                     "row-added",
                                     sigc::mem_fun(this, &HomeLens::ModelMerger::OnSourceRowAdded)));
}

void HomeLens::ModelMerger::OnSourceRowAdded(DeeModel *model, DeeModelIter *iter)
{
  EnsureRowBuf(model);

  dee_model_get_row (model, iter, row_buf_);
  dee_model_append_row (target_, row_buf_);

  for (unsigned int i = 0; i < n_cols_; i++) g_variant_unref(row_buf_[i]);
}

void HomeLens::ModelMerger::EnsureRowBuf(DeeModel *model)
{
  if (G_UNLIKELY (n_cols_ == 0))
  {
    n_cols_ = dee_model_get_n_columns(model);

    if (n_cols_ == 0)
    {
      LOG_ERROR(logger) << "Source model has not provided a schema for the model merger!";
      return;
    }

    /* Lazily adopt schema from source if we don't have one.
     * If we do have a schema let's validate that they match the source */
    if (dee_model_get_n_columns(target_) == 0)
    {
      dee_model_set_schema_full(target_,
                                dee_model_get_schema(model, NULL),
                                n_cols_);
    }
    else
    {
      unsigned int n_cols1;
      const gchar* const *schema1 = dee_model_get_schema(target_, &n_cols1);
      const gchar* const *schema2 = dee_model_get_schema(model, NULL);

      if (n_cols_ != n_cols1)
      {
        LOG_ERROR(logger) << "Schema mismatch between source and target model. Expected "
                          << n_cols1 << " columns, but found"
                          << n_cols_ << ".";
        return;
      }

      for (unsigned int i = 0; i < n_cols_; i++)
      {
        if (g_strcmp0(schema1[i], schema2[i]) != 0)
        {
          LOG_ERROR(logger) << "Schema mismatch between source and target model. Expected column "
                            << i << " to be '" << schema1[i] << "', but found '"
                            << schema2[i] << "'.";
          return;
        }
      }
    }

    row_buf_ = g_new0 (GVariant*, n_cols_);
  }
}

HomeLens::Impl::Impl(HomeLens *owner)
  : owner_(owner)
  , results_merger_(owner->results()->model())
  , categories_merger_(owner->categories()->model())
  , filters_merger_(owner->filters()->model())
{
  DeeModel* model = owner->results()->model();
  bool has_results_schema = dee_model_get_n_columns(model) > 0;

  if (!has_results_schema)
  {
    dee_model_set_schema(model, "s", "s", "u", "s", "s", "s", "s", NULL);
  }

  // FIXME filters and categories schemas
}

HomeLens::Impl::~Impl()
{

}

// FIXME i18n _("Home") description, searchhint
// FIXME should use home icon

void HomeLens::Impl::OnLensAdded (Lens::Ptr& lens)
{
  lenses_.push_back (lens);
  owner_->lens_added.emit(lens);

  nux::ROProperty<glib::Object<DeeModel>>& results_prop = lens->global_results()->model;
  nux::ROProperty<glib::Object<DeeModel>>& categories_prop = lens->categories()->model;
  nux::ROProperty<glib::Object<DeeModel>>& filters_prop = lens->filters()->model;

  /* Most lenses add models lazily, but we can't know that;
   * so try to see if we can add them up front */
  if (results_prop().RawPtr())
    results_merger_.AddSource(results_prop());

  if (categories_prop().RawPtr())
    categories_merger_.AddSource(categories_prop());

  if (filters_prop().RawPtr())
    filters_merger_.AddSource(filters_prop());

  // FIXME
  results_prop.changed.connect(sigc::mem_fun(this, &HomeLens::Impl::OnGlobalResultsModelChanged));
  /*categories_prop.changed.connect(sigc::mem_fun(this, &HomeLens::Impl::OnCategoriesModelChanged));
  filters_prop.changed.connect(sigc::mem_fun(this, &HomeLens::Impl::OnFiltersModelChanged));*/
}

void HomeLens::Impl::OnGlobalResultsModelChanged(glib::Object<DeeModel> model)
{
  results_merger_.AddSource(model);
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
