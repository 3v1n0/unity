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
 * Authored by: Andrea Cimitan <andrea.cimitan@canonical.com>
 *
 */

#include "ApplicationScreenshot.h"
#include "unity-shared/IntrospectableWrappers.h"
#include <NuxCore/Logger.h>

namespace unity
{
namespace dash
{
namespace previews
{

namespace
{
nux::logging::Logger logger("unity.dash.previews.applicationscreenshot");
}

NUX_IMPLEMENT_OBJECT_TYPE(ApplicationScreenshot);

ApplicationScreenshot::ApplicationScreenshot(std::string const& image_hint)
  : View(NUX_TRACKER_LOCATION)
{
  texture_screenshot_.Adopt(nux::CreateTexture2DFromFile(image_hint.c_str(), -1, true));
  SetupViews();
}

ApplicationScreenshot::~ApplicationScreenshot()
{
}

void ApplicationScreenshot::Draw(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
  nux::Geometry const& base = GetGeometry();

  gfx_engine.PushClippingRectangle(base);
  nux::GetPainter().PaintBackground(gfx_engine, base);

  gfx_engine.PopClippingRectangle();
}

void ApplicationScreenshot::DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
  nux::Geometry const& base = GetGeometry();
  gfx_engine.PushClippingRectangle(base);

  if (texture_screenshot_)
  {
    nux::Geometry imageDest = base;
    nux::TexCoordXForm texxform;

    float base_apsect = float(base.GetWidth()) / base.GetHeight();
    float image_aspect = float(texture_screenshot_->GetWidth()) / texture_screenshot_->GetHeight();

    if (image_aspect > base_apsect)
    {
      imageDest.SetHeight(float(imageDest.GetWidth())/image_aspect);
    } 
    if (image_aspect < base_apsect)
    {
      imageDest.SetWidth(image_aspect*imageDest.GetHeight());
    }

    int border_width = 1;

    //DrawBorder(gfx_engine, imageDest, 0, nux::Color(0.15f, 0.15f, 0.15f));
    gfx_engine.QRP_Color(imageDest.x + (float(base.GetWidth() - imageDest.GetWidth()) / 2),
                      imageDest.y + (float(base.GetHeight() - imageDest.GetHeight()) / 2),
                      imageDest.GetWidth(),
                      imageDest.GetHeight(),
                      nux::Color(0.15f, 0.15f, 0.15f));

    texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_SCALE_COORD);
    texxform.SetWrap(nux::TEXWRAP_CLAMP_TO_BORDER, nux::TEXWRAP_CLAMP_TO_BORDER);
    texxform.SetFilter(nux::TEXFILTER_LINEAR, nux::TEXFILTER_LINEAR);

    texxform.u0 = 0;
    texxform.v0 = 0;
    texxform.u1 = imageDest.width;
    texxform.v1 = imageDest.height;

    gfx_engine.QRP_1Tex(imageDest.x + (float(base.GetWidth() - imageDest.GetWidth()) / 2) + border_width,
                        imageDest.y + (float(base.GetHeight() - imageDest.GetHeight()) / 2) + border_width,
                        imageDest.width - (border_width * 2),
                        imageDest.height - (border_width * 2),
                        texture_screenshot_.GetPointer()->GetDeviceTexture(),
                        texxform,
                        nux::color::White);
  }

  gfx_engine.PopClippingRectangle();
}

std::string ApplicationScreenshot::GetName() const
{
  return "ApplicationScreenshot";
}

void ApplicationScreenshot::SetupViews()
{
}

}
}
}