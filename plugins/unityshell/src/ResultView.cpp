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


#include "ResultView.h"

#include <Nux/HLayout.h>
#include <Nux/Button.h>
namespace unity
{
namespace dash
{

ResultView::ResultView(NUX_FILE_LINE_DECL)
    : View(NUX_FILE_LINE_PARAM)
    , expanded (true)
    , preview_layout_ (NULL)
    , preview_result_ (NULL)
    , renderer_ (NULL)
{

}

ResultView::~ResultView()
{
  renderer_->UnReference();
}

long int ResultView::ProcessEvent(nux::IEvent& ievent, long int TraverseInfo, long int ProcessEventInfo) {
  return TraverseInfo;
}

void ResultView::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{

}

void ResultView::SetModelRenderer(ResultRenderer* renderer)
{
  if (renderer_ != NULL)
    renderer_->UnReference();

  renderer_ = renderer;
  renderer->NeedsRedraw.connect ([&] () { NeedRedraw(); });
  renderer_->SinkReference();

  NeedRedraw();
}

void ResultView::AddResult (Result& result)
{
  results_.push_back (&result);

  auto queue_redraw_lambda = [&] (std::string value) { NeedRedraw(); };
  result.icon_hint.changed.connect (queue_redraw_lambda);
  result.name.changed.connect (queue_redraw_lambda);

  renderer_->Preload(result);

  NeedRedraw();
}

void ResultView::RemoveResult (Result& result)
{
  ResultList::iterator it;
  for (it = results_.begin (); it != results_.end(); it++)
  {
    if (&result == (*it))
    {
      results_.erase (it);
      break;
    }
  }

  renderer_->Unload(result);

  NeedRedraw ();
}

void ResultView::SetPreview (PreviewBase *preview, Result& related_result)
{
  if (preview == NULL)
  {
    preview_result_ = NULL;
    preview_layout_ = NULL;
    SetLayout(NULL);
  }
  else
  {
    if (preview_layout_ != NULL)
    {
      preview_layout_->UnReference();
    }

    preview_layout_ = new nux::HLayout(NUX_TRACKER_LOCATION);
    preview_layout_->Reference();
    //FIXME - replace with nicer button subclass widgets
    nux::Button *left_arrow = new nux::Button("previous", NUX_TRACKER_LOCATION);
    nux::Button *right_arrow = new nux::Button("next", NUX_TRACKER_LOCATION);

    preview_layout_->AddView(left_arrow, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);
    preview_layout_->AddView(preview, 1);
    preview_layout_->AddView(right_arrow, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);
    preview_result_ = &related_result;
  }
}

void ResultView::DrawContent (nux::GraphicsEngine &GfxContent, bool force_draw) {
  nux::Geometry base = GetGeometry ();
  GfxContent.PushClippingRectangle (base);

  if (GetCompositionLayout ())
    GetCompositionLayout ()->ProcessDraw (GfxContent, force_draw);

  GfxContent.PopClippingRectangle();
}

}
}
