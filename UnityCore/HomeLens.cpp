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
#include <map>

#include "GLibSignal.h"
#include "HomeLens.h"
#include "Lens.h"
#include "Model.h"

#include "config.h"

namespace unity
{
namespace dash
{

namespace
{

nux::logging::Logger logger("unity.dash.homelens");

}

/*
 * Helper class that maps category offsets between the merged lens and
 * source lenses. We also use it to merge categories from different lenses
 * with the same display name into the same category.
 *
 * NOTE: The model pointers passed in are expected to be pointers to the
 *       results models - and not the category models!
 */
class HomeLens::CategoryRegistry
{
public:
  CategoryRegistry() {}

  int FindCategoryOffset(DeeModel* model, unsigned int source_cat_offset)
  {
    gchar* c_id = g_strdup_printf("%u+%p", source_cat_offset, model);
    std::map<std::string,unsigned int>::iterator i = reg_by_id_.find(c_id);
    g_free(c_id);

    if (i != reg_by_id_.end())
      return i->second;

    return -1;
  }

  int FindCategoryOffset(const gchar* display_name)
    {
      std::map<std::string,unsigned int>::iterator i =
                                        reg_by_display_name_.find(display_name);
      if (i != reg_by_display_name_.end())
        return i->second;

      return -1;
    }

  void RegisterCategoryOffset(DeeModel*     model,
                                 unsigned int source_cat_offset,
                                 const gchar*  display_name,
                                 unsigned int target_cat_offset)
  {
    gchar* c_id = g_strdup_printf("%u+%p", source_cat_offset, model);

    std::map<std::string,unsigned int>::iterator i = reg_by_id_.find(c_id);
    if (i != reg_by_id_.end())
    {
      LOG_ERROR(logger) << "Category '" << c_id << "' already registered!";
      return;
    }

    reg_by_id_[c_id] = target_cat_offset;

    /* Callers pass a NULL display_name when they already have a category
     * with the right display registered */
    if (display_name != NULL)
    {
      reg_by_display_name_[display_name] = target_cat_offset;
      LOG_DEBUG(logger) << "Registered category '" << display_name
                        << "' with source offset " << source_cat_offset
                        << " and target offset " << target_cat_offset
                        << ". Id " << c_id;
    }
    else
    {
      LOG_DEBUG(logger) << "Registered category with source offset "
                        << source_cat_offset << " and target offset "
                        << target_cat_offset << ". Id " << c_id;
    }

    g_free(c_id);
  }

private:
  std::map<std::string,unsigned int> reg_by_id_;
  std::map<std::string,unsigned int> reg_by_display_name_;
};

/*
 * Helper class that merges a set of DeeModels into one super model
 */
class HomeLens::ModelMerger : public sigc::trackable
{
public:
  ModelMerger(glib::Object<DeeModel> target);
  virtual ~ModelMerger();

  void AddSource(glib::Object<DeeModel> source);

protected:
  virtual void OnSourceRowAdded(DeeModel *model, DeeModelIter *iter);
  virtual void OnSourceRowRemoved(DeeModel* model, DeeModelIter* iter);
  virtual void OnSourceRowChanged(DeeModel* model, DeeModelIter* iter);
  void EnsureRowBuf(DeeModel *source);

