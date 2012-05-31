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
  int SECTION_NAME_FONT_SIZE = 17/1.33;
  int SHORTKEY_ENTRY_FONT_SIZE = 13/1.33;
  int INTER_SPACE_SHORTKEY_DESCRIPTION = 10;
  int SHORTKEY_COLUMN_WIDTH = 150;
  int DESCRIPTION_COLUMN_WIDTH = 265;
  int LINE_SPACING = 5;

  // We need this class because SetVisible doesn't work for layouts.
  class SectionView : public nux::View
  {
    public:
      SectionView(NUX_FILE_LINE_DECL)
        : nux::View(NUX_FILE_LINE_PARAM)
      {
      }

    protected:
      void Draw(nux::GraphicsEngine& graphics_engine, bool force_draw)
      {
      }

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
  layout_ = new nux::VLayout();
  layout_->SetPadding(50, 38);
  layout_->SetSpaceBetweenChildren(20);
  SetLayout(layout_);

  std::string header = "<b>";
  header += _("Keyboard Shortcuts");
  header += "</b>";

  nux::StaticText* header_view = new nux::StaticText(header, NUX_TRACKER_LOCATION);
  header_view->SetTextPointSize(20/1.33);
  header_view->SetFontName("Ubuntu");
  layout_->AddView(header_view, 1 , nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);

  layout_->AddView(new HSeparator(), 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);

  columns_layout_ = new nux::HLayout();
  columns_layout_->SetSpaceBetweenChildren(30);
  layout_->AddLayout(columns_layout_, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);

  // Column 1...
  columns_.push_back(new nux::VLayout());
  columns_layout_->AddLayout(columns_[0], 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);

  // Column 2...
  columns_.push_back(new nux::VLayout());
  columns_layout_->AddLayout(columns_[1], 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
}

View::~View()
{
}

void View::SetModel(Model::Ptr model)
{
  model_ = model;

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

nux::LinearLayout* View::CreateSectionLayout(const char* section_name)
{
  nux::VLayout* layout = new nux::VLayout(NUX_TRACKER_LOCATION);

  std::string name("<b>");
  name += glib::String(g_markup_escape_text(section_name, -1)).Str();
  name += "</b>";

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

  std::string skey = "<b>";
  skey += shortkey.Str();
  skey += "</b>";

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

   auto on_shortkey_changed = [](std::string const& new_shortkey, nux::View* main_view, nux::StaticText* view) {
      std::string skey("<b>");
      skey += new_shortkey;
      skey += "</b>";

      view->SetText(skey);
      main_view->SetVisible(!new_shortkey.empty());
   };

  hint->shortkey.changed.connect(sigc::bind(sigc::slot<void, std::string const&, nux::View*, nux::StaticText*>(on_shortkey_changed), view, shortkey_view));

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
  layout_->ProcessDraw(GfxContext, force_draw);
}

void View::RenderColumns()
{
  int i = 0;
  int column = 0;

  for (auto category : model_->categories())
  {
    // Three sections in the fist column...
    if (i > 2)
      column = 1;

    nux::LinearLayout* section_layout = CreateSectionLayout(category.c_str());
    nux::LinearLayout* intermediate_layout = CreateIntermediateLayout();
    intermediate_layout->SetContentDistribution(nux::MAJOR_POSITION_START);

    for (auto hint : model_->hints()[category])
    {
      nux::View* view = CreateShortKeyEntryView(hint);
      view->SetVisible(!hint->shortkey().empty());
      intermediate_layout->AddView(view, 0, nux::MINOR_POSITION_START, nux::MINOR_SIZE_FULL);
    }

    section_layout->AddLayout(intermediate_layout, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);

    if (i == 0 or i==1 or i==3 or i==4)
    {
      // Add space before the line
      section_layout->AddView(new nux::SpaceLayout(23, 23, 23, 23), 0, nux::MINOR_POSITION_START, nux::MINOR_SIZE_MATCHCONTENT);
      section_layout->AddView(new HSeparator(), 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
      // Add space after the line
      section_layout->AddView(new nux::SpaceLayout(20, 20, 20, 20), 0, nux::MINOR_POSITION_START, nux::MINOR_SIZE_MATCHCONTENT);
    }

    columns_[column]->AddView(section_layout, 1, nux::MINOR_POSITION_START, nux::MINOR_SIZE_FULL);

    i++;
  }
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
