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

#ifndef RESULTVIEW_H
#define RESULTVIEW_H

#include <Nux/Nux.h>
#include <Nux/View.h>
#include <dee.h>

#include <UnityCore/GLibSignal.h>
#include <UnityCore/Results.h>

#include "unity-shared/Introspectable.h"
#include "ResultRenderer.h"

namespace unity
{
namespace debug
{
class ResultWrapper;
}

namespace dash
{

struct ResultViewTexture
{
  typedef std::shared_ptr<ResultViewTexture> Ptr;

  unsigned int category_index;
  nux::Geometry abs_geo;
  int row_index;
  nux::ObjectPtr<nux::IOpenGLBaseTexture> texture;
};


class ResultView : public nux::View, public debug::Introspectable
{
public:
  typedef enum ActivateType_
  {
    DIRECT,
    PREVIEW
  } ActivateType;

  NUX_DECLARE_OBJECT_TYPE(ResultView, nux::View);

  ResultView(NUX_FILE_LINE_DECL);
  virtual ~ResultView();

  void SetModelRenderer(ResultRenderer* renderer);
  void SetResultsModel(Results::Ptr const& results);

  unsigned int GetIndexForLocalResult(LocalResult const& local_result);
  LocalResult GetLocalResultForIndex(unsigned int);

  nux::Property<bool> expanded;
  nux::Property<int> results_per_row;
  nux::Property<std::string> unique_id;
  nux::Property<float> desaturation_progress;
  nux::Property<bool> enable_texture_render;
  sigc::signal<void, LocalResult const&, ActivateType, GVariant*> ResultActivated;

  std::string GetName() const;
  ResultIterator GetIteratorAtRow(unsigned row);
  void AddProperties(GVariantBuilder* builder);
  IntrospectableList GetIntrospectableChildren();

  virtual void Activate(LocalResult const& local_result, int index, ActivateType type) = 0;

  std::vector<ResultViewTexture::Ptr> const& GetResultTextureContainers();
  virtual void RenderResultTexture(ResultViewTexture::Ptr const& result_texture);

  virtual void GetResultDimensions(int& rows, int& columns);

protected:
  virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);

  virtual void UpdateRenderTextures();

  virtual void AddResult(Result const& result);
  virtual void RemoveResult(Result const& result);

  unsigned GetNumResults();

  virtual debug::ResultWrapper* CreateResultWrapper(Result const& result, int index);
  virtual void UpdateResultWrapper(debug::ResultWrapper* wrapper, Result const& result, int index);

  void OnEnableRenderToTexture(bool enable_render_to_texture);

  // properties
  ResultRenderer* renderer_;
  Results::Ptr result_model_;
  std::map<std::string, debug::ResultWrapper*> introspectable_children_;

  std::vector<ResultViewTexture::Ptr> result_textures_;

private:
  void OnRowAdded(DeeModel* model, DeeModelIter* iter);
  void OnRowRemoved(DeeModel* model, DeeModelIter* iter);

  Result cached_result_;
  sigc::connection result_added_connection_;
  sigc::connection result_removed_connection_;
};

}
}

#endif //RESULTVIEW_H