  /* The merge tag lives on the source models, pointing to the mapped
   * row in the target model */
  DeeModelTag* FindMergerTag(DeeModel *model);
  DeeModelTag* RegisterMergerTag(DeeModel *model);

protected:
  glib::SignalManager sig_manager_;
  GVariant** row_buf_;
  unsigned int n_cols_;
  glib::Object<DeeModel> target_;
  std::map<DeeModel*,DeeModelTag*> merger_tags_;
};

/*
 * Specialized ModelMerger that takes care merging results models.
 * We need special handling here because rows in each lens' results model
 * specifies an offset into the lens' categories model where the display
 * name of the category is defined.
 *
 * This class converts the offset of the source lens' categories into
 * offsets into the merged category model.
 *
 * Each row added to the target is tagged with a pointer to the Lens instance
 * from which it came
 */
class HomeLens::ResultsMerger : public ModelMerger
{
public:
  ResultsMerger(glib::Object<DeeModel> target,
                  HomeLens::CategoryRegistry* cat_registry);

protected:
  void OnSourceRowAdded(DeeModel *model, DeeModelIter *iter);

private:
  HomeLens::CategoryRegistry* cat_registry_;
};

/*
 * Specialized ModelMerger that takes care merging category models.
 * We need special handling here because rows in each lens' results model
 * specifies an offset into the lens' categories model where the display
 * name of the category is defined.
 *
 * This class records a map of the offsets from the original source category
 * models to the offsets in the combined categories model.
 */
class HomeLens::CategoryMerger : public ModelMerger
{
public:
  CategoryMerger(glib::Object<DeeModel> target,
                   HomeLens::CategoryRegistry* cat_registry);

protected:
  void OnSourceRowAdded(DeeModel *model, DeeModelIter *iter);

private:
  HomeLens::CategoryRegistry* cat_registry_;
};

/*
 * Pimpl for HomeLens
 */
class HomeLens::Impl : public sigc::trackable
{
public:
  Impl(HomeLens* owner);
  ~Impl();

  void OnLensAdded(Lens::Ptr& lens);
  void EnsureCategoryAnnotation (DeeModel* results, DeeModel* categories);
  Lens::Ptr FindLensForUri(std::string const& uri);

  HomeLens* owner_;
  Lenses::LensList lenses_;
  HomeLens::CategoryRegistry cat_registry_;
  HomeLens::ResultsMerger results_merger_;
  HomeLens::CategoryMerger categories_merger_;
  HomeLens::ModelMerger filters_merger_;
};

/*
 * IMPLEMENTATION
 */

HomeLens::ModelMerger::ModelMerger(glib::Object<DeeModel> target)
  : n_cols_(0)
  , target_(target)
{}

HomeLens::ResultsMerger::ResultsMerger(glib::Object<DeeModel> target,
                                            CategoryRegistry *cat_registry)
  : HomeLens::ModelMerger::ModelMerger(target)
  , cat_registry_(cat_registry)
{}

HomeLens::CategoryMerger::CategoryMerger(glib::Object<DeeModel> target,
                                              CategoryRegistry *cat_registry)
  : HomeLens::ModelMerger::ModelMerger(target)
  , cat_registry_(cat_registry)
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

  RegisterMergerTag(source);

  sig_manager_.Add(new RowSignalType(source.RawPtr(),
                                     "row-added",
                                     sigc::mem_fun(this, &HomeLens::ModelMerger::OnSourceRowAdded)));

  sig_manager_.Add(new RowSignalType(source.RawPtr(),
                                       "row-removed",
                                       sigc::mem_fun(this, &HomeLens::ModelMerger::OnSourceRowRemoved)));

