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
 * Authored by: Andrea Azzarone <azzaronea@gmail.com>
 */
 
#include "ShortcutView.h"

#include <glib/gi18n-lib.h>
#include <boost/algorithm/string.hpp>

namespace unity
{

namespace shortcut
{

NUX_IMPLEMENT_OBJECT_TYPE(View);

View::View(NUX_FILE_LINE_DECL)
  : nux::View(NUX_FILE_LINE_PARAM)
{
  layout_ = new nux::VLayout();
  //layout_->SetVerticalInternalMargin(10);
  layout_->SetHorizontalInternalMargin(25);
  layout_->SetVerticalExternalMargin(30);
  SetLayout(layout_);

  background_top_ = nux::CreateTexture2DFromFile(PKGDATADIR"/switcher_top.png", -1, true);
  background_left_ = nux::CreateTexture2DFromFile(PKGDATADIR"/switcher_left.png", -1, true);
  background_corner_ = nux::CreateTexture2DFromFile(PKGDATADIR"/switcher_corner.png", -1, true);
  rounding_texture_ = nux::CreateTexture2DFromFile(PKGDATADIR"/switcher_round_rect.png", -1, true);

  text_view_ = new nux::StaticCairoText("Testing");
  text_view_->SinkReference();
  text_view_->SetLines(1);
  text_view_->SetTextColor(nux::color::White);
  text_view_->SetFont("Ubuntu Bold 20");
  text_view_->SetText(_("Keyboard Shortcuts"));
  layout_->AddView(text_view_, 1 , nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
  
  // FIXME: add a separator
  
  columns_layout_ = new nux::HLayout();
  columns_layout_->SetHorizontalExternalMargin(50);
  
  layout_->AddLayout(columns_layout_, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
  
  // Column 1...
  columns_.push_back(new nux::VLayout());
  columns_[0]->SetVerticalInternalMargin(10);
  columns_layout_->AddLayout(columns_[0], 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
  
  // Column 2...
  columns_.push_back(new nux::VLayout());
  columns_layout_->AddLayout(columns_[1], 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
  
  bg_effect_helper_.owner = this;
}

View::~View()
{
  if (background_top_ != NULL)
    background_top_->UnReference();
  
  if (background_left_ != NULL)  
    background_left_->UnReference();
  
  if (background_corner_ != NULL)
    background_corner_->UnReference();
  
  if (rounding_texture_ != NULL)
    rounding_texture_->UnReference();
}

void View::SetModel(Model::Ptr model)
{
  model_ = model;

  // Fills the columns...
  UpdateColumns();
  
}

Model::Ptr View::GetModel()
{
  return model_;
}

void View::SetupBackground()
{
  bg_effect_helper_.enabled = true;
}

void View::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  return;
}

void View::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw)
{  
  nux::Geometry base = GetGeometry();
  GfxContext.PushClippingRectangle(base);

  // clear region
  gPainter.PaintBackground(GfxContext, base);

  nux::Geometry background_geo;
  
  // FIXME
  background_geo.width = base.width;
  background_geo.height = base.height;
  background_geo.x = (base.width - background_geo.width)/2;
  background_geo.y = (base.height - background_geo.height)/2;

  // magic constant comes from texture contents (distance to cleared area)
  const int internal_offset = 20;
  nux::Geometry internal_clip(background_geo.x + internal_offset, 
                              background_geo.y + internal_offset, 
                              background_geo.width - internal_offset * 2, 
                              background_geo.height - internal_offset * 2);
  GfxContext.PushClippingRectangle(internal_clip);

  nux::Geometry geo_absolute = GetAbsoluteGeometry();
  if (BackgroundEffectHelper::blur_type != BLUR_NONE)
  {
    nux::Geometry blur_geo(geo_absolute.x, geo_absolute.y, base.width, base.height);
    auto blur_texture = bg_effect_helper_.GetBlurRegion(blur_geo);

    if (blur_texture.IsValid())
    {
      nux::TexCoordXForm texxform_blur_bg;
      texxform_blur_bg.flip_v_coord = true;
      texxform_blur_bg.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
      texxform_blur_bg.uoffset = ((float) base.x) / geo_absolute.width;
      texxform_blur_bg.voffset = ((float) base.y) / geo_absolute.height;

      nux::ROPConfig rop;
      rop.Blend = false;
      rop.SrcBlend = GL_ONE;
      rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

      gPainter.PushDrawTextureLayer(GfxContext, base,
                                    blur_texture,
                                    texxform_blur_bg,
                                    nux::color::White,
                                    true,
                                    rop);
    }
  }

  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
  gPainter.PushDrawColorLayer(GfxContext, internal_clip, background_color, false, rop);

  // Make round corners
  rop.Blend = true;
  rop.SrcBlend = GL_ZERO;
  rop.DstBlend = GL_SRC_ALPHA;
  gPainter.PaintShapeCornerROP(GfxContext,
                               internal_clip,
                               nux::color::White,
                               nux::eSHAPE_CORNER_ROUND4,
                               nux::eCornerTopLeft | nux::eCornerTopRight |
                               nux::eCornerBottomLeft | nux::eCornerBottomRight,
                               true,
                               rop);

  GfxContext.GetRenderStates().SetPremultipliedBlend(nux::SRC_OVER);

  GfxContext.PopClippingRectangle();
  GfxContext.PopClippingRectangle();

  DrawBackground(GfxContext, background_geo);


  layout_->ProcessDraw(GfxContext, force_draw);

  /*text_view_->SetBaseY(background_geo.y + 40);
  text_view_->SetBaseX(background_geo.x + 200);
  text_view_->Draw(GfxContext, force_draw);*/
}


void View::DrawBackground(nux::GraphicsEngine& GfxContext, nux::Geometry const& geo)
{
  int border = 30;

  GfxContext.GetRenderStates().SetBlend(TRUE, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  nux::TexCoordXForm texxform;
  texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
  texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);

  // Draw TOP-LEFT CORNER
  texxform.u0 = 0;
  texxform.v0 = 0;
  texxform.u1 = border;
  texxform.v1 = border;
  GfxContext.QRP_1Tex(geo.x, geo.y, 
                      border, border, background_corner_->GetDeviceTexture(), texxform, nux::color::White);
  
  // Draw TOP-RIGHT CORNER
  texxform.u0 = 0;
  texxform.v0 = 0;
  texxform.u1 = border;
  texxform.v1 = border;
  texxform.flip_u_coord = true;
  texxform.flip_v_coord = false;
  GfxContext.QRP_1Tex(geo.x + geo.width - border, geo.y, 
                      border, border, background_corner_->GetDeviceTexture(), texxform, nux::color::White);
  
  // Draw BOTTOM-LEFT CORNER
  texxform.u0 = 0;
  texxform.v0 = 0;
  texxform.u1 = border;
  texxform.v1 = border;
  texxform.flip_u_coord = false;
  texxform.flip_v_coord = true;
  GfxContext.QRP_1Tex(geo.x, geo.y + geo.height - border, 
                      border, border, background_corner_->GetDeviceTexture(), texxform, nux::color::White);
  
  // Draw BOTTOM-RIGHT CORNER
  texxform.u0 = 0;
  texxform.v0 = 0;
  texxform.u1 = border;
  texxform.v1 = border;
  texxform.flip_u_coord = true;
  texxform.flip_v_coord = true;
  GfxContext.QRP_1Tex(geo.x + geo.width - border, geo.y + geo.height - border, 
                      border, border, background_corner_->GetDeviceTexture(), texxform, nux::color::White);
  
  int top_width = background_top_->GetWidth();
  int top_height = background_top_->GetHeight();

  // Draw TOP BORDER
  texxform.u0 = 0;
  texxform.v0 = 0;
  texxform.u1 = top_width;
  texxform.v1 = top_height;
  texxform.flip_u_coord = false;
  texxform.flip_v_coord = false;
  GfxContext.QRP_1Tex(geo.x + border, geo.y, geo.width - border - border, border, background_top_->GetDeviceTexture(), texxform, nux::color::White);
  
  // Draw BOTTOM BORDER
  texxform.u0 = 0;
  texxform.v0 = 0;
  texxform.u1 = top_width;
  texxform.v1 = top_height;
  texxform.flip_u_coord = false;
  texxform.flip_v_coord = true;
  GfxContext.QRP_1Tex(geo.x + border, geo.y + geo.height - border, geo.width - border - border, border, background_top_->GetDeviceTexture(), texxform, nux::color::White);
  

  int left_width = background_left_->GetWidth();
  int left_height = background_left_->GetHeight();

  // Draw LEFT BORDER
  texxform.u0 = 0;
  texxform.v0 = 0;
  texxform.u1 = left_width;
  texxform.v1 = left_height;
  texxform.flip_u_coord = false;
  texxform.flip_v_coord = false;
  GfxContext.QRP_1Tex(geo.x, geo.y + border, border, geo.height - border - border, background_left_->GetDeviceTexture(), texxform, nux::color::White);
  
  // Draw RIGHT BORDER
  texxform.u0 = 0;
  texxform.v0 = 0;
  texxform.u1 = left_width;
  texxform.v1 = left_height;
  texxform.flip_u_coord = true;
  texxform.flip_v_coord = false;
  GfxContext.QRP_1Tex(geo.x + geo.width - border, geo.y + border, border, geo.height - border - border, background_left_->GetDeviceTexture(), texxform, nux::color::White);

  GfxContext.GetRenderStates().SetBlend(FALSE);
}

void View::UpdateColumns()
{
  for (auto category : model_->categories())
  {
    int column = (category == "Launcher" or category == "Dash" or category == "Top Bar") ? 0 : 1;
    
    nux::StaticCairoText* header = new nux::StaticCairoText("Testing");
    header->SinkReference();
    header->SetLines(1);
    header->SetTextColor(nux::color::White);
    header->SetFont("Ubuntu Bold 13");
    header->SetText(category);
    columns_[column]->AddView(header, 1 , nux::MINOR_POSITION_LEFT, nux::MINOR_SIZE_FULL);
    columns_[column]->AddSpace(1, 1);
  
    for (auto hint : model_->hints()[category])
    {
      nux::HLayout *hlayout = new nux::HLayout();
      hlayout->SinkReference();
      columns_[column]->AddLayout(hlayout);
      
       // "Files & folders", you are the culprit!
       std::string value = hint->Prefix() + hint->Value() + hint->Postfix();
       boost::replace_all(value, "&", "&amp;");
       boost::replace_all(value, "<", "&lt;");
       boost::replace_all(value, ">", "&gt;");
      
      nux::StaticText* shortcut_text = new nux::StaticText("Testing");
      shortcut_text->SinkReference();
      //shortcut_text->SetLines(1);
      shortcut_text->SetTextColor(nux::color::White);
      //shortcut_text->SetAlignment(nux::NUX_ALIGN_LEFT);
      shortcut_text->SetFontName("Ubuntu Bold");
      shortcut_text->SetText("<b>" + value + "</b>");
      shortcut_text->SetMinimumWidth(200);
      shortcut_text->SetMaximumWidth(200);
      hlayout->AddView(shortcut_text, 0);
      
      nux::StaticCairoText* description_text = new nux::StaticCairoText("Testing");
      description_text->SinkReference();
      description_text->SetLines(1);
      description_text->SetTextColor(nux::color::White);
      description_text->SetFont("Ubuntu 11");
      description_text->SetText(hint->Description());
      hlayout->AddView(description_text);
    }
  }
    
}

  
} // namespace shortcut
} // namespace unity
