// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 */


#include "OverlayRenderer.h"

#include <NuxGraphics/GraphicsEngine.h>
#include <Nux/PaintLayer.h>
#include <NuxCore/Logger.h>

#include "unity-shared/BackgroundEffectHelper.h"
#include "DashStyle.h"
#include "unity-shared/UnitySettings.h"

#include "unity-shared/UBusMessages.h"
#include "unity-shared/UBusWrapper.h"

namespace unity
{
DECLARE_LOGGER(logger, "unity.overlayrenderer");
namespace
{
const int INNER_CORNER_RADIUS = 5;
const int EXCESS_BORDER = 10;
}

// Impl class
class OverlayRendererImpl
{
public:
  OverlayRendererImpl(OverlayRenderer *parent_);
  ~OverlayRendererImpl();

  void Init();
  void UpdateTextures();

  void OnBackgroundColorChanged(GVariant* args);

  void Draw(nux::GraphicsEngine& gfx_context, nux::Geometry const& content_geo, nux::Geometry const& absolute_geo, nux::Geometry const& geometry, bool force_draw);
  void DrawContent(nux::GraphicsEngine& gfx_context, nux::Geometry const& content_geo, nux::Geometry const& absolute_geo, nux::Geometry const& geometry);
  void DrawContentCleanup(nux::GraphicsEngine& gfx_context, nux::Geometry const& content_geo, nux::Geometry const& absolute_geo, nux::Geometry const& geometry);

  BackgroundEffectHelper bg_effect_helper_;
  nux::ColorLayer* bg_layer_;
  nux::ColorLayer* bg_darken_layer_;
  nux::Color bg_color_;

  nux::Geometry content_geo;
  nux::ObjectPtr <nux::IOpenGLBaseTexture> bg_blur_texture_;
  nux::ObjectPtr <nux::IOpenGLBaseTexture> bg_shine_texture_;

  std::unique_ptr<nux::TextureLayer> bg_refine_gradient_;

  // temporary variable that stores the number of backgrounds we have rendered
  int bgs;
  bool visible;


  UBusManager ubus_manager_;

  OverlayRenderer *parent;

  void InitASMInverseTextureMaskShader();
  void InitSlInverseTextureMaskShader();

#ifndef NUX_OPENGLES_20
  nux::ObjectPtr<nux::IOpenGLAsmShaderProgram> inverse_texture_mask_asm_prog_;
  nux::ObjectPtr<nux::IOpenGLAsmShaderProgram> inverse_texture_rect_mask_asm_prog_;
#endif
  nux::ObjectPtr<nux::IOpenGLShaderProgram> inverse_texture_mask_prog_;

