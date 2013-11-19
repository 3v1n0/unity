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

#include <Nux/Layout.h>
#include <UnityCore/Variant.h>

#include "unity-shared/IntrospectableWrappers.h"
#include "unity-shared/GraphicsUtils.h"

namespace unity
{
namespace dash
{

NUX_IMPLEMENT_OBJECT_TYPE(ResultView);

ResultView::ResultView(NUX_FILE_LINE_DECL)
  : View(NUX_FILE_LINE_PARAM)
  , expanded(true)
  , desaturation_progress(0.0)
  , enable_texture_render(false)
  , renderer_(NULL)
  , cached_result_(nullptr, nullptr, nullptr)
{
  expanded.changed.connect([this](bool value)
  {
    QueueRelayout();
    NeedRedraw();
  });

  desaturation_progress.changed.connect([this](float value)
  {
    NeedRedraw();
  });

  enable_texture_render.changed.connect(sigc::mem_fun(this, &ResultView::OnEnableRenderToTexture));
}

ResultView::~ResultView()
{
  for( auto wrapper: introspectable_children_)
  {
    delete wrapper.second;
  }
  introspectable_children_.clear();

  for (ResultIterator it(GetIteratorAtRow(0)); !it.IsLast(); ++it)
  {
    renderer_->Unload(*it);
  }

  renderer_->UnReference();
}

void ResultView::SetModelRenderer(ResultRenderer* renderer)
{
  if (renderer_ != NULL)
    renderer_->UnReference();

  renderer_ = renderer;
  renderer->NeedsRedraw.connect([this]()
  {
    NeedRedraw();
  });
  renderer_->SinkReference();

  NeedRedraw();
}

void ResultView::AddResult(Result const& result)
{
  renderer_->Preload(result);

  NeedRedraw();
}

void ResultView::RemoveResult(Result const& result)
{
  renderer_->Unload(result);
}

void ResultView::SetResultsModel(Results::Ptr const& result_model)
{
  // cleanup
  if (result_model_)
  {
    result_connections_.Clear();

    for (ResultIterator it(GetIteratorAtRow(0)); !it.IsLast(); ++it)
      RemoveResult(*it);
  }

  result_model_ = result_model;

  if (result_model_)
  {
    result_connections_.Add(result_model_->result_added.connect(sigc::mem_fun(this, &ResultView::AddResult)));
    result_connections_.Add(result_model_->result_removed.connect(sigc::mem_fun(this, &ResultView::RemoveResult)));
  }
}

int ResultView::GetSelectedIndex() const
{
  return -1;
}

void ResultView::SetSelectedIndex(int index)
{  
}

unsigned ResultView::GetNumResults()
{
  if (result_model_)
    return result_model_->count();

  return 0;
}

ResultIterator ResultView::GetIteratorAtRow(unsigned row)
{
  DeeModelIter* iter = NULL;
  if (result_model_)
  {
    if (result_model_->model())
    {
      iter = row > 0 ? dee_model_get_iter_at_row(result_model_->model(), row) :
        dee_model_get_first_iter(result_model_->model());
      
      return ResultIterator(result_model_->model(), iter, result_model_->GetTag());
    }
  }
  return ResultIterator(glib::Object<DeeModel>());
}

unsigned int ResultView::GetIndexForLocalResult(LocalResult const& local_result)
{
  unsigned int index = 0;
  for (ResultIterator it(GetIteratorAtRow(0)); !it.IsLast(); ++it)
  {
    if ((*it).uri == local_result.uri)
      break;

    index++;
  }

  return index;
}

LocalResult ResultView::GetLocalResultForIndex(unsigned int index)
{
  if (index >= GetNumResults())
    return LocalResult();

  return LocalResult(*GetIteratorAtRow(index));
}

void ResultView::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{}

void ResultView::DrawContent(nux::GraphicsEngine& GfxContent, bool force_draw)
{
  nux::Geometry base = GetGeometry();
  GfxContent.PushClippingRectangle(base);

  if (GetCompositionLayout())
    GetCompositionLayout()->ProcessDraw(GfxContent, force_draw);

  GfxContent.PopClippingRectangle();
}

void ResultView::OnEnableRenderToTexture(bool enable_render_to_texture)
{
  if (!enable_render_to_texture)
  {
    result_textures_.clear();
  }
}

std::vector<ResultViewTexture::Ptr> const& ResultView::GetResultTextureContainers()
{
  UpdateRenderTextures();
  return result_textures_;
}

void ResultView::RenderResultTexture(ResultViewTexture::Ptr const& result_texture)
{
  // Do we need to re-create the texture?
  if (!result_texture->texture.IsValid() ||
       result_texture->texture->GetWidth() != GetWidth() ||
       result_texture->texture->GetHeight() != GetHeight())
  {
    result_texture->texture = nux::GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableDeviceTexture(GetHeight(),
                                                                                                         GetWidth(),
                                                                                                         1,
                                                                                                         nux::BITFMT_R8G8B8A8);

    if (!result_texture->texture.IsValid())
      return;
  }

  nux::GetPainter().PushBackgroundStack();

  graphics::PushOffscreenRenderTarget(result_texture->texture);

  // clear the texture.
  CHECKGL(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));
  CHECKGL(glClear(GL_COLOR_BUFFER_BIT));