  sig_manager_.Add(new RowSignalType(source.RawPtr(),
                                       "row-changed",
                                       sigc::mem_fun(this, &HomeLens::ModelMerger::OnSourceRowChanged)));
}

void HomeLens::ModelMerger::OnSourceRowAdded(DeeModel *model, DeeModelIter *iter)
{
  DeeModelIter* target_iter;
  DeeModelTag*  target_tag;

  EnsureRowBuf(model);

  dee_model_get_row (model, iter, row_buf_);
  target_tag = FindMergerTag(model);

  /* NOTE: This is a silly merge strategy (always appending).
   * Consider some clever sorting? */
  target_iter = dee_model_append_row (target_, row_buf_);
  dee_model_set_tag (model, iter, target_tag, target_iter);

  for (unsigned int i = 0; i < n_cols_; i++) g_variant_unref(row_buf_[i]);
}

void HomeLens::ResultsMerger::OnSourceRowAdded(DeeModel *model, DeeModelIter *iter)
{
  DeeModelIter* target_iter;
  DeeModelTag*  target_tag;
  int target_cat_offset, source_cat_offset;
  const unsigned int CATEGORY_COLUMN = 2;

  EnsureRowBuf(model);

  dee_model_get_row (model, iter, row_buf_);
  target_tag = FindMergerTag(model);

  /* Update the row with the corrected category offset */
  source_cat_offset = dee_model_get_uint32(model, iter, CATEGORY_COLUMN);
  target_cat_offset = cat_registry_->FindCategoryOffset(model, source_cat_offset);

  if (target_cat_offset >= 0)
  {
    g_variant_unref (row_buf_[CATEGORY_COLUMN]);
    row_buf_[CATEGORY_COLUMN] = g_variant_new_uint32(target_cat_offset);

    /* Sink the ref on the new row member. By Dee API contract they must all
     * be strong refs, not floating */
    g_variant_ref_sink(row_buf_[CATEGORY_COLUMN]);

    target_iter = dee_model_append_row (target_, row_buf_);
    dee_model_set_tag(model, iter, target_tag, target_iter);

    LOG_DEBUG(logger) << "Found " << dee_model_get_string(model, iter, 0)
                      << " (source cat " << source_cat_offset << ", target cat "
                      << target_cat_offset << ")";
  }
  else
  {
    LOG_ERROR(logger) << "No category registered for model "
                      << model << ", offset " << source_cat_offset << ".";
  }

  for (unsigned int i = 0; i < n_cols_; i++) g_variant_unref(row_buf_[i]);
}

void HomeLens::CategoryMerger::OnSourceRowAdded(DeeModel *model, DeeModelIter *iter)
{
  DeeModel* results_model;
  DeeModelIter* target_iter;
  DeeModelTag*  target_tag;
  int target_cat_offset, source_cat_offset;
  const gchar* display_name;
  const unsigned int DISPLAY_NAME_COLUMN = 0;

  EnsureRowBuf(model);

  results_model = static_cast<DeeModel*>(g_object_get_data(
                              G_OBJECT(model), "unity-homelens-results-model"));
  if (results_model == NULL)
  {
    LOG_DEBUG(logger) << "Category model " << model
                      << " does not have a results model yet";
    return;
  }

  dee_model_get_row (model, iter, row_buf_);
  target_tag = FindMergerTag(model);
  source_cat_offset = dee_model_get_position(model, iter);

  /* If we already have a category registered with the same display name
   * then we just use that. Otherwise register a new category for it */
  display_name = dee_model_get_string(model, iter, DISPLAY_NAME_COLUMN);
  target_cat_offset = cat_registry_->FindCategoryOffset(display_name);
  if (target_cat_offset >= 0)
  {
    cat_registry_->RegisterCategoryOffset(results_model, source_cat_offset,
                                          NULL, target_cat_offset);
  }
  else
  {
    target_iter = dee_model_append_row (target_, row_buf_);
    dee_model_set_tag (model, iter, target_tag, target_iter);
    target_cat_offset = dee_model_get_position(target_, target_iter);
    cat_registry_->RegisterCategoryOffset(results_model, source_cat_offset,
                                          display_name, target_cat_offset);
  }

  for (unsigned int i = 0; i < n_cols_; i++) g_variant_unref(row_buf_[i]);
}

void HomeLens::ModelMerger::OnSourceRowRemoved(DeeModel *model, DeeModelIter *iter)
{
  DeeModelIter* target_iter;
  DeeModelTag*  target_tag;

  EnsureRowBuf(model);

  target_tag = FindMergerTag(model);
  target_iter = static_cast<DeeModelIter*>(dee_model_get_tag(model,
                                                                iter,
                                                                target_tag));

  /* We might not have registered a target iter for the row.
   * This fx. happens if we re-used a category based on display_name */
  if (target_iter != NULL)
    dee_model_remove(target_, target_iter);
}

void HomeLens::ModelMerger::OnSourceRowChanged(DeeModel *model, DeeModelIter *iter)
{
  DeeModelIter* target_iter;
  DeeModelTag*  target_tag;

  EnsureRowBuf(model);

  dee_model_get_row (model, iter, row_buf_);
  target_tag = FindMergerTag(model);
  target_iter = static_cast<DeeModelIter*>(dee_model_get_tag(model,
                                                                 iter,
                                                                 target_tag));

  dee_model_set_row (target_, target_iter, row_buf_);

  for (unsigned int i = 0; i < n_cols_; i++) g_variant_unref(row_buf_[i]);
}

void HomeLens::ModelMerger::EnsureRowBuf(DeeModel *model)
{
  if (G_UNLIKELY (n_cols_ == 0))
  {
    /* We have two things to accomplish here.
     * 1) Allocate the row_buf_, and
     * 2) Make sure that the target model has the correct schema set.
     *
     * INVARIANT: n_cols_ == 0 iff row_buf_ == NULL.
     */

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

      /* At the very least we should have an equal number of rows */
      if (n_cols_ != n_cols1)
      {
        LOG_ERROR(logger) << "Schema mismatch between source and target model. Expected "
                          << n_cols1 << " columns, but found"
                          << n_cols_ << ".";
        n_cols_ = 0;
        return;
      }

      /* Compare schemas */
      for (unsigned int i = 0; i < n_cols_; i++)
      {
        if (g_strcmp0(schema1[i], schema2[i]) != 0)
        {
          LOG_ERROR(logger) << "Schema mismatch between source and target model. Expected column "
                            << i << " to be '" << schema1[i] << "', but found '"
                            << schema2[i] << "'.";
          n_cols_ = 0;
          return;
        }
      }
    }

    row_buf_ = g_new0 (GVariant*, n_cols_);
  }
}

