/*
 * Copyright (C) 2010 Canonical Ltd
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

#include "SwitcherView.h"
#include "IconRenderer.h"

#include <NuxCore/Object.h>
#include <Nux/Nux.h>
#include <Nux/WindowCompositor.h>

using namespace unity::ui;

namespace unity {
namespace switcher {

NUX_IMPLEMENT_OBJECT_TYPE (SwitcherView);

SwitcherView::SwitcherView(NUX_FILE_LINE_DECL) 
: View (NUX_FILE_LINE_PARAM)
{
  icon_renderer_ = AbstractIconRenderer::Ptr (new IconRenderer ());
}

SwitcherView::~SwitcherView()
{
  
}

void SwitcherView::SetModel (SwitcherModel::Ptr model)
{
  model_ = model;
}

SwitcherModel::Ptr SwitcherView::GetModel ()
{
  return model_;
}

long SwitcherView::ProcessEvent (nux::IEvent &ievent, long TraverseInfo, long ProcessEventInfo)
{
  return TraverseInfo;
}

void SwitcherView::Draw (nux::GraphicsEngine& GfxContext, bool force_draw)
{
  return;
}

void SwitcherView::DrawContent (nux::GraphicsEngine &GfxContext, bool force_draw)
{
  GfxContext.PushClippingRectangle (GetGeometry() );

  // draw content
  gPainter.Paint2DQuadColor (GfxContext, nux::Geometry (0, 0, 100, 100), nux::Color(0xAAAA0000));

  GfxContext.PopClippingRectangle();
}



}
}
