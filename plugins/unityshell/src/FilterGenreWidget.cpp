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

#include "config.h"
#include "Nux/Nux.h"
#include <glib.h>
#include <glib/gi18n-lib.h>

#include "FilterGenreWidget.h"
#include "FilterGenreButton.h"
#include "FilterBasicButton.h"
#include "PlacesStyle.h"

namespace unity {

  FilterGenre::FilterGenre (NUX_FILE_LINE_DECL)
      : FilterExpanderLabel (_("Categories"), NUX_FILE_LINE_PARAM)
      , all_selected (false) {
    InitTheme();

    all_button_ = new FilterBasicButton(_("All"), NUX_TRACKER_LOCATION);
    all_button_->activated.connect(sigc::mem_fun(this, &FilterGenre::OnAllActivated));
    all_button_->label = _("All");
    all_button_->Reference();

    PlacesStyle* style = PlacesStyle::GetDefault();

    genre_layout_ = new nux::GridHLayout(NUX_TRACKER_LOCATION);
    genre_layout_->ForceChildrenSize(true);
    genre_layout_->SetHeightMatchContent(true);
    genre_layout_->SetVerticalInternalMargin (0);
    genre_layout_->SetHorizontalInternalMargin (0);
    genre_layout_->EnablePartialVisibility (false);

    DashStyle *dash_style = DashStyle::GetDefault();
    int garnish = 2 * dash_style->GetButtonGarnishSize();

    genre_layout_->SetChildrenSize (style->GetTileWidth() - 12,
                                    garnish + style->GetTextLineHeight() * 2);
    genre_layout_->Reference();

    SetRightHandView(all_button_);
    SetContents(genre_layout_);
  }

  FilterGenre::~FilterGenre() {
    all_button_->UnReference();
  }

  void FilterGenre::SetFilter(dash::Filter::Ptr filter)
  {
    filter_ = std::static_pointer_cast<dash::CheckOptionFilter>(filter);

    filter_->option_added.connect (sigc::mem_fun (this, &FilterGenre::OnOptionAdded));
    filter_->option_removed.connect(sigc::mem_fun (this, &FilterGenre::OnOptionRemoved));

    // finally - make sure we are up-todate with our filter list
    dash::CheckOptionFilter::CheckOptions::iterator it;
    dash::CheckOptionFilter::CheckOptions options = filter_->options;
    for (it = options.begin(); it < options.end(); it++)
      OnOptionAdded(*it);

    SetLabel(filter_->name);
  }

  void FilterGenre::OnOptionAdded(dash::FilterOption::Ptr new_filter)
  {
    std::string tmp_label = new_filter->name;

    char* escape = g_markup_escape_text(tmp_label.c_str(), -1);
    std::string label = escape;
    g_free(escape);

    FilterGenreButton* button = new FilterGenreButton (label, NUX_TRACKER_LOCATION);
    button->SetFilter (new_filter);
    genre_layout_->AddView (button, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
    buttons_.push_back (button);
  }

  void FilterGenre::OnOptionRemoved(dash::FilterOption::Ptr removed_filter)
  {
    std::vector<FilterGenreButton*>::iterator it;
    FilterGenreButton* found_filter = NULL;
    for ( it=buttons_.begin() ; it < buttons_.end(); it++ )
    {
      if ((*it)->GetFilter() == removed_filter)
      {
        found_filter = *it;
        break;
      }
    }

    if (found_filter)
    {
      genre_layout_->RemoveChildObject(*it);
      buttons_.erase (it);
    }
  }

  std::string FilterGenre::GetFilterType ()
  {
    return "FilterBasicButton";
  }

  void FilterGenre::InitTheme()
  {
    //FIXME - build theme here - store images, cache them, fun fun fun
  }

  void FilterGenre::OnAllActivated(nux::View *view)
  {
    if (filter_)
      filter_->Clear();
  }

  void FilterGenre::OnGenreActivated(nux::View *view)
  {
    // just check to see if all the filters are active or not
    // if they are, then set the all status to true
  }


  long int FilterGenre::ProcessEvent(nux::IEvent& ievent, long int TraverseInfo, long int ProcessEventInfo) {
    return GetLayout()->ProcessEvent(ievent, TraverseInfo, ProcessEventInfo);
  }

  void FilterGenre::Draw(nux::GraphicsEngine& GfxContext, bool force_draw) {
    nux::Geometry geo = GetGeometry();

    GfxContext.PushClippingRectangle(geo);
    nux::GetPainter().PaintBackground(GfxContext, geo);
    GfxContext.PopClippingRectangle();
  }

  void FilterGenre::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw) {
    GfxContext.PushClippingRectangle(GetGeometry());

    GetLayout()->ProcessDraw(GfxContext, force_draw);

    GfxContext.PopClippingRectangle();
  }

  void FilterGenre::PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw) {
    nux::View::PostDraw(GfxContext, force_draw);
  }

};
