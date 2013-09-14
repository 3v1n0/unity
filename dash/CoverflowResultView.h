// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Jason Smith <jason.smith@canonical.com>
 */

#ifndef UNITY_COVERFLOWRESULTVIEW_H
#define UNITY_COVERFLOWRESULTVIEW_H

#include "ResultView.h"

namespace unity
{
namespace dash
{

class CoverflowResultView : public ResultView
{
  NUX_DECLARE_OBJECT_TYPE(CoverflowResultView, ResultView);

public:
  CoverflowResultView(NUX_FILE_LINE_DECL);
  ~CoverflowResultView();

  virtual void SetModelRenderer(ResultRenderer* renderer);

  virtual void AddResult(Result& result);
  virtual void RemoveResult(Result& result);

  virtual void Activate(LocalResult const& local_result, int index, ActivateType type);

protected:
  virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual long ComputeContentSize();


private:
  struct Impl;
  Impl* pimpl;
};

}
}

#endif
