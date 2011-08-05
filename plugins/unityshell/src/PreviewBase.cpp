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
#include <Nux/Nux.h>
#include <Nux/Layout.h>
#include "PreviewBase.h"

namespace unity {
  PreviewBase::PreviewBase (dash::Preview::Ptr preview, NUX_FILE_LINE_DECL)
    : View (NUX_FILE_LINE_PARAM)
  {
  }
  PreviewBase::PreviewBase (NUX_FILE_LINE_DECL)
    : View (NUX_FILE_LINE_PARAM)
  {
  }

  PreviewBase::~PreviewBase ()
  {
  }

  void PreviewBase::Draw (nux::GraphicsEngine &GfxContext, bool force_draw) {
  }

  void PreviewBase::DrawContent (nux::GraphicsEngine &GfxContent, bool force_draw) {
    nux::Geometry base = GetGeometry ();
    GfxContent.PushClippingRectangle (base);

    if (GetCompositionLayout ())
      GetCompositionLayout ()->ProcessDraw (GfxContent, force_draw);

    GfxContent.PopClippingRectangle();
  }

  long int PreviewBase::ProcessEvent(nux::IEvent& ievent, long int TraverseInfo, long int ProcessEventInfo) {
    return PostProcessEvent2 (ievent, TraverseInfo, ProcessEventInfo);
  }

  void PreviewBase::PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw) {
    nux::View::PostDraw(GfxContext, force_draw);
  }
}
