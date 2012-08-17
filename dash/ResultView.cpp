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
  child_map_.clear();
  RemoveAllChildren(&ResultView::ChildResultDestructor);

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
 
  if (results_.size() == 0)
  {
    child_map_.clear();
    RemoveAllChildren(&ResultView::ChildResultDestructor);  // clear children (with delete).
  }

  renderer_->Unload(result);
}

ResultView::ResultList ResultView::GetResultList() const
{
  return results_;
}

// it would be nice to return a result here, but c++ does not have a good mechanism
// for indicating out of bounds errors. so i return the index
unsigned int ResultView::GetIndexForUri(const std::string& uri) const
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

void ResultView::ChildResultDestructor(debug::Introspectable* child)
{
  delete child;
}

debug::Introspectable::IntrospectableList
ResultView::GetIntrospectableChildren()
{
  // Because the children are in fact wrappers for the results, we can't just re-crate them every time the
  // GetIntrospectableChildren is called; otherwise result property introspection will not work correctly (objects change each time this is called).
  // Therefore, we need to be a bit more clever, and acculumate and cache the wrappers.

  // clear children (no delete).
  RemoveAllChildren();

  std::set<std::string> existing_results;

  // re-create list of children.
  int index = 0;
  for (auto result: results_)
  {
    debug::Introspectable* result_wrapper = NULL;
    auto iter = child_map_.find(result.uri);
    // Create new result.
    if (iter == child_map_.end())
    {
      result_wrapper = CreateResultWrapper(result, index);
      child_map_[result.uri] = result_wrapper;
    }
    else
      result_wrapper = iter->second;

    AddChild(result_wrapper);

    existing_results.insert(result.uri);
    index++;
  }

  // Delete old children.
  auto child_iter = child_map_.begin();
  for (; child_iter != child_map_.end(); )
  {
    if (existing_results.find(child_iter->first) == existing_results.end())
    {
      RemoveChild(child_iter->second, &ResultView::ChildResultDestructor);
      child_map_.erase(child_iter);
    }
    else
    {
      ++child_iter;
    }
  }

  return debug::Introspectable::GetIntrospectableChildren();
}

debug::Introspectable* ResultView::CreateResultWrapper(Result const& result, int index) const
{
  return new debug::ResultWrapper(result);
}


}
}
