// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright 2011 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 *
 */

#include <glib.h>
#include "config.h"
#include <glib/gi18n-lib.h>
#include <UnityCore/GLibWrapper.h>

#include "unity-shared/DashStyle.h"
#include "unity-shared/GraphicsUtils.h"
#include "FilterGenreWidget.h"
#include "FilterGenreButton.h"
#include "FilterBasicButton.h"

namespace unity
{
namespace dash
{

NUX_IMPLEMENT_OBJECT_TYPE(FilterGenre);

FilterGenre::FilterGenre(int columns, NUX_FILE_LINE_DECL)
: FilterExpanderLabel(_("Categories"), NUX_FILE_LINE_PARAM)
, all_button_(nullptr)
{
  dash::Style& style = dash::Style::Instance();

  InitTheme();


  genre_layout_ = new nux::GridHLayout(NUX_TRACKER_LOCATION);
  genre_layout_->ForceChildrenSize(true);
  genre_layout_->MatchContentSize(true);
  genre_layout_->SetTopAndBottomPadding(style.GetSpaceBetweenFilterWidgets() - style.GetFilterHighlightPadding(), style.GetFilterHighlightPadding());
  genre_layout_->EnablePartialVisibility(false);

  if (columns == 3)
  {
    genre_layout_->SetChildrenSize((style.GetFilterBarWidth() - 12 * 2) / 3, style.GetFilterButtonHeight());
    genre_layout_->SetSpaceBetweenChildren (12, 12);
  }
  else
  {
    genre_layout_->SetChildrenSize((style.GetFilterBarWidth() - 10 ) / 2, style.GetFilterButtonHeight());
    genre_layout_->SetSpaceBetweenChildren (10, 12);
  }

  SetContents(genre_layout_);
}

FilterGenre::~FilterGenre()
{
}

void FilterGenre::SetFilter(Filter::Ptr const& filter)
{
  filter_ = std::static_pointer_cast<CheckOptionFilter>(filter);

  // all button
  auto show_button_func = [this] (bool show_all_button)
  {
    all_button_ = show_all_button ? new FilterAllButton(NUX_TRACKER_LOCATION) : nullptr;
    SetRightHandView(all_button_);
    if (all_button_)
      all_button_->SetFilter(filter_);
  };
  show_button_func(filter_->show_all_button);
  filter_->show_all_button.changed.connect(show_button_func);
  
  expanded = !filter_->collapsed();

  filter_->option_added.connect(sigc::mem_fun(this, &FilterGenre::OnOptionAdded));
  filter_->option_removed.connect(sigc::mem_fun(this, &FilterGenre::OnOptionRemoved));

  // finally - make sure we are up-todate with our filter list
  for (auto it : filter_->options())
    OnOptionAdded(it);

  SetLabel(filter_->name);
}

void FilterGenre::OnOptionAdded(FilterOption::Ptr const& new_filter)
{
  std::string tmp_label(new_filter->name);

  glib::String escape(g_markup_escape_text(tmp_label.c_str(), -1));
  std::string label(escape.Value());

  FilterGenreButton* button = new FilterGenreButton(label, NUX_TRACKER_LOCATION);
  button->SetFilter(new_filter);
  genre_layout_->AddView(button, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
  buttons_.push_back(button);

  QueueRelayout();
}

void FilterGenre::OnOptionRemoved(FilterOption::Ptr const& removed_filter)
{
  for (auto it=buttons_.begin() ; it != buttons_.end(); ++it)
  {
    if ((*it)->GetFilter() == removed_filter)
    {
      genre_layout_->RemoveChildObject(*it);
      buttons_.erase(it);
      
      QueueRelayout();
      break;
    }
  }
}

std::string FilterGenre::GetFilterType()
{
  return "FilterBasicButton";
}

void FilterGenre::InitTheme()
{
  //FIXME - build theme here - store images, cache them, fun fun fun
}

void FilterGenre::ClearRedirectedRenderChildArea()
{
  for (auto button : buttons_)
  {
    if (button->IsRedrawNeeded())
      graphics::ClearGeometry(button->GetGeometry());
  }
}

} // namespace dash
} // namespace unity