  void RenderInverseMask_GLSL(nux::GraphicsEngine& gfx_context, int x, int y, int width, int height, nux::ObjectPtr<nux::IOpenGLBaseTexture> DeviceTexture, nux::TexCoordXForm &texxform0, const nux::Color &color0);
  void RenderInverseMask_ASM(nux::GraphicsEngine& gfx_context, int x, int y, int width, int height, nux::ObjectPtr<nux::IOpenGLBaseTexture> DeviceTexture, nux::TexCoordXForm &texxform0, const nux::Color &color0);
  void RenderInverseMask(nux::GraphicsEngine& gfx_context, int x, int y, int width, int height, nux::ObjectPtr<nux::IOpenGLBaseTexture> DeviceTexture, nux::TexCoordXForm &texxform0, const nux::Color &color0);

};

OverlayRendererImpl::OverlayRendererImpl(OverlayRenderer *parent_)
  : visible(false)
  , parent(parent_)
{
  bg_effect_helper_.enabled = false;
  Init();
}

OverlayRendererImpl::~OverlayRendererImpl()
{
  delete bg_layer_;
  delete bg_darken_layer_;
}

void OverlayRendererImpl::Init()
{  
  UpdateTextures();

  ubus_manager_.RegisterInterest(UBUS_BACKGROUND_COLOR_CHANGED,
                                 sigc::mem_fun(this, &OverlayRendererImpl::OnBackgroundColorChanged));

  ubus_manager_.SendMessage(UBUS_BACKGROUND_REQUEST_COLOUR_EMIT);
}

void OverlayRendererImpl::OnBackgroundColorChanged(GVariant* args)
{
  gdouble red, green, blue, alpha;
  g_variant_get (args, "(dddd)", &red, &green, &blue, &alpha);

  nux::Color color = nux::Color(red, green, blue, alpha);
  bg_layer_->SetColor(color);
  bg_color_ = color;

  //When we are in low gfx mode then our darken layer will act as a background.
  if (Settings::Instance().GetLowGfxMode())
  {
    bg_darken_layer_->SetColor(bg_color_);
  }

  parent->need_redraw.emit();
}

void OverlayRendererImpl::UpdateTextures()
{  
  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
  bg_layer_ = new nux::ColorLayer(nux::Color(0.0f, 0.0f, 0.0f, 0.9), true, rop);

  rop.Blend = true;
  rop.SrcBlend = GL_ZERO;
  rop.DstBlend = GL_SRC_COLOR;
  nux::Color darken_colour = nux::Color(0.9f, 0.9f, 0.9f, 1.0f);
  
  //When we are in low gfx mode then our darken layer will act as a background.
  if (Settings::Instance().GetLowGfxMode())
  {
    rop.Blend = false;
    rop.SrcBlend = GL_ONE;
    rop.DstBlend = GL_SRC_COLOR;
    darken_colour = bg_color_;
  }
  
  bg_darken_layer_ = new nux::ColorLayer(darken_colour, false, rop);
  bg_shine_texture_ = unity::dash::Style::Instance().GetDashShine()->GetDeviceTexture();

  nux::BaseTexture* bg_refine_tex = unity::dash::Style::Instance().GetRefineTextureDash();
  if (bg_refine_tex)
  {
    rop.Blend = true;
    rop.SrcBlend = GL_ONE;
    rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
    nux::TexCoordXForm texxform;
    
    bg_refine_gradient_.reset(new nux::TextureLayer(bg_refine_tex->GetDeviceTexture(), 
                              texxform, 
                              nux::color::White,
                              false,
                              rop));
  }
}

void OverlayRendererImpl::InitASMInverseTextureMaskShader()
{
  std::string AsmVtx = 
      "!!ARBvp1.0                                 \n\
      ATTRIB iPos         = vertex.position;      \n\
      ATTRIB iColor       = vertex.attrib[3];     \n\
      PARAM  mvp[4]       = {state.matrix.mvp};   \n\
      OUTPUT oPos         = result.position;      \n\
      OUTPUT oColor       = result.color;         \n\
      OUTPUT oTexCoord0   = result.texcoord[0];   \n\
      # Transform the vertex to clip coordinates. \n\
      DP4   oPos.x, mvp[0], iPos;                     \n\
      DP4   oPos.y, mvp[1], iPos;                     \n\
      DP4   oPos.z, mvp[2], iPos;                     \n\
      DP4   oPos.w, mvp[3], iPos;                     \n\
      MOV   oColor, iColor;                           \n\
      MOV   oTexCoord0, vertex.attrib[8];             \n\
      END";

  std::string AsmFrg = 
      "!!ARBfp1.0                                       \n\
      TEMP tex0;                                        \n\
      TEMP temp0;                                       \n\
      TEX tex0, fragment.texcoord[0], texture[0], 2D;   \n\
      MUL temp0, fragment.color, tex0;                  \n\
      SUB result.color, {1.0, 1.0, 1.0, 1.0}, temp0.aaaa;\n\
      END";

  std::string AsmFrgRect = 
    "!!ARBfp1.0                                         \n\
    TEMP tex0;                                          \n\
    TEMP temp0;                                         \n\
    TEX tex0, fragment.texcoord[0], texture[0], RECT;   \n\
    MUL temp0, fragment.color, tex0;                    \n\
    SUB result.color, {1.0, 1.0, 1.0, 1.0}, temp0.aaaa;  \n\
    END";

#ifndef NUX_OPENGLES_20
  inverse_texture_mask_asm_prog_ = nux::GetGraphicsDisplay()->GetGpuDevice()->CreateAsmShaderProgram();
  inverse_texture_mask_asm_prog_->LoadVertexShader(AsmVtx.c_str());
  inverse_texture_mask_asm_prog_->LoadPixelShader(AsmFrg.c_str());
  inverse_texture_mask_asm_prog_->Link();

  inverse_texture_rect_mask_asm_prog_ = nux::GetGraphicsDisplay()->GetGpuDevice()->CreateAsmShaderProgram();
  inverse_texture_rect_mask_asm_prog_->LoadVertexShader(AsmVtx.c_str());
  inverse_texture_rect_mask_asm_prog_->LoadPixelShader(AsmFrgRect.c_str());
  inverse_texture_rect_mask_asm_prog_->Link();
#endif
}

void OverlayRendererImpl::RenderInverseMask_ASM(nux::GraphicsEngine& gfx_context, int x, int y, int width, int height, nux::ObjectPtr<nux::IOpenGLBaseTexture> device_texture, nux::TexCoordXForm &texxform, const nux::Color &color)
{
#ifndef NUX_OPENGLES_20
  if (!inverse_texture_mask_asm_prog_.IsValid() || !inverse_texture_rect_mask_asm_prog_.IsValid())
  {
    InitASMInverseTextureMaskShader();
  }

  QRP_Compute_Texture_Coord(width, height, device_texture, texxform);
  float fx = x, fy = y;
  float VtxBuffer[] =
  {
    fx,          fy,          0.0f, 1.0f, texxform.u0, texxform.v0, 0, 1.0f, color.red, color.green, color.blue, color.alpha,
    fx,          fy + height, 0.0f, 1.0f, texxform.u0, texxform.v1, 0, 1.0f, color.red, color.green, color.blue, color.alpha,
    fx + width,  fy + height, 0.0f, 1.0f, texxform.u1, texxform.v1, 0, 1.0f, color.red, color.green, color.blue, color.alpha,
    fx + width,  fy,          0.0f, 1.0f, texxform.u1, texxform.v0, 0, 1.0f, color.red, color.green, color.blue, color.alpha,
  };

  CHECKGL(glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0));
  CHECKGL(glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0));
  
  nux::ObjectPtr<nux::IOpenGLAsmShaderProgram> shader_program = inverse_texture_mask_asm_prog_;
  if (device_texture->Type().IsDerivedFromType(nux::IOpenGLRectangleTexture::StaticObjectType))
  {
    shader_program = inverse_texture_rect_mask_asm_prog_;
  }
  shader_program->Begin();

  gfx_context.SetTexture(GL_TEXTURE0, device_texture);

  CHECKGL(glMatrixMode(GL_MODELVIEW));
  CHECKGL(glLoadIdentity());
  CHECKGL(glLoadMatrixf((FLOAT *) gfx_context.GetOpenGLModelViewMatrix().m));
  CHECKGL(glMatrixMode(GL_PROJECTION));
  CHECKGL(glLoadIdentity());
  CHECKGL(glLoadMatrixf((FLOAT *) gfx_context.GetOpenGLProjectionMatrix().m));


  int VertexLocation          = nux::VTXATTRIB_POSITION;
  int TextureCoord0Location   = nux::VTXATTRIB_TEXCOORD0;
  int VertexColorLocation     = nux::VTXATTRIB_COLOR;

  CHECKGL(glEnableVertexAttribArrayARB(VertexLocation));
  CHECKGL(glVertexAttribPointerARB((GLuint) VertexLocation, 4, GL_FLOAT, GL_FALSE, 48, VtxBuffer));

  if (TextureCoord0Location != -1)
  {
    CHECKGL(glEnableVertexAttribArrayARB(TextureCoord0Location));
    CHECKGL(glVertexAttribPointerARB((GLuint) TextureCoord0Location, 4, GL_FLOAT, GL_FALSE, 48, VtxBuffer + 4));
  }

  if (VertexColorLocation != -1)
  {
    CHECKGL(glEnableVertexAttribArrayARB(VertexColorLocation));
    CHECKGL(glVertexAttribPointerARB((GLuint) VertexColorLocation, 4, GL_FLOAT, GL_FALSE, 48, VtxBuffer + 8));
  }

  CHECKGL(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));

  CHECKGL(glDisableVertexAttribArrayARB(VertexLocation));

  if (TextureCoord0Location != -1)
    CHECKGL(glDisableVertexAttribArrayARB(TextureCoord0Location));

  if (VertexColorLocation != -1)
    CHECKGL(glDisableVertexAttribArrayARB(VertexColorLocation));

  shader_program->End();
