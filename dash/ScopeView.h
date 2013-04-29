// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010, 2011 Canonical Ltd
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

#ifndef UNITY_SCOPE_VIEW_H_
#define UNITY_SCOPE_VIEW_H_

#include <string>

#include <NuxGraphics/GraphicsEngine.h>
#include <Nux/Nux.h>
#include <Nux/HLayout.h>
#include <Nux/View.h>
#include <Nux/VLayout.h>
#include <UnityCore/Scope.h>
#include <UnityCore/GLibSource.h>

#include "FilterBar.h"
#include "unity-shared/Introspectable.h"
#include "PlacesGroup.h"
#include "ResultViewGrid.h"
#include "unity-shared/UBusWrapper.h"
#include "unity-shared/PlacesVScrollBar.h"

namespace unity
{
namespace dash
{

class ScopeScrollView;

class ScopeView : public nux::View, public unity::debug::Introspectable
{
  NUX_DECLARE_OBJECT_TYPE(ScopeView, nux::View);
  typedef std::vector<PlacesGroup::Ptr> CategoryGroups;
  typedef std::map<PlacesGroup::Ptr, unsigned int> ResultCounts;

public:
  ScopeView(Scope::Ptr scope, nux::Area* show_filters);
  ~ScopeView();

  CategoryGroups GetOrderedCategoryViews() const;
  FilterBar* filter_bar() const { return filter_bar_; }
  Scope::Ptr scope() const;
  nux::Area* fscroll_view() const;

  int GetNumRows();
  void AboutToShow();
  void JumpToTop();

  virtual void ActivateFirst();

  nux::ROProperty<std::string> search_string;
  nux::Property<bool> filters_expanded;
  nux::Property<ScopeViewType> view_type;
  nux::Property<bool> can_refine_search;

  sigc::signal<void, ResultView::ActivateType, LocalResult const&, GVariant*, std::string const&> result_activated;

  typedef std::function<void(std::string const& scope_id, std::string const& search_query, glib::Error const& err)> SearchCallback;
  bool PerformSearch(std::string const& search_query, SearchCallback const& callback);

  void ForceCategoryExpansion(std::string const& view_id, bool expand);
  void PushFilterExpansion(bool expand);
  void PopFilterExpansion();
  bool GetPushedFilterExpansion() const;

  void SetResultsPreviewAnimationValue(float preview_animation);

  void EnableResultTextures(bool enable_result_textures);
  std::vector<ResultViewTexture::Ptr> GetResultTextureContainers();
  void RenderResultTexture(ResultViewTexture::Ptr const& result_texture);

private:
  void SetupViews(nux::Area* show_filters);
  void SetupCategories(Categories::Ptr const& categories);
  void SetupResults(Results::Ptr const& results);
  void SetupFilters(Filters::Ptr const& filters);

  void OnCategoryAdded(Category const& category);
  void OnCategoryChanged(Category const& category);
  void OnCategoryRemoved(Category const& category);

  void OnResultAdded(Result const& result);
  void OnResultRemoved(Result const& result);

  void OnSearchComplete(std::string const& search_string, glib::HintsMap const& hints, glib::Error const& err);
  
  void OnGroupExpanded(PlacesGroup* group);
  void CheckScrollBarState();
  void OnColumnsChanged();
  void OnFilterAdded(Filter::Ptr filter);
  void OnFilterRemoved(Filter::Ptr filter);
  void OnViewTypeChanged(ScopeViewType view_type);
  void OnScopeFilterExpanded(bool expanded);
  void QueueReinitializeFilterCategoryModels(unsigned int index);
  bool ReinitializeCategoryResultModels();
  void ClearCategories();
  void OnCategoryOrderChanged();

  void QueueCategoryCountsCheck();
  void CheckCategoryCounts();

  void CheckNoResults(glib::HintsMap const& hints);
  void HideResultsMessage();

  ResultView* GetResultViewForCategory(unsigned int category_index);

  virtual PlacesGroup::Ptr CreatePlacesGroup(Category const& category);

  void BuildPreview(std::string const& uri, Preview::Ptr model);

  virtual void Draw(nux::GraphicsEngine& gfx_context, bool force_draw);
  virtual void DrawContent(nux::GraphicsEngine& gfx_context, bool force_draw);
  
  virtual bool AcceptKeyNavFocus();
  virtual std::string GetName() const;
  virtual void AddProperties(GVariantBuilder* builder);

  std::string get_search_string() const;

  CategoryGroups category_views_;

  Scope::Ptr scope_;
  glib::Object<GCancellable> cancellable_;
  glib::Object<GCancellable> search_cancellable_;
  std::vector<unsigned int> category_order_;
  ResultCounts counts_;
  bool initial_activation_;
  bool no_results_active_;
  std::string search_string_;
  PlacesGroup::Ptr last_expanded_group_;

  nux::HLayout* layout_;
  ScopeScrollView* scroll_view_;
  nux::VLayout* scroll_layout_;
  ScopeScrollView* fscroll_view_;
  nux::VLayout* fscroll_layout_;
  FilterBar* filter_bar_;
  StaticCairoText* no_results_;

  UBusManager ubus_manager_;
  glib::Source::UniquePtr model_updated_timeout_;
  int last_good_filter_model_;
  glib::Source::UniquePtr fix_filter_models_idle_;
  glib::Source::UniquePtr hide_message_delay_;

  bool filter_expansion_pushed_;

  sigc::connection results_updated;
  sigc::connection result_added_connection;
  sigc::connection result_removed_connection;

  sigc::connection categories_updated;
  sigc::connection category_added_connection;
  sigc::connection category_changed_connection;
  sigc::connection category_removed_connection;

  sigc::connection filters_updated;
  sigc::connection filter_added_connection;
  sigc::connection filter_removed_connection;

  bool scope_connected_;
  bool search_on_next_connect_;

  int current_focus_category_position_;
  int current_focus_result_index_;

  friend class TestScopeView;
};

} // namespace dash
} // namespace unity

#endif // UNITY_SCOPE_VIEW_H_
