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
#include <set>
#include <utility>

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
 *       result source models - and not the category source models!
 */
class HomeLens::CategoryRegistry
{
public:
  CategoryRegistry(HomeLens* owner)
    : owner_(owner)
    , next_category_index_ (0) {}

  typedef std::pair<DeeModel*, unsigned> ModelOffsetPair;

  int FindCategoryOffset(DeeModel* model, unsigned int source_cat_index)
  {
    ModelOffsetPair key = std::make_pair(model, source_cat_index);

    std::map<ModelOffsetPair, unsigned>::iterator it = reg_category_map_.find(key);

    if (it != reg_category_map_.end())
      return it->second;

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

  /* Register a new category */
  unsigned RegisterCategoryOffset(DeeModel*    model,
                                  unsigned int source_cat_index,
                                  const gchar* display_name)
  {
    ModelOffsetPair key = std::make_pair(model, source_cat_index);
    std::map<ModelOffsetPair, unsigned>::iterator it = reg_category_map_.find(key);

    if (it != reg_category_map_.end())
    {
      std::string name(display_name ? display_name : "(null)");
      LOG_ERROR(logger) << "Category '" << name
        << "' (source index: " << source_cat_index << ") already registered!";
      return it->second;
    }

    unsigned target_cat_index;
    bool target_index_found = false;

    if (display_name != NULL)
    {
      std::map<std::string, unsigned>::iterator name_it = reg_by_display_name_.find(display_name);
      if (name_it != reg_by_display_name_.end())
      {
        target_cat_index = name_it->second;
        target_index_found = true;
      }
    }

    if (!target_index_found) target_cat_index = next_category_index_++;
    reg_category_map_[key] = target_cat_index;

    /* Callers pass a NULL display_name when they already have a category
     * with the right display registered */
    if (display_name != NULL)
    {
      reg_by_display_name_[display_name] = target_cat_index;
      LOG_DEBUG(logger) << "Registered category '" << display_name
                        << "' with source index " << source_cat_index
                        << " and target " << target_cat_index;
    }
    else
    {
      LOG_DEBUG(logger) << "Registered category with source index "
                        << source_cat_index << " and target "
                        << target_cat_index;
    }

    return target_cat_index;
  }

  void NotifyOrderChanged ()
  {
    owner_->categories_reordered();
  }

private:
  std::map<std::string,unsigned int> reg_by_display_name_;
  std::map<ModelOffsetPair, unsigned> reg_category_map_;
  HomeLens* owner_;
  unsigned next_category_index_;
};

/*
 * Helper class that merges a set of DeeModels into one super model
 */
class HomeLens::ModelMerger : public sigc::trackable
{
public:
  ModelMerger(glib::Object<DeeModel> target);
  virtual ~ModelMerger();

  void AddSource(Lens::Ptr& owner_lens, glib::Object<DeeModel> source);

protected:
  virtual void OnSourceRowAdded(DeeModel *model, DeeModelIter *iter);
  virtual void OnSourceRowRemoved(DeeModel* model, DeeModelIter* iter);
  virtual void OnSourceRowChanged(DeeModel* model, DeeModelIter* iter);
  void EnsureRowBuf(DeeModel *source);

  /* The merge tag lives on the source models, pointing to the mapped
   * row in the target model */
  DeeModelTag* FindSourceToTargetTag(DeeModel *model);

protected:
  std::map<Lens::Ptr, glib::Object<DeeModel> > sources_by_owner_;
  glib::SignalManager sig_manager_;
  GVariant** row_buf_;
  unsigned int n_cols_;
  glib::Object<DeeModel> target_;
  std::map<DeeModel*,DeeModelTag*> source_to_target_tags_;
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
  void OnSourceRowRemoved(DeeModel *model, DeeModelIter *iter);
  void OnSourceRowChanged(DeeModel *model, DeeModelIter *iter);

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

  void OnSourceRowAdded(DeeModel *model, DeeModelIter *iter);
  void OnSourceRowRemoved(DeeModel *model, DeeModelIter *iter);

  std::vector<unsigned> GetOrder();

private:
  HomeLens::CategoryRegistry* cat_registry_;
  std::multimap<unsigned, unsigned, std::greater<unsigned> > category_ordering_;
};

/*
 * Specialized ModelMerger that is reponsible for filters, currently ignores
 * everything.
 */
class HomeLens::FiltersMerger : public ModelMerger
{
public:
  FiltersMerger(glib::Object<DeeModel> target);