#endif
}

void OverlayRendererImpl::InitSlInverseTextureMaskShader()
{
  nux::ObjectPtr<nux::IOpenGLVertexShader> VS = nux::GetGraphicsDisplay()->GetGpuDevice()->CreateVertexShader();
  nux::ObjectPtr<nux::IOpenGLPixelShader> PS = nux::GetGraphicsDisplay()->GetGpuDevice()->CreatePixelShader();
  std::string VSString;
  std::string PSString;

  VSString =  
             NUX_VERTEX_SHADER_HEADER
             "attribute vec4 AVertex;                                \n\
             attribute vec4 MyTextureCoord0;                         \n\
             attribute vec4 VertexColor;                             \n\
             uniform mat4 ViewProjectionMatrix;                      \n\
             varying vec4 varyTexCoord0;                             \n\
             varying vec4 varyVertexColor;                           \n\
             void main()                                             \n\
             {                                                       \n\
               gl_Position =  ViewProjectionMatrix * (AVertex);      \n\
               varyTexCoord0 = MyTextureCoord0;                      \n\
               varyVertexColor = VertexColor;                        \n\
             }";

  PSString =  
            NUX_FRAGMENT_SHADER_HEADER
             "varying vec4 varyTexCoord0;                               \n\
             varying vec4 varyVertexColor;                              \n\
             uniform sampler2D TextureObject0;                          \n\
             void main()                                                \n\
             {                                                          \n\
               vec4 v = varyVertexColor*texture2D(TextureObject0, varyTexCoord0.xy);       \n\
               gl_FragColor = vec4(1.0-v.a);                            \n\
             }";

  // Textured 2D Primitive Shader
  inverse_texture_mask_prog_ = nux::GetGraphicsDisplay()->GetGpuDevice()->CreateShaderProgram();
  VS->SetShaderCode(TCHAR_TO_ANSI(VSString.c_str()));
  PS->SetShaderCode(TCHAR_TO_ANSI(PSString.c_str()), "#define SAMPLERTEX2D");

  inverse_texture_mask_prog_->ClearShaderObjects();
  inverse_texture_mask_prog_->AddShaderObject(VS);
  inverse_texture_mask_prog_->AddShaderObject(PS);
  CHECKGL(glBindAttribLocation(inverse_texture_mask_prog_->GetOpenGLID(), 0, "AVertex"));
  inverse_texture_mask_prog_->Link();
}

