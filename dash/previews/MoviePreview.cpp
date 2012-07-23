// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright 2012 Canonical Ltd.
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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 *
 */

#include "MoviePreview.h"
#include "unity-shared/IntrospectableWrappers.h"
#include <NuxCore/Logger.h>
#include <Nux/HLayout.h>
#include <Nux/VLayout.h>
#include <UnityCore/MoviePreview.h>
#include "unity-shared/StaticCairoText.h"

namespace unity
{
namespace dash
{
namespace previews
{

namespace
{
nux::logging::Logger logger("unity.dash.previews.moviepreview");

}

NUX_IMPLEMENT_OBJECT_TYPE(MoviePreview);

MoviePreview::MoviePreview(dash::Preview::Ptr preview_model)
: Preview(preview_model)
{
  SetupView();
}

MoviePreview::~MoviePreview()
{
}

std::string MoviePreview::GetName() const
{
  return "MoviePreview";
}

void MoviePreview::AddProperties(GVariantBuilder* builder)
{
  Preview::AddProperties(builder);
}

void MoviePreview::Draw(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
  Preview::Draw(gfx_engine, force_draw);  
}

void MoviePreview::DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
  if (GetCompositionLayout())
    GetCompositionLayout()->ProcessDraw(gfx_engine, force_draw);
}

void MoviePreview::OnNavigateInComplete()
{
}

void MoviePreview::OnNavigateOut()
{
}

void MoviePreview::SetupBackground()
{
}

void MoviePreview::SetupView()
{
  dash::MoviePreview* music_preview_model = dynamic_cast<dash::MoviePreview*>(preview_model_.get());
  if (!music_preview_model)
    return;

  nux::HLayout* layout = new nux::HLayout();
  SetLayout(layout);
}

}
}
}