DeeModelTag* HomeLens::ModelMerger::FindMergerTag(DeeModel *model)
{
  return merger_tags_[model];
}

DeeModelTag* HomeLens::ModelMerger::RegisterMergerTag(DeeModel *model)
{
  DeeModelTag* merger_tag = dee_model_register_tag(model, NULL);
  merger_tags_[model] = merger_tag;
  return merger_tag;
}

HomeLens::Impl::Impl(HomeLens *owner)
  : owner_(owner)
  , results_merger_(owner->results()->model(), &cat_registry_)
  , categories_merger_(owner->categories()->model(), &cat_registry_)
  , filters_merger_(owner->filters()->model())
{
  DeeModel* results = owner->results()->model();
  if (dee_model_get_n_columns(results) == 0)
  {
    dee_model_set_schema(results, "s", "s", "u", "s", "s", "s", "s", NULL);
  }

  DeeModel* categories = owner->categories()->model();
  if (dee_model_get_n_columns(categories) == 0)
  {
    dee_model_set_schema(categories, "s", "s", "s", "a{sv}", NULL);
  }

  DeeModel* filters = owner->filters()->model();
  if (dee_model_get_n_columns(filters) == 0)
  {
    dee_model_set_schema(filters, "s", "s", "s", "s", "a{sv}", "b", "b", "b", NULL);
  }
}

HomeLens::Impl::~Impl()
{

}

void HomeLens::Impl::EnsureCategoryAnnotation (DeeModel* categories,
                                                     DeeModel* results)
{
  if (categories && results)
  {
    if (!(DEE_IS_MODEL(results) && DEE_IS_MODEL(categories)))
    {
      LOG_ERROR(logger) << "The "
                        << std::string(DEE_IS_MODEL(results) ? "categories" : "results")
                        << " model is not a valid DeeModel";
      return;
    }

    g_object_set_data(G_OBJECT(categories),
                      "unity-homelens-results-model",
                      results);

    LOG_DEBUG(logger) << "Registering results model "  << results
                      << " on category model " << categories;
  }
}

Lens::Ptr HomeLens::Impl::FindLensForUri(std::string const& uri)
{
  /* We iterate over all lenses looking for the given uri in their
   * global results. This might seem like a sucky approach, but it
   * saves us from a ship load of book keeping */

  for (auto lens : lenses_)
  {
    DeeModel* results = lens->global_results()->model();
    DeeModelIter* iter = dee_model_get_first_iter(results);
    DeeModelIter* end = dee_model_get_last_iter(results);
    const int URI_COLUMN = 0;

    while (iter != end)
      {
        if (g_strcmp0(uri.c_str(), dee_model_get_string(results, iter, URI_COLUMN)) == 0)
        {
          return lens;
        }
        iter = dee_model_next(results, iter);
      }
  }

  return Lens::Ptr();
}

