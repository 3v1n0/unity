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

#include "LockScreenSettings.h"
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

  cg.DrawRoundedRectangle(cr,
                          1.0,
                          0, 0,
                          Settings::GRID_SIZE * 0.3,
                          width, height);

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

UserPromptView::UserPromptView(std::string const& name)
  : nux::View(NUX_TRACKER_LOCATION)
{
  SetLayout(new nux::VLayout());

  GetLayout()->SetLeftAndRightPadding(10);
  GetLayout()->SetTopAndBottomPadding(10);
  static_cast<nux::VLayout*>(GetLayout())->SetVerticalInternalMargin(5);

  unity::StaticCairoText* username = new unity::StaticCairoText(name);
  username->SetFont("Ubuntu 13");
  GetLayout()->AddView(username);

  GetLayout()->AddSpace(0, 1);

  message_ = new unity::StaticCairoText("Invalid password, please try again");
  message_->SetFont("Ubuntu 10");
  message_->SetTextColor(nux::Color("#df382c"));
  message_->SetVisible(false);
  GetLayout()->AddView(message_);

  text_input_ = new unity::TextInput();
  text_input_->input_hint = _("Password");
  text_input_->text_entry()->SetPasswordMode(true);
  text_input_->SetMinimumHeight(Settings::GRID_SIZE);
  text_input_->SetMaximumHeight(Settings::GRID_SIZE);
  /*const char password_char = '*';
  text_input_->text_entry()->SetPasswordChar(&password_char);*/
  GetLayout()->AddView(text_input_, 1);
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

void UserPromptView::ShowErrorMessage()
{
  message_->SetVisible(true);
  QueueRelayout();
}

}
}