void OverlayRendererImpl::RenderInverseMask_GLSL(nux::GraphicsEngine& gfx_context, int x, int y, int width, int height, nux::ObjectPtr<nux::IOpenGLBaseTexture> DeviceTexture, nux::TexCoordXForm &texxform0, const nux::Color &color0)
{
  if (!inverse_texture_mask_prog_.IsValid())
    InitSlInverseTextureMaskShader();

  QRP_Compute_Texture_Coord(width, height, DeviceTexture, texxform0);
  float fx = x, fy = y;
  float VtxBuffer[] =
  {
    fx,          fy,          0.0f, 1.0f, texxform0.u0, texxform0.v0, 0, 0, color0.red, color0.green, color0.blue, color0.alpha,
    fx,          fy + height, 0.0f, 1.0f, texxform0.u0, texxform0.v1, 0, 0, color0.red, color0.green, color0.blue, color0.alpha,
    fx + width,  fy + height, 0.0f, 1.0f, texxform0.u1, texxform0.v1, 0, 0, color0.red, color0.green, color0.blue, color0.alpha,
    fx + width,  fy,          0.0f, 1.0f, texxform0.u1, texxform0.v0, 0, 0, color0.red, color0.green, color0.blue, color0.alpha,
  };

  nux::ObjectPtr<nux::IOpenGLShaderProgram> ShaderProg;

  if (DeviceTexture->Type().IsDerivedFromType(nux::IOpenGLTexture2D::StaticObjectType))
  {
    ShaderProg = inverse_texture_mask_prog_;
  }

  CHECKGL(glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0));
  CHECKGL(glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0));
  ShaderProg->Begin();

  int TextureObjectLocation = ShaderProg->GetUniformLocationARB("TextureObject0");
  int VertexLocation = ShaderProg->GetAttributeLocation("AVertex");
  int TextureCoord0Location = ShaderProg->GetAttributeLocation("MyTextureCoord0");
  int VertexColorLocation = ShaderProg->GetAttributeLocation("VertexColor");

  gfx_context.SetTexture(GL_TEXTURE0, DeviceTexture);
  CHECKGL(glUniform1iARB(TextureObjectLocation, 0));

  int     VPMatrixLocation = ShaderProg->GetUniformLocationARB("ViewProjectionMatrix");
  nux::Matrix4 MVPMatrix = gfx_context.GetOpenGLModelViewProjectionMatrix();
  ShaderProg->SetUniformLocMatrix4fv((GLint) VPMatrixLocation, 1, false, (GLfloat *) & (MVPMatrix.m));

  CHECKGL(glEnableVertexAttribArrayARB(VertexLocation));
  CHECKGL(glVertexAttribPointerARB((GLuint) VertexLocation, 4, GL_FLOAT, GL_FALSE, 48, VtxBuffer));

  if (TextureCoord0Location != -1)
  {
    CHECKGL(glEnableVertexAttribArrayARB(TextureCoord0Location));
    CHECKGL(glVertexAttribPointerARB((GLuint) TextureCoord0Location, 4, GL_FLOAT, GL_FALSE, 48, VtxBuffer + 4));
  }

  if (VertexColorLocation != -1)
  {
    CHECKGL(glEnableVertexAttribArrayARB(VertexColorLocation));
    CHECKGL(glVertexAttribPointerARB((GLuint) VertexColorLocation, 4, GL_FLOAT, GL_FALSE, 48, VtxBuffer + 8));
  }

  CHECKGL(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));

  CHECKGL(glDisableVertexAttribArrayARB(VertexLocation));

  if (TextureCoord0Location != -1)
    CHECKGL(glDisableVertexAttribArrayARB(TextureCoord0Location));

  if (VertexColorLocation != -1)
    CHECKGL(glDisableVertexAttribArrayARB(VertexColorLocation));

  ShaderProg->End();
}

void OverlayRendererImpl::RenderInverseMask(nux::GraphicsEngine& gfx_context, int x, int y, int width, int height, nux::ObjectPtr<nux::IOpenGLBaseTexture> DeviceTexture, nux::TexCoordXForm &texxform0, const nux::Color &color0)
{
  if (nux::GetWindowThread()->GetGraphicsEngine().UsingGLSLCodePath())
  {
    RenderInverseMask_GLSL(gfx_context, x, y, width, height, DeviceTexture, texxform0, color0);
  }
  else
  {
    RenderInverseMask_ASM(gfx_context, x, y, width, height, DeviceTexture, texxform0, color0); 
  }
}

