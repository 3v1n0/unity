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

#ifndef UNITY_LENS_VIEW_H_
#define UNITY_LENS_VIEW_H_

#include <string>

#include <NuxGraphics/GraphicsEngine.h>
#include <Nux/Nux.h>
#include <Nux/HLayout.h>
#include <Nux/View.h>
#include <Nux/VLayout.h>
#include <UnityCore/Lens.h>
#include <UnityCore/GLibSource.h>

#include "FilterBar.h"
#include "unity-shared/Introspectable.h"
#include "PlacesGroup.h"
#include "ResultViewGrid.h"
#include "unity-shared/UBusWrapper.h"
#include "unity-shared/PlacesVScrollBar.h"
#include "previews/Preview.h"

namespace unity
{
namespace dash
{

class LensScrollView;
class LensView : public nux::View, public unity::debug::Introspectable
{
  NUX_DECLARE_OBJECT_TYPE(LensView, nux::View);
  typedef std::vector<PlacesGroup*> CategoryGroups;
  typedef std::map<PlacesGroup*, unsigned int> ResultCounts;

public:
  LensView();
  LensView(Lens::Ptr lens, nux::Area* show_filters);

  CategoryGroups& categories() { return categories_; }
  FilterBar* filter_bar() const { return filter_bar_; }
  Lens::Ptr lens() const;
  nux::Area* fscroll_view() const;

  int GetNumRows();
  void JumpToTop();

  virtual void ActivateFirst();

  nux::ROProperty<std::string> search_string;
  nux::Property<bool> filters_expanded;
  nux::Property<ViewType> view_type;
  nux::Property<bool> can_refine_search;

  sigc::signal<void, std::string const&> uri_activated;

  void PerformSearch(std::string const& search_query);
  void CheckNoResults(Lens::Hints const& hints);
  void HideResultsMessage();

private:
  void SetupViews(nux::Area* show_filters);
  void SetupCategories();
  void SetupResults();
  void SetupFilters();

  void OnCategoryAdded(Category const& category);
  void OnResultAdded(Result const& result);
  void OnResultRemoved(Result const& result);
  void UpdateCounts(PlacesGroup* group);
  void OnGroupExpanded(PlacesGroup* group);
  void OnColumnsChanged();
  void OnFilterAdded(Filter::Ptr filter);
  void OnFilterRemoved(Filter::Ptr filter);
  void OnViewTypeChanged(ViewType view_type);
  void QueueFixRenderering();
  bool FixRenderering();

  virtual void Draw(nux::GraphicsEngine& gfx_context, bool force_draw);
  virtual void DrawContent(nux::GraphicsEngine& gfx_context, bool force_draw);

  virtual bool AcceptKeyNavFocus();
  virtual std::string GetName() const;
  virtual void AddProperties(GVariantBuilder* builder);

  std::string get_search_string() const;

private:
  Lens::Ptr lens_;
  CategoryGroups categories_;
  ResultCounts counts_;
  bool initial_activation_;
  bool no_results_active_;
  std::string search_string_;

  nux::HLayout* layout_;
  LensScrollView* scroll_view_;
  nux::VLayout* scroll_layout_;
  LensScrollView* fscroll_view_;
  nux::VLayout* fscroll_layout_;
  FilterBar* filter_bar_;
  nux::StaticCairoText* no_results_;

  previews::Preview::Ptr preview_;
  std::string last_activated_result_uri_;
  UBusManager ubus_manager_;
  glib::Source::UniquePtr fix_rendering_idle_;
};


}
}
#endif
