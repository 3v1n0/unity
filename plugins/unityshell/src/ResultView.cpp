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


#include "ResultView.h"

#include <Nux/HLayout.h>
#include <Nux/VLayout.h>
#include <Nux/Button.h>
#include <NuxCore/Logger.h>

namespace unity
{
namespace dash
{
namespace
{
nux::logging::Logger logger("unity.dash.results");
}

NUX_IMPLEMENT_OBJECT_TYPE(ResultView);

ResultView::ResultView(NUX_FILE_LINE_DECL)
  : View(NUX_FILE_LINE_PARAM)
  , expanded(true)
  , preview_layout_(NULL)
  , renderer_(NULL)
{
  expanded.changed.connect([&](bool value)
  {
    if (!value && preview_layout_)
    {
      RemoveLayout();
    }
    else if (value && preview_layout_)
    {
      nux::VLayout* layout = new nux::VLayout(NUX_TRACKER_LOCATION);
      preview_spacer_ = new nux::SpaceLayout(200, 200, 200, 200);
      layout->AddLayout(preview_spacer_, 0, nux::MINOR_POSITION_TOP, nux::MINOR_SIZE_FULL);
      layout->AddLayout(preview_layout_, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);

      SetLayout(layout);
    }
    QueueRelayout();
    NeedRedraw();
  });
}

ResultView::~ResultView()
{
  for (auto result : results_)
  {
    renderer_->Unload(result);
  }
  renderer_->UnReference();
}

long int ResultView::ProcessEvent(nux::IEvent& ievent, long int TraverseInfo, long int ProcessEventInfo)
{
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
  renderer->NeedsRedraw.connect([&]()
  {
    NeedRedraw();
  });
  renderer_->SinkReference();

  NeedRedraw();
}

void ResultView::AddResult(Result& result)
{
  results_.push_back(result);
  renderer_->Preload(result);

  NeedRedraw();
}

void ResultView::RemoveResult(Result& result)
{
  ResultList::iterator it;
  std::string uri = result.uri;

  for (it = results_.begin(); it != results_.end(); it++)
  {
    if (result.uri == (*it).uri)
    {
      results_.erase(it);
      break;
    }
  }
  renderer_->Unload(result);
}

void ResultView::SetPreview(PreviewBase* preview, Result& related_result)
{
  if (preview == NULL)
  {
    preview_result_uri_ = "";
    preview_layout_ = NULL;
    RemoveLayout();
  }
  else
  {
    if (preview_layout_ != NULL)
    {
      preview_layout_->UnReference();
    }

    nux::VLayout* other_layout = new nux::VLayout(NUX_TRACKER_LOCATION);

    preview->SetMinimumHeight(600);
    preview_layout_ = new nux::HLayout(NUX_TRACKER_LOCATION);
    preview_layout_->Reference();
    //FIXME - replace with nicer button subclass widgets
    nux::Button* left_arrow = new nux::Button("previous", NUX_TRACKER_LOCATION);
    left_arrow->state_change.connect([&](nux::View * view)
    {
      ResultList::reverse_iterator it;
      std::string next_uri;
      for (it = results_.rbegin(); it != results_.rend(); it++)
      {
        if (preview_result_uri_ == (*it).uri)
        {
          it++;
          if (it == results_.rend())
            next_uri = results_.front().uri;
          else
            next_uri = (*it).uri;

          break;
        }
      }

      ChangePreview.emit(next_uri);
    });

    nux::Button* right_arrow = new nux::Button("next", NUX_TRACKER_LOCATION);
    right_arrow->state_change.connect([&](nux::View * view)
    {
      ResultList::iterator it;
      std::string next_uri;
      for (it = results_.begin(); it != results_.end(); it++)
      {
        if (preview_result_uri_ == (*it).uri)
        {
          it++;
          if (it == results_.end())
            next_uri = results_.front().uri;
          else
            next_uri = (*it).uri;

          break;
        }
      }

      ChangePreview.emit(next_uri);
    });


    preview_layout_->AddView(left_arrow, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);
    preview_layout_->AddView(preview, 1, nux::MINOR_POSITION_CENTER, nux::eFix);
    preview_layout_->AddView(right_arrow, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);
    preview_result_uri_ = related_result.uri;

    if (expanded)
    {
      preview_spacer_ = new nux::SpaceLayout(200, 200, 200, 200);
      other_layout->AddLayout(preview_spacer_, 0, nux::MINOR_POSITION_TOP, nux::MINOR_SIZE_FULL);
      other_layout->AddLayout(preview_layout_, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
      SetLayout(other_layout);
    }
  }
}

long ResultView::ComputeContentSize()
{
  return View::ComputeContentSize();
}


void ResultView::DrawContent(nux::GraphicsEngine& GfxContent, bool force_draw)
{
  nux::Geometry base = GetGeometry();
  GfxContent.PushClippingRectangle(base);

  if (GetCompositionLayout())
    GetCompositionLayout()->ProcessDraw(GfxContent, force_draw);

  GfxContent.PopClippingRectangle();
}

}
}