void OverlayRendererImpl::Draw(nux::GraphicsEngine& gfx_context, nux::Geometry const& content_geo, nux::Geometry const& absolute_geo, nux::Geometry const& geometry, bool force_edges)
{
  nux::Geometry geo(content_geo);
  int excess_border = (Settings::Instance().form_factor() != FormFactor::NETBOOK || force_edges) ? EXCESS_BORDER : 0;

  nux::Geometry larger_content_geo = content_geo;
  larger_content_geo.OffsetSize(excess_border, excess_border);

  nux::Geometry larger_absolute_geo = absolute_geo;
  larger_absolute_geo.OffsetSize(excess_border, excess_border);

  nux::TexCoordXForm texxform_absolute_bg;
  texxform_absolute_bg.flip_v_coord = true;
  texxform_absolute_bg.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
  texxform_absolute_bg.uoffset = 0.0f;
  texxform_absolute_bg.voffset = 0.0f;
  texxform_absolute_bg.SetWrap(nux::TEXWRAP_CLAMP, nux::TEXWRAP_CLAMP);

  if (BackgroundEffectHelper::blur_type != BLUR_NONE)
  {
    bg_blur_texture_ = bg_effect_helper_.GetBlurRegion();
  }
  else
  {
    bg_blur_texture_ = bg_effect_helper_.GetRegion();
  }

  if (bg_blur_texture_.IsValid())
  {
    gfx_context.GetRenderStates().SetBlend(false);
#ifndef NUX_OPENGLES_20
    if (gfx_context.UsingGLSLCodePath())
      gfx_context.QRP_GLSL_ColorLayerOverTexture(larger_content_geo.x, larger_content_geo.y,
                                           larger_content_geo.width, larger_content_geo.height,
                                           bg_blur_texture_, texxform_absolute_bg, nux::color::White,
                                           bg_color_, nux::LAYER_BLEND_MODE_OVERLAY);

    else
      gfx_context.QRP_1Tex (larger_content_geo.x, larger_content_geo.y,
                            larger_content_geo.width, larger_content_geo.height,
                            bg_blur_texture_, texxform_absolute_bg, nux::color::White);
#else
      gfx_context.QRP_GLSL_ColorLayerOverTexture(larger_content_geo.x, larger_content_geo.y,
                                      larger_content_geo.width, larger_content_geo.height,
                                      bg_blur_texture_, texxform_absolute_bg, nux::color::White,
                                      bg_color_, nux::LAYER_BLEND_MODE_OVERLAY);

#endif
  }
  
  //Draw the left and top lines.
  dash::Style& style = dash::Style::Instance();
    
  gfx_context.GetRenderStates().SetColorMask(true, true, true, true);
  gfx_context.GetRenderStates().SetBlend(true);
  gfx_context.GetRenderStates().SetPremultipliedBlend(nux::SRC_OVER);
  
  const double line_opacity = 0.1f;
  const int gradient_height = 50;
  const int vertical_padding = 20;

  // Now that we mask the corners of the dash,
  // draw longer lines to fill the minimal gaps
  const int corner_overlap = 3;

  nux::Color line_color = nux::color::White * line_opacity;

  // Vertical lancher/dash separator
  nux::GetPainter().Paint2DQuadColor(gfx_context,
                                     nux::Geometry(geometry.x,
                                     geometry.y + vertical_padding,
                                     style.GetVSeparatorSize(),
                                     gradient_height),
                                     nux::color::Transparent,
                                     line_color * 0.7f, // less opacity
                                     line_color * 0.7f, // less opacity
                                     nux::color::Transparent);
  nux::GetPainter().Draw2DLine(gfx_context,
                               geometry.x,
                               geometry.y + vertical_padding + gradient_height,
                               style.GetVSeparatorSize(),
                               geometry.y + content_geo.height + INNER_CORNER_RADIUS + corner_overlap,
                               line_color * 0.7f); // less opacity

  //Draw the background
  bg_darken_layer_->SetGeometry(larger_content_geo);
  nux::GetPainter().RenderSinglePaintLayer(gfx_context, larger_content_geo, bg_darken_layer_);

  if (Settings::Instance().GetLowGfxMode() == false)
  {
#ifndef NUX_OPENGLES_20
    if (gfx_context.UsingGLSLCodePath() == false)
    {
      bg_layer_->SetGeometry(larger_content_geo);
      nux::GetPainter().RenderSinglePaintLayer(gfx_context, larger_content_geo, bg_layer_);
    }
#endif

    if (bg_shine_texture_)
    {
      texxform_absolute_bg.flip_v_coord = false;
      texxform_absolute_bg.uoffset = (1.0f / bg_shine_texture_->GetWidth()) * parent->x_offset;
      texxform_absolute_bg.voffset = (1.0f / bg_shine_texture_->GetHeight()) * parent->y_offset;

      gfx_context.GetRenderStates().SetColorMask(true, true, true, false);
      gfx_context.GetRenderStates().SetBlend(true, GL_DST_COLOR, GL_ONE);

      gfx_context.QRP_1Tex(larger_content_geo.x, larger_content_geo.y,
                           larger_content_geo.width, larger_content_geo.height,
                           bg_shine_texture_, texxform_absolute_bg, nux::color::White);

      gfx_context.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    }

    if (bg_refine_gradient_)
    {
      // We draw to the edge of largetr gradient so that we include the corner. A bit cheeky, but faster than draing another
      // texture for the corner.
      int gradien_width = std::min(bg_refine_gradient_->GetDeviceTexture()->GetWidth(), larger_content_geo.width);
      int gradien_height = std::min(bg_refine_gradient_->GetDeviceTexture()->GetHeight(), larger_content_geo.height);

      nux::Geometry geo_refine(larger_content_geo.x + larger_content_geo.width - gradien_width, 
                               larger_content_geo.y,
                               gradien_width, 
                               gradien_height);

      bg_refine_gradient_->SetGeometry(geo_refine);
      bg_refine_gradient_->Renderlayer(gfx_context);
    }
  }

  if (Settings::Instance().form_factor() != FormFactor::NETBOOK || force_edges)
  {
    nux::Geometry geo_border(content_geo.x, content_geo.y, larger_absolute_geo.width - content_geo.x, larger_absolute_geo.height - content_geo.y);
    gfx_context.PushClippingRectangle(geo_border);
  
    // Paint the edges
    {
      gfx_context.GetRenderStates().SetColorMask(true, true, true, true);
      gfx_context.GetRenderStates().SetBlend(true);
      gfx_context.GetRenderStates().SetPremultipliedBlend(nux::SRC_OVER);

      dash::Style& style = dash::Style::Instance();
      nux::BaseTexture* bottom = style.GetDashBottomTile();
      nux::BaseTexture* bottom_mask = style.GetDashBottomTileMask();
      nux::BaseTexture* right = style.GetDashRightTile();
      nux::BaseTexture* right_mask = style.GetDashRightTileMask();
      nux::BaseTexture* corner = style.GetDashCorner();
      nux::BaseTexture* corner_mask = style.GetDashCornerMask();
      nux::BaseTexture* left_corner = style.GetDashLeftCorner();
      nux::BaseTexture* left_corner_mask = style.GetDashLeftCornerMask();
      nux::BaseTexture* left_tile = style.GetDashLeftTile();
      nux::BaseTexture* top_corner = style.GetDashTopCorner();
      nux::BaseTexture* top_corner_mask = style.GetDashTopCornerMask();
      nux::BaseTexture* top_tile = style.GetDashTopTile();
      nux::TexCoordXForm texxform;

      int left_corner_offset = 10;
      int top_corner_offset = 10;

      geo.width += corner->GetWidth() - 10;
      geo.height += corner->GetHeight() - 10;
      { 
        // Corner
        texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
        texxform.SetWrap(nux::TEXWRAP_CLAMP_TO_BORDER, nux::TEXWRAP_CLAMP_TO_BORDER);

        // Selectively erase blur region in the curbe
        gfx_context.QRP_ColorModTexAlpha(geo.x + (geo.width - corner->GetWidth()),
                                         geo.y + (geo.height - corner->GetHeight()),
                                         corner->GetWidth(),
                                         corner->GetHeight(),                                         
                                         corner_mask->GetDeviceTexture(),
                                         texxform,
                                         nux::color::Black);

        // Write correct alpha
        gfx_context.GetRenderStates().SetBlend(false);
        gfx_context.GetRenderStates().SetColorMask(false, false, false, true);
        RenderInverseMask(gfx_context, geo.x + (geo.width - corner->GetWidth()),
                             geo.y + (geo.height - corner->GetHeight()),
                             corner->GetWidth(),
                             corner->GetHeight(),
                             corner_mask->GetDeviceTexture(),
                             texxform,
                             nux::color::White);

        gfx_context.GetRenderStates().SetBlend(true);
        gfx_context.GetRenderStates().SetPremultipliedBlend(nux::SRC_OVER);
        gfx_context.GetRenderStates().SetColorMask(true, true, true, true);

        gfx_context.QRP_1Tex(geo.x + (geo.width - corner->GetWidth()),
                             geo.y + (geo.height - corner->GetHeight()),
                             corner->GetWidth(),
                             corner->GetHeight(),
                             corner->GetDeviceTexture(),
                             texxform,
                             nux::color::White);
      }
      {
        // Bottom repeated texture
        int real_width = geo.width - (left_corner->GetWidth() - left_corner_offset) - corner->GetWidth();
        int offset = real_width % bottom->GetWidth();

        texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
        texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);

        // Selectively erase blur region in the curbe
        gfx_context.QRP_ColorModTexAlpha(left_corner->GetWidth() - left_corner_offset - offset,
                                         geo.y + (geo.height - bottom->GetHeight()),
                                         real_width + offset,
                                         bottom->GetHeight(),                                         
                                         bottom_mask->GetDeviceTexture(),
                                         texxform,
                                         nux::color::Black);

        // Write correct alpha
        gfx_context.GetRenderStates().SetBlend(false);
        gfx_context.GetRenderStates().SetColorMask(false, false, false, true);
        RenderInverseMask(gfx_context, left_corner->GetWidth() - left_corner_offset - offset,
                             geo.y + (geo.height - bottom->GetHeight()),
                             real_width + offset,
                             bottom->GetHeight(),
                             bottom_mask->GetDeviceTexture(),
                             texxform,
                             nux::color::White);

        gfx_context.GetRenderStates().SetBlend(true);
        gfx_context.GetRenderStates().SetPremultipliedBlend(nux::SRC_OVER);
        gfx_context.GetRenderStates().SetColorMask(true, true, true, true);

        gfx_context.QRP_1Tex(left_corner->GetWidth() - left_corner_offset - offset,
                             geo.y + (geo.height - bottom->GetHeight()),
                             real_width + offset,
                             bottom->GetHeight(),
                             bottom->GetDeviceTexture(),
                             texxform,
                             nux::color::White);
      }
      {
        // Bottom left corner
        texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
        texxform.SetWrap(nux::TEXWRAP_CLAMP_TO_BORDER, nux::TEXWRAP_CLAMP_TO_BORDER);

        // Selectively erase blur region in the curbe
        gfx_context.QRP_ColorModTexAlpha(geo.x - left_corner_offset,
                                         geo.y + (geo.height - left_corner->GetHeight()),
                                         left_corner->GetWidth(),
                                         left_corner->GetHeight(),                                         
                                         left_corner_mask->GetDeviceTexture(),
                                         texxform,
                                         nux::color::Black);

        // Write correct alpha
        gfx_context.GetRenderStates().SetBlend(false);
        gfx_context.GetRenderStates().SetColorMask(false, false, false, true);
        RenderInverseMask(gfx_context, geo.x - left_corner_offset,
                             geo.y + (geo.height - left_corner->GetHeight()),
                             left_corner->GetWidth(),
                             left_corner->GetHeight(),
                             left_corner_mask->GetDeviceTexture(),
                             texxform,
                             nux::color::White);

        gfx_context.GetRenderStates().SetBlend(true);
        gfx_context.GetRenderStates().SetPremultipliedBlend(nux::SRC_OVER);
        gfx_context.GetRenderStates().SetColorMask(true, true, true, true);

        gfx_context.QRP_1Tex(geo.x - left_corner_offset,
                             geo.y + (geo.height - left_corner->GetHeight()),
                             left_corner->GetWidth(),
                             left_corner->GetHeight(),
                             left_corner->GetDeviceTexture(),
                             texxform,
                             nux::color::White);
      }
      {
        // Left repeated texture
        nux::Geometry real_geo = geometry;
        int real_height = real_geo.height - geo.height;
        int offset = real_height % left_tile->GetHeight();

        texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
        texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);

        gfx_context.QRP_1Tex(geo.x - 10,
                             geo.y + geo.height,
                             left_tile->GetWidth(),
                             real_height + offset,
                             left_tile->GetDeviceTexture(),
                             texxform,
                             nux::color::White);
      }
      {
        // Right edge
        texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
        texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);

        // Selectively erase blur region in the curbe
        gfx_context.QRP_ColorModTexAlpha(geo.x + geo.width - right->GetWidth(),
                                         geo.y + top_corner->GetHeight() - top_corner_offset,
                                         right->GetWidth(),
                                         geo.height - corner->GetHeight() - (top_corner->GetHeight() - top_corner_offset),                                       
                                         right_mask->GetDeviceTexture(),
                                         texxform,
                                         nux::color::Black);

        // Write correct alpha
        gfx_context.GetRenderStates().SetBlend(false);
        gfx_context.GetRenderStates().SetColorMask(false, false, false, true);
        RenderInverseMask(gfx_context, geo.x + geo.width - right->GetWidth(),
                             geo.y + top_corner->GetHeight() - top_corner_offset,
                             right->GetWidth(),
                             geo.height - corner->GetHeight() - (top_corner->GetHeight() - top_corner_offset),
                             right_mask->GetDeviceTexture(),
                             texxform,
                             nux::color::White);

        gfx_context.GetRenderStates().SetBlend(true);
        gfx_context.GetRenderStates().SetPremultipliedBlend(nux::SRC_OVER);
        gfx_context.GetRenderStates().SetColorMask(true, true, true, true);

        gfx_context.QRP_1Tex(geo.x + geo.width - right->GetWidth(),
                             geo.y + top_corner->GetHeight() - top_corner_offset,
                             right->GetWidth(),
                             geo.height - corner->GetHeight() - (top_corner->GetHeight() - top_corner_offset),
                             right->GetDeviceTexture(),
                             texxform,
                             nux::color::White);
      }
      {
        // Top right corner
        texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
        texxform.SetWrap(nux::TEXWRAP_CLAMP_TO_BORDER, nux::TEXWRAP_CLAMP_TO_BORDER);

        // Selectively erase blur region in the curbe
        gfx_context.QRP_ColorModTexAlpha(geo.x + geo.width - right->GetWidth(),
                                        geo.y - top_corner_offset,
                                        top_corner->GetWidth(),
                                        top_corner->GetHeight(),
                                        top_corner_mask->GetDeviceTexture(),
                                        texxform,
                                        nux::color::Black);

        // Write correct alpha
        gfx_context.GetRenderStates().SetBlend(false);
        gfx_context.GetRenderStates().SetColorMask(false, false, false, true);
        RenderInverseMask(gfx_context, geo.x + geo.width - right->GetWidth(),
                                        geo.y - top_corner_offset,
                                        top_corner->GetWidth(),
                                        top_corner->GetHeight(),
                                        top_corner_mask->GetDeviceTexture(),
                                        texxform,
                                        nux::color::White);

        gfx_context.GetRenderStates().SetBlend(true);
        gfx_context.GetRenderStates().SetPremultipliedBlend(nux::SRC_OVER);
        gfx_context.GetRenderStates().SetColorMask(true, true, true, true);
        gfx_context.QRP_1Tex(geo.x + geo.width - right->GetWidth(),
                             geo.y - top_corner_offset,
                             top_corner->GetWidth(),
                             top_corner->GetHeight(),
                             top_corner->GetDeviceTexture(),
                             texxform,
                             nux::color::White);
      }
      {
        // Top edge
        texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
        texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);

        gfx_context.GetRenderStates().SetPremultipliedBlend(nux::SRC_OVER);
        gfx_context.QRP_1Tex(geo.x + geo.width,
                             geo.y - 10,
                             geometry.width - (geo.x + geo.width),
                             top_tile->GetHeight(),
                             top_tile->GetDeviceTexture(),
                             texxform,
                             nux::color::White);
      }
    }
    
    gfx_context.PopClippingRectangle();
  }

  gfx_context.GetRenderStates().SetPremultipliedBlend(nux::SRC_OVER);
  gfx_context.GetRenderStates().SetColorMask(true, true, true, true);
  gfx_context.GetRenderStates().SetBlend(false);

}

