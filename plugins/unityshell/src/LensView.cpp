/*
 * Copyright (C) 2010 Canonical Ltd
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

#include "PlacesStyle.h"
#include "ResultRendererTile.h"
#include "UBusMessages.h"

namespace unity
{
namespace dash
{

namespace
{
nux::logging::Logger logger("unity.dash.lensview");
}

NUX_IMPLEMENT_OBJECT_TYPE(LensView);

LensView::LensView()
  : nux::View(NUX_TRACKER_LOCATION)
  , search_string("")
  , filters_expanded(false)
{}

LensView::LensView(Lens::Ptr lens)
  : nux::View(NUX_TRACKER_LOCATION)
  , search_string("")
  , filters_expanded(false)
  , lens_(lens)
{
  SetupViews();

  Categories::Ptr categories = lens_->categories;
  categories->category_added.connect(sigc::mem_fun(this, &LensView::OnCategoryAdded));

  Results::Ptr results = lens_->results;
  results->result_added.connect(sigc::mem_fun(this, &LensView::OnResultAdded));
  results->result_removed.connect(sigc::mem_fun(this, &LensView::OnResultRemoved));

  Filters::Ptr filters = lens_->filters;
  filters->filter_added.connect(sigc::mem_fun(this, &LensView::OnFilterAdded));
  filters->filter_removed.connect(sigc::mem_fun(this, &LensView::OnFilterRemoved));

  PlacesStyle::GetDefault()->columns_changed.connect(sigc::mem_fun(this, &LensView::OnColumnsChanged));

  search_string.changed.connect([&](std::string const& search) { lens_->Search(search);  });
  filters_expanded.changed.connect([&](bool expanded) { fscroll_view_->SetVisible(expanded); ubus_manager_.SendMessage(UBUS_PLACE_VIEW_QUEUE_DRAW); }); 
}

LensView::~LensView()
{}

void LensView::SetupViews()
{
  layout_ = new nux::HLayout();

  scroll_view_ = new nux::ScrollView();
  scroll_view_->EnableVerticalScrollBar(true);
  scroll_view_->EnableHorizontalScrollBar(false);
  layout_->AddView(scroll_view_);
  
  scroll_layout_ = new nux::VLayout();
  scroll_view_->SetLayout(scroll_layout_);

  fscroll_view_ = new nux::ScrollView();
  fscroll_view_->SetMaximumWidth(1);
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
  categories_.push_back(group);
  counts_[group] = 0;
  
  ResultViewGrid* grid = new ResultViewGrid(NUX_TRACKER_LOCATION);
  grid->expanded = false;
  grid->SetModelRenderer(new ResultRendererTile(NUX_TRACKER_LOCATION));
  grid->UriActivated.connect([&] (std::string const& uri) { uri_activated.emit(uri); lens_->Activate(uri); });
  group->SetChildView(grid);

  scroll_layout_->AddView(group, 0);
}

void LensView::OnResultAdded(Result const& result)
{
  PlacesGroup* group = categories_[result.category_index];
  ResultViewGrid* grid = static_cast<ResultViewGrid*>(group->GetChildView());

  std::string uri = result.uri;
  LOG_TRACE(logger) << "Result added: " << uri;

  grid->AddResult(const_cast<Result&>(result));
  counts_[group]++;
  UpdateCounts(group);
}

void LensView::OnResultRemoved(Result const& result)
{
  PlacesGroup* group = categories_[result.category_index];
  ResultViewGrid* grid = static_cast<ResultViewGrid*>(group->GetChildView());

  std::string uri = result.uri;
  LOG_TRACE(logger) << "Result removed: " << uri;

  grid->RemoveResult(const_cast<Result&>(result));
  counts_[group]--;
  UpdateCounts(group);
}

void LensView::UpdateCounts(PlacesGroup* group)
{
  PlacesStyle* style = PlacesStyle::GetDefault();

  group->SetCounts(style->GetDefaultNColumns(), counts_[group]);
  group->SetVisible(counts_[group]);
}

void LensView::OnGroupExpanded(PlacesGroup* group)
{
  ResultViewGrid* grid = static_cast<ResultViewGrid*>(group->GetChildView());
  grid->expanded = group->GetExpanded();
  ubus_manager_.SendMessage(UBUS_PLACE_VIEW_QUEUE_DRAW);
}

void LensView::OnColumnsChanged()
{
  unsigned int columns = PlacesStyle::GetDefault()->GetDefaultNColumns();

  for (auto group: categories_)
  {
    group->SetCounts(columns, counts_[group]);
  }
}

void LensView::OnFilterAdded(Filter::Ptr filter)
{
  std::string id = filter->id;
  filter_bar_->AddFilter(filter);

  int width = PlacesStyle::GetDefault()->GetTileWidth();
  fscroll_view_->SetMinimumWidth(width*2);
  fscroll_view_->SetMaximumWidth(width*2);
}

void LensView::OnFilterRemoved(Filter::Ptr filter)
{
  filter_bar_->RemoveFilter(filter);
}

long LensView::ProcessEvent(nux::IEvent& ievent, long traverse_info, long event_info)
{
  return layout_->ProcessEvent(ievent, traverse_info, event_info);
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

// Keyboard navigation
bool LensView::AcceptKeyNavFocus()
{
  return false;
}

// Introspectable
const gchar* LensView::GetName()
{
  return "LensView";
}

void LensView::AddProperties(GVariantBuilder* builder)
{}


}
}
