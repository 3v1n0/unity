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

#include "LensView.h"

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
#include "unity-shared/PlacesVScrollBar.h"
#include "unity-shared/PlacesOverlayVScrollBar.h"
#include "unity-shared/GraphicsUtils.h"

#include "config.h"
#include <glib/gi18n-lib.h>

namespace unity
{
namespace dash
{
DECLARE_LOGGER(logger, "unity.dash.scopeview");
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
  , initial_activation_(true)
  , no_results_active_(false)
  , last_expanded_group_(nullptr)
  , last_good_filter_model_(-1)
  , filter_expansion_pushed_(false)
{
  SetupViews(show_filters);
  SetupCategories();
  SetupResults();
  SetupFilters();

  dash::Style::Instance().columns_changed.connect(sigc::mem_fun(this, &ScopeView::OnColumnsChanged));

  search_string.SetGetterFunction(sigc::mem_fun(this, &ScopeView::get_search_string));
  filters_expanded.changed.connect(sigc::mem_fun(this, &ScopeView::OnScopeFilterExpanded));
  view_type.changed.connect(sigc::mem_fun(this, &ScopeView::OnViewTypeChanged));
  if (scope_)
  {
    scope_->connected.changed.connect([&](bool is_connected) {
      if (is_connected)
        initial_activation_ = true;
    });
  }

  ubus_manager_.RegisterInterest(UBUS_RESULT_VIEW_KEYNAV_CHANGED, [&] (GVariant* data) {
    // we get this signal when a result view keynav changes,
    // its a bad way of doing this but nux ABI needs to be broken
    // to do it properly
    nux::Geometry focused_pos;
    g_variant_get (data, "(iiii)", &focused_pos.x, &focused_pos.y, &focused_pos.width, &focused_pos.height);

    for (auto category : categories_)
    {
      if (category->GetLayout() != nullptr)
      {
        auto expand_label = category->GetHeaderFocusableView();
        auto child = category->GetChildView();

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
  result_added_connection.disconnect();
  result_removed_connection.disconnect();
  category_added_connection.disconnect();
  category_removed_connection.disconnect();
  filter_added_connection.disconnect();
  filter_removed_connection.disconnect();
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

  scroll_view_->geometry_changed.connect([this] (nux::Area *area, nux::Geometry& geo)
  {
    CheckScrollBarState();
  });

  scroll_layout_ = new nux::VLayout(NUX_TRACKER_LOCATION);
  scroll_view_->SetLayout(scroll_layout_);
  scroll_view_->SetRightArea(show_filters);

  no_results_ = new StaticCairoText("", NUX_TRACKER_LOCATION);
  no_results_->SetTextColor(nux::color::White);
  no_results_->SetVisible(false);
  scroll_layout_->AddView(no_results_, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);

  fscroll_view_ = new ScopeScrollView(new PlacesVScrollBar(NUX_TRACKER_LOCATION), NUX_TRACKER_LOCATION);
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

void ScopeView::SetupCategories()
{
  if (!scope_)
    return;

  Categories::Ptr categories = scope_->categories;
  category_added_connection = categories->category_added.connect(sigc::mem_fun(this, &ScopeView::OnCategoryAdded));
  category_removed_connection = categories->category_removed.connect(sigc::mem_fun(this, &ScopeView::OnCategoryRemoved));

  auto resync_categories = [categories, this] (glib::Object<DeeModel> model)
  {
    ClearCategories();
    for (unsigned int i = 0; i < categories->count(); ++i)
      OnCategoryAdded(categories->RowAtIndex(i));
  };

  categories->model.changed.connect(resync_categories);
  resync_categories(categories->model());
}

void ScopeView::SetupResults()
{
  if (!scope_)
    return;

  Results::Ptr results = scope_->results;
  result_added_connection = results->result_added.connect(sigc::mem_fun(this, &ScopeView::OnResultAdded));
  result_added_connection = results->result_removed.connect(sigc::mem_fun(this, &ScopeView::OnResultRemoved));

  results->model.changed.connect([this] (glib::Object<DeeModel> model)
  {
    for (unsigned int i = 0; i < categories_.size(); ++i)
    {
      ResultViewGrid* grid = GetGridForCategory(i);
      if (grid)
      {
        grid->SetResultsModel(scope_->GetResultsForCategory(i));
      }
    }
  });

  for (unsigned int i = 0; i < results->count(); ++i)
    OnResultAdded(results->RowAtIndex(i));
}

void ScopeView::SetupFilters()
{
  if (!scope_)
    return;

  Filters::Ptr filters = scope_->filters;
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
  unsigned index = (category.index == unsigned(-1)) ? categories_.size() : category.index;
  bool reset_filter_models = false;

  LOG_DEBUG(logger) << "Category added: " << name
                    << "(" << icon_hint
                    << ", " << renderer_name
                    << ", " << boost::lexical_cast<int>(index) << ")";

  if (index < categories_.size())
  {
    // the scope might have restarted and we don't want to create
    // new PlacesGroup if we can reuse the old one
    PlacesGroup* existing_group = categories_.at(index);
    if (existing_group->GetCategoryIndex() == index)
      return;
  }

  PlacesGroup* group = CreatePlacesGroup();
  AddChild(group);
  group->SetName(name);
  group->SetIcon(icon_hint);
  group->SetCategoryIndex(index);
  group->SetExpanded(false);
  group->SetVisible(false);
  group->expanded.connect(sigc::mem_fun(this, &ScopeView::OnGroupExpanded));

  reset_filter_models = index < categories_.size();
  /* Add the group at the correct offset into the categories vector */
  categories_.insert(categories_.begin() + index, group);

  /* Reset result count */
  counts_[group] = 0;

  ResultView* grid;

  if (renderer_name == "tile-horizontal")
  {
    grid = new ResultViewGrid(NUX_TRACKER_LOCATION);
    grid->SetModelRenderer(new ResultRendererHorizontalTile(NUX_TRACKER_LOCATION));
    static_cast<ResultViewGrid*> (grid)->horizontal_spacing = CARD_VIEW_GAP_HORIZ;
    static_cast<ResultViewGrid*> (grid)->vertical_spacing = CARD_VIEW_GAP_VERT;
  }
  /*
   * The flow renderer is disabled for now, expecting return later
  else if (renderer_name == "flow" && nux::GetWindowThread()->GetGraphicsEngine().UsingGLSLCodePath())
  {
    grid = new CoverflowResultView(NUX_TRACKER_LOCATION);
    grid->SetModelRenderer(new ResultRendererTile(NUX_TRACKER_LOCATION));
    group->SetHeaderCountVisible(false);
  }
  */
  else
  {
    grid = new ResultViewGrid(NUX_TRACKER_LOCATION);
    grid->SetModelRenderer(new ResultRendererTile(NUX_TRACKER_LOCATION));
  }
  group->SetChildView(grid);

  if (scope_)
  {
    std::string unique_id = name + scope_->name();
    grid->unique_id = unique_id;
    grid->expanded = false;

    group->SetRendererName(renderer_name.c_str());
    grid->UriActivated.connect([this, unique_id] (std::string const& uri, ResultView::ActivateType type, GVariant* data) 
    {
      uri_activated.emit(type, uri, data, unique_id); 
      switch (type)
      {
        case ResultView::ActivateType::DIRECT:
        {
          scope_->Activate(uri);
        } break;
        case ResultView::ActivateType::PREVIEW:
        {
          scope_->Preview(uri);
        } break;
        default: break;
      };
    });


    /* Set up filter model for this category */
    grid->SetResultsModel(scope_->GetResultsForCategory(index));

    if (reset_filter_models)
    {
      /* HomeLens is reodering the categories, and since their index is based
       * on the row position in the model, we need to re-initialize the filter
       * models if we got insert and not an append */
      for (auto it = categories_.begin() + (index + 1); it != categories_.end(); ++it)
      {
        grid = static_cast<ResultViewGrid*>((*it)->GetChildView());
        grid->SetResultsModel(Results::Ptr());
      }

      if (static_cast<int>(index) < last_good_filter_model_ || last_good_filter_model_ < 0)
      {
        last_good_filter_model_ = index;
      }
      if (!fix_filter_models_idle_)
      {
        fix_filter_models_idle_.reset(new glib::Idle(sigc::mem_fun(this, &ScopeView::ReinitializeFilterModels), glib::Source::Priority::HIGH));
      }
    }
  }

  /* We need the full range of method args so we can specify the offset
   * of the group into the layout */
  scroll_layout_->AddView(group, 0, nux::MinorDimensionPosition::MINOR_POSITION_START,
                          nux::MinorDimensionSize::MINOR_SIZE_FULL, 100.0f,
                          (nux::LayoutPosition)index);
}

void ScopeView::OnCategoryRemoved(Category const& category)
{
  std::string name = category.name;
  std::string icon_hint = category.icon_hint;
  std::string renderer_name = category.renderer_name;
  unsigned index = (category.index == unsigned(-1)) ? categories_.size() : category.index;

  LOG_DEBUG(logger) << "Category removed: " << name
                    << "(" << icon_hint
                    << ", " << renderer_name
                    << ", " << boost::lexical_cast<int>(index) << ")";

  if (index >= categories_.size())
    return;

  auto category_position = categories_.begin() + index;
  PlacesGroup* existing_group = *category_position;
  categories_.erase(category_position);
  counts_.erase(existing_group);

  RemoveChild(existing_group);
  scroll_layout_->RemoveChildObject(existing_group);
  QueueRelayout();
}

void ScopeView::ClearCategories()
{
  for (auto category_position = categories_.begin(), end = categories_.end(); category_position != end; ++category_position)
  {
    PlacesGroup* group = *category_position;
    RemoveChild(group);
    scroll_layout_->RemoveChildObject(group);
  }
  counts_.clear();
  categories_.clear();
  QueueRelayout();
}

bool ScopeView::ReinitializeFilterModels()
{
  if (!scope_)
    return false;

  Results::Ptr results_model = scope_->results;
  for (unsigned i = last_good_filter_model_ + 1; i < categories_.size(); ++i)
  {
    ResultViewGrid* grid = GetGridForCategory(i);
    if (grid)
      grid->SetResultsModel(scope_->GetResultsForCategory(i));
  }

  last_good_filter_model_ = -1;
  fix_filter_models_idle_.reset();
  return false;
}

ResultViewGrid* ScopeView::GetGridForCategory(unsigned category_index)
{
  if (category_index >= categories_.size()) return nullptr;
  PlacesGroup* group = categories_.at(category_index);
  return static_cast<ResultViewGrid*>(group->GetChildView());
}

ResultView* ScopeView::GetResultViewForCategory(unsigned category_index)
{
  if (category_index >= categories_.size()) return nullptr;
  PlacesGroup* group = categories_.at(category_index);
  return static_cast<ResultView*>(group->GetChildView());
}

void ScopeView::OnResultAdded(Result const& result)
{
  try {
    // Anything done in this method needs to be super fast, if in doubt, add
    // it to the model_updated_timeout_ callback!
    PlacesGroup* group = categories_.at(result.category_index);

    std::string uri = result.uri;
    LOG_TRACE(logger) << "Result added: " << uri;

    counts_[group]++;
    UpdateCounts(group);
    // make sure we don't display the no-results-hint if we do have results
    if (G_UNLIKELY (no_results_active_))
    {
      CheckNoResults(glib::HintsMap());
    }

    if (!model_updated_timeout_)
    {
      model_updated_timeout_.reset(new glib::Idle([&] () {
        // Check if all results so far are from one category
        // If so, then expand that category.
        CheckCategoryExpansion();
        model_updated_timeout_.reset();
        return false;
      }, glib::Source::Priority::HIGH));
    }
  } catch (std::out_of_range& oor) {
    LOG_WARN(logger) << "Result does not have a valid category index: "
                     << boost::lexical_cast<unsigned int>(result.category_index)
                     << ". Is out of range.";
  }
}

void ScopeView::OnResultRemoved(Result const& result)
{
  try {
    PlacesGroup* group = categories_.at(result.category_index);

    std::string uri = result.uri;
    LOG_TRACE(logger) << "Result removed: " << uri;

    counts_[group]--;
    UpdateCounts(group);
  } catch (std::out_of_range& oor) {
    LOG_WARN(logger) << "Result does not have a valid category index: "
                     << boost::lexical_cast<unsigned int>(result.category_index)
                     << ". Is out of range.";
  }
}

void ScopeView::UpdateCounts(PlacesGroup* group)
{
  unsigned int columns = dash::Style::Instance().GetDefaultNColumns();
  columns -= filters_expanded ? 2 : 0;

  group->SetCounts(columns, counts_[group]);
  group->SetVisible(counts_[group]);
}

void ScopeView::CheckNoResults(glib::HintsMap const& hints)
{
  gint count = scope_->results()->count();

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

void ScopeView::CheckCategoryExpansion()
{
    int number_of_displayed_categories = 0;

    // Check if we had expanded a group in last run
    // If so, collapse it for now
    if (last_expanded_group_ != nullptr)
    {
        last_expanded_group_->SetExpanded(false);
        last_expanded_group_ = nullptr;
    }

    // Cycle through all categories
    for (auto category : categories_)
    {
        if (counts_[category] > 0) {
            number_of_displayed_categories++;
            last_expanded_group_ = category;
        }
    }

    if (number_of_displayed_categories == 1 && last_expanded_group_ != nullptr)
        last_expanded_group_->SetExpanded(true);
    else
        last_expanded_group_ = nullptr;
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

void ScopeView::PerformSearch(std::string const& search_query, Scope::SearchCallback const& cb)
{
  search_string_ = search_query;
  if (scope_)
  {
    scope_->Search(search_query, cb);
  }
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
  if (scroll_layout_->GetGeometry().height > scroll_view_->GetGeometry().height)
  {
    scroll_view_->EnableVerticalScrollBar(true); 
  }
  else
  {
    scroll_view_->EnableVerticalScrollBar(false); 
  }
}

void ScopeView::OnColumnsChanged()
{
  unsigned int columns = dash::Style::Instance().GetDefaultNColumns();
  columns -= filters_expanded ? 2 : 0;

  for (auto group: categories_)
  {
    group->SetCounts(columns, counts_[group]);
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

  if (view_type != ScopeViewType::HIDDEN && initial_activation_)
  {
    /* We reset the scope for ourselves, in case this is a restart or something */
    scope_->Search(search_string_, [] (glib::HintsMap const&, glib::Error const&) {});
    initial_activation_ = false;
  }

  scope_->view_type = view_type;
}

void ScopeView::OnScopeFilterExpanded(bool expanded)
{
  if (fscroll_view_->IsVisible() != expanded)
  {
    fscroll_view_->SetVisible(expanded);
    QueueRelayout();
    OnColumnsChanged();
  }

  for (auto it = categories_.begin(); it != categories_.end(); ++it)
  {
    (*it)->SetFiltersExpanded(expanded);
  }
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

  if (!IsFullRedraw() && RedirectedAncestor())
  {
    for (PlacesGroup* category : categories_)
    {
      if (category->IsRedrawNeeded() && category->IsVisible())
        graphics::ClearGeometry(category->GetGeometry());  
    }
    if (filter_bar_ && filter_bar_->IsVisible() && filter_bar_->IsRedrawNeeded())
      graphics::ClearGeometry(filter_bar_->GetGeometry());  
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
  unsigned int columns = dash::Style::Instance().GetDefaultNColumns();
  columns -= filters_expanded ? 2 : 0;

  int num_rows = 0;
  for (auto group: categories_)
  {
    if (group->IsVisible())
    {
      num_rows += 1; // The category header

      if (group->GetExpanded() && columns)
        num_rows += ceil(counts_[group] / static_cast<double>(columns));
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
    // the first displayed category might not be categories_[0]
    for (unsigned int i = 0; i < categories_.size(); i++)
    {
      ResultView* result_view = GetResultViewForCategory(i);
      if (result_view == nullptr) continue;
      auto it = result_view->GetIteratorAtRow(0);
      if (!it.IsLast())
      {
        Result result(*it);
        result_view->Activate(result.uri, result_view->GetIndexForUri(result.uri), ResultView::ActivateType::DIRECT);
        return;
      }
    }

    // Fallback
    Result result = results->RowAtIndex(0);
    if (result.uri != "")
    {
      uri_activated.emit(ResultView::ActivateType::DIRECT, result.uri, nullptr, "");
      scope_->Activate(result.uri);
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
  for (auto it = categories_.begin(); it != categories_.end(); ++it)
  {
    if ((*it)->GetChildView()->unique_id == view_id)
    {  if (expand)
      {
        (*it)->PushExpanded();
        (*it)->SetExpanded(true);
      }
      else
      {
        (*it)->PopExpanded();
      }
    }
  }
}

void ScopeView::SetResultsPreviewAnimationValue(float preview_animation)
{
  for (auto it = categories_.begin(); it != categories_.end(); ++it)
  {
    (*it)->SetResultsPreviewAnimationValue(preview_animation);
  }
}

void ScopeView::EnableResultTextures(bool enable_result_textures)
{
  scroll_view_->EnableScrolling(!enable_result_textures);

  for (auto it = categories_.begin(); it != categories_.end(); ++it)
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
  for (unsigned int category_index = 0; category_index < categories_.size(); category_index++)
  {
    PlacesGroup* category = categories_.at(category_index);
    if (!category || !category->IsVisible())
      continue;

    ResultView* result_view = category->GetChildView();
    if (result_view)
    {
      // concatenate textures
      std::vector<ResultViewTexture::Ptr> const& category_textures = result_view->GetResultTextureContainers();
      for (auto it = category_textures.begin(); it != category_textures.end(); ++it)
      {
        ResultViewTexture::Ptr const& result_texture = *it;
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

PlacesGroup* ScopeView::CreatePlacesGroup()
{
  return new PlacesGroup(dash::Style::Instance());
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

}
}
