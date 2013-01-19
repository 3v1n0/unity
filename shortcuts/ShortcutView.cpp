/*
 * Copyright (C) 2011 Canonical Ltd
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
 * Authored by: Andrea Azzarone <azzaronea@gmail.com>
 *              Jay Taoko <jay.taoko@canonical.com>
 */

#include "ShortcutView.h"

#include <glib/gi18n-lib.h>
#include <boost/algorithm/string.hpp>
#include <UnityCore/GLibWrapper.h>

#include "unity-shared/LineSeparator.h"
#include "unity-shared/StaticCairoText.h"
#include "unity-shared/UScreen.h"

namespace unity
{
namespace shortcut
{
namespace
{
  const unsigned SECTION_NAME_FONT_SIZE = 17/1.33;
  const unsigned SHORTKEY_ENTRY_FONT_SIZE = 13/1.33;
  const unsigned INTER_SPACE_SHORTKEY_DESCRIPTION = 10;
  const unsigned SHORTKEY_COLUMN_WIDTH = 150;
  const unsigned DESCRIPTION_COLUMN_WIDTH = 265;
  const unsigned LINE_SPACING = 5;

  // We need this class because SetVisible doesn't work for layouts.
  class SectionView : public nux::View
  {
    public:
      SectionView(NUX_FILE_LINE_DECL)
        : nux::View(NUX_FILE_LINE_PARAM)
      {}

    protected:
      void Draw(nux::GraphicsEngine& graphics_engine, bool force_draw)
      {}

