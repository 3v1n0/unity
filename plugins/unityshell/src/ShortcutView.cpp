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
 *              Jay Taoko <jay.taoko@canonical.com>
 */
 
#include "ShortcutView.h"

#include <glib/gi18n-lib.h>
#include <boost/algorithm/string.hpp>

#include "LineSeparator.h"

namespace unity
{
namespace shortcut
{
namespace
{
  int SECTION_NAME_FONT_SIZE = 12;
  int SHORTKEY_ENTRY_FONT_SIZE = 10;
  int INTER_SPACE_SHORTKEY_DESCRIPTION = 10;
  int SHORTKEY_COLUMN_WIDTH = 150;
  int DESCRIPTION_COLUMN_WIDTH = 265;
  int LINE_SPACING = 4;
} // namespace anonymouse

NUX_IMPLEMENT_OBJECT_TYPE(View);

View::View(NUX_FILE_LINE_DECL)
  : nux::View(NUX_FILE_LINE_PARAM)
{
  layout_ = new nux::VLayout();
  layout_->SetPadding(50, 50);
  layout_->SetSpaceBetweenChildren(25);
  SetLayout(layout_);

  background_top_ = nux::CreateTexture2DFromFile(PKGDATADIR"/switcher_top.png", -1, true);
  background_left_ = nux::CreateTexture2DFromFile(PKGDATADIR"/switcher_left.png", -1, true);
  background_corner_ = nux::CreateTexture2DFromFile(PKGDATADIR"/switcher_corner.png", -1, true);
  rounding_texture_ = nux::CreateTexture2DFromFile(PKGDATADIR"/switcher_round_rect.png", -1, true);

  std::string header = "<b>";
  header += _("Keyboard Shortcuts");
  header += "</b>";
  
  nux::StaticText* header_view = new nux::StaticText(header.c_str(), NUX_TRACKER_LOCATION);
  header_view->SetTextPointSize(15);
  header_view->SetFontName("Ubuntu");
  layout_->AddView(header_view, 1 , nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
  
  layout_->AddView(new HSeparator(), 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);  
    
  columns_layout_ = new nux::HLayout();  
  columns_layout_->SetSpaceBetweenChildren(40);
  layout_->AddLayout(columns_layout_, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
  
  // Column 1...
  columns_.push_back(new nux::VLayout());
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
  RenderColumns();
}

Model::Ptr View::GetModel()
{
  return model_;
}

void View::SetupBackground()
{
  bg_effect_helper_.enabled = true;
}

nux::LinearLayout* View::CreateSectionLayout(const char* section_name)
{
  nux::VLayout* layout = new nux::VLayout(NUX_TRACKER_LOCATION);
  
  std::string name = "<b>";
  name += std::string(section_name);
  name += "</b>";

  nux::StaticText* section_name_view = new nux::StaticText(name.c_str(), NUX_TRACKER_LOCATION);
  section_name_view->SetTextPointSize(SECTION_NAME_FONT_SIZE);
  section_name_view->SetFontName("Ubuntu");
  layout->AddView(section_name_view, 0, nux::MINOR_POSITION_START, nux::MINOR_SIZE_MATCHCONTENT);
  layout->AddView(new nux::SpaceLayout(20, 20, 20, 20), 0, nux::MINOR_POSITION_START, nux::MINOR_SIZE_MATCHCONTENT);

  return layout;
}

nux::LinearLayout* View::CreateShortKeyEntryLayout(const char* shortkey, const char* description)
{
  nux::HLayout* layout = new nux::HLayout("EntryLayout", NUX_TRACKER_LOCATION);
  nux::HLayout* shortkey_layout = new nux::HLayout(NUX_TRACKER_LOCATION);
  nux::HLayout* description_layout = new nux::HLayout(NUX_TRACKER_LOCATION);

  std::string value = "<b>";
  value += std::string(shortkey);
  value += "</b>";

  nux::StaticText* shortkey_view = new nux::StaticText(value.c_str(), NUX_TRACKER_LOCATION);
  shortkey_view->SetTextAlignment(nux::StaticText::ALIGN_LEFT);
  shortkey_view->SetFontName("Ubuntu");
  shortkey_view->SetTextPointSize(SHORTKEY_ENTRY_FONT_SIZE);
  shortkey_view->SetMinimumWidth(SHORTKEY_COLUMN_WIDTH);
  shortkey_view->SetMaximumWidth(SHORTKEY_COLUMN_WIDTH);

  nux::StaticText* description_view = new nux::StaticText(description, NUX_TRACKER_LOCATION);
  description_view->SetTextAlignment(nux::StaticText::ALIGN_LEFT);
  shortkey_view->SetFontName("Ubuntu");
  description_view->SetTextPointSize(SHORTKEY_ENTRY_FONT_SIZE);
  description_view->SetMinimumWidth(DESCRIPTION_COLUMN_WIDTH);
  description_view->SetMaximumWidth(DESCRIPTION_COLUMN_WIDTH);

  shortkey_layout->AddView(shortkey_view, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);
  shortkey_layout->SetContentDistribution(nux::MAJOR_POSITION_START);
  shortkey_layout->SetMinimumWidth(SHORTKEY_COLUMN_WIDTH);
  shortkey_layout->SetMaximumWidth(SHORTKEY_COLUMN_WIDTH);

  description_layout->AddView(description_view, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);
  description_layout->SetContentDistribution(nux::MAJOR_POSITION_START);
  description_layout->SetMinimumWidth(DESCRIPTION_COLUMN_WIDTH);
  description_layout->SetMaximumWidth(DESCRIPTION_COLUMN_WIDTH);

  layout->AddLayout(shortkey_layout, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);
  layout->AddLayout(description_layout, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);
  layout->SetSpaceBetweenChildren(INTER_SPACE_SHORTKEY_DESCRIPTION);
  description_layout->SetContentDistribution(nux::MAJOR_POSITION_START);

  return layout;
}

nux::LinearLayout* View::CreateIntermediateLayout()
{
  nux::VLayout* layout = new nux::VLayout(NUX_TRACKER_LOCATION);
  layout->SetSpaceBetweenChildren(LINE_SPACING);

  return layout;
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

void View::RenderColumns()
{
  int i = 0;
  int column = 0;
  
  for (auto category : model_->categories())
  {
    // Three sections in the fist column...
    if (i > 2)
      column = 1;
      
    nux::LinearLayout* section_layout = CreateSectionLayout(category.c_str());
    nux::LinearLayout* intermediate_layout = CreateIntermediateLayout();
    intermediate_layout->SetContentDistribution(nux::MAJOR_POSITION_START);
    
    for (auto hint : model_->hints()[category])
    {
      std::string str_value = hint->Prefix() + hint->Value() + hint->Postfix();
      boost::replace_all(str_value, "&", "&amp;");
      boost::replace_all(str_value, "<", "&lt;");
      boost::replace_all(str_value, ">", "&gt;");
      
      intermediate_layout->AddLayout(CreateShortKeyEntryLayout(str_value.c_str(), hint->Description().c_str()), 0, nux::MINOR_POSITION_START, nux::MINOR_SIZE_FULL);
    }
    
    section_layout->AddLayout(intermediate_layout, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);

    if (i == 0 or i==1 or i==3 or i==4)
    {
      // Add space before the line
      section_layout->AddView(new nux::SpaceLayout(20, 20, 20, 20), 0, nux::MINOR_POSITION_START, nux::MINOR_SIZE_MATCHCONTENT);
      section_layout->AddView(new HSeparator(), 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
      // Add space after the line
      section_layout->AddView(new nux::SpaceLayout(30, 30, 30, 30), 0, nux::MINOR_POSITION_START, nux::MINOR_SIZE_MATCHCONTENT);
    }

    columns_[column]->AddView(section_layout, 1, nux::MINOR_POSITION_START, nux::MINOR_SIZE_FULL);
      
    i++;
  }
}
  
} // namespace shortcut
} // namespace unity