  void OnSourceRowAdded(DeeModel *model, DeeModelIter *iter);
  void OnSourceRowRemoved(DeeModel *model, DeeModelIter *iter);
  void OnSourceRowChanged(DeeModel *model, DeeModelIter *iter);
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
  gsize FindLensPriority (Lens::Ptr& lens);
  void EnsureCategoryAnnotation(Lens::Ptr& lens, DeeModel* results, DeeModel* categories);
  Lens::Ptr FindLensForUri(std::string const& uri);
  std::vector<unsigned> GetCategoriesOrder();

  HomeLens* owner_;
  Lenses::LensList lenses_;
  HomeLens::CategoryRegistry cat_registry_;
  HomeLens::ResultsMerger results_merger_;
  HomeLens::CategoryMerger categories_merger_;
  HomeLens::FiltersMerger filters_merger_;
  int running_searches_;
  glib::Object<GSettings> settings_;
};

/*
 * IMPLEMENTATION
 */

HomeLens::ModelMerger::ModelMerger(glib::Object<DeeModel> target)
  : row_buf_(NULL)
  , n_cols_(0)
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

HomeLens::FiltersMerger::FiltersMerger(glib::Object<DeeModel> target)
  : HomeLens::ModelMerger::ModelMerger(target)
{}

HomeLens::ModelMerger::~ModelMerger()
{
  if (row_buf_)
    g_free(row_buf_);
}

void HomeLens::ModelMerger::AddSource(Lens::Ptr& owner_lens,
                                      glib::Object<DeeModel> source)
{
  typedef glib::Signal<void, DeeModel*, DeeModelIter*> RowSignalType;

  if (!source)
  {
    LOG_ERROR(logger) << "Trying to add NULL source to ModelMerger";
    return;
  }

  /* We always have just one model per Lens instance, so let's make sure
   * we're not keeping any dangling model signal connections */
  std::map<Lens::Ptr, glib::Object<DeeModel> >::iterator it =
    sources_by_owner_.find(owner_lens);
  if (it != sources_by_owner_.end())
  {
    if (it->second == source)
      return; // this model was already added
    sig_manager_.Disconnect(it->second.RawPtr());
  }
  sources_by_owner_[owner_lens] = source;

  DeeModelTag* merger_tag = dee_model_register_tag(source, NULL);
  source_to_target_tags_[source.RawPtr()] = merger_tag;

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
  // Default impl. does nothing.
}

void HomeLens::ResultsMerger::OnSourceRowAdded(DeeModel *model, DeeModelIter *iter)
{
  DeeModelIter* target_iter;
  int target_cat_offset, source_cat_offset;
  const unsigned int CATEGORY_COLUMN = 2;

  EnsureRowBuf(model);

  dee_model_get_row (model, iter, row_buf_);

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
    DeeModelTag* target_tag = FindSourceToTargetTag(model);
    dee_model_set_tag(model, iter, target_tag, target_iter);

    /*LOG_DEBUG(logger) << "Found " << dee_model_get_string(model, iter, 0)
                      << " (source cat " << source_cat_offset << ", target cat "
                      << target_cat_offset << ")";*/
  }
  else
  {
    LOG_ERROR(logger) << "No category registered for model "
                      << model << ", source offset " << source_cat_offset
                      << ": " << dee_model_get_string(model, iter, 0);
  }

  for (unsigned int i = 0; i < n_cols_; i++) g_variant_unref(row_buf_[i]);
}

void HomeLens::CategoryMerger::OnSourceRowAdded(DeeModel *model, DeeModelIter *iter)
{
  DeeModel* results_model;
  DeeModelIter* target_iter;
  DeeModelTag*  target_tag;
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

  target_tag = FindSourceToTargetTag(model);
  unsigned source_cat_offset = dee_model_get_position(model, iter);

  /* If we already have a category registered with the same display name
   * then we just use that. Otherwise register a new category for it */
  display_name = dee_model_get_string(model, iter, DISPLAY_NAME_COLUMN);
  if (cat_registry_->FindCategoryOffset(display_name) >= 0)
  {
    /* Make sure the <results_model, source_cat_offset> pair is registered */
    cat_registry_->RegisterCategoryOffset(results_model, source_cat_offset,
                                          display_name);
    return;
  }

  /*
   * Below we can assume that we have a genuinely new category.
   */

  /* Add the row to the merged categories model and store required metadata */
  dee_model_get_row(model, iter, row_buf_);
  target_iter = dee_model_append_row(target_, row_buf_);
  dee_model_set_tag(model, iter, target_tag, target_iter);
  unsigned target_cat_index = 
    cat_registry_->RegisterCategoryOffset(results_model, source_cat_offset,
                                          display_name);

  gsize lens_priority = GPOINTER_TO_SIZE(g_object_get_data(
                                   G_OBJECT(model), "unity-homelens-priority"));
  unsigned lens_prio = static_cast<unsigned>(lens_priority);
  category_ordering_.insert(std::pair<unsigned, unsigned>(lens_prio, target_cat_index));
  if (category_ordering_.rbegin()->second != target_cat_index)
  {
    cat_registry_->NotifyOrderChanged();
  }

  for (unsigned int i = 0; i < n_cols_; i++) g_variant_unref(row_buf_[i]);
}

