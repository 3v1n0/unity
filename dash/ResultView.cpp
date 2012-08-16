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
#include "unity-shared/IntrospectableWrappers.h"
#include <UnityCore/Variant.h>
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
  , renderer_(NULL)
{
  expanded.changed.connect([&](bool value)
  {
    QueueRelayout();
    NeedRedraw();
  });
}

ResultView::~ResultView()
{
  ClearIntrospectableWrappers();

  for (auto result : results_)
  {
    renderer_->Unload(result);
  }

  renderer_->UnReference();
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

  for (it = results_.begin(); it != results_.end(); ++it)
  {
    if (result.uri == (*it).uri)
    {
      results_.erase(it);
      break;
    }
  }
  renderer_->Unload(result);
}

ResultView::ResultList ResultView::GetResultList()
{
  return results_;
}

// it would be nice to return a result here, but c++ does not have a good mechanism
// for indicating out of bounds errors. so i return the index
unsigned int ResultView::GetIndexForUri(const std::string& uri)
{
  unsigned int index = 0;
  for (auto result : results_)
  {
    if (G_UNLIKELY(result.uri == uri))
      break;

    index++;
  }
  
  return index;
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

std::string ResultView::GetName() const
{
  return "ResultView";
}

void ResultView::AddProperties(GVariantBuilder* builder)
{
  unity::variant::BuilderWrapper(builder)
    .add("expanded", expanded);
}

debug::Introspectable::IntrospectableList ResultView::GetIntrospectableChildren()
{
  ClearIntrospectableWrappers();

  for (auto result: results_)
  {
    introspectable_children_.push_back(new debug::ResultWrapper(result));
  }
  return introspectable_children_;
}

void ResultView::ClearIntrospectableWrappers()
{
  // delete old results, then add new results
  for (auto old_result: introspectable_children_)
  {
    delete old_result;
  }
  introspectable_children_.clear();
}

}
}
