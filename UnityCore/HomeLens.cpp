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
 *              Michal Hruby <michal.hruby@canonical.com>
 */

#include <glib.h>
#include <dee-icu.h>
#include <string>
#include <stdexcept>
#include <map>
#include <set>
#include <utility>
#include <algorithm>

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

const gchar* const HOMELENS_PRIORITY = "unity-homelens-priority";
const gchar* const HOMELENS_RESULTS_MODEL = "unity-homelens-results-model";

const unsigned RESULTS_NAME_COLUMN = 4;
const unsigned RESULTS_COMMENT_COLUMN = 5;
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

  void UnregisterAllForModel(DeeModel* model)
  {
    auto it = reg_category_map_.begin();

    // References and iterators to the erased elements are invalidated. 
    // Other references and iterators are not affected.
    while (it != reg_category_map_.end())
    {
      if (it->first.first == model)
        reg_category_map_.erase(it++);
      else
        ++it;
    }
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

  virtual void AddSource(Lens::Ptr& owner_lens, glib::Object<DeeModel> source);
  virtual void RemoveSource(glib::Object<DeeModel> const& old_source);

protected:
  virtual void OnSourceRowAdded(DeeModel* model, DeeModelIter* iter);
  virtual void OnSourceRowRemoved(DeeModel* model, DeeModelIter* iter);
  virtual void OnSourceRowChanged(DeeModel* model, DeeModelIter* iter);
  void EnsureRowBuf(DeeModel* source);

  /* The merge tag lives on the source models, pointing to the mapped
   * row in the target model */
  DeeModelTag* FindSourceToTargetTag(DeeModel* model);

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
  void OnSourceRowAdded(DeeModel* model, DeeModelIter* iter);
  void OnSourceRowRemoved(DeeModel* model, DeeModelIter* iter);
  void OnSourceRowChanged(DeeModel* model, DeeModelIter* iter);

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
                 HomeLens::CategoryRegistry* cat_registry,
                 MergeMode merge_mode);

  void OnSourceRowAdded(DeeModel* model, DeeModelIter* iter);
  void OnSourceRowRemoved(DeeModel* model, DeeModelIter* iter);

  std::vector<unsigned> GetDefaultOrder();
  std::string GetLensIdForCategory(unsigned) const;
  std::map<unsigned, Lens::Ptr> const& GetCategoryToLensMap() const;
  MergeMode GetMergeMode() const { return merge_mode_; }

protected:
  void RemoveSource(glib::Object<DeeModel> const& old_source);

private:
  HomeLens::CategoryRegistry* cat_registry_;
  MergeMode merge_mode_;
  std::multimap<unsigned, unsigned, std::greater<unsigned> > category_ordering_;
  std::map<unsigned, Lens::Ptr> category_to_owner_;
};

/*
 * Specialized ModelMerger that is reponsible for filters, currently ignores
 * everything.
 */
class HomeLens::FiltersMerger : public ModelMerger
{
public:
  FiltersMerger(glib::Object<DeeModel> target);

  void OnSourceRowAdded(DeeModel* model, DeeModelIter* iter);
  void OnSourceRowRemoved(DeeModel* model, DeeModelIter* iter);
  void OnSourceRowChanged(DeeModel* model, DeeModelIter* iter);
};

/*
 * Pimpl for HomeLens
 */
class HomeLens::Impl : public sigc::trackable
{
public:
  Impl(HomeLens* owner, MergeMode merge_mode);
  ~Impl();

  struct CategorySorter
  {
    CategorySorter(std::map<unsigned, unsigned>& results_per_category,
                   std::map<unsigned, Lens::Ptr> const& category_owner_map)
      : results_per_category_(results_per_category)
      , category_to_owner_(category_owner_map)
    {}

    bool operator() (unsigned cat_a, unsigned cat_b)
    {
      bool a_has_personal_content = false;
      bool b_has_personal_content = false;

      auto it = category_to_owner_.find(cat_a);
      if (it != category_to_owner_.end() && it->second)
      {
        a_has_personal_content = it->second->provides_personal_content();
      }
      it = category_to_owner_.find(cat_b);
      if (it != category_to_owner_.end() && it->second)
      {
        b_has_personal_content = it->second->provides_personal_content();
      }

      // prioritize categories that have private content
      if (a_has_personal_content != b_has_personal_content)
      {
        return a_has_personal_content ? true : false;
      }

      return results_per_category_[cat_a] > results_per_category_[cat_b];
    }