void HomeLens::FiltersMerger::OnSourceRowAdded(DeeModel *model, DeeModelIter *iter)
{
  /* Supporting filters on the home screen is possible, but *quite* tricky.
   * So... Discard ALL the rows!
   */
}

void HomeLens::CategoryMerger::OnSourceRowRemoved(DeeModel *model, DeeModelIter *iter)
{
  /* We don't support removals of categories.
   * You can check out any time you like, but you can never leave
   *
   * The category registry code is spaghettified enough already.
   * No more please.
   */
  LOG_DEBUG(logger) << "Removal of categories not supported.";
}

void HomeLens::ModelMerger::OnSourceRowRemoved(DeeModel *model, DeeModelIter *iter)
{
  DeeModelIter* target_iter;
  DeeModelTag*  target_tag;

  EnsureRowBuf(model);

  target_tag = FindSourceToTargetTag(model);
  target_iter = static_cast<DeeModelIter*>(dee_model_get_tag(model,
                                                             iter,
                                                             target_tag));

  /* We might not have registered a target iter for the row.
   * This fx. happens if we re-used a category based on display_name */
  if (target_iter != NULL)
    dee_model_remove(target_, target_iter);
}

void HomeLens::ResultsMerger::OnSourceRowRemoved(DeeModel *model, DeeModelIter *iter)
{
  ModelMerger::OnSourceRowRemoved(model, iter);
}

void HomeLens::FiltersMerger::OnSourceRowRemoved(DeeModel *model, DeeModelIter *iter)
{
  /* We aren't adding any rows to the merged model, so nothing to do here */
}

void HomeLens::ModelMerger::OnSourceRowChanged(DeeModel *model, DeeModelIter *iter)
{
  DeeModelIter* target_iter;
  DeeModelTag*  target_tag;

  EnsureRowBuf(model);

  dee_model_get_row (model, iter, row_buf_);
  target_tag = FindSourceToTargetTag(model);
  target_iter = static_cast<DeeModelIter*>(dee_model_get_tag(model,
                                                                 iter,
                                                                 target_tag));

  dee_model_set_row (target_, target_iter, row_buf_);

  for (unsigned int i = 0; i < n_cols_; i++) g_variant_unref(row_buf_[i]);
}

void HomeLens::ResultsMerger::OnSourceRowChanged(DeeModel *model, DeeModelIter *iter)
{
  // FIXME: We can support this, but we need to re-calculate the category offset
  LOG_WARN(logger) << "In-line changing of results not supported in the home lens. Sorry.";
}