  nux::GraphicsEngine& graphics_engine(nux::GetWindowThread()->GetGraphicsEngine());
  nux::Geometry offset_rect = graphics_engine.ModelViewXFormRect(GetGeometry());
  graphics_engine.PushModelViewMatrix(nux::Matrix4::TRANSLATE(-offset_rect.x, -offset_rect.y, 0));

  ProcessDraw(graphics_engine, true);

  graphics_engine.PopModelViewMatrix();
  graphics::PopOffscreenRenderTarget();

  nux::GetPainter().PopBackgroundStack();
}

void ResultView::UpdateRenderTextures()
{
  if (!enable_texture_render)
    return;

  nux::Geometry root_geo(GetAbsoluteGeometry());

  if (result_textures_.size() > 0)
  {
    ResultViewTexture::Ptr const& result_texture = result_textures_[0];
    result_texture->abs_geo.x = root_geo.x;
    result_texture->abs_geo.y = root_geo.y;
    result_texture->abs_geo.width = GetWidth();
    result_texture->abs_geo.height = GetHeight();
  }
  else
  {
    ResultViewTexture::Ptr result_texture(new ResultViewTexture);
    result_texture->abs_geo = root_geo;
    result_texture->row_index = 0;
    result_textures_.push_back(result_texture);
  }
}

std::string ResultView::GetName() const
{
  return "ResultView";
}

void ResultView::GetResultDimensions(int& rows, int& columns)
{
  columns = results_per_row;  
  rows = result_model_ ? ceil(static_cast<double>(result_model_->count()) / static_cast<double>(std::max(1, columns))) : 0.0;
}

void ResultView::AddProperties(GVariantBuilder* builder)
{
  unity::variant::BuilderWrapper(builder)
    .add("expanded", expanded);
}

debug::Introspectable::IntrospectableList ResultView::GetIntrospectableChildren()
{
  // Because the children are in fact wrappers for the results, we can't just re-crate them every time the
  // GetIntrospectableChildren is called; otherwise result property introspection will not work correctly (objects change each time this is called).
  // Therefore, we need to be a bit more clever, and acculumate and cache the wrappers.

  // clear children (no delete).
  RemoveAllChildren();

  std::set<std::string> existing_results;
  // re-create list of children.
  int index = 0;
  if (result_model_)
  {
    for (ResultIterator iter(result_model_->model()); !iter.IsLast(); ++iter)
    {
      Result const& result = *iter;

      debug::ResultWrapper* result_wrapper = NULL;
      auto map_iter = introspectable_children_.find(result.uri);
      // Create new result.
      if (map_iter == introspectable_children_.end())
      {
        result_wrapper = CreateResultWrapper(result, index);
        introspectable_children_[result.uri] = result_wrapper;
      }
      else
      {
        result_wrapper = map_iter->second;
        UpdateResultWrapper(result_wrapper, result, index);
      }

      AddChild(result_wrapper);

      existing_results.insert(result.uri);
      index++;
    }
  }

  // Delete old children.
  auto child_iter = introspectable_children_.begin();
  for (; child_iter != introspectable_children_.end(); )
  {
    if (existing_results.find(child_iter->first) == existing_results.end())
    {
      // delete and remove the child from the map.
      delete child_iter->second;
      child_iter = introspectable_children_.erase(child_iter);
    }
    else
    {
      ++child_iter;
    }
  }

  return debug::Introspectable::GetIntrospectableChildren();
}

debug::ResultWrapper* ResultView::CreateResultWrapper(Result const& result, int index)
{
  return new debug::ResultWrapper(result);
}

void ResultView::UpdateResultWrapper(debug::ResultWrapper* wrapper, Result const& result, int index)
{
}

}
}