// FIXME i18n _("Home") description, searchhint
// FIXME Filter feedback to the lenses

void HomeLens::Impl::OnLensAdded (Lens::Ptr& lens)
{
  lenses_.push_back (lens);
  owner_->lens_added.emit(lens);

  nux::ROProperty<glib::Object<DeeModel>>& results_prop = lens->global_results()->model;
  nux::ROProperty<glib::Object<DeeModel>>& categories_prop = lens->categories()->model;
  nux::ROProperty<glib::Object<DeeModel>>& filters_prop = lens->filters()->model;

  /*
   * Important: We must ensure that the categories model is annotated
   *            with the results model in the "unity-homelens-results-model"
   *            data slot. We need it later to compute the transfermed offsets
   *            of the categories in the merged category model.
   */

  /* Most lenses add models lazily, but we can't know that;
   * so try to see if we can add them up front */
  if (results_prop().RawPtr())
  {
    EnsureCategoryAnnotation(categories_prop(), results_prop());
    results_merger_.AddSource(results_prop());
  }

  if (categories_prop().RawPtr())
  {
    EnsureCategoryAnnotation(categories_prop(), results_prop());
    categories_merger_.AddSource(categories_prop());
  }

  if (filters_prop().RawPtr())
    filters_merger_.AddSource(filters_prop());

  /*
   * Pick it up when the lens set models lazily.
   */
  results_prop.changed.connect([&] (glib::Object<DeeModel> model)
  {
    EnsureCategoryAnnotation(lens->categories()->model(), model);
    results_merger_.AddSource(model);
  });

  categories_prop.changed.connect([&] (glib::Object<DeeModel> model)
  {
    EnsureCategoryAnnotation(model, lens->global_results()->model());
    categories_merger_.AddSource(model);
  });

  filters_prop.changed.connect([&] (glib::Object<DeeModel> model)
  {
    filters_merger_.AddSource(model);
  });
}

HomeLens::HomeLens()
  : Lens("home.lens", "", "", "FIXME: name", PKGDATADIR"/lens-nav-home.svg",
         "FIXME: description", "FIXME: searchhint", true, "", ModelType::LOCAL)
  , pimpl(new Impl(this))
{
  count.SetGetterFunction(sigc::mem_fun(&pimpl->lenses_, &Lenses::LensList::size));
  search_in_global = false;
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

void HomeLens::GlobalSearch(std::string const& search_string)
{
  LOG_WARN(logger) << "Global search not enabled for HomeLens class."
                   << " Ignoring query '" << search_string << "'";
}

void HomeLens::Search(std::string const& search_string)
{
  LOG_DEBUG(logger) << "Search '" << search_string << "'";

  for (auto lens: pimpl->lenses_)
  {
    if (lens->search_in_global())
    {
      LOG_DEBUG(logger) << " - Global search on '" << lens->id() << "' for '"
          << search_string << "'";
      lens->view_type = ViewType::HOME_VIEW;
      lens->GlobalSearch(search_string);
    }
  }
}

void HomeLens::Activate(std::string const& uri)
{
  LOG_DEBUG(logger) << "Activate '" << uri << "'";

  Lens::Ptr lens = pimpl->FindLensForUri(uri);

  /* Fall back to default handling of URIs if no lens is found.
   *  - Although, this shouldn't really happen */
  if (lens)
  {
    LOG_DEBUG(logger) << "Activation request passed to '" << lens->id() << "'";
    lens->Activate(uri);
  }
  else
  {
    LOG_WARN(logger) << "Unable to find a lens for activating '" << uri
                     << "'. Using fallback activation.";
    activated.emit(uri, HandledType::NOT_HANDLED, Hints());
  }
}

void HomeLens::Preview(std::string const& uri)
{
  LOG_DEBUG(logger) << "Preview '" << uri << "'";

  Lens::Ptr lens = pimpl->FindLensForUri(uri);

  if (lens)
    lens->Preview(uri);
  else
    LOG_WARN(logger) << "Unable to find a lens for previewing '" << uri << "'";
}

}
}