void OverlayRendererImpl::DrawContent(nux::GraphicsEngine& gfx_context, nux::Geometry const& content_geo, nux::Geometry const& absolute_geo, nux::Geometry const& geometry)
{
  bgs = 0;

  int excess_border = (Settings::Instance().form_factor() != FormFactor::NETBOOK) ? EXCESS_BORDER : 0;

  nux::Geometry larger_content_geo = content_geo;
  larger_content_geo.OffsetSize(excess_border, excess_border);

  nux::Geometry larger_geo(larger_content_geo);

  nux::Geometry larger_absolute_geo = absolute_geo;
  larger_absolute_geo.OffsetSize(excess_border, excess_border);
  
  gfx_context.PushClippingRectangle(larger_geo);

  unsigned int blend_alpha, blend_src, blend_dest = 0;
  gfx_context.GetRenderStates().GetBlend(blend_alpha, blend_src, blend_dest);
  gfx_context.GetRenderStates().SetBlend(true);
  gfx_context.GetRenderStates().SetPremultipliedBlend(nux::SRC_OVER);

  nux::TexCoordXForm texxform_absolute_bg;
  texxform_absolute_bg.flip_v_coord = true;
  texxform_absolute_bg.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
  texxform_absolute_bg.uoffset = 0.0f;
  texxform_absolute_bg.voffset = 0.0f;
  texxform_absolute_bg.SetWrap(nux::TEXWRAP_CLAMP, nux::TEXWRAP_CLAMP);

  nux::ROPConfig rop;
  rop.Blend = false;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

  if (bg_blur_texture_.IsValid())
  {
#ifndef NUX_OPENGLES_20
    if (gfx_context.UsingGLSLCodePath())
      gPainter.PushCompositionLayer(gfx_context, larger_content_geo,
                                    bg_blur_texture_,
                                    texxform_absolute_bg,
                                    nux::color::White,
                                    bg_color_, nux::LAYER_BLEND_MODE_OVERLAY,
                                    true, rop);
    else
      gPainter.PushTextureLayer(gfx_context, larger_content_geo,
                                bg_blur_texture_,
                                texxform_absolute_bg,
                                nux::color::White,
                                true, // write alpha?
                                rop);
#else
      gPainter.PushCompositionLayer(gfx_context, larger_content_geo,
                                    bg_blur_texture_,
                                    texxform_absolute_bg,
                                    nux::color::White,
                                    bg_color_, nux::LAYER_BLEND_MODE_OVERLAY,
                                    true, rop);
#endif
    bgs++;
  }

  // draw the darkening behind our paint
  nux::GetPainter().PushLayer(gfx_context, bg_darken_layer_->GetGeometry(), bg_darken_layer_);
  bgs++;

  //Only apply shine if we aren't in low gfx mode.
  if (Settings::Instance().GetLowGfxMode() == false)
  {
#ifndef NUX_OPENGLES_20
    if (gfx_context.UsingGLSLCodePath() == false)
    {
      nux::GetPainter().PushLayer(gfx_context, bg_layer_->GetGeometry(), bg_layer_);
      bgs++;
    }
#endif

    if (bg_shine_texture_)
    {
      rop.Blend = true;
      rop.SrcBlend = GL_DST_COLOR;
      rop.DstBlend = GL_ONE;
      texxform_absolute_bg.flip_v_coord = false;
      texxform_absolute_bg.uoffset = (1.0f / bg_shine_texture_->GetWidth()) * parent->x_offset;
      texxform_absolute_bg.voffset = (1.0f / bg_shine_texture_->GetHeight()) * parent->y_offset;
      nux::Color shine_colour = nux::color::White;

      nux::GetPainter().PushTextureLayer(gfx_context, larger_content_geo,
                                        bg_shine_texture_,
                                        texxform_absolute_bg,
                                        shine_colour,
                                        true,
                                        rop);
      bgs++;
    }

    if (bg_refine_gradient_)
    {
      nux::GetPainter().PushLayer(gfx_context, bg_refine_gradient_->GetGeometry(), bg_refine_gradient_.get());
      bgs++;
    }
  }

  gfx_context.GetRenderStates().SetBlend(blend_alpha, blend_src, blend_dest);
  gfx_context.PopClippingRectangle();
}

