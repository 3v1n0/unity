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
#include <UnityCore/ResultIterator.h>

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
  void SetModel(glib::Object<DeeModel> const& model, DeeModelTag* tag);

  unsigned int GetIndexForUri(const std::string& uri); 
  std::string GetUriForIndex(unsigned int);

  nux::Property<bool> expanded;
  nux::Property<int> results_per_row;
  nux::Property<std::string> unique_id;  
  sigc::signal<void, std::string const&, ActivateType, GVariant*> UriActivated;

  std::string GetName() const;
  ResultIterator GetIteratorAtRow(unsigned row);
  void AddProperties(GVariantBuilder* builder);
  IntrospectableList GetIntrospectableChildren();

  virtual void Activate(std::string const& uri, int index, ActivateType type) = 0;

protected:
  virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual long ComputeContentSize();

  virtual void AddResult(Result& result);
  virtual void RemoveResult(Result& result);

  unsigned GetNumResults();

  static void ChildResultDestructor(debug::Introspectable* child);
  virtual debug::ResultWrapper* CreateResultWrapper(Result const& result, int index);
  virtual void UpdateResultWrapper(debug::ResultWrapper* wrapper, Result const& result, int index);

  // properties
  ResultRenderer* renderer_;
  glib::Object<DeeModel> result_model_;
  DeeModelTag* renderer_tag_;
  glib::SignalManager sig_manager_;
  std::map<std::string, debug::ResultWrapper*> introspectable_children_;

private:
  void OnRowAdded(DeeModel* model, DeeModelIter* iter);
  void OnRowRemoved(DeeModel* model, DeeModelIter* iter);

  Result cached_result_;
};

}
}

#endif //RESULTVIEW_H
