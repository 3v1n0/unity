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

#include "HomeView.h"

#include <boost/lexical_cast.hpp>

#include <NuxCore/Logger.h>

#include "DashStyle.h"
#include "ResultRendererTile.h"
#include "UBusMessages.h"

namespace unity
{
namespace dash
{

namespace
{
nux::logging::Logger logger("unity.dash.homeview");
}

// This is so we can override the scroll bar for the view.
class HomeScrollView: public nux::ScrollView
{
public:
  HomeScrollView(nux::VScrollBar* scroll_bar, NUX_FILE_LINE_DECL)
    : nux::ScrollView(NUX_FILE_LINE_PARAM)
  {
    SetVScrollBar(scroll_bar);
  }
};



NUX_IMPLEMENT_OBJECT_TYPE(HomeView);

HomeView::HomeView()
  : fix_renderering_id_(0)
{
  SetupViews();

  search_string.changed.connect([&](std::string const& search)
  {
    for (auto lens : lenses_)
      lens->GlobalSearch(search);

    for (auto group: categories_)
    {
      group->SetVisible(search != "" && counts_[group]);
    }
    home_view_->SetVisible(search == "");
    scroll_view_->SetVisible(search != "");

    QueueDraw();
  });
}

HomeView::~HomeView()
{
  if (fix_renderering_id_)
    g_source_remove(fix_renderering_id_);
}

void HomeView::SetupViews()
{
  layout_ = new nux::HLayout(NUX_TRACKER_LOCATION);

  scroll_view_ = new HomeScrollView(new PlacesVScrollBar(NUX_TRACKER_LOCATION),
                                    NUX_TRACKER_LOCATION);
  scroll_view_->EnableVerticalScrollBar(true);
  scroll_view_->EnableHorizontalScrollBar(false);
  scroll_view_->SetVisible(false);
  layout_->AddView(scroll_view_);

  scroll_layout_ = new nux::VLayout();
  scroll_view_->SetLayout(scroll_layout_);

  home_view_ = new PlacesHomeView();
  layout_->AddView(home_view_);

  SetLayout(layout_);
}

void HomeView::AddLens(Lens::Ptr lens)
{
  lenses_.push_back(lens);

  std::string name = lens->name;
  std::string icon_hint = lens->icon_hint;

  LOG_DEBUG(logger) << "Lens added " << name;

  PlacesGroup* group = new PlacesGroup();
  group->SetName(name.c_str());
  group->SetIcon(icon_hint.c_str());
  group->SetExpanded(false);
  group->SetVisible(false);
  group->expanded.connect(sigc::mem_fun(this, &HomeView::OnGroupExpanded));
  categories_.push_back(group);
  counts_[group] = 0;

  ResultViewGrid* grid = new ResultViewGrid(NUX_TRACKER_LOCATION);
  grid->expanded = false;
  grid->SetModelRenderer(new ResultRendererTile(NUX_TRACKER_LOCATION));
  grid->UriActivated.connect([&, lens] (std::string const& uri) { uri_activated.emit(uri); lens->Activate(uri); });
  group->SetChildView(grid);

  Results::Ptr results = lens->global_results;
  results->result_added.connect([&, group, grid] (Result const& result)
  {
    grid->AddResult(const_cast<Result&>(result));
    counts_[group]++;
    UpdateCounts(group);
  });

  results->result_removed.connect([&, group, grid] (Result const& result)
  {
    grid->RemoveResult(const_cast<Result&>(result));
    counts_[group]--;
    UpdateCounts(group);
  });


  scroll_layout_->AddView(group, 0);
}

void HomeView::UpdateCounts(PlacesGroup* group)
{
  group->SetCounts(dash::Style::Instance().GetDefaultNColumns(), counts_[group]);
  group->SetVisible(counts_[group]);

  QueueFixRenderering();
}

void HomeView::OnGroupExpanded(PlacesGroup* group)
{
  ResultViewGrid* grid = static_cast<ResultViewGrid*>(group->GetChildView());
  grid->expanded = group->GetExpanded();
  ubus_manager_.SendMessage(UBUS_PLACE_VIEW_QUEUE_DRAW);
}

void HomeView::OnColumnsChanged()
{
  unsigned int columns = dash::Style::Instance().GetDefaultNColumns();

  for (auto group: categories_)
  {
    group->SetCounts(columns, counts_[group]);
  }
}

void HomeView::QueueFixRenderering()
{
  if (fix_renderering_id_)
    return;

  fix_renderering_id_ = g_idle_add_full (G_PRIORITY_DEFAULT, (GSourceFunc)FixRenderering, this, NULL);
}

gboolean HomeView::FixRenderering(HomeView* self)
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

void HomeView::Draw(nux::GraphicsEngine& gfx_context, bool force_draw)
{
  nux::Geometry geo = GetGeometry();

  gfx_context.PushClippingRectangle(geo);
  nux::GetPainter().PaintBackground(gfx_context, geo);
  gfx_context.PopClippingRectangle();
}

void HomeView::DrawContent(nux::GraphicsEngine& gfx_context, bool force_draw)
{
  gfx_context.PushClippingRectangle(GetGeometry());

  layout_->ProcessDraw(gfx_context, force_draw);

  gfx_context.PopClippingRectangle();
}

void HomeView::ActivateFirst()
{
  for (auto lens: lenses_)
  {
    Results::Ptr results = lens->global_results;
    if (results->count())
    {
      Result result = results->RowAtIndex(0);
      if (result.uri != "")
      {
        uri_activated(result.uri);
        lens->Activate(result.uri);
        return;
      }
    }
  }
}


// Keyboard navigation
bool HomeView::AcceptKeyNavFocus()
{
  return false;
}

// Introspectable
std::string HomeView::GetName() const
{
  return "HomeView";
}

void HomeView::AddProperties(GVariantBuilder* builder)
{}


}
}