void OverlayRendererImpl::DrawContentCleanup(nux::GraphicsEngine& gfx_context, nux::Geometry const& content_geo, nux::Geometry const& absolute_geo, nux::Geometry const& geometry)
{
  nux::GetPainter().PopBackground(bgs);
  bgs = 0;
}



OverlayRenderer::OverlayRenderer()
  : pimpl_(new OverlayRendererImpl(this))
{

}


OverlayRenderer::~OverlayRenderer()
{
  delete pimpl_;
}

void OverlayRenderer::AboutToHide()
{
  pimpl_->visible = false;
  pimpl_->bg_effect_helper_.enabled = false;
  need_redraw.emit();
}

void OverlayRenderer::UpdateBlurBackgroundSize(nux::Geometry const& content_geo,
                                               nux::Geometry const& absolute_geo,
                                               bool                 force_edges)
{
  int excess_border = (Settings::Instance().form_factor() != FormFactor::NETBOOK || force_edges) ? EXCESS_BORDER : 0;

  nux::Geometry larger_absolute_geo = absolute_geo;
  larger_absolute_geo.OffsetSize(excess_border, excess_border);

  nux::Geometry larger_content_geo = content_geo;
  larger_content_geo.OffsetSize(excess_border, excess_border);

  nux::Geometry blur_geo(larger_absolute_geo.x, larger_absolute_geo.y,
                         larger_content_geo.width, larger_content_geo.height);

  pimpl_->bg_effect_helper_.SetBackbufferRegion(blur_geo);
}

