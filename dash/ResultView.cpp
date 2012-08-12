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

  if (result_model_)
  {
    for (ResultIterator iter(result_model_); !iter.IsLast(); ++iter)
    {
      Result result(*iter);
      renderer_->Unload(result);
    }
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
  renderer_->Preload(result);

  NeedRedraw();
}

void ResultView::RemoveResult(Result& result)
{
  renderer_->Unload(result);
}

void ResultView::OnRowAdded(DeeModel* model, DeeModelIter* iter)
{
  Result result(model, iter, renderer_tag_);
  AddResult(result);
}

void ResultView::OnRowRemoved(DeeModel* model, DeeModelIter* iter)
{
  Result result(model, iter, renderer_tag_);
  RemoveResult(result);
}

void ResultView::SetModel(glib::Object<DeeModel> const& model, DeeModelTag* tag)
{
  // cleanup
  if (result_model_)
  {
    sig_manager_.Disconnect(result_model_.RawPtr(), "row-added");
    sig_manager_.Disconnect(result_model_.RawPtr(), "row-removed");

    for (ResultIterator it(GetIteratorAtRow(0)); !it.IsLast(); ++it)
    {
      Result result(*it);
      RemoveResult(result);
    }
  }

  result_model_ = model;
  renderer_tag_ = tag;

  if (model)
  {
    typedef glib::Signal<void, DeeModel*, DeeModelIter*> RowSignalType;

    sig_manager_.Add(new RowSignalType(model,
                                       "row-added",
                                       sigc::mem_fun(this, &ResultView::OnRowAdded)));
    sig_manager_.Add(new RowSignalType(model,
                                       "row-removed",
                                       sigc::mem_fun(this, &ResultView::OnRowRemoved)));

    for (ResultIterator it(GetIteratorAtRow(0)); !it.IsLast(); ++it)
    {
      Result result(*it);
      AddResult(result);
    }
  }
}

int ResultView::GetNumResults()
{
  if (result_model_)
    return static_cast<int>(dee_model_get_n_rows(result_model_));

  return 0;
}

ResultIterator ResultView::GetIteratorAtRow(int row)
{
  DeeModelIter* iter = NULL;
  if (result_model_)
  {
    iter = row > 0 ? dee_model_get_iter_at_row(result_model_, row) :
      dee_model_get_first_iter(result_model_);
  }
  return ResultIterator(result_model_, iter, renderer_tag_);
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

  if (result_model_)
  {
    for (ResultIterator iter(result_model_); !iter.IsLast(); ++iter)
    {
      introspectable_children_.push_back(new debug::ResultWrapper(*iter));
    }
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