      void DrawContent(nux::GraphicsEngine& graphics_engine, bool force_draw)
      {
        if (GetLayout())
          GetLayout()->ProcessDraw(graphics_engine, force_draw);
      }
  };

} // unnamed namespace

NUX_IMPLEMENT_OBJECT_TYPE(View);

View::View()
  : ui::UnityWindowView()
  , x_adjustment_(0)
  , y_adjustment_(0)
{
  auto main_layout = new nux::VLayout();
  main_layout->SetPadding(50, 38);
  main_layout->SetSpaceBetweenChildren(20);
  SetLayout(main_layout);

  std::string header = "<b>"+std::string(_("Keyboard Shortcuts"))+"</b>";

  nux::StaticText* header_view = new nux::StaticText(header, NUX_TRACKER_LOCATION);
  header_view->SetTextPointSize(20/1.33);
  header_view->SetFontName("Ubuntu");
  main_layout->AddView(header_view, 1 , nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);

  main_layout->AddView(new HSeparator(), 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);

  columns_layout_ = new nux::HLayout();
  columns_layout_->SetSpaceBetweenChildren(30);
  main_layout->AddLayout(columns_layout_, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
}

void View::SetModel(Model::Ptr model)
{
  model_ = model;
  model_->categories_per_column.changed.connect(sigc::hide(sigc::mem_fun(this, &View::RenderColumns)));

  // Fills the columns...
  RenderColumns();
}

Model::Ptr View::GetModel()
{
  return model_;
}

void View::SetAdjustment(int x, int y)
{
  x_adjustment_ = x;
  y_adjustment_ = y;
}

bool View::GetBaseGeometry(nux::Geometry& geo)
{
  UScreen* uscreen = UScreen::GetDefault();
  int primary_monitor = uscreen->GetMonitorWithMouse();
  auto monitor_geo = uscreen->GetMonitorGeometry(primary_monitor);

  int w = GetAbsoluteWidth();
  int h = GetAbsoluteHeight();

  if (x_adjustment_ + w > monitor_geo.width ||
      y_adjustment_ + h > monitor_geo.height)
    return false;

  geo.width = w;
  geo.height = h;

  geo.x = monitor_geo.x + x_adjustment_ + (monitor_geo.width - geo.width -  x_adjustment_) / 2;
  geo.y = monitor_geo.y + y_adjustment_ + (monitor_geo.height - geo.height -  y_adjustment_) / 2;
  return true;
}

nux::LinearLayout* View::CreateSectionLayout(std::string const& section_name)
{
  nux::VLayout* layout = new nux::VLayout(NUX_TRACKER_LOCATION);

  std::string name("<b>"+glib::String(g_markup_escape_text(section_name.c_str(), -1)).Str()+"</b>");
  nux::StaticText* section_name_view = new nux::StaticText(name, NUX_TRACKER_LOCATION);
  section_name_view->SetTextPointSize(SECTION_NAME_FONT_SIZE);
  section_name_view->SetFontName("Ubuntu");
  layout->AddView(new nux::SpaceLayout(10, 10, 10, 10), 0, nux::MINOR_POSITION_START, nux::MINOR_SIZE_MATCHCONTENT);
  layout->AddView(section_name_view, 0, nux::MINOR_POSITION_START, nux::MINOR_SIZE_MATCHCONTENT);
  layout->AddView(new nux::SpaceLayout(15, 15, 15, 15), 0, nux::MINOR_POSITION_START, nux::MINOR_SIZE_MATCHCONTENT);

  return layout;
}

nux::View* View::CreateShortKeyEntryView(AbstractHint::Ptr const& hint)
{
  nux::View* view = new SectionView(NUX_TRACKER_LOCATION);

  nux::HLayout* layout = new nux::HLayout("EntryLayout", NUX_TRACKER_LOCATION);
  view->SetLayout(layout);

  nux::HLayout* shortkey_layout = new nux::HLayout(NUX_TRACKER_LOCATION);
  nux::HLayout* description_layout = new nux::HLayout(NUX_TRACKER_LOCATION);

  glib::String shortkey(g_markup_escape_text(hint->shortkey().c_str(), -1));

  std::string skey = "<b>"+shortkey.Str()+"</b>";
  nux::StaticText* shortkey_view = new nux::StaticText(skey, NUX_TRACKER_LOCATION);
  shortkey_view->SetTextAlignment(nux::StaticText::ALIGN_LEFT);
  shortkey_view->SetFontName("Ubuntu");
  shortkey_view->SetTextPointSize(SHORTKEY_ENTRY_FONT_SIZE);
  shortkey_view->SetMinimumWidth(SHORTKEY_COLUMN_WIDTH);
  shortkey_view->SetMaximumWidth(SHORTKEY_COLUMN_WIDTH);

  glib::String es_desc(g_markup_escape_text(hint->description().c_str(), -1));

  nux::StaticText* description_view = new nux::StaticText(es_desc.Value(), NUX_TRACKER_LOCATION);
  description_view->SetTextAlignment(nux::StaticText::ALIGN_LEFT);
  shortkey_view->SetFontName("Ubuntu");
  description_view->SetTextPointSize(SHORTKEY_ENTRY_FONT_SIZE);
  description_view->SetMinimumWidth(DESCRIPTION_COLUMN_WIDTH);
  description_view->SetMaximumWidth(DESCRIPTION_COLUMN_WIDTH);

  shortkey_layout->AddView(shortkey_view, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);
  shortkey_layout->SetContentDistribution(nux::MAJOR_POSITION_START);
  shortkey_layout->SetMinimumWidth(SHORTKEY_COLUMN_WIDTH);
  shortkey_layout->SetMaximumWidth(SHORTKEY_COLUMN_WIDTH);

  description_layout->AddView(description_view, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);
  description_layout->SetContentDistribution(nux::MAJOR_POSITION_START);
  description_layout->SetMinimumWidth(DESCRIPTION_COLUMN_WIDTH);
  description_layout->SetMaximumWidth(DESCRIPTION_COLUMN_WIDTH);

  layout->AddLayout(shortkey_layout, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);
  layout->AddLayout(description_layout, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);
  layout->SetSpaceBetweenChildren(INTER_SPACE_SHORTKEY_DESCRIPTION);
  description_layout->SetContentDistribution(nux::MAJOR_POSITION_START);

  hint->shortkey.changed.connect([this, view, shortkey_view] (std::string const& new_key) {
    bool enabled = !new_key.empty();
    shortkey_view->SetText(enabled ? "<b>"+new_key+"</b>" : "");
    view->SetVisible(enabled);
    QueueRelayout();
  });

  view->SetVisible(!shortkey.Str().empty());

  return view;
}

nux::LinearLayout* View::CreateIntermediateLayout()
{
  nux::VLayout* layout = new nux::VLayout(NUX_TRACKER_LOCATION);
  layout->SetSpaceBetweenChildren(LINE_SPACING);

  return layout;
}

nux::Geometry View::GetBackgroundGeometry()
{
  nux::Geometry base = GetGeometry();
  nux::Geometry background_geo;

  background_geo.width = base.width;
  background_geo.height = base.height;
  background_geo.x = (base.width - background_geo.width)/2;
  background_geo.y = (base.height - background_geo.height)/2;

  return background_geo;
}

void View::DrawOverlay(nux::GraphicsEngine& GfxContext, bool force_draw, nux::Geometry clip)
{
  view_layout_->ProcessDraw(GfxContext, force_draw);
}

void View::RenderColumns()
{
  columns_layout_->Clear();

  int i = 0;
  int column_idx = 0;
  auto const& columns = columns_layout_->GetChildren();

  for (auto const& category : model_->categories())
  {
    // Three sections in the fist column...
    column_idx = i/model_->categories_per_column();

    nux::LinearLayout* section_layout = CreateSectionLayout(category);
    nux::LinearLayout* intermediate_layout = CreateIntermediateLayout();
    intermediate_layout->SetContentDistribution(nux::MAJOR_POSITION_START);

    for (auto const& hint : model_->hints().at(category))
    {
      nux::View* view = CreateShortKeyEntryView(hint);
      intermediate_layout->AddView(view, 0, nux::MINOR_POSITION_START, nux::MINOR_SIZE_FULL);
    }

    section_layout->AddLayout(intermediate_layout, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);

    if ((i + 1) % model_->categories_per_column() != 0 && category != model_->categories().back())
    {
      // Add a line with some padding after and before each category that is not
      // the last of the column.
      section_layout->AddView(new nux::SpaceLayout(23, 23, 23, 23), 0, nux::MINOR_POSITION_START, nux::MINOR_SIZE_MATCHCONTENT);
      section_layout->AddView(new HSeparator(), 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
      section_layout->AddView(new nux::SpaceLayout(20, 20, 20, 20), 0, nux::MINOR_POSITION_START, nux::MINOR_SIZE_MATCHCONTENT);
    }

    nux::VLayout* column = nullptr;
    auto column_it = std::next(columns.begin(), column_idx);

    if (column_it == columns.end())
    {
      column = new nux::VLayout();
      columns_layout_->AddLayout(column, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
    }
    else
    {
      column = static_cast<nux::VLayout*>(*column_it);
    }

    column->AddView(section_layout, 1, nux::MINOR_POSITION_START, nux::MINOR_SIZE_FULL);

    i++;
  }

  QueueRelayout();
}

//
// Introspectable methods
//
std::string View::GetName() const
{
  return "ShortcutView";
}

} // namespace shortcut
} // namespace unity
