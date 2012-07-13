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

#ifndef APPLICATIONSCREENSHOT_H
#define APPLICATIONSCREENSHOT_H

#include <Nux/Nux.h>
#include <Nux/View.h>
#include <UnityCore/ApplicationPreview.h>
#include "unity-shared/StaticCairoText.h"
#include <NuxCore/ObjectPtr.h>

namespace unity
{
namespace dash
{
namespace previews
{

class ApplicationScreenshot : public nux::View
{
public:
  typedef nux::ObjectPtr<ApplicationScreenshot> Ptr;
  NUX_DECLARE_OBJECT_TYPE(ApplicationScreenshot, nux::View);

  ApplicationScreenshot();
  virtual ~ApplicationScreenshot();

  void SetImage(std::string const& image_hint);

  // From debug::Introspectable
  std::string GetName() const;

  void SetFont(std::string const& font);

protected:
  virtual void Draw(nux::GraphicsEngine& gfx_engine, bool force_draw);
  virtual void DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw);

  void SetupViews();

private:
  nux::ObjectPtr<nux::BaseTexture> texture_screenshot_;
  nux::StaticCairoText* overlay_text_;
};

}
}
}

#endif // APPLICATIONSCREENSHOT_H