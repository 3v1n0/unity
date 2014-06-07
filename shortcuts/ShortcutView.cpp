/*
 * Copyright (C) 2011-2013 Canonical Ltd
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
 *              Marco Trevisan <marco.trevisan@canonical.com>
 */

#include "ShortcutView.h"

#include <glib/gi18n-lib.h>
#include <Nux/VLayout.h>
#include <UnityCore/ConnectionManager.h>
#include <UnityCore/GLibWrapper.h>

#include "unity-shared/LineSeparator.h"
#include "unity-shared/StaticCairoText.h"

namespace unity
{
namespace shortcut
{
namespace
{
  const std::string FONT_NAME = "Ubuntu";
  const unsigned MAIN_TITLE_FONT_SIZE = 15;
  const unsigned SECTION_NAME_FONT_SIZE = 12;
  const unsigned SHORTKEY_ENTRY_FONT_SIZE = 9;
  const RawPixel INTER_SPACE_SHORTKEY_DESCRIPTION = 10_em;
  const RawPixel SHORTKEY_COLUMN_DEFAULT_WIDTH = 150_em;
  const RawPixel SHORTKEY_COLUMN_MAX_WIDTH = 350_em;
  const RawPixel DESCRIPTION_COLUMN_DEFAULT_WIDTH = 265_em;
  const RawPixel DESCRIPTION_COLUMN_MAX_WIDTH = 500_em;
  const RawPixel LINE_SPACING = 3_em;
  const RawPixel MAIN_HORIZONTAL_PADDING = 30_em;
  const RawPixel MAIN_VERTICAL_PADDING = 18_em;
  const RawPixel MAIN_CHILDREN_SPACE = 20_em;
  const RawPixel COLUMNS_CHILDREN_SPACE = 30_em;

  // We need this class because SetVisible doesn't work for layouts.
  class SectionView : public nux::View
  {
    public:
      SectionView(NUX_FILE_LINE_DECL)
        : nux::View(NUX_FILE_LINE_PARAM)
      {}