    private:
      std::map<unsigned, unsigned>& results_per_category_;
      std::map<unsigned, Lens::Ptr> const& category_to_owner_;
  };

  void OnLensAdded(Lens::Ptr& lens);
  gsize FindLensPriority (Lens::Ptr& lens);
  void EnsureCategoryAnnotation(Lens::Ptr& lens, DeeModel* results, DeeModel* categories);
  Lens::Ptr FindLensForUri(std::string const& uri);
  std::vector<unsigned> GetCategoriesOrder();
  void LensSearchFinished(Lens::Ptr const& lens);
  bool ResultsContainVisibleMatch(unsigned category);

  std::string const& last_search_string() const { return last_search_string_; }

  HomeLens* owner_;
  Lenses::LensList lenses_;
  HomeLens::CategoryRegistry cat_registry_;
  HomeLens::ResultsMerger results_merger_;
  HomeLens::CategoryMerger categories_merger_;
  HomeLens::FiltersMerger filters_merger_;
  int running_searches_;
  bool apps_lens_contains_visible_match;
  std::string last_search_string_;
  glib::Object<GSettings> settings_;
  std::vector<unsigned> cached_categories_order_;
  std::map<unsigned, glib::Object<DeeModel> > category_filter_models_;
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
                                         CategoryRegistry *cat_registry,
                                         MergeMode merge_mode)
  : HomeLens::ModelMerger::ModelMerger(target)
  , cat_registry_(cat_registry)
  , merge_mode_(merge_mode)
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
    RemoveSource(it->second);
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

void HomeLens::ModelMerger::RemoveSource(glib::Object<DeeModel> const& source)
{
  if (!source)
  {
    LOG_ERROR(logger) << "Trying to remove NULL source from ModelMerger";
    return;
  }

  sig_manager_.Disconnect(source);
}

void HomeLens::ModelMerger::OnSourceRowAdded(DeeModel* model, DeeModelIter* iter)
{
  // Default impl. does nothing.
}

void HomeLens::ResultsMerger::OnSourceRowAdded(DeeModel* model, DeeModelIter* iter)
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

    LOG_DEBUG(logger) << "Found " << dee_model_get_string(model, iter, 0)
                      << " (source cat " << source_cat_offset << ", target cat "
                      << target_cat_offset << ")";
  }
  else
  {
    LOG_ERROR(logger) << "No category registered for model "
                      << model << ", source offset " << source_cat_offset
                      << ": " << dee_model_get_string(model, iter, 0);
  }

  for (unsigned int i = 0; i < n_cols_; i++) g_variant_unref(row_buf_[i]);
}

void HomeLens::CategoryMerger::OnSourceRowAdded(DeeModel* model, DeeModelIter* iter)
{
  DeeModel* results_model;
  DeeModelIter* target_iter;
  DeeModelTag*  target_tag;
  const gchar* display_name;
  const unsigned int DISPLAY_NAME_COLUMN = 0;
  std::string lens_name("Unknown");

  EnsureRowBuf(model);

  results_model = static_cast<DeeModel*>(g_object_get_data(
                              G_OBJECT(model), HOMELENS_RESULTS_MODEL));
  if (results_model == NULL)
  {
    LOG_DEBUG(logger) << "Category model " << model
                      << " does not have a results model yet";
    return;
  }

  target_tag = FindSourceToTargetTag(model);
  unsigned source_cat_offset = dee_model_get_position(model, iter);

  Lens::Ptr owner_lens;
  for (auto it = sources_by_owner_.begin(); it != sources_by_owner_.end(); ++it)
  {
    if (it->second == model)
    {
      owner_lens = it->first;
      break;
    }
  }

  if (merge_mode_ == MergeMode::OWNER_LENS)
  {
    if (owner_lens) lens_name = owner_lens->name();
    display_name = lens_name.c_str();
  }
  else
  {
    display_name = dee_model_get_string(model, iter, DISPLAY_NAME_COLUMN);
  }

  /* If we already have a category registered with the same display name
   * then we just use that. Otherwise register a new category for it */
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

  /* Rename the category */
  g_variant_unref(row_buf_[DISPLAY_NAME_COLUMN]);
  row_buf_[DISPLAY_NAME_COLUMN] = g_variant_new_string(display_name);
  // need to ref sink the floating variant
  g_variant_ref_sink(row_buf_[DISPLAY_NAME_COLUMN]);

  target_iter = dee_model_append_row(target_, row_buf_);
  dee_model_set_tag(model, iter, target_tag, target_iter);
  unsigned target_cat_index = 
    cat_registry_->RegisterCategoryOffset(results_model, source_cat_offset,
                                          display_name);

  if (owner_lens) category_to_owner_[target_cat_index] = owner_lens;

  // ensure priorities are taken into account, so default order works
  gsize lens_priority = GPOINTER_TO_SIZE(g_object_get_data(
                                   G_OBJECT(model), HOMELENS_PRIORITY));
  unsigned lens_prio = static_cast<unsigned>(lens_priority);
  category_ordering_.insert(std::pair<unsigned, unsigned>(lens_prio, target_cat_index));
  if (category_ordering_.rbegin()->second != target_cat_index)
  {
    cat_registry_->NotifyOrderChanged();
  }

  for (unsigned int i = 0; i < n_cols_; i++) g_variant_unref(row_buf_[i]);
}

