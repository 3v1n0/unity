/*
 * Copyright (C) 2011 Canonical Ltd
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
 * Authored by: Jay Taoko <jaytaoko@inalogic.com>
 *
 */

#ifndef UNITYSHELL_HSEPARATOR_H
#define UNITYSHELL_HSEPARATOR_H

#include <Nux/Nux.h>
#include <Nux/View.h>

namespace unity
{

class HSeparator: public nux::View
{
public:
  HSeparator(NUX_FILE_LINE_PROTO);
  HSeparator(nux::Color const& color, float alpha0, float alpha1, int border, NUX_FILE_LINE_PROTO);

  void SetColor(nux::Color const& color);
  void SetAlpha(float alpha0, float alpha1);
  void SetBorderSize(int border);

protected:
  bool AcceptKeyNavFocus() override { return false; }
  void Draw(nux::GraphicsEngine& GfxContext, bool force_draw) override;
  void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw) override {};

  nux::Color color_;
  float alpha0_;
  float alpha1_;
  int border_size_;
};

} // namespace unity

#endif // UNITYSHELL_HSEPARATOR_H