      connection::Wrapper key_changed_conn_;

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
{
  auto main_layout = new nux::VLayout();
  main_layout->SetPadding(MAIN_HORIZONTAL_PADDING.CP(scale), MAIN_VERTICAL_PADDING.CP(scale));
  main_layout->SetSpaceBetweenChildren(MAIN_CHILDREN_SPACE.CP(scale));
  SetLayout(main_layout);

  std::string header = "<b>"+std::string(_("Keyboard Shortcuts"))+"</b>";

  auto* header_view = new StaticCairoText(header, NUX_TRACKER_LOCATION);
  header_view->SetFont(FONT_NAME+" "+std::to_string(MAIN_TITLE_FONT_SIZE));
  header_view->SetLines(-1);
  header_view->SetScale(scale);
  main_layout->AddView(header_view, 1 , nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);

  main_layout->AddView(new HSeparator(), 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);

  columns_layout_ = new nux::HLayout();
  columns_layout_->SetSpaceBetweenChildren(COLUMNS_CHILDREN_SPACE.CP(scale));
  main_layout->AddLayout(columns_layout_, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);

  scale.changed.connect([this, main_layout, header_view] (double scale) {
    main_layout->SetPadding(MAIN_HORIZONTAL_PADDING.CP(scale), MAIN_VERTICAL_PADDING.CP(scale));
    main_layout->SetSpaceBetweenChildren(MAIN_CHILDREN_SPACE.CP(scale));
    columns_layout_->SetSpaceBetweenChildren(COLUMNS_CHILDREN_SPACE.CP(scale));
    header_view->SetScale(scale);
    RenderColumns();
  });
}

void View::SetModel(Model::Ptr model)
{
  model_ = model;

  if (model_)
    model_->categories_per_column.changed.connect(sigc::hide(sigc::mem_fun(this, &View::RenderColumns)));

  // Fills the columns...
  RenderColumns();
}

Model::Ptr View::GetModel()
{
  return model_;
}

nux::LinearLayout* View::CreateSectionLayout(std::string const& section_name)
{
  nux::VLayout* layout = new nux::VLayout(NUX_TRACKER_LOCATION);

  std::string name("<b>"+glib::String(g_markup_escape_text(section_name.c_str(), -1)).Str()+"</b>");
  auto* section_name_view = new StaticCairoText(name, NUX_TRACKER_LOCATION);
  section_name_view->SetFont(FONT_NAME+" "+std::to_string(SECTION_NAME_FONT_SIZE));
  section_name_view->SetLines(-1);
  section_name_view->SetScale(scale);
  const int top_space = (10_em).CP(scale);
  const int bottom_space = (15_em).CP(scale);
  layout->AddView(new nux::SpaceLayout(top_space, top_space, top_space, top_space), 0, nux::MINOR_POSITION_START, nux::MINOR_SIZE_MATCHCONTENT);
  layout->AddView(section_name_view, 0, nux::MINOR_POSITION_START, nux::MINOR_SIZE_MATCHCONTENT);
  layout->AddView(new nux::SpaceLayout(bottom_space, bottom_space, bottom_space, bottom_space), 0, nux::MINOR_POSITION_START, nux::MINOR_SIZE_MATCHCONTENT);

  return layout;
}

nux::View* View::CreateShortKeyEntryView(AbstractHint::Ptr const& hint, StaticCairoText* shortkey_view, StaticCairoText* description_view)
{
  auto* view = new SectionView(NUX_TRACKER_LOCATION);

  nux::HLayout* layout = new nux::HLayout("EntryLayout", NUX_TRACKER_LOCATION);
  view->SetLayout(layout);

  nux::HLayout* shortkey_layout = new nux::HLayout(NUX_TRACKER_LOCATION);
  nux::HLayout* description_layout = new nux::HLayout(NUX_TRACKER_LOCATION);

  shortkey_layout->AddView(shortkey_view, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);
  shortkey_layout->SetContentDistribution(nux::MAJOR_POSITION_START);

  description_layout->AddView(description_view, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);
  description_layout->SetContentDistribution(nux::MAJOR_POSITION_START);

  layout->AddLayout(shortkey_layout, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);
  layout->AddLayout(description_layout, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);
  layout->SetSpaceBetweenChildren(INTER_SPACE_SHORTKEY_DESCRIPTION.CP(scale));
  description_layout->SetContentDistribution(nux::MAJOR_POSITION_START);

  view->key_changed_conn_ = hint->shortkey.changed.connect([this, view, shortkey_view] (std::string const& key) {
    std::string escaped = glib::String(g_markup_escape_text(key.c_str(), -1)).Str();

    if (!escaped.empty())
      escaped = "<b>"+escaped+"</b>";

    shortkey_view->SetText(escaped);
    shortkey_view->SetVisible(!escaped.empty());
    view->SetVisible(shortkey_view->IsVisible());
    QueueRelayout();
    QueueDraw();
  });

  view->SetVisible(shortkey_view->IsVisible());

  return view;
}

StaticCairoText* View::CreateShortcutTextView(std::string const& text, bool bold)
{
  std::string escaped = glib::String(g_markup_escape_text(text.c_str(), -1)).Str();

  if (bold && !text.empty())
    escaped = "<b>"+escaped+"</b>";

  auto* text_view = new StaticCairoText(escaped, NUX_TRACKER_LOCATION);
  text_view->SetTextAlignment(StaticCairoText::AlignState::NUX_ALIGN_LEFT);
  text_view->SetFont(FONT_NAME+" "+std::to_string(SHORTKEY_ENTRY_FONT_SIZE));
  text_view->SetLines(-1);
  text_view->SetScale(scale);
  text_view->SetVisible(!escaped.empty());

  return text_view;
}

nux::LinearLayout* View::CreateIntermediateLayout()
{
  nux::VLayout* layout = new nux::VLayout(NUX_TRACKER_LOCATION);
  layout->SetSpaceBetweenChildren(LINE_SPACING.CP(scale));

  return layout;
}

nux::Geometry View::GetBackgroundGeometry()
{
  return GetGeometry();
}

void View::DrawOverlay(nux::GraphicsEngine& GfxContext, bool force_draw, nux::Geometry const& clip)
{
  view_layout_->ProcessDraw(GfxContext, force_draw);
}

void View::PreLayoutManagement()
{
  UnityWindowView::PreLayoutManagement();

  for (auto const& column : shortkeys_)
  {
    int min_width = SHORTKEY_COLUMN_DEFAULT_WIDTH.CP(scale);

    for (auto* shortkey : column)
      min_width = std::min(std::max(min_width, shortkey->GetTextExtents().width), shortkey->GetMaximumWidth());

    for (auto* shortkey : column)
      shortkey->SetMinimumWidth(min_width);
  }

  for (auto const& column : descriptions_)
  {
    int min_width = DESCRIPTION_COLUMN_DEFAULT_WIDTH.CP(scale);

    for (auto* description : column)
      min_width = std::min(std::max(min_width, description->GetTextExtents().width), description->GetMaximumWidth());

    for (auto* description : column)
      description->SetMinimumWidth(min_width);
  }
}

void View::RenderColumns()
{
  columns_layout_->Clear();
  shortkeys_.clear();
  descriptions_.clear();

  if (!model_)
  {
    ComputeContentSize();
    QueueRelayout();
    return;
  }

  int i = 0;
  int column_idx = 0;
  auto const& columns = columns_layout_->GetChildren();
  auto const& categories = model_->categories();
  const int categories_per_column = model_->categories_per_column();
  const int columns_number = categories.size() / categories_per_column + 1;
  const int top_space = (23_em).CP(scale);
  const int bottom_space = (20_em).CP(scale);
  const int max_shortkeys_width = SHORTKEY_COLUMN_MAX_WIDTH.CP(scale);
  const int max_descriptions_width = DESCRIPTION_COLUMN_MAX_WIDTH.CP(scale);

  shortkeys_.resize(columns_number);
  descriptions_.resize(columns_number);

  for (auto const& category : categories)
  {
    // Computing column index based on current index
    column_idx = i/categories_per_column;

    nux::LinearLayout* section_layout = CreateSectionLayout(category);
    nux::LinearLayout* intermediate_layout = CreateIntermediateLayout();
    intermediate_layout->SetContentDistribution(nux::MAJOR_POSITION_START);

    for (auto const& hint : model_->hints().at(category))
    {
      StaticCairoText* shortkey = CreateShortcutTextView(hint->shortkey(), true);
      shortkey->SetMaximumWidth(max_shortkeys_width);
      shortkeys_[column_idx].push_back(shortkey);

      StaticCairoText* description = CreateShortcutTextView(hint->description(), false);
      description->SetMaximumWidth(max_descriptions_width);
      descriptions_[column_idx].push_back(description);

      nux::View* view = CreateShortKeyEntryView(hint, shortkey, description);
      intermediate_layout->AddView(view, 0, nux::MINOR_POSITION_START, nux::MINOR_SIZE_FULL);
    }

    section_layout->AddLayout(intermediate_layout, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);

    if ((i + 1) % categories_per_column != 0 && category != categories.back())
    {
      // Add a line with some padding after and before each category that is not
      // the last of the column.
      section_layout->AddView(new nux::SpaceLayout(top_space, top_space, top_space, top_space), 0, nux::MINOR_POSITION_START, nux::MINOR_SIZE_MATCHCONTENT);
      section_layout->AddView(new HSeparator(), 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
      section_layout->AddView(new nux::SpaceLayout(bottom_space, bottom_space, bottom_space, bottom_space), 0, nux::MINOR_POSITION_START, nux::MINOR_SIZE_MATCHCONTENT);
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

    ++i;
  }

  ComputeContentSize();
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