void HomeLens::FiltersMerger::OnSourceRowAdded(DeeModel* model, DeeModelIter* iter)
{
  /* Supporting filters on the home screen is possible, but *quite* tricky.
   * So... Discard ALL the rows!
   */
}

void HomeLens::CategoryMerger::OnSourceRowRemoved(DeeModel* model, DeeModelIter* iter)
{
  /* We don't support removals of categories.
   * You can check out any time you like, but you can never leave
   *
   * The category registry code is spaghettified enough already.
   * No more please.
   */
  LOG_DEBUG(logger) << "Removal of categories not supported.";
}

void HomeLens::ModelMerger::OnSourceRowRemoved(DeeModel* model, DeeModelIter* iter)
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

void HomeLens::ResultsMerger::OnSourceRowRemoved(DeeModel* model, DeeModelIter* iter)
{
  ModelMerger::OnSourceRowRemoved(model, iter);
}

void HomeLens::FiltersMerger::OnSourceRowRemoved(DeeModel* model, DeeModelIter* iter)
{
  /* We aren't adding any rows to the merged model, so nothing to do here */
}

void HomeLens::ModelMerger::OnSourceRowChanged(DeeModel* model, DeeModelIter* iter)
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

void HomeLens::ResultsMerger::OnSourceRowChanged(DeeModel* model, DeeModelIter* iter)
{
  // FIXME: We can support this, but we need to re-calculate the category offset
  LOG_WARN(logger) << "In-line changing of results not supported in the home lens. Sorry.";
}

void HomeLens::FiltersMerger::OnSourceRowChanged(DeeModel* model, DeeModelIter* iter)
{
  /* We aren't adding any rows to the merged model, so nothing to do here */
}

void HomeLens::ModelMerger::EnsureRowBuf(DeeModel* model)
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

DeeModelTag* HomeLens::ModelMerger::FindSourceToTargetTag(DeeModel* model)
{
  return source_to_target_tags_[model];
}

void HomeLens::CategoryMerger::RemoveSource(glib::Object<DeeModel> const& source)
{
  // call base()
  HomeLens::ModelMerger::RemoveSource(source);

  cat_registry_->UnregisterAllForModel(source);
}

std::vector<unsigned> HomeLens::CategoryMerger::GetDefaultOrder()
{
  std::vector<unsigned> result;
  for (auto it = category_ordering_.begin(); it != category_ordering_.end(); ++it)
  {
    result.push_back(it->second);
  }

  return result;
}

std::string HomeLens::CategoryMerger::GetLensIdForCategory(unsigned cat) const
{
  auto lens_it = category_to_owner_.find(cat);
  if (lens_it != category_to_owner_.end())
  {
    if (lens_it->second) return lens_it->second->id();
  }

  return "";
}

std::map<unsigned, Lens::Ptr> const&
HomeLens::CategoryMerger::GetCategoryToLensMap() const
{
  return category_to_owner_;
}

