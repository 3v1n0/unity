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

#include "ScopeView.h"

#include <boost/lexical_cast.hpp>

#include <NuxCore/Logger.h>
#include <Nux/HScrollBar.h>
#include <Nux/VScrollBar.h>

#include "unity-shared/DashStyle.h"
#include "CoverflowResultView.h"

#include "ResultRendererTile.h"
#include "ResultRendererHorizontalTile.h"
#include "unity-shared/UBusMessages.h"
#include "unity-shared/UBusWrapper.h"
#include "unity-shared/PlacesOverlayVScrollBar.h"
#include "unity-shared/GraphicsUtils.h"

#include "config.h"
#include <glib/gi18n-lib.h>

namespace unity
{
namespace dash
{
DECLARE_LOGGER(logger, "unity.dash.scopeview");
DECLARE_LOGGER(focus_logger, "unity.dash.scopeview.focus");

namespace
{
const int CARD_VIEW_GAP_VERT  = 24; // pixels
const int CARD_VIEW_GAP_HORIZ = 25; // pixels
}

// This is so we can access some protected members in scrollview.
class ScopeScrollView: public nux::ScrollView
{
public:
  ScopeScrollView(nux::VScrollBar* scroll_bar, NUX_FILE_LINE_DECL)
    : nux::ScrollView(NUX_FILE_LINE_PARAM)
    , right_area_(nullptr)
    , up_area_(nullptr)
  {
    SetVScrollBar(scroll_bar);

    OnVisibleChanged.connect([&] (nux::Area* /*area*/, bool visible) {
      if (m_horizontal_scrollbar_enable)
        _hscrollbar->SetVisible(visible);
      if (m_vertical_scrollbar_enable)
        _vscrollbar->SetVisible(visible);
    });
  }

  void ScrollToPosition(nux::Geometry const& position)
  {
    // much of this code is copied from Nux/ScrollView.cpp
    nux::Geometry const& geo = GetGeometry();

    int child_y = position.y - geo.y;
    int child_y_diff = child_y - abs (_delta_y);

    if (child_y_diff + position.height < geo.height && child_y_diff >= 0)
    {
      return;
    }

    if (child_y_diff < 0)
    {
      ScrollUp (1, abs (child_y_diff));
    }
    else
    {
      int size = child_y_diff - geo.height;

      // always keeps the top of a view on the screen
      size += position.height;

      ScrollDown (1, size);
    }
  }

  void SetRightArea(nux::Area* area)
  {
    right_area_ = area;
  }

  void SetUpArea(nux::Area* area)
  {
    up_area_ = area;
  }

  void DrawContent(nux::GraphicsEngine& graphics_engine, bool force_draw)
  {
    if (RedirectedAncestor())
    {
      if (m_horizontal_scrollbar_enable && _hscrollbar->IsRedrawNeeded())
        graphics::ClearGeometry(_hscrollbar->GetGeometry());
      if (m_vertical_scrollbar_enable && _vscrollbar->IsRedrawNeeded())
        graphics::ClearGeometry(_vscrollbar->GetGeometry());
    }

    ScrollView::DrawContent(graphics_engine, force_draw);
  }

  void EnableScrolling(bool enable_scrolling)
  {
    _vscrollbar->SetInputEventSensitivity(enable_scrolling);    
  }

protected:

  // This is so we can break the natural key navigation path.
  nux::Area* KeyNavIteration(nux::KeyNavDirection direction)
  {
    nux::Area* focus_area = nux::GetWindowCompositor().GetKeyFocusArea();

    if (direction == nux::KEY_NAV_RIGHT && focus_area && focus_area->IsChildOf(this))
      return right_area_;
    else if (direction == nux::KEY_NAV_UP && focus_area && focus_area->IsChildOf(this))
      return up_area_;
    else
      return nux::ScrollView::KeyNavIteration(direction);
  }

private:
  nux::Area* right_area_;
  nux::Area* up_area_;
};


NUX_IMPLEMENT_OBJECT_TYPE(ScopeView);

ScopeView::ScopeView(Scope::Ptr scope, nux::Area* show_filters)
: nux::View(NUX_TRACKER_LOCATION)
, filters_expanded(false)
, can_refine_search(false)
, scope_(scope)
, cancellable_(g_cancellable_new())
, no_results_active_(false)
, last_good_filter_model_(-1)
, filter_expansion_pushed_(false)
, scope_connected_(scope ? scope->connected : false)
, search_on_next_connect_(false)
, current_focus_category_position_(-1)
{
  SetupViews(show_filters);

  search_string.SetGetterFunction(sigc::mem_fun(this, &ScopeView::get_search_string));
  filters_expanded.changed.connect(sigc::mem_fun(this, &ScopeView::OnScopeFilterExpanded));
  view_type.changed.connect(sigc::mem_fun(this, &ScopeView::OnViewTypeChanged));

  key_nav_focus_connection_ = nux::GetWindowCompositor().key_nav_focus_change.connect(sigc::mem_fun(this, &ScopeView::OnCompositorKeyNavFocusChanged));

  if (scope_)
  {
    categories_updated = scope_->categories.changed.connect([this](Categories::Ptr const& categories) { SetupCategories(categories); });
    SetupCategories(scope->categories);

    results_updated = scope_->results.changed.connect([this](Results::Ptr const& results) { SetupResults(results); });
    SetupResults(scope->results);

    filters_updated = scope_->filters.changed.connect([this](Filters::Ptr const& filters) { SetupFilters(filters); });
    SetupFilters(scope->filters);

    scope_->connected.changed.connect([&](bool is_connected)
    {
      // We need to search again if we were reconnected after being connected before.
      if (scope_connected_ && !is_connected)
        search_on_next_connect_ = true;
      else if (is_connected && search_on_next_connect_)
      {
        search_on_next_connect_ = false;
        if (IsVisible())
          PerformSearch(search_string_, nullptr);
      }
      scope_connected_ = is_connected;
    });
  }

  ubus_manager_.RegisterInterest(UBUS_RESULT_VIEW_KEYNAV_CHANGED, [&] (GVariant* data) {
    // we get this signal when a result view keynav changes,
    // its a bad way of doing this but nux ABI needs to be broken
    // to do it properly
    nux::Geometry focused_pos;
    g_variant_get (data, "(iiii)", &focused_pos.x, &focused_pos.y, &focused_pos.width, &focused_pos.height);

    for (auto group : category_views_)
    {
      if (group->GetLayout() != nullptr)
      {
        auto expand_label = group->GetHeaderFocusableView();
        auto child = group->GetChildView();

        if ((child && child->HasKeyFocus()) ||
            (expand_label && expand_label->HasKeyFocus()))
        {
          focused_pos.x += child->GetGeometry().x;
          focused_pos.y += child->GetGeometry().y - 30;
          focused_pos.height += 30;
          scroll_view_->ScrollToPosition(focused_pos);
          break;
        }
      }
    }
  });

  OnVisibleChanged.connect([&] (nux::Area* area, bool visible) {
    scroll_view_->SetVisible(visible);
  });
}

ScopeView::~ScopeView()
{
  results_updated.disconnect();
  result_added_connection.disconnect();
  result_removed_connection.disconnect();
  
  categories_updated.disconnect();
  category_added_connection.disconnect();
  category_changed_connection.disconnect();
  category_removed_connection.disconnect();

  filters_updated.disconnect();
  filter_added_connection.disconnect();
  filter_removed_connection.disconnect();

  g_cancellable_cancel(cancellable_);
  if (search_cancellable_) g_cancellable_cancel(search_cancellable_);
}

void ScopeView::SetupViews(nux::Area* show_filters)
{
  dash::Style& style = dash::Style::Instance();

  layout_ = new nux::HLayout(NUX_TRACKER_LOCATION);
  layout_->SetSpaceBetweenChildren(style.GetSpaceBetweenScopeAndFilters());

  scroll_view_ = new ScopeScrollView(new PlacesOverlayVScrollBar(NUX_TRACKER_LOCATION),
                                    NUX_TRACKER_LOCATION);
  scroll_view_->EnableVerticalScrollBar(true);
  scroll_view_->EnableHorizontalScrollBar(false);
  layout_->AddView(scroll_view_);

  scroll_layout_ = new nux::VLayout(NUX_TRACKER_LOCATION);
  scroll_view_->SetLayout(scroll_layout_);
  scroll_view_->SetRightArea(show_filters);

  no_results_ = new StaticCairoText("", NUX_TRACKER_LOCATION);
  no_results_->SetTextColor(nux::color::White);
  no_results_->SetVisible(false);
  scroll_layout_->AddView(no_results_, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);

  fscroll_view_ = new ScopeScrollView(new PlacesOverlayVScrollBar(NUX_TRACKER_LOCATION), NUX_TRACKER_LOCATION);
  fscroll_view_->EnableVerticalScrollBar(true);
  fscroll_view_->EnableHorizontalScrollBar(false);
  fscroll_view_->SetVisible(false);
  fscroll_view_->SetUpArea(show_filters);
  layout_->AddView(fscroll_view_, 1);

  fscroll_layout_ = new nux::VLayout();
  fscroll_view_->SetLayout(fscroll_layout_);

  filter_bar_ = new FilterBar();
  int width = style.GetFilterBarWidth() +
              style.GetFilterBarLeftPadding() +
              style.GetFilterBarRightPadding();

  fscroll_view_->SetMinimumWidth(width + style.GetFilterViewRightPadding());
  fscroll_view_->SetMaximumWidth(width + style.GetFilterViewRightPadding());
  filter_bar_->SetMinimumWidth(width);
  filter_bar_->SetMaximumWidth(width);
  AddChild(filter_bar_);
  fscroll_layout_->AddView(filter_bar_, 0);

  SetLayout(layout_);
}

void ScopeView::SetupCategories(Categories::Ptr const& categories)
{
  category_added_connection.disconnect();
  category_changed_connection.disconnect();
  category_removed_connection.disconnect();

  if (!categories)
    return;

  QueueCategoryCountsCheck();

  category_added_connection = categories->category_added.connect(sigc::mem_fun(this, &ScopeView::OnCategoryAdded));
  category_changed_connection = categories->category_changed.connect(sigc::mem_fun(this, &ScopeView::OnCategoryChanged));
  category_removed_connection = categories->category_removed.connect(sigc::mem_fun(this, &ScopeView::OnCategoryRemoved));

  auto resync_categories = [categories, this] (glib::Object<DeeModel> model)
  {
    ClearCategories();
    for (unsigned int i = 0; i < categories->count(); ++i)
      OnCategoryAdded(categories->RowAtIndex(i));
  };

  categories->model.changed.connect(resync_categories);
  resync_categories(categories->model());

  scope_->category_order.changed.connect(sigc::mem_fun(this, &ScopeView::OnCategoryOrderChanged));
}


void ScopeView::OnCategoryOrderChanged(std::vector<unsigned int> const& order)
{
  LOG_DEBUG(logger) << "Reordering categories for " << scope_->name();

  //////////////////////////////////////////////////
  // Find the current focus category position && result index
  // This is to keep the focus in the same place if categories are being added/removed/reordered
  PushResultFocus("reorder");
  key_nav_focus_connection_.block(true);
  //////////////////////////////////////////////////

  category_order_ = order;

  for (auto iter = category_views_.begin(); iter != category_views_.end(); ++iter)
  {
    PlacesGroup::Ptr group = *iter;
    scroll_layout_->RemoveChildObject(group.GetPointer());
  }

  if (scope_)
  {
    // there should be ~10 categories, so this shouldn't be too big of a deal
    for (unsigned i = 0; i < category_order_.size(); i++)
    {
      unsigned int desired_category_index = category_order_[i];

      if (category_views_.size() <= desired_category_index)
        continue;

      scroll_layout_->AddView(category_views_[desired_category_index].GetPointer(), 0);       
    }
  }

  //////////////////////////////////////////////////
  // Update current focus category position && result index
  // This is to keep the focus in the same place if categories are being added/removed/reordered
  PopResultFocus("reorder");
  key_nav_focus_connection_.block(false);
  //////////////////////////////////////////////////

  QueueRelayout();
}

void ScopeView::SetupResults(Results::Ptr const& results)
{
  result_added_connection.disconnect();
  result_removed_connection.disconnect();

  if (!results)
    return;

  result_added_connection = results->result_added.connect(sigc::mem_fun(this, &ScopeView::OnResultAdded));
  result_added_connection = results->result_removed.connect(sigc::mem_fun(this, &ScopeView::OnResultRemoved));

  results->model.changed.connect([this] (glib::Object<DeeModel> model)
  {
    for (unsigned int i = 0; i < category_views_.size(); ++i)
    {
      ResultView* result_view = GetResultViewForCategory(i);
      if (result_view)
      {
        result_view->SetResultsModel(scope_->GetResultsForCategory(i));
      }
    }
  });

  for (unsigned int i = 0; i < results->count(); ++i)
    OnResultAdded(results->RowAtIndex(i));
}

void ScopeView::SetupFilters(Filters::Ptr const& filters)
{
  filter_added_connection.disconnect();
  filter_removed_connection.disconnect();

  if (!filters)
    return;

  filter_added_connection = filters->filter_added.connect(sigc::mem_fun(this, &ScopeView::OnFilterAdded));
  filter_removed_connection = filters->filter_removed.connect(sigc::mem_fun(this, &ScopeView::OnFilterRemoved));

  auto resync_filters = [filters, this] (glib::Object<DeeModel> model)
  {
    bool blocked = filter_added_connection.block(true);

    filter_bar_->ClearFilters();
    for (unsigned int i = 0; i < filters->count(); ++i)
      OnFilterAdded(filters->FilterAtIndex(i));

    filter_added_connection.block(blocked);
  };

  filters->model.changed.connect(resync_filters);
  resync_filters(filters->model());
}

void ScopeView::OnCategoryAdded(Category const& category)
{
  std::string name = category.name;
  std::string icon_hint = category.icon_hint;
  std::string renderer_name = category.renderer_name;
  if (category.index == unsigned(-1))
    return;
  unsigned index = category.index;
  bool reset_filter_models = index < category_views_.size();

  LOG_DEBUG(logger) << "Category added '" << (scope_ ? scope_->name() : "unknown") << "': "
                    << name
                    << "(" << icon_hint
                    << ", " << renderer_name
                    << ", " << boost::lexical_cast<int>(index) << ")";

  //////////////////////////////////////////////////
  // Find the current focus category position && result index
  // This is to keep the focus in the same place if categories are being added/removed/reordered
  PushResultFocus("add");
  key_nav_focus_connection_.block(true);
  //////////////////////////////////////////////////

  PlacesGroup::Ptr group(CreatePlacesGroup(category));
  AddChild(group.GetPointer());
  group->SetName(name);
  group->SetIcon(icon_hint);
  group->SetExpanded(false);
  group->SetVisible(false);

  int view_index = category_order_.size();
  auto find_view_index = std::find(category_order_.begin(), category_order_.end(), index);
  if (find_view_index == category_order_.end())
  {
    category_order_.push_back(index);
  }
  else
  {
    view_index = find_view_index - category_order_.begin();
  }

  category_views_.insert(category_views_.begin() + index, group);

  group->expanded.connect(sigc::mem_fun(this, &ScopeView::OnGroupExpanded));

  /* Reset result count */
  counts_[group] = 0;

  ResultView* results_view = nullptr;
  if (category.renderer_name == "tile-horizontal")
  {
    results_view = new ResultViewGrid(NUX_TRACKER_LOCATION);
    results_view->SetModelRenderer(new ResultRendererHorizontalTile(NUX_TRACKER_LOCATION));
    static_cast<ResultViewGrid*> (results_view)->horizontal_spacing = CARD_VIEW_GAP_HORIZ;
    static_cast<ResultViewGrid*> (results_view)->vertical_spacing = CARD_VIEW_GAP_VERT;
  }
  else
  {
    results_view = new ResultViewGrid(NUX_TRACKER_LOCATION);
    results_view->SetModelRenderer(new ResultRendererTile(NUX_TRACKER_LOCATION));
  }

  if (scope_)
  {
    std::string unique_id = category.name() + scope_->name();
    results_view->unique_id = unique_id;
    results_view->expanded = false;

    results_view->ResultActivated.connect([this, unique_id] (LocalResult const& local_result, ResultView::ActivateType type, GVariant* data) 
    {
      result_activated.emit(type, local_result, data, unique_id); 
      switch (type)
      {
        case ResultView::ActivateType::DIRECT:
        {
          scope_->Activate(local_result, nullptr, cancellable_);
        } break;
        case ResultView::ActivateType::PREVIEW:
        {
          scope_->Preview(local_result, nullptr, cancellable_);
        } break;
        default: break;
      };
    });

    /* Set up filter model for this category */
    Results::Ptr results_model = scope_->GetResultsForCategory(index);
    counts_[group] = results_model ? results_model->count() : 0;

    results_view->SetResultsModel(results_model);
  }

  group->SetChildView(results_view);

  /* We need the full range of method args so we can specify the offset
   * of the group into the layout */
  scroll_layout_->AddView(group.GetPointer(), 0, nux::MinorDimensionPosition::MINOR_POSITION_START,
                          nux::MinorDimensionSize::MINOR_SIZE_FULL, 100.0f,
                          (nux::LayoutPosition)view_index);

  //////////////////////////////////////////////////
  // Update current focus category position && result index
  // This is to keep the focus in the same place if categories are being added/removed/reordered
  PopResultFocus("add");
  key_nav_focus_connection_.block(false);
  //////////////////////////////////////////////////

  if (reset_filter_models)
  {
    QueueReinitializeFilterCategoryModels(index);
  }

  QueueCategoryCountsCheck();
}

void ScopeView::OnCategoryChanged(Category const& category)
{
  if (category_views_.size() <= category.index)
    return;

  PlacesGroup::Ptr group = category_views_[category.index];

  group->SetName(category.name);
  group->SetIcon(category.icon_hint);

  QueueCategoryCountsCheck();
}

void ScopeView::OnCategoryRemoved(Category const& category)
{
  std::string name = category.name;
  std::string icon_hint = category.icon_hint;
  std::string renderer_name = category.renderer_name;
  unsigned index = category.index;
  if (index == unsigned(-1) || category_views_.size() <= index)
    return;
  bool reset_filter_models = index < category_views_.size()-1;

  LOG_DEBUG(logger) << "Category removed '" << (scope_ ? scope_->name() : "unknown") << "': "
                    << name
                    << "(" << icon_hint
                    << ", " << renderer_name
                    << ", " << boost::lexical_cast<int>(index) << ")";

  auto category_pos = category_views_.begin() + index;
  PlacesGroup::Ptr group = *category_pos;

  if (last_expanded_group_ == group)
    last_expanded_group_.Release();

  //////////////////////////////////////////////////
  // Find the current focus category position && result index
  // This is to keep the focus in the same place if categories are being added/removed/reordered
  PushResultFocus("remove");
  key_nav_focus_connection_.block(true);
  //////////////////////////////////////////////////

  counts_.erase(group);
  category_views_.erase(category_pos);

  // remove from order
  auto order_pos = std::find(category_order_.begin(), category_order_.end(), index);
  if (order_pos != category_order_.end())
    category_order_.erase(order_pos);
 
  scroll_layout_->RemoveChildObject(group.GetPointer());
  RemoveChild(group.GetPointer());

  //////////////////////////////////////////////////
  // Update current focus category position && result index
  // This is to keep the focus in the same place if categories are being added/removed/reordered
  PopResultFocus("remove");
  key_nav_focus_connection_.block(false);
  //////////////////////////////////////////////////

  QueueRelayout();
  if (reset_filter_models)
  {
    QueueReinitializeFilterCategoryModels(index);
  }
}

void ScopeView::ClearCategories()
{
  for (auto iter = category_views_.begin(), end = category_views_.end(); iter != end; ++iter)
  {
    PlacesGroup::Ptr group = *iter;
    RemoveChild(group.GetPointer());
    scroll_layout_->RemoveChildObject(group.GetPointer());
  }
  counts_.clear();
  category_views_.clear();
  last_expanded_group_.Release();
  QueueRelayout();
}

void ScopeView::QueueReinitializeFilterCategoryModels(unsigned int start_category_index)
{
  if (!scope_)
    return;

  Categories::Ptr category_model = scope_->categories();
  unsigned int category_count = 0;
  if (!category_model || (category_count=category_model->count()) <= start_category_index)
    return;

  if (category_views_.size() <= (start_category_index + 1))
    return;

  /* Scope is reodering the categories, and since their category index is based
   * on the row position in the model, we need to re-initialize the category result
   * models if we got insert and not an append */
  for (auto iter = category_views_.begin() + start_category_index +1, end = category_views_.end(); iter != end; ++iter)
  {
    ResultView* result_view = (*iter)->GetChildView();
    if (result_view)
      result_view->SetResultsModel(Results::Ptr());
  }

  if (last_good_filter_model_ == -1 || static_cast<int>(start_category_index) < last_good_filter_model_)
  {
    last_good_filter_model_ = static_cast<int>(start_category_index);
  }
  if (!fix_filter_models_idle_)
  {
    fix_filter_models_idle_.reset(new glib::Idle(sigc::mem_fun(this, &ScopeView::ReinitializeCategoryResultModels), glib::Source::Priority::HIGH));
  }
}

bool ScopeView::ReinitializeCategoryResultModels()
{
  if (!scope_)
    return false;

  if (last_good_filter_model_ < 0)
    return false;

  if (category_views_.size() > static_cast<unsigned int>(last_good_filter_model_)+1)
  {
    unsigned int category_index =  static_cast<unsigned int>(last_good_filter_model_) +1;
    for (auto iter = category_views_.begin() + category_index, end = category_views_.end(); iter != end; ++iter, category_index++)
    {
      ResultView* result_view = (*iter)->GetChildView();
      if (result_view)
        result_view->SetResultsModel(scope_->GetResultsForCategory(category_index));
    }
  }

  last_good_filter_model_ = -1;
  fix_filter_models_idle_.reset();
  return false;
}

ResultView* ScopeView::GetResultViewForCategory(unsigned int category_index)
{  
  if (category_views_.size() <= category_index)
    return nullptr;

  auto category_pos = category_views_.begin() + category_index;
  PlacesGroup::Ptr group = *category_pos;
  return static_cast<ResultView*>(group->GetChildView());
}

void ScopeView::OnResultAdded(Result const& result)
{
  // category not added yet.
  if (category_views_.size() <= result.category_index)
    return;

  std::string uri = result.uri;
  LOG_TRACE(logger) << "Result added '" << (scope_ ? scope_->name() : "unknown") << "': " << uri;

  counts_[category_views_[result.category_index]]++;
  // make sure we don't display the no-results-hint if we do have results
  CheckNoResults(glib::HintsMap());

  QueueCategoryCountsCheck();
}

void ScopeView::OnResultRemoved(Result const& result)
{
  // category not added yet.
  if (category_views_.size() <= result.category_index)
    return;
  
  std::string uri = result.uri;
  LOG_TRACE(logger) << "Result removed '" << (scope_ ? scope_->name() : "unknown") << "': " << uri;

  counts_[category_views_[result.category_index]]--;
  // make sure we don't display the no-results-hint if we do have results
  CheckNoResults(glib::HintsMap());

  QueueCategoryCountsCheck();
}

void ScopeView::CheckNoResults(glib::HintsMap const& hints)
{
  gint count = scope_->results() ? scope_->results()->count() : 0;

  if (count == 0 && !no_results_active_ && !search_string_.empty())
  {
    std::stringstream markup;
    glib::HintsMap::const_iterator it;

    it = hints.find("no-results-hint");
    markup << "<span size='larger' weight='bold'>";

    if (it != hints.end())
    {
      markup << it->second.GetString();
    }
    else
    {
      markup << _("Sorry, there is nothing that matches your search.");
    }
    markup << "</span>";

    LOG_DEBUG(logger) << "The no-result-hint is: " << markup.str();

    scroll_layout_->SetContentDistribution(nux::MAJOR_POSITION_CENTER);

    no_results_active_ = true;
    no_results_->SetText(markup.str());
    no_results_->SetVisible(true);
  }
  else if (count && no_results_active_)
  {
    scroll_layout_->SetContentDistribution(nux::MAJOR_POSITION_START);

    no_results_active_ = false;
    no_results_->SetText("");
    no_results_->SetVisible(false);
  }
}

void ScopeView::QueueCategoryCountsCheck()
{
  if (!model_updated_timeout_)
  {
    model_updated_timeout_.reset(new glib::Idle([&] () {
      // Check if all results so far are from one category
      // If so, then expand that category.
      CheckCategoryCounts();
      model_updated_timeout_.reset();
      return false;
    }, glib::Source::Priority::HIGH));
  }
}

void ScopeView::CheckCategoryCounts()
{
  int number_of_displayed_categories = 0;

  PlacesGroup::Ptr new_expanded_group;

  PushResultFocus("count check");

  for (auto category_index : category_order_)
  {
    if (category_views_.size() <= category_index)
     continue;

    PlacesGroup::Ptr const& group = category_views_[category_index];

    group->SetCounts(counts_[group]);
    group->SetVisible(counts_[group] > 0);

    if (counts_[group] > 0)
    {
      ++number_of_displayed_categories;
      new_expanded_group = group;
    }
  }

  if (last_expanded_group_ and last_expanded_group_ != new_expanded_group) {
    last_expanded_group_->PopExpanded();
    last_expanded_group_ = nullptr;
  }

  if (new_expanded_group and number_of_displayed_categories <= 2)
  {
    new_expanded_group->PushExpanded();
    new_expanded_group->SetExpanded(true);
    last_expanded_group_ = new_expanded_group;
  }

  PopResultFocus("count check");
}

void ScopeView::HideResultsMessage()
{
  if (no_results_active_)
  {
    scroll_layout_->SetContentDistribution(nux::MAJOR_POSITION_START);
    no_results_active_ = false;
    no_results_->SetText("");
    no_results_->SetVisible(false);
  }
}

bool ScopeView::PerformSearch(std::string const& search_query, SearchCallback const& callback)
{
  if (search_string_ != search_query)
    for (auto const& group : category_views_)
      group->SetExpanded(false);

  search_string_ = search_query;
  if (scope_)
  {
    // 150ms to hide the no reults message if its take a while to return results
    hide_message_delay_.reset(new glib::Timeout(150, [&] () {
      HideResultsMessage();
      return false;
    }));

    // cancel old search.
    if (search_cancellable_) g_cancellable_cancel (search_cancellable_);
    search_cancellable_ = g_cancellable_new ();

    scope_->Search(search_query, [this, callback] (std::string const& search_string, glib::HintsMap const& hints, glib::Error const& err)
    {
      if (err && !scope_connected_)
      {
        // if we've failed a search due to connection issue, we need to try again when we re-connect
        search_on_next_connect_ = true;
      }

      CheckNoResults(hints);
      hide_message_delay_.reset();
      if (callback)
        callback(scope_->id(), search_string, err);
    }, search_cancellable_);
    return true;
  }
  return false;
}

std::string ScopeView::get_search_string() const
{
  return search_string_;
}

void ScopeView::OnGroupExpanded(PlacesGroup* group)
{
  ResultViewGrid* grid = static_cast<ResultViewGrid*>(group->GetChildView());
  grid->expanded = group->GetExpanded();

  QueueRelayout();
}

void ScopeView::CheckScrollBarState()
{
  if (scroll_layout_->GetHeight() > scroll_view_->GetHeight())
  {
    scroll_view_->EnableVerticalScrollBar(true);
  }
  else
  {
    scroll_view_->EnableVerticalScrollBar(false);
  }
}

void ScopeView::OnFilterAdded(Filter::Ptr filter)
{
  filter_bar_->AddFilter(filter);
  can_refine_search = true;
}

void ScopeView::OnFilterRemoved(Filter::Ptr filter)
{
  filter_bar_->RemoveFilter(filter);
}

void ScopeView::OnViewTypeChanged(ScopeViewType view_type)
{
  if (!scope_)
    return;

  scope_->view_type = view_type;
}

void ScopeView::OnScopeFilterExpanded(bool expanded)
{
  if (fscroll_view_->IsVisible() != expanded)
  {
    fscroll_view_->SetVisible(expanded);
    QueueRelayout();
  }

  for (auto category_view : category_views_)
    category_view->SetFiltersExpanded(expanded);
}

void ScopeView::Draw(nux::GraphicsEngine& graphics_engine, bool force_draw)
{
  if (RedirectedAncestor())
    graphics::ClearGeometry(GetGeometry());
}

void ScopeView::DrawContent(nux::GraphicsEngine& graphics_engine, bool force_draw)
{
  nux::Geometry const& geo(GetGeometry());
  graphics_engine.PushClippingRectangle(geo);
  CheckScrollBarState();

  if (!IsFullRedraw() && RedirectedAncestor())
  {
    if (filter_bar_ && filter_bar_->IsVisible() && filter_bar_->IsRedrawNeeded())
    {
      graphics::ClearGeometry(filter_bar_->GetGeometry());
    }
  }

  layout_->ProcessDraw(graphics_engine, force_draw);
  graphics_engine.PopClippingRectangle();
}

Scope::Ptr ScopeView::scope() const
{
  return scope_;
}

nux::Area* ScopeView::fscroll_view() const
{
  return fscroll_view_;
}

int ScopeView::GetNumRows()
{
  int num_rows = 0;
  for (PlacesGroup::Ptr const& group : category_views_)
  {
    if (group->IsVisible() && group->GetChildView())
    {
      num_rows += 1; // The category header

      if (group->GetExpanded())
      {
        int result_rows = 0, result_columns = 0;
        group->GetChildView()->GetResultDimensions(result_rows, result_columns);
        num_rows += result_rows;
      }
      else
        num_rows += 1;
    }
  }

  return num_rows;
}

void ScopeView::AboutToShow()
{
  JumpToTop();
  OnScopeFilterExpanded(filters_expanded);
}

void ScopeView::JumpToTop()
{
  scroll_view_->ScrollToPosition(nux::Geometry(0, 0, 0, 0));
}

void ScopeView::ActivateFirst()
{
  if (!scope_)
    return;

  Results::Ptr results = scope_->results;
  if (results->count())
  {
    // the first displayed category might not be category_views_[0]
    for (auto iter = category_order_.begin(); iter != category_order_.end(); ++iter)
    {
      unsigned int category_index = *iter;
      if (category_views_.size() <= category_index)
       continue;
      PlacesGroup::Ptr group = category_views_[category_index];

      ResultView* result_view = group->GetChildView();
      if (result_view == nullptr) continue;

      auto it = result_view->GetIteratorAtRow(0);
      if (!it.IsLast())
      {
        Result result(*it);
        result_view->Activate(result, result_view->GetIndexForLocalResult(result), ResultView::ActivateType::DIRECT);
        return;
      }
    }

    // Fallback
    Result result = results->RowAtIndex(0);
    if (result.uri != "")
    {
      result_activated.emit(ResultView::ActivateType::DIRECT, LocalResult(result), nullptr, "");
      scope_->Activate(result);
    }
  }
}

// Keyboard navigation
bool ScopeView::AcceptKeyNavFocus()
{
  return false;
}

void ScopeView::ForceCategoryExpansion(std::string const& view_id, bool expand)
{
  for (auto iter = category_views_.begin(); iter != category_views_.end(); ++iter)
  {
    PlacesGroup::Ptr group = *iter;
    if (group->GetChildView()->unique_id == view_id)
    {  
      if (expand)
      {
        group->PushExpanded();
        group->SetExpanded(true);
      }
      else
      {
        group->PopExpanded();
      }
    }
  }
}

void ScopeView::SetResultsPreviewAnimationValue(float preview_animation)
{
  for (auto it = category_views_.begin(); it != category_views_.end(); ++it)
  {
    (*it)->SetResultsPreviewAnimationValue(preview_animation);
  }
}

void ScopeView::EnableResultTextures(bool enable_result_textures)
{
  scroll_view_->EnableScrolling(!enable_result_textures);

  for (auto it = category_views_.begin(); it != category_views_.end(); ++it)
  {
    ResultView* result_view = (*it)->GetChildView();
    if (result_view)
    {
      result_view->enable_texture_render = enable_result_textures;
    }  
  } 
}

std::vector<ResultViewTexture::Ptr> ScopeView::GetResultTextureContainers()
{
  // iterate in visual order
  std::vector<ResultViewTexture::Ptr> textures;

  for (auto iter = category_order_.begin(); iter != category_order_.end(); ++iter)
  {
    unsigned int category_index = *iter;
    if (category_views_.size() <= category_index)
     continue;
    PlacesGroup::Ptr cateogry_view = category_views_[category_index];

    if (!cateogry_view || !cateogry_view->IsVisible())
      continue;

    ResultView* result_view = cateogry_view->GetChildView();
    if (result_view)
    {
      // concatenate textures
      std::vector<ResultViewTexture::Ptr> const& category_textures = result_view->GetResultTextureContainers();
      for (auto iter2 = category_textures.begin(); iter2 != category_textures.end(); ++iter2)
      {
        ResultViewTexture::Ptr const& result_texture = *iter2;
        result_texture->category_index = category_index;
        textures.push_back(result_texture);
      }
    }
  }
  return textures;
}

void ScopeView::RenderResultTexture(ResultViewTexture::Ptr const& result_texture)
{
  ResultView* result_view = GetResultViewForCategory(result_texture->category_index);
  if (result_view)
    result_view->RenderResultTexture(result_texture);
}

void ScopeView::PushFilterExpansion(bool expand)
{
  filter_expansion_pushed_ = filters_expanded;
  filters_expanded = expand;
}

void ScopeView::PopFilterExpansion()
{
  filters_expanded = GetPushedFilterExpansion();
}

bool ScopeView::GetPushedFilterExpansion() const
{
  return filter_expansion_pushed_;
}

PlacesGroup::Ptr ScopeView::CreatePlacesGroup(Category const& category)
{
  return PlacesGroup::Ptr(new PlacesGroup(dash::Style::Instance()));
}

ScopeView::CategoryGroups ScopeView::GetOrderedCategoryViews() const
{
  CategoryGroups category_view_ordered;
  for (auto iter = category_order_.begin(); iter != category_order_.end(); ++iter)
  {
    unsigned int category_index = *iter;
    if (category_views_.size() <= category_index)
     continue;

    PlacesGroup::Ptr group = category_views_[category_index];
    category_view_ordered.push_back(group);
  }
  return category_view_ordered;
}

// Introspectable
std::string ScopeView::GetName() const
{
  return "ScopeView";
}

void ScopeView::AddProperties(GVariantBuilder* builder)
{
  unity::variant::BuilderWrapper(builder)
    .add("name", scope_->id)
    .add("scope-name", scope_->name)
    .add("visible", IsVisible())
    .add("no-results-active", no_results_active_);
}

void ScopeView::OnCompositorKeyNavFocusChanged(nux::Area* area, bool has_focus, nux::KeyNavDirection)
{
  if (!IsVisible())
    return;

  LOG_DEBUG(focus_logger) << "Global focus changed to  " << (area ? area->Type().name : "NULL");

  if (area && has_focus)
  {
    // If we've change the focus to a places group child, then we need to update it's focus.
    bool found_group = false;
    while(area)
    {
      if (area->Type().IsDerivedFromType(PlacesGroup::StaticObjectType))
      {
        found_group = true;
        break;
      }
      // opimise to break out if we reach this level as it will never be a group.
      else if (area == this)
        break;
      area = area->GetParentObject();
    }

    if (!found_group && current_focus_category_position_ != -1)
    {
      LOG_DEBUG(focus_logger) << "Resetting focus for position " << current_focus_category_position_;
      current_focus_category_position_ = -1;
      current_focus_variant_ = nullptr;
    }
  }
}

void ScopeView::PushResultFocus(const char* reason)
{  
  int current_category_position = 0;
  for (auto iter = category_order_.begin(); iter != category_order_.end(); ++iter)
  {
    unsigned category_index = *iter;
    if (category_views_.size() <= category_index)
      continue;
    PlacesGroup::Ptr group = category_views_[category_index];
    if (!group || !group->IsVisible())
      continue;

    nux::Area* focus_area = nux::GetWindowCompositor().GetKeyFocusArea();
    while(focus_area)
    {
      if (focus_area == group.GetPointer())
      {
        current_focus_category_position_ = current_category_position;
        current_focus_variant_ = group->GetCurrentFocus();
        LOG_DEBUG(focus_logger) << "Saving focus for position " << current_focus_category_position_ << " due to '" << reason << "'";
        break;
      }
      // opimise to break out if we reach this level as it will never be a group.
      else if (focus_area == this)
        break;
      focus_area = focus_area->GetParentObject();
    }
    current_category_position++;
  }
}

void ScopeView::PopResultFocus(const char* reason)
{
  int current_category_position = 0;
  for (auto iter = category_order_.begin(); iter != category_order_.end(); ++iter)
  {
    unsigned category_index = *iter;
    if (category_views_.size() <= category_index)
      continue;
    PlacesGroup::Ptr group = category_views_[category_index];
    if (!group || !group->IsVisible())
      continue;

    if (current_category_position == current_focus_category_position_)
    {
      group->SetCurrentFocus(current_focus_variant_);
      LOG_DEBUG(focus_logger) << "Restoring focus for position " << current_focus_category_position_ << " due to '" << reason << "'";
      break;
    }
    current_category_position++;
  }
}

}
}
