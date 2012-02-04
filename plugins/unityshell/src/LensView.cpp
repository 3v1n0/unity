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

#include "DashStyle.h"
#include "ResultRendererTile.h"
#include "ResultRendererHorizontalTile.h"
#include "UBusMessages.h"
#include "UBusWrapper.h"
#include "PlacesVScrollBar.h"

namespace unity
{
namespace dash
{

namespace
{
nux::logging::Logger logger("unity.dash.lensview");
}

// This is so we can access some protected members in scrollview.
class LensScrollView: public nux::ScrollView
{
public:
  LensScrollView(nux::VScrollBar* scroll_bar, NUX_FILE_LINE_DECL)
    : nux::ScrollView(NUX_FILE_LINE_PARAM)
  {
    SetVScrollBar(scroll_bar);
  }

  void ScrollToPosition(nux::Geometry & position)
  {
    // much of this code is copied from Nux/ScrollView.cpp
    int child_y = position.y - GetGeometry ().y;
    int child_y_diff = child_y - abs (_delta_y);

    if (child_y_diff + position.height < GetGeometry ().height && child_y_diff >= 0)
    {
      return;
    }

    if (child_y_diff < 0)
    {
      ScrollUp (1, abs (child_y_diff));
    }
    else
    {
      int size = child_y_diff - GetGeometry ().height;

      // always keeps the top of a view on the screen
      size += position.height;

      ScrollDown (1, size);
    }
  }
};


NUX_IMPLEMENT_OBJECT_TYPE(LensView);

LensView::LensView()
  : nux::View(NUX_TRACKER_LOCATION)
  , search_string("")
  , filters_expanded(false)
  , can_refine_search(false)
  , no_results_active_(false)
  , fix_renderering_id_(0)
{}

LensView::LensView(Lens::Ptr lens)
  : nux::View(NUX_TRACKER_LOCATION)
  , search_string("")
  , filters_expanded(false)
  , can_refine_search(false)
  , lens_(lens)
  , initial_activation_(true)
  , no_results_active_(false)
  , fix_renderering_id_(0)
{
  SetupViews();
  SetupCategories();
  SetupResults();
  SetupFilters();

  dash::Style::Instance().columns_changed.connect(sigc::mem_fun(this, &LensView::OnColumnsChanged));

  lens_->connected.changed.connect([&](bool is_connected) { if (is_connected) initial_activation_ = true; });
  search_string.changed.connect([&](std::string const& search) { lens_->Search(search);  });
  filters_expanded.changed.connect([&](bool expanded) { fscroll_view_->SetVisible(expanded); QueueRelayout(); OnColumnsChanged(); });
  view_type.changed.connect(sigc::mem_fun(this, &LensView::OnViewTypeChanged));

  ubus_.RegisterInterest(UBUS_RESULT_VIEW_KEYNAV_CHANGED, [this] (GVariant* data) {
    // we get this signal when a result view keynav changes,
    // its a bad way of doing this but nux ABI needs to be broken
    // to do it properly
    nux::Geometry focused_pos;
    g_variant_get (data, "(iiii)", &focused_pos.x, &focused_pos.y, &focused_pos.width, &focused_pos.height);

    for (auto it = categories_.begin(); it != categories_.end(); it++)
    {
      if ((*it)->GetLayout() != nullptr)
      {
        nux::View *child = (*it)->GetChildView();
        if (child->HasKeyFocus())
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

}

LensView::~LensView()
{
  if (fix_renderering_id_)
    g_source_remove(fix_renderering_id_);
}

void LensView::SetupViews()
{
  layout_ = new nux::HLayout(NUX_TRACKER_LOCATION);
  
  layout_->SetHorizontalExternalMargin(8);

  scroll_view_ = new LensScrollView(new PlacesVScrollBar(NUX_TRACKER_LOCATION),
                                    NUX_TRACKER_LOCATION);
  scroll_view_->EnableVerticalScrollBar(true);
  scroll_view_->EnableHorizontalScrollBar(false);
  layout_->AddView(scroll_view_);

  scroll_layout_ = new nux::VLayout(NUX_TRACKER_LOCATION);
  scroll_view_->SetLayout(scroll_layout_);

  no_results_ = new nux::StaticCairoText ("", NUX_TRACKER_LOCATION);
  no_results_->SetTextColor (nux::Color(1.0f,1.0f,1.0f,1.0f));
  scroll_layout_->AddView (no_results_, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);

  fscroll_view_ = new LensScrollView(new PlacesVScrollBar(NUX_TRACKER_LOCATION),
                                     NUX_TRACKER_LOCATION);
  fscroll_view_->EnableVerticalScrollBar(true);
  fscroll_view_->EnableHorizontalScrollBar(false);
  fscroll_view_->SetVisible(false);
  layout_->AddView(fscroll_view_, 1);

  fscroll_layout_ = new nux::VLayout();
  fscroll_view_->SetLayout(fscroll_layout_);

  filter_bar_ = new FilterBar();
  fscroll_layout_->AddView(filter_bar_);

  SetLayout(layout_);
}

void LensView::SetupCategories()
{
  Categories::Ptr categories = lens_->categories;
  categories->category_added.connect(sigc::mem_fun(this, &LensView::OnCategoryAdded));

  for (unsigned int i = 0; i < categories->count(); ++i)
    OnCategoryAdded(categories->RowAtIndex(i));
}

void LensView::SetupResults()
{
  Results::Ptr results = lens_->results;
  results->result_added.connect(sigc::mem_fun(this, &LensView::OnResultAdded));
  results->result_removed.connect(sigc::mem_fun(this, &LensView::OnResultRemoved));

  for (unsigned int i = 0; i < results->count(); ++i)
    OnResultAdded(results->RowAtIndex(i));
}

void LensView::SetupFilters()
{
  Filters::Ptr filters = lens_->filters;
  filters->filter_added.connect(sigc::mem_fun(this, &LensView::OnFilterAdded));
  filters->filter_removed.connect(sigc::mem_fun(this, &LensView::OnFilterRemoved));

  for (unsigned int i = 0; i < filters->count(); ++i)
    OnFilterAdded(filters->FilterAtIndex(i));
}

void LensView::OnCategoryAdded(Category const& category)
{
  std::string name = category.name;
  std::string icon_hint = category.icon_hint;
  std::string renderer_name = category.renderer_name;
  int index = category.index;

  LOG_DEBUG(logger) << "Category added: " << name
                    << "(" << icon_hint
                    << ", " << renderer_name
                    << ", " << boost::lexical_cast<int>(index) << ")";

  PlacesGroup* group = new PlacesGroup();
  group->SetName(name.c_str());
  group->SetIcon(icon_hint.c_str());
  group->SetExpanded(false);
  group->SetVisible(false);
  group->expanded.connect(sigc::mem_fun(this, &LensView::OnGroupExpanded));


  /* Add the group at the correct offset into the categories vector */
  categories_.insert(categories_.begin() + index, group);

  /* Reset result count */
  counts_[group] = 0;

  ResultViewGrid* grid = new ResultViewGrid(NUX_TRACKER_LOCATION);
  grid->expanded = false;
  if (renderer_name == "tile-horizontal")
    grid->SetModelRenderer(new ResultRendererHorizontalTile(NUX_TRACKER_LOCATION));
  else
    grid->SetModelRenderer(new ResultRendererTile(NUX_TRACKER_LOCATION));

  grid->UriActivated.connect([&] (std::string const& uri) { uri_activated.emit(uri); lens_->Activate(uri); });
  group->SetChildView(grid);

  /* We need the full range of method args so we can specify the offset
   * of the group into the layout */
  scroll_layout_->AddView(group, 0, nux::MinorDimensionPosition::eAbove,
                          nux::MinorDimensionSize::eFull, 100.0f,
                          (nux::LayoutPosition)index);
}

void LensView::OnResultAdded(Result const& result)
{
  try {
    PlacesGroup* group = categories_.at(result.category_index);
    ResultViewGrid* grid = static_cast<ResultViewGrid*>(group->GetChildView());

    std::string uri = result.uri;
    LOG_TRACE(logger) << "Result added: " << uri;

    grid->AddResult(const_cast<Result&>(result));
    counts_[group]++;
    UpdateCounts(group);
  } catch (std::out_of_range& oor) {
    LOG_WARN(logger) << "Result does not have a valid category index: "
                     << boost::lexical_cast<unsigned int>(result.category_index)
                     << ". Is out of range.";
  }
}

void LensView::OnResultRemoved(Result const& result)
{
  try {
    PlacesGroup* group = categories_.at(result.category_index);
    ResultViewGrid* grid = static_cast<ResultViewGrid*>(group->GetChildView());

    std::string uri = result.uri;
    LOG_TRACE(logger) << "Result removed: " << uri;

    grid->RemoveResult(const_cast<Result&>(result));
    counts_[group]--;
    UpdateCounts(group);
  } catch (std::out_of_range& oor) {
    LOG_WARN(logger) << "Result does not have a valid category index: "
                     << boost::lexical_cast<unsigned int>(result.category_index)
                     << ". Is out of range.";
  }
}

void LensView::UpdateCounts(PlacesGroup* group)
{
  unsigned int columns = dash::Style::Instance().GetDefaultNColumns();
  columns -= filters_expanded ? 2 : 0;

  group->SetCounts(columns, counts_[group]);
  group->SetVisible(counts_[group]);

  QueueFixRenderering();
}

void LensView::QueueFixRenderering()
{
  if (fix_renderering_id_)
    return;

  fix_renderering_id_ = g_idle_add_full (G_PRIORITY_DEFAULT, (GSourceFunc)FixRenderering, this, NULL);
}

gboolean LensView::FixRenderering(LensView* self)
{
  std::list<Area*> children = self->scroll_layout_->GetChildren();
  std::list<Area*>::reverse_iterator rit;
  bool found_one = false;

  for (rit = children.rbegin(); rit != children.rend(); ++rit)
  {
    PlacesGroup* group = static_cast<PlacesGroup*>(*rit);

    if (group->IsVisible())
      group->SetDrawSeparator(found_one);

    found_one = group->IsVisible();
  }

  self->fix_renderering_id_ = 0;
  return FALSE;
}

void LensView::CheckNoResults(Lens::Hints const& hints)
{
  gint count = lens_->results()->count();

  if (count == 0 && !no_results_active_)
  {
    gchar *markup;
    Lens::Hints::const_iterator it;
    it = hints.find ("no-results-hint");

    if (it != hints.end())
    {
      markup = g_strdup_printf (
        "<span font_size='large'> %s </span>",
        g_variant_get_string (it->second, NULL));
    }
    else
    {
      markup = g_strdup_printf (
        "<span font_size='large'> Sorry, there is nothing that matches your search. </span>");
    }

    LOG_DEBUG(logger) << "The no-result-hint is: " << markup;

    scroll_layout_->SetContentDistribution (nux::MAJOR_POSITION_CENTER);  

    no_results_active_ = true;
    no_results_->SetText (markup);
  }
  else if (count && no_results_active_)
  {
    scroll_layout_->SetContentDistribution (nux::MAJOR_POSITION_START);  

    no_results_active_ = false;
    no_results_->SetText ("");
  }
}

void LensView::OnGroupExpanded(PlacesGroup* group)
{
  ResultViewGrid* grid = static_cast<ResultViewGrid*>(group->GetChildView());
  grid->expanded = group->GetExpanded();
  ubus_manager_.SendMessage(UBUS_PLACE_VIEW_QUEUE_DRAW);
}

void LensView::OnColumnsChanged()
{
  unsigned int columns = dash::Style::Instance().GetDefaultNColumns();
  columns -= filters_expanded ? 2 : 0;

  for (auto group: categories_)
  {
    group->SetCounts(columns, counts_[group]);
  }
}

void LensView::OnFilterAdded(Filter::Ptr filter)
{
  std::string id = filter->id;
  filter_bar_->AddFilter(filter);

  int width = dash::Style::Instance().GetTileWidth();
  fscroll_view_->SetMinimumWidth(width*2);
  fscroll_view_->SetMaximumWidth(width*2);

  can_refine_search = true;
}

void LensView::OnFilterRemoved(Filter::Ptr filter)
{
  filter_bar_->RemoveFilter(filter);
}

void LensView::OnViewTypeChanged(ViewType view_type)
{
  if (view_type != HIDDEN && initial_activation_)
  {
    /* We reset the lens for ourselves, in case this is a restart or something */
    lens_->Search("");
    initial_activation_ = false;
  }

  lens_->view_type = view_type;
}

void LensView::Draw(nux::GraphicsEngine& gfx_context, bool force_draw)
{
  nux::Geometry geo = GetGeometry();

  gfx_context.PushClippingRectangle(geo);
  nux::GetPainter().PaintBackground(gfx_context, geo);
  gfx_context.PopClippingRectangle();
}

void LensView::DrawContent(nux::GraphicsEngine& gfx_context, bool force_draw)
{
  gfx_context.PushClippingRectangle(GetGeometry());

  layout_->ProcessDraw(gfx_context, force_draw);

  gfx_context.PopClippingRectangle();
}

Lens::Ptr LensView::lens() const
{
  return lens_;
}

void LensView::ActivateFirst()
{
  Results::Ptr results = lens_->results;
  if (results->count())
  {
    for (unsigned int c = 0; c < scroll_layout_->GetChildren().size(); ++c)
    {
      for (unsigned int i = 0; i < results->count(); ++i)
      {
        Result result = results->RowAtIndex(i);
        if (result.category_index == c && result.uri != "")
        {
          uri_activated(result.uri);
          lens_->Activate(result.uri);
          return;
        }
      }
    }
    // Fallback
    Result result = results->RowAtIndex(0);
    if (result.uri != "")
    {
      uri_activated(result.uri);
      lens_->Activate(result.uri);
    }
  }
}

// Keyboard navigation
bool LensView::AcceptKeyNavFocus()
{
  return false;
}

// Introspectable
std::string LensView::GetName() const
{
  return "LensView";
}

void LensView::AddProperties(GVariantBuilder* builder)
{}


}
}