void HomeLens::FiltersMerger::OnSourceRowChanged(DeeModel *model, DeeModelIter *iter)
{
  /* We aren't adding any rows to the merged model, so nothing to do here */
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
                          << n_cols1 << " columns, but found "
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

DeeModelTag* HomeLens::ModelMerger::FindSourceToTargetTag(DeeModel *model)
{
  return source_to_target_tags_[model];
}

std::vector<unsigned> HomeLens::CategoryMerger::GetOrder()
{
  std::vector<unsigned> result;
  for (auto it = category_ordering_.begin(); it != category_ordering_.end(); ++it)
  {
    result.push_back(it->second);
  }

  return result;
}

HomeLens::Impl::Impl(HomeLens *owner)
  : owner_(owner)
  , cat_registry_(owner)
  , results_merger_(owner->results()->model(), &cat_registry_)
  , categories_merger_(owner->categories()->model(), &cat_registry_)
  , filters_merger_(owner->filters()->model())
  , running_searches_(0)
  , settings_(g_settings_new("com.canonical.Unity.Dash"))
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

/*void HomeLens::Impl::CheckCategories()
{

}*/

gsize HomeLens::Impl::FindLensPriority (Lens::Ptr& lens)
{
  gchar** lenses = g_settings_get_strv(settings_, "home-lens-ordering");
  gsize pos = 0, len = g_strv_length(lenses);

  for (pos = 0; pos < len; pos++)
  {
    if (g_strcmp0(lenses[pos], lens->id().c_str()) == 0)
      break;
  }

  g_strfreev(lenses);

  return len - pos;
}

void HomeLens::Impl::EnsureCategoryAnnotation (Lens::Ptr& lens,
                                               DeeModel* categories,
                                               DeeModel* results)
{
  if (categories && results)
  {
    if (!(DEE_IS_MODEL(results) && DEE_IS_MODEL(categories)))
    {
      LOG_ERROR(logger) << "The "
                        << std::string(DEE_IS_MODEL(results) ? "categories" : "results")
                        << " model is not a valid DeeModel. ("
                        << lens->id() << ")";
      return;
    }

    g_object_set_data(G_OBJECT(categories),
                      "unity-homelens-results-model",
                      results);

    gsize lens_priority = FindLensPriority(lens);
    g_object_set_data(G_OBJECT(categories),
                      "unity-homelens-priority",
                      GSIZE_TO_POINTER(lens_priority));

    LOG_DEBUG(logger) << "Registering results model "  << results
                      << " and lens priority " << lens_priority
                      << " on category model " << categories << ". ("
                      << lens->id() << ")";
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

// FIXME: Coordinated sorting between the lens bar and home screen categories. Make void FilesystemLenses::Impl::DecrementAndCheckChildrenWaiting() use the gsettings key
// FIXME: on no results https://bugs.launchpad.net/unity/+bug/711199

void HomeLens::Impl::OnLensAdded (Lens::Ptr& lens)
{
  lenses_.push_back (lens);
  owner_->lens_added.emit(lens);

  /* When we dispatch a search we inc the search count and when we finish
   * one we decrease it. When we reach 0 we'll emit search_finished. */
  lens->global_search_finished.connect([&] (Hints const& hints) {
      running_searches_--;

      if (running_searches_ <= 0)
      {
        owner_->search_finished.emit(Hints());
        LOG_INFO(logger) << "Search finished";
      }
  });

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
  if (lens->connected())
  {
    if (results_prop())
    {
      EnsureCategoryAnnotation(lens, categories_prop(), results_prop());
      results_merger_.AddSource(lens, results_prop());
    }

    if (categories_prop())
    {
      EnsureCategoryAnnotation(lens, categories_prop(), results_prop());
      categories_merger_.AddSource(lens, categories_prop());
    }

    if (filters_prop())
      filters_merger_.AddSource(lens, filters_prop());
  }

  /*
   * We'll assume that the models' swarm names do not change during life cycle
   * of a lens.
   * Otherwise we might run into a race where we would associate category
   * model to a results model that is about to be replaced by a new one.
   */
  lens->connected.changed.connect([&] (bool is_connected)
  {
    if (is_connected)
    {
      EnsureCategoryAnnotation(lens, lens->categories()->model(),
                               lens->global_results()->model());
      categories_merger_.AddSource(lens, lens->categories()->model());
      results_merger_.AddSource(lens, lens->global_results()->model());
      filters_merger_.AddSource(lens, lens->filters()->model());
    }
  });
  /*
  results_prop.changed.connect([&] (glib::Object<DeeModel> model)
  {
    EnsureCategoryAnnotation(lens, lens->categories()->model(), model);
    results_merger_.AddSource(model);
  });

  categories_prop.changed.connect([&] (glib::Object<DeeModel> model)
  {
    EnsureCategoryAnnotation(lens, model, lens->global_results()->model());
    categories_merger_.AddSource(model);
  });

  filters_prop.changed.connect([&] (glib::Object<DeeModel> model)
  {
    filters_merger_.AddSource(model);
  });
  */

  /*
   * Register pre-existing categories up front
   * FIXME: Do the same for results?
   */
  DeeModel* cats = categories_prop();
  if (cats)
  {
    DeeModelIter* cats_iter;
    DeeModelIter* cats_end;
    for (cats_iter = dee_model_get_first_iter(cats), cats_end = dee_model_get_last_iter(cats);
        cats_iter != cats_end;
        cats_iter = dee_model_next(cats, cats_iter))
    {
      categories_merger_.OnSourceRowAdded(cats, cats_iter);
    }
  }
}

std::vector<unsigned> HomeLens::Impl::GetCategoriesOrder()
{
  return categories_merger_.GetOrder();
}

HomeLens::HomeLens(std::string const& name, std::string const& description, std::string const& search_hint)
  : Lens("home.lens", "", "", name, PKGDATADIR"/lens-nav-home.svg",
         description, search_hint, true, "",
         ModelType::LOCAL)
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

  /* Reset running search counter */
  pimpl->running_searches_ = 0;

  for (auto lens: pimpl->lenses_)
  {
    if (lens->search_in_global())
    {
      LOG_INFO(logger) << " - Global search on '" << lens->id() << "' for '"
          << search_string << "'";
      lens->view_type = ViewType::HOME_VIEW;
      lens->GlobalSearch(search_string);
      pimpl->running_searches_++;
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

std::vector<unsigned> HomeLens::GetCategoriesOrder()
{
  return pimpl->GetCategoriesOrder();
}

}
}
