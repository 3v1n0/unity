/*
 * Copyright 2016 Canonical Ltd.
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
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 *
 */

#ifndef UNITY_LOCKSCREEN_BUTTON_H
#define UNITY_LOCKSCREEN_BUTTON_H

#include <Nux/Nux.h>
#include <Nux/Button.h>
#include <Nux/CairoWrapper.h>

namespace unity
{

class IconTexture;

namespace lockscreen
{

class LockScreenButton : public nux::Button
{
  NUX_DECLARE_OBJECT_TYPE(LockScreenButton, nux::Button);

public:
  LockScreenButton(std::string const&, NUX_FILE_LINE_PROTO);

  nux::Property<double> scale;

protected:
  long ComputeContentSize() override;
  void Draw(nux::GraphicsEngine&, bool) override;
  void DrawContent(nux::GraphicsEngine&, bool) override;
  bool InspectKeyEvent(unsigned int eventType, unsigned int key_sym, const char* character) override;

private:
  void InitTheme();
  void RedrawTheme(nux::Geometry const&, cairo_t*);

  typedef std::unique_ptr<nux::CairoWrapper> NuxCairoPtr;

  std::string label_;
  nux::Geometry cached_geometry_;

  NuxCairoPtr normal_;

  nux::HLayout* hlayout_;
  IconTexture* activator_;
};

} // namespace lockscreen
} // namespace unity

#endif