HomeLens::Impl::Impl(HomeLens *owner, MergeMode merge_mode)
  : owner_(owner)
  , cat_registry_(owner)
  , results_merger_(owner->results()->model(), &cat_registry_)
  , categories_merger_(owner->categories()->model(), &cat_registry_, merge_mode)
  , filters_merger_(owner->filters()->model())
  , running_searches_(0)
  , apps_lens_contains_visible_match(false)
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
                      HOMELENS_RESULTS_MODEL,
                      results);

    gsize lens_priority = FindLensPriority(lens);
    g_object_set_data(G_OBJECT(categories),
                      HOMELENS_PRIORITY,
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
  lens->global_search_finished.connect([this, lens] (Hints const& hints) {
      running_searches_--;

      LensSearchFinished(lens);
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

  /* Make sure the models are properly annotated when they change */
  lens->models_changed.connect([&] ()
  {
    EnsureCategoryAnnotation(lens, lens->categories()->model(),
                             lens->global_results()->model());
    categories_merger_.AddSource(lens, lens->categories()->model());
    results_merger_.AddSource(lens, lens->global_results()->model());
    filters_merger_.AddSource(lens, lens->filters()->model());
  });

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

void HomeLens::Impl::LensSearchFinished(Lens::Ptr const& lens)
{
  auto order_vector = categories_merger_.GetDefaultOrder();

  // get number of results per category
  std::map<unsigned, unsigned> results_per_cat;
  for (unsigned i = 0; i < order_vector.size(); i++)
  {
    unsigned category = order_vector.at(i);
    auto model = owner_->GetFilterModelForCategory(category);
    results_per_cat[category] = model ? dee_model_get_n_rows(model) : 0;
  }

  CategorySorter sorter(results_per_cat,
                        categories_merger_.GetCategoryToLensMap());
  // stable sort based on number of results in each cat
  std::stable_sort(order_vector.begin(), order_vector.end(), sorter);

  // ensure shopping is second, need map[cat] = lens
  int shopping_index = -1;
  int apps_index = -1;
  for (unsigned i = 0; i < order_vector.size(); i++)
  {
    // get lens that owns this category
    std::string const& lens_id(categories_merger_.GetLensIdForCategory(order_vector.at(i)));
    if (lens_id == "shopping.lens")
      shopping_index = i;
    else if (lens_id == "applications.lens")
      apps_index = i;
  }

  if (lens->id() == "applications.lens")
  {
    // checking the results isn't extermely fast, so cache the result
    apps_lens_contains_visible_match = ResultsContainVisibleMatch(order_vector[apps_index]);
  }

  // if there are no results in the apps category, we can't reorder,
  // otherwise shopping won't end up being 2nd
  if (apps_lens_contains_visible_match && apps_index > 0 && 
      results_per_cat[order_vector[apps_index]] > 0)
  {
    // we want apps first
    unsigned apps_cat_num = order_vector.at(apps_index);
    order_vector.erase(order_vector.begin() + apps_index);
    order_vector.insert(order_vector.begin(), apps_cat_num);

    // we might shift the shopping index
    if (shopping_index >= 0 && shopping_index < apps_index) shopping_index++;
  }

  if (shopping_index >= 0 && shopping_index != 2)
  {
    unsigned shopping_cat_num = order_vector.at(shopping_index);
    order_vector.erase(order_vector.begin() + shopping_index);
    order_vector.insert(order_vector.size() > 2 ? order_vector.begin() + 2 : order_vector.end(), shopping_cat_num);
  }

  if (cached_categories_order_ != order_vector)
  {
    cached_categories_order_ = order_vector;
    owner_->categories_reordered();
  }
}

bool HomeLens::Impl::ResultsContainVisibleMatch(unsigned category)
{
  // this method searches for match of the search string in the display name
  // or comment fields
  auto filter_model = owner_->GetFilterModelForCategory(category);
  if (!filter_model) return false;
  if (last_search_string_.empty()) return true;

  int checked_results = 5;

  glib::Object<DeeModel> model(dee_sequence_model_new());
  dee_model_set_schema(model, "s", "s", NULL);

  DeeModelIter* iter = dee_model_get_first_iter(filter_model);
  DeeModelIter* end_iter = dee_model_get_last_iter(filter_model);

  // add first few results to the temporary model
  while (iter != end_iter)
  {
    glib::Variant name(dee_model_get_value(filter_model, iter, RESULTS_NAME_COLUMN),
                       glib::StealRef());
    glib::Variant comment(dee_model_get_value(filter_model, iter, RESULTS_COMMENT_COLUMN),
                          glib::StealRef());
    GVariant* members[2] = { name, comment };
    dee_model_append_row(model, members);

    iter = dee_model_next(filter_model, iter);
    if (--checked_results <= 0) break;
  }

  if (dee_model_get_n_rows(model) == 0) return false;

  // setup model reader, analyzer and instantiate an index
  DeeModelReader reader;
  dee_model_reader_new([] (DeeModel* m, DeeModelIter* iter, gpointer data) -> gchar*
    {
      return g_strdup_printf("%s\n%s",
        dee_model_get_string(m, iter, 0),
        dee_model_get_string(m, iter, 1));
    }, NULL, NULL, &reader);
  glib::Object<DeeAnalyzer> analyzer(DEE_ANALYZER(dee_text_analyzer_new()));
  dee_analyzer_add_term_filter(analyzer,
                               [] (DeeTermList* terms_in, DeeTermList* terms_out, gpointer data) -> void
                               {
                                 auto filter = static_cast<DeeICUTermFilter*>(data);
                                 for (unsigned i = 0; i < dee_term_list_num_terms(terms_in); i++)
                                 {
                                   dee_term_list_add_term(terms_out, dee_icu_term_filter_apply(filter, dee_term_list_get_term(terms_in, i)));
                                 }
                               },
                               dee_icu_term_filter_new_ascii_folder(),
                               (GDestroyNotify)dee_icu_term_filter_destroy);
  // ready to instantiate the index
  glib::Object<DeeIndex> index(DEE_INDEX(dee_tree_index_new(model, analyzer, &reader)));

  // tokenize the search string, so this will work with multiple words
  glib::Object<DeeTermList> search_terms(DEE_TERM_LIST(g_object_new(DEE_TYPE_TERM_LIST, NULL)));
  dee_analyzer_tokenize(analyzer, last_search_string_.c_str(), search_terms);

  std::set<DeeModelIter*> iters;
  for (unsigned i = 0; i < dee_term_list_num_terms(search_terms); i++)
  {
    glib::Object<DeeResultSet> results(dee_index_lookup(index, dee_term_list_get_term(search_terms, i), DEE_TERM_MATCH_PREFIX));
    if (i == 0)
    {
      while (dee_result_set_has_next(results))
      {
        iters.insert(dee_result_set_next(results));
      }
    }
    else
    {
      std::set<DeeModelIter*> iters2;
      while (dee_result_set_has_next(results))
      {
        iters2.insert(dee_result_set_next(results));
      }
      // intersect the sets, set iterators are stable, so we can do this
      auto it = iters.begin();
      while (it != iters.end())
      {
        if (iters2.find(*it) == iters2.end())
          iters.erase(it++);
        else
          ++it;
      }
      // no need to check more terms if the base set is already empty
      if (iters.empty()) break;
    }
  }

  // there is a match if the iterator is isn't empty
  return !iters.empty();
}

std::vector<unsigned> HomeLens::Impl::GetCategoriesOrder()
{
  auto default_order = categories_merger_.GetDefaultOrder();
  if (!last_search_string_.empty() &&
      cached_categories_order_.size() == default_order.size())
  {
    return cached_categories_order_;
  }
  return default_order;
}

HomeLens::HomeLens(std::string const& name,
                   std::string const& description,
                   std::string const& search_hint,
                   MergeMode merge_mode)
  : Lens("home.lens", "", "", name, PKGDATADIR"/lens-nav-home.svg",
         description, search_hint, true, "",
         ModelType::LOCAL)
  , pimpl(new Impl(this, merge_mode))
{
  count.SetGetterFunction(sigc::mem_fun(&pimpl->lenses_, &Lenses::LensList::size));
  last_search_string.SetGetterFunction(sigc::mem_fun(pimpl, &HomeLens::Impl::last_search_string));
  last_global_search_string.SetGetterFunction(sigc::mem_fun(pimpl, &HomeLens::Impl::last_search_string));

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
  pimpl->last_search_string_ = search_string;
  pimpl->apps_lens_contains_visible_match = false;

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

glib::Object<DeeModel> HomeLens::GetFilterModelForCategory(unsigned category)
{
  auto it = pimpl->category_filter_models_.find(category);
  if (it != pimpl->category_filter_models_.end()) return it->second;

  auto model = Lens::GetFilterModelForCategory(category);
  pimpl->category_filter_models_[category] = model;
  return model;
}

}
}
