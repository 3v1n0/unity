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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 *
 */

#include "GenericPreview.h"
#include "unity-shared/IntrospectableWrappers.h"
#include <NuxCore/Logger.h>
#include <Nux/Layout.h>
#include <PreviewFactory.h>

namespace unity
{
namespace dash
{
namespace previews
{

namespace
{
nux::logging::Logger logger("unity.dash.genericpreview");

bool registered(PreviewFactory::Instance().RegisterItem("preview-generic", new PreviewFactoryItem<dash::GenericPreview, previews::GenericPreview>()));

}

NUX_IMPLEMENT_OBJECT_TYPE(GenericPreview);

GenericPreview::GenericPreview(dash::Preview::Ptr preview_model)
: Preview(preview_model)
{
}

GenericPreview::~GenericPreview()
{
}

void GenericPreview::Draw(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
  Preview::Draw(gfx_engine, force_draw);  
}

void GenericPreview::DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
  Preview::DrawContent(gfx_engine, force_draw);

  nux::Geometry base = GetGeometry();
  gfx_engine.PushClippingRectangle(base);


  gfx_engine.PopClippingRectangle();
}

std::string GenericPreview::GetName() const
{
  return "GenericPreview";
}

void GenericPreview::AddProperties(GVariantBuilder* builder)
{
  Preview::AddProperties(builder);
}

}
}
}
