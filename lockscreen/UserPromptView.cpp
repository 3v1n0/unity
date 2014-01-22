// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2014 Canonical Ltd
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
* Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
*/

#include "UserPromptView.h"

#include <Nux/VLayout.h>

#include "unity-shared/CairoTexture.h"
#include "unity-shared/TextInput.h"
#include "unity-shared/StaticCairoText.h"

namespace unity
{
namespace lockscreen
{

nux::AbstractPaintLayer* Foo(int width, int height)
{
  nux::CairoGraphics cg(CAIRO_FORMAT_ARGB32, width, height);
  cairo_t* cr = cg.GetInternalContext();

  // FIXME (andy) add radious
  cairo_set_source_rgba(cr, 0.1, 0.1, 0.1, 0.4);
  cairo_rectangle(cr, 0, 0, width, height);
  cairo_fill_preserve(cr);

  cairo_set_source_rgba (cr, 0.4, 0.4, 0.4, 0.4);
  cairo_set_line_width (cr, 1);
  cairo_stroke (cr);

  // Create the texture layer
  nux::TexCoordXForm texxform;

  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

  return (new nux::TextureLayer(texture_ptr_from_cairo_graphics(cg)->GetDeviceTexture(),
                                texxform,
                                nux::color::White,
                                true,
                                rop));
}

UserPromptView::UserPromptView()
  : nux::View(NUX_TRACKER_LOCATION)
{
  SetLayout(new nux::VLayout());

  GetLayout()->SetLeftAndRightPadding(10);
  GetLayout()->SetTopAndBottomPadding(10);

  unity::StaticCairoText* username = new unity::StaticCairoText("<b>Andrea Azzarone</b>");
  GetLayout()->AddView(username);

  text_input_ = new unity::TextInput();
  text_input_->input_hint = _("Password");
  text_input_->text_entry()->SetPasswordMode(true);
  /*const char password_char = '*';
  text_input_->text_entry()->SetPasswordChar(&password_char);*/
  GetLayout()->AddView(text_input_);
}

void UserPromptView::Draw(nux::GraphicsEngine& graphics_engine, bool /* force_draw */)
{
  nux::Geometry const& geo = GetGeometry();
  
  graphics_engine.PushClippingRectangle(geo);
  nux::GetPainter().PaintBackground(graphics_engine, geo);

  bg_layer_.reset(Foo(geo.width, geo.height));
  nux::GetPainter().PushDrawLayer(graphics_engine, geo, bg_layer_.get());

  nux::GetPainter().PopBackground();
  graphics_engine.PopClippingRectangle();
}

void UserPromptView::DrawContent(nux::GraphicsEngine& graphics_engine, bool force_draw)
{
  nux::Geometry const& geo = GetGeometry();
  graphics_engine.PushClippingRectangle(geo);

  if (!IsFullRedraw())
  {
    bg_layer_.reset(Foo(geo.width, geo.height));
    nux::GetPainter().PushLayer(graphics_engine, geo, bg_layer_.get());
  }

  if (GetLayout())
    GetLayout()->ProcessDraw(graphics_engine, force_draw);

  if (!IsFullRedraw())
    nux::GetPainter().PopBackground();

  graphics_engine.PopClippingRectangle();
}

nux::TextEntry* UserPromptView::text_entry()
{
  return text_input_->text_entry();
}

}
}
