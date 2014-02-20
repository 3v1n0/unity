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
 * Authored by: Marco Trevisan <3v1n0@ubuntu.com>
 */

#ifndef CAIRO_BASEWINDOW_H
#define CAIRO_BASEWINDOW_H

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>
#include <NuxCore/Animation.h>
#include "unity-shared/EMConverter.h"

namespace unity
{
class CairoBaseWindow : public nux::BaseWindow
{
  NUX_DECLARE_OBJECT_TYPE(CairoBaseWindow, nux::BaseWindow);
public:
  CairoBaseWindow(int monitor = 0);
  virtual ~CairoBaseWindow() = default;

  virtual void Show();
  virtual void Hide();

  void RedrawBlur();
  bool HasBlurredBackground() const;

protected:
  void Draw(nux::GraphicsEngine& gfxContext, bool forceDraw);

  nux::ObjectPtr<nux::BaseTexture> texture_bg_;
  nux::ObjectPtr<nux::BaseTexture> texture_mask_;
  nux::ObjectPtr<nux::BaseTexture> texture_outline_;
  EMConverter::Ptr cv_;

private:
  bool use_blurred_background_;
  bool compute_blur_bkg_;
  nux::ObjectPtr<nux::IOpenGLBaseTexture> bg_blur_texture_;
  nux::animation::AnimateValue<double> fade_animator_;
};
}

#endif // CAIRO_BASEWINDOW_H