void OverlayRenderer::AboutToShow()
{
  pimpl_->ubus_manager_.SendMessage(UBUS_BACKGROUND_REQUEST_COLOUR_EMIT);
  pimpl_->visible = true;
  pimpl_->bg_effect_helper_.enabled = true;
  need_redraw.emit();
}

void OverlayRenderer::SetOwner(nux::View* owner)
{
  pimpl_->bg_effect_helper_.owner= owner;
}

void OverlayRenderer::DisableBlur()
{
  pimpl_->bg_effect_helper_.blur_type = BLUR_NONE;
}

void OverlayRenderer::DrawFull(nux::GraphicsEngine& gfx_context, nux::Geometry const& content_geo, nux::Geometry const& absolute_geo, nux::Geometry const& geo, bool force_edges)
{
  pimpl_->Draw(gfx_context, content_geo, absolute_geo, geo, force_edges);
  LOG_DEBUG(logger) << "OverlayRenderer::DrawFull(): content_geo:  " << content_geo.width << "/" << content_geo.height;
  LOG_DEBUG(logger) << "OverlayRenderer::DrawFull(): absolute_geo: " << absolute_geo.width << "/" << absolute_geo.height;
  LOG_DEBUG(logger) << "OverlayRenderer::DrawFull(): geo:          " << geo.width << "/" << geo.height;
}

void OverlayRenderer::DrawInner(nux::GraphicsEngine& gfx_context, nux::Geometry const& content_geo, nux::Geometry const& absolute_geo, nux::Geometry const& geo)
{
  pimpl_->DrawContent(gfx_context, content_geo, absolute_geo, geo);
  LOG_DEBUG(logger) << "OverlayRenderer::DrawInner(): content_geo:  " << content_geo.width << "/" << content_geo.height;
  LOG_DEBUG(logger) << "OverlayRenderer::DrawInner(): absolute_geo: " << absolute_geo.width << "/" << absolute_geo.height;
  LOG_DEBUG(logger) << "OverlayRenderer::DrawInner(): geo:          " << geo.width << "/" << geo.height;
}

void OverlayRenderer::DrawInnerCleanup(nux::GraphicsEngine& gfx_context, nux::Geometry const& content_geo, nux::Geometry const& absolute_geo, nux::Geometry const& geo)
{
  pimpl_->DrawContentCleanup(gfx_context, content_geo, absolute_geo, geo);
  LOG_DEBUG(logger) << "OverlayRenderer::DrawInnerCleanup(): content_geo:  " << content_geo.width << "/" << content_geo.height;
  LOG_DEBUG(logger) << "OverlayRenderer::DrawInnerCleanup(): absolute_geo: " << absolute_geo.width << "/" << absolute_geo.height;
  LOG_DEBUG(logger) << "OverlayRenderer::DrawInnerCleanup(): geo:          " << geo.width << "/" << geo.height;
}

}


