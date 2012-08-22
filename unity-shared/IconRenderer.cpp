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
 * Authored by: Jason Smith <jason.smith@canonical.com>
 */

#include "config.h"
#include <math.h>

#include <Nux/Nux.h>
#include <Nux/WindowCompositor.h>
#include <NuxGraphics/NuxGraphics.h>
#include <NuxGraphics/GpuDevice.h>
#include <NuxGraphics/GLTextureResourceManager.h>

#include <NuxGraphics/CairoGraphics.h>

#include <gtk/gtk.h>

#include <pango/pango.h>
#include <pango/pangocairo.h>

#include "IconRenderer.h"

namespace unity
{
namespace ui
{

#ifdef USE_GLES
  #define VertexShaderHeader   "#version 100\n"
  #define FragmentShaderHeader "#version 100\n precision mediump float;\n"
#else
  #define VertexShaderHeader   "#version 120\n"
  #define FragmentShaderHeader "#version 110\n"
#endif

/*
  Use this shader to pass vertices in screen coordinates in the C++ code and compute use
  the fragment shader to perform the texture perspective correct division.
  This shader assume the following:
    - the projection matrix is orthogonal: glOrtho(0, ScreenWidth, ScreenWidth, 0, Near, Far)
    - vertices x and y are in screen coordinates: Vertex(x_screen, y_screen, 0, 1.0)
    - the vertices w coordinates has been computed 'manually'
    - vertices uv textture coordinates are passed to the shader as:  (u/w, v/w, 0, 1/w)

  The texture coordinates s=u/w, t=v/w and q=1w are interpolated linearly in screen coordinates.
  In the fragment shader we get the texture coordinates used for the sampling by dividing
  s and t resulting from the interpolation by q.

*/

// halfed lumin values to provide darkening while desaturating in shader
#define LUMIN_RED "0.30"
#define LUMIN_GREEN "0.59"
#define LUMIN_BLUE "0.11"

nux::NString gPerspectiveCorrectShader = TEXT(
"[Vertex Shader]                                    \n"
VertexShaderHeader
"uniform mat4 ViewProjectionMatrix;                 \n\
                                                    \n\
attribute vec4 iTexCoord0;                          \n\
attribute vec4 iVertex;                             \n\
                                                    \n\
varying vec4 varyTexCoord0;                         \n\
                                                    \n\
void main()                                         \n\
{                                                   \n\
    varyTexCoord0 = iTexCoord0;                     \n\
    gl_Position =  ViewProjectionMatrix * iVertex;  \n\
}                                                   \n\
                                                    \n\
[Fragment Shader]                                   \n"
FragmentShaderHeader
"                                                   \n\
varying vec4 varyTexCoord0;                         \n\
                                                    \n\
uniform sampler2D TextureObject0;                   \n\
uniform vec4 color0;                                \n\
uniform vec4 desat_factor;                          \n\
uniform vec4 colorify_color;                        \n\
vec4 SampleTexture(sampler2D TexObject, vec4 TexCoord) \n\
{                                                   \n\
  return texture2D(TexObject, TexCoord.st);         \n\
}                                                   \n\
                                                    \n\
void main()                                         \n\
{                                                   \n\
  vec4 tex = varyTexCoord0;                         \n\
  tex.s = tex.s/varyTexCoord0.w;                    \n\
  tex.t = tex.t/varyTexCoord0.w;                    \n\
                                                    \n\
  vec4 texel = color0 * SampleTexture(TextureObject0, tex);  \n\
  vec4 desat = vec4 (" LUMIN_RED "*texel.r + " LUMIN_GREEN "*texel.g + " LUMIN_BLUE "*texel.b);  \n\
  vec4 final_color = (vec4 (1.0, 1.0, 1.0, 1.0) - desat_factor) * desat + desat_factor * texel; \n\
  final_color = colorify_color * final_color;       \n\
  final_color.a = texel.a;                          \n\
  gl_FragColor = final_color;                       \n\
}                                                   \n\
");

nux::NString PerspectiveCorrectVtx = TEXT(
"!!ARBvp1.0                                 \n\
ATTRIB iPos         = vertex.position;      \n\
ATTRIB iColor       = vertex.attrib[3];     \n\
PARAM  mvp[4]       = {state.matrix.mvp};   \n\
OUTPUT oPos         = result.position;      \n\
OUTPUT oColor       = result.color;         \n\
OUTPUT oTexCoord0   = result.texcoord[0];   \n\
# Transform the vertex to clip coordinates. \n\
DP4   oPos.x, mvp[0], iPos;                 \n\
DP4   oPos.y, mvp[1], iPos;                 \n\
DP4   oPos.z, mvp[2], iPos;                 \n\
DP4   oPos.w, mvp[3], iPos;                 \n\
MOV   oColor, iColor;                       \n\
MOV   oTexCoord0, vertex.attrib[8];         \n\
END");

nux::NString PerspectiveCorrectTexFrg = TEXT(
"!!ARBfp1.0                                                   \n\
PARAM color0 = program.local[0];                              \n\
PARAM factor = program.local[1];                              \n\
PARAM colorify_color = program.local[2];                      \n\
PARAM luma = {" LUMIN_RED ", " LUMIN_GREEN ", " LUMIN_BLUE ", 0.0}; \n\
TEMP temp;                                                    \n\
TEMP pcoord;                                                  \n\
TEMP tex0;                                                    \n\
TEMP desat;                                                   \n\
TEMP color;                                                   \n\
MOV pcoord, fragment.texcoord[0].w;                           \n\
RCP temp, fragment.texcoord[0].w;                             \n\
MUL pcoord.xy, fragment.texcoord[0], temp;                    \n\
TEX tex0, pcoord, texture[0], 2D;                             \n\
MUL color, color0, tex0;                                      \n\
DP4 desat, luma, color;                                       \n\
LRP temp, factor.x, color, desat;                             \n\
MUL result.color.rgb, temp, colorify_color;                   \n\
MOV result.color.a, color;                                    \n\
END");

nux::NString PerspectiveCorrectTexRectFrg = TEXT(
"!!ARBfp1.0                                                   \n\
PARAM color0 = program.local[0];                              \n\
PARAM factor = program.local[1];                              \n\
PARAM colorify_color = program.local[2];                      \n\
PARAM luma = {" LUMIN_RED ", " LUMIN_GREEN ", " LUMIN_BLUE ", 0.0}; \n\
TEMP temp;                                                    \n\
TEMP pcoord;                                                  \n\
TEMP tex0;                                                    \n\
MOV pcoord, fragment.texcoord[0].w;                           \n\
RCP temp, fragment.texcoord[0].w;                             \n\
MUL pcoord.xy, fragment.texcoord[0], temp;                    \n\
TEX tex0, pcoord, texture[0], RECT;                           \n\
MUL color, color0, tex0;                                      \n\
DP4 desat, luma, color;                                       \n\
LRP temp, factor.x, color, desat;                             \n\
MUL result.color.rgb, temp, colorify_color;                   \n\
MOV result.color.a, color;                                    \n\
END");

// The local namespace is purely for namespacing the file local variables below.
namespace local
{
namespace
{
enum IconSize
{
  SMALL = 0,
  BIG,
  LAST,
};

bool textures_created = false;
nux::BaseTexture* progress_bar_trough = 0;
nux::BaseTexture* progress_bar_fill = 0;
nux::BaseTexture* pip_ltr = 0;
nux::BaseTexture* pip_rtl = 0;
nux::BaseTexture* arrow_ltr = 0;
nux::BaseTexture* arrow_rtl = 0;
nux::BaseTexture* arrow_empty_ltr = 0;
nux::BaseTexture* arrow_empty_rtl = 0;

nux::BaseTexture* squircle_base = 0;
nux::BaseTexture* squircle_base_selected = 0;
nux::BaseTexture* squircle_edge = 0;
nux::BaseTexture* squircle_glow = 0;
nux::BaseTexture* squircle_shadow = 0;
nux::BaseTexture* squircle_shine = 0;

std::vector<nux::BaseTexture*> icon_background;
std::vector<nux::BaseTexture*> icon_selected_background;
std::vector<nux::BaseTexture*> icon_edge;
std::vector<nux::BaseTexture*> icon_glow;
std::vector<nux::BaseTexture*> icon_shadow;
std::vector<nux::BaseTexture*> icon_shine;
nux::ObjectPtr<nux::IOpenGLBaseTexture> offscreen_progress_texture;
nux::ObjectPtr<nux::IOpenGLShaderProgram> shader_program_uv_persp_correction;
nux::ObjectPtr<nux::IOpenGLAsmShaderProgram> asm_shader;
std::map<char, nux::BaseTexture*> label_map;

void generate_textures();
void destroy_textures();
}
}

IconRenderer::IconRenderer()
{
  pip_style = OUTSIDE_TILE;

  if (!local::textures_created)
    local::generate_textures();
}

IconRenderer::~IconRenderer()
{
}

void IconRenderer::SetTargetSize(int tile_size, int image_size_, int spacing_)
{
  icon_size = tile_size;
  image_size = image_size_;
  spacing = spacing_;
}

void IconRenderer::PreprocessIcons(std::list<RenderArg>& args, nux::Geometry const& geo)
{
  nux::Matrix4 ObjectMatrix;
  nux::Matrix4 ViewMatrix;
  nux::Matrix4 ProjectionMatrix;
  nux::Matrix4 ViewProjectionMatrix;

  _stored_projection_matrix = nux::GetWindowThread()->GetGraphicsEngine().GetOpenGLModelViewProjectionMatrix();

  GetInverseScreenPerspectiveMatrix(ViewMatrix, ProjectionMatrix, geo.width, geo.height, 0.1f, 1000.0f, DEGTORAD(90));

  nux::Matrix4 PremultMatrix = ProjectionMatrix * ViewMatrix;

  std::list<RenderArg>::iterator it;
  int i;
  for (it = args.begin(), i = 0; it != args.end(); ++it, i++)
  {

    IconTextureSource* launcher_icon = it->icon;

    float w = icon_size;
    float h = icon_size;
    float x = it->render_center.x - w / 2.0f; // x: top left corner
    float y = it->render_center.y - h / 2.0f; // y: top left corner
    float z = it->render_center.z;

    if (it->skip)
    {
      w = 1;
      h = 1;
      x = -100;
      y = -100;
    }

    ObjectMatrix = nux::Matrix4::TRANSLATE(geo.width / 2.0f, geo.height / 2.0f, z) * // Translate the icon to the center of the viewport
                   nux::Matrix4::ROTATEX(it->x_rotation) *              // rotate the icon
                   nux::Matrix4::ROTATEY(it->y_rotation) *
                   nux::Matrix4::ROTATEZ(it->z_rotation) *
                   nux::Matrix4::TRANSLATE(-x - w / 2.0f, -y - h / 2.0f, -z); // Put the center the icon to (0, 0)

    ViewProjectionMatrix = PremultMatrix * ObjectMatrix;

    UpdateIconTransform(launcher_icon, ViewProjectionMatrix, geo, x, y, w, h, z, ui::IconTextureSource::TRANSFORM_TILE);

    w = image_size;
    h = image_size;
    x = it->render_center.x - icon_size / 2.0f + (icon_size - image_size) / 2.0f;
    y = it->render_center.y - icon_size / 2.0f + (icon_size - image_size) / 2.0f;
    z = it->render_center.z;

    UpdateIconTransform(launcher_icon, ViewProjectionMatrix, geo, x, y, w, h, z, ui::IconTextureSource::TRANSFORM_IMAGE);

    // hardcode values for now until SVG's are in place and we can remove this
    // 200 == size of large glow
    // 150 == size of large tile
    // 62 == size of small glow
    // 54 == size of small tile
    float icon_glow_size = 0.0f;
    if (icon_size > 100)
      icon_glow_size = icon_size * (200.0f / 150.0f);
    else
      icon_glow_size = icon_size * (62.0f / 54.0f);
    w = icon_glow_size;
    h = icon_glow_size;
    x = it->render_center.x - icon_glow_size / 2.0f;
    y = it->render_center.y - icon_glow_size / 2.0f;
    z = it->render_center.z;

    UpdateIconTransform(launcher_icon, ViewProjectionMatrix, geo, x, y, w, h, z, ui::IconTextureSource::TRANSFORM_GLOW);

    w = geo.width + 2;
    h = icon_size + spacing;
    if (i == (int) args.size() - 1)
      h += 4;
    x = it->logical_center.x - w / 2.0f;
    y = it->logical_center.y - h / 2.0f;
    z = it->logical_center.z;

    UpdateIconTransform(launcher_icon, ViewProjectionMatrix, geo, x, y, w, h, z, ui::IconTextureSource::TRANSFORM_HIT_AREA);

    if (launcher_icon->Emblem())
    {
      nux::BaseTexture* emblem = launcher_icon->Emblem();

      float w = icon_size;
      float h = icon_size;

      float emb_w = emblem->GetWidth();
      float emb_h = emblem->GetHeight();

      x = it->render_center.x + (icon_size * 0.50f - emb_w - icon_size * 0.05f); // puts right edge of emblem just over the edge of the launcher icon
      y = it->render_center.y - icon_size * 0.50f;     // y = top left corner position of emblem
      z = it->render_center.z;

      ObjectMatrix = nux::Matrix4::TRANSLATE(geo.width / 2.0f, geo.height / 2.0f, z) * // Translate the icon to the center of the viewport
                     nux::Matrix4::ROTATEX(it->x_rotation) *              // rotate the icon
                     nux::Matrix4::ROTATEY(it->y_rotation) *
                     nux::Matrix4::ROTATEZ(it->z_rotation) *
                     nux::Matrix4::TRANSLATE(-(it->render_center.x - w / 2.0f) - w / 2.0f, -(it->render_center.y - h / 2.0f) - h / 2.0f, -z); // Put the center the icon to (0, 0)

      ViewProjectionMatrix = PremultMatrix * ObjectMatrix;

      UpdateIconSectionTransform(launcher_icon, ViewProjectionMatrix, geo, x, y, emb_w, emb_h, z,
                                 it->render_center.x - w / 2.0f, it->render_center.y - h / 2.0f, w, h, ui::IconTextureSource::TRANSFORM_EMBLEM);
    }
  }
}

void IconRenderer::UpdateIconTransform(ui::IconTextureSource* icon, nux::Matrix4 ViewProjectionMatrix, nux::Geometry const& geo,
                                       float x, float y, float w, float h, float z, ui::IconTextureSource::TransformIndex index)
{
  UpdateIconSectionTransform (icon, ViewProjectionMatrix, geo, x, y, w, h, z, x, y, w, h, index);
}

void IconRenderer::UpdateIconSectionTransform(ui::IconTextureSource* icon, nux::Matrix4 ViewProjectionMatrix, nux::Geometry const& geo,
                                              float x, float y, float w, float h, float z, float xx, float yy, float ww, float hh, ui::IconTextureSource::TransformIndex index)
{
  nux::Vector4 v0 = nux::Vector4(x,     y,     z, 1.0f);
  nux::Vector4 v1 = nux::Vector4(x,     y + h, z, 1.0f);
  nux::Vector4 v2 = nux::Vector4(x + w, y + h, z, 1.0f);
  nux::Vector4 v3 = nux::Vector4(x + w, y,     z, 1.0f);

  v0 = ViewProjectionMatrix * v0;
  v1 = ViewProjectionMatrix * v1;
  v2 = ViewProjectionMatrix * v2;
  v3 = ViewProjectionMatrix * v3;

  v0.divide_xyz_by_w();
  v1.divide_xyz_by_w();
  v2.divide_xyz_by_w();
  v3.divide_xyz_by_w();

  // normalize to the viewport coordinates and translate to the correct location
  v0.x =  geo.width  * (v0.x + 1.0f) / 2.0f - geo.width  / 2.0f + xx + ww / 2.0f;
  v0.y = -geo.height * (v0.y - 1.0f) / 2.0f - geo.height / 2.0f + yy + hh / 2.0f;
  v1.x =  geo.width  * (v1.x + 1.0f) / 2.0f - geo.width  / 2.0f + xx + ww / 2.0f;;
  v1.y = -geo.height * (v1.y - 1.0f) / 2.0f - geo.height / 2.0f + yy + hh / 2.0f;
  v2.x =  geo.width  * (v2.x + 1.0f) / 2.0f - geo.width  / 2.0f + xx + ww / 2.0f;
  v2.y = -geo.height * (v2.y - 1.0f) / 2.0f - geo.height / 2.0f + yy + hh / 2.0f;
  v3.x =  geo.width  * (v3.x + 1.0f) / 2.0f - geo.width  / 2.0f + xx + ww / 2.0f;
  v3.y = -geo.height * (v3.y - 1.0f) / 2.0f - geo.height / 2.0f + yy + hh / 2.0f;


  std::vector<nux::Vector4>& vectors = icon->GetTransform(index, monitor);

  vectors[0] = v0;
  vectors[1] = v1;
  vectors[2] = v2;
  vectors[3] = v3;
}

void IconRenderer::RenderIcon(nux::GraphicsEngine& GfxContext, RenderArg const& arg, nux::Geometry const& geo, nux::Geometry const& owner_geo)
{
  // This check avoids a crash when the icon is not available on the system.
  if (arg.icon->TextureForSize(image_size) == 0 || arg.skip)
    return;

  local::IconSize size = icon_size > 100 ? local::IconSize::BIG : local::IconSize::SMALL;

  GfxContext.GetRenderStates().SetBlend(true);
  GfxContext.GetRenderStates().SetPremultipliedBlend(nux::SRC_OVER);
  GfxContext.GetRenderStates().SetColorMask(true, true, true, true);

  nux::Color background_tile_color = arg.icon->BackgroundColor();
  nux::Color glow_color = arg.icon->GlowColor();
  nux::Color edge_color(0x55555555);
  nux::Color colorify = arg.colorify;
  nux::Color background_tile_colorify = arg.colorify;
  float backlight_intensity = arg.backlight_intensity;
  float glow_intensity = arg.glow_intensity;
  float shadow_intensity = 0.6f;

  nux::BaseTexture* background = local::icon_background[size];
  nux::BaseTexture* edge = local::icon_edge[size];
  nux::BaseTexture* glow = local::icon_glow[size];
  nux::BaseTexture* shine = local::icon_shine[size];
  nux::BaseTexture* shadow = local::icon_shadow[size];

  bool force_filter = icon_size != background->GetWidth();

  if (arg.keyboard_nav_hl)
  {
    background_tile_color = nux::color::White;
    glow_color = nux::color::White;
    edge_color = nux::color::White;
    colorify = nux::color::White;
    background_tile_colorify = nux::color::White;
    backlight_intensity = 0.95f;
    glow_intensity = 1.0f;
    shadow_intensity = 0.0f;

    background = local::icon_selected_background[size];
  }
  else
  {
    colorify.red +=   (0.5f + 0.5f * arg.saturation) * (1.0f - colorify.red);
    colorify.blue +=  (0.5f + 0.5f * arg.saturation) * (1.0f - colorify.blue);
    colorify.green += (0.5f + 0.5f * arg.saturation) * (1.0f - colorify.green);

    if (arg.colorify_background)
    {
      background_tile_colorify = background_tile_colorify * 0.7f;
    }
    else
    {
      background_tile_colorify.red +=   (0.5f + 0.5f * arg.saturation) * (1.0f - background_tile_colorify.red);
      background_tile_colorify.green += (0.5f + 0.5f * arg.saturation) * (1.0f - background_tile_colorify.green);
      background_tile_colorify.blue +=  (0.5f + 0.5f * arg.saturation) * (1.0f - background_tile_colorify.blue);
    }
  }

  if (arg.system_item)
  {
    // 0.9f is BACKLIGHT_STRENGTH in Launcher.cpp
    backlight_intensity = (arg.keyboard_nav_hl) ? 0.95f : 0.9f;
    glow_intensity = (arg.keyboard_nav_hl) ? 1.0f : 0.0f ;

    background = local::squircle_base_selected;
    edge = local::squircle_edge;
    glow = local::squircle_glow;
    shine = local::squircle_shine;
    shadow = local::squircle_shadow;
  }

  // draw shadow
  if (shadow_intensity > 0)
  {
    nux::Color shadow_color = background_tile_colorify * 0.3f;

    // FIXME it is using the same transformation of the glow,
    // should have its own transformation.
    RenderElement(GfxContext,
                  arg,
                  shadow->GetDeviceTexture(),
                  nux::color::White,
                  shadow_color,
                  shadow_intensity * arg.alpha,
                  force_filter,
                  arg.icon->GetTransform(ui::IconTextureSource::TRANSFORM_GLOW, monitor));
  }

  auto tile_transform = arg.icon->GetTransform(ui::IconTextureSource::TRANSFORM_TILE, monitor);

  // draw tile
  if (backlight_intensity > 0 && !arg.draw_edge_only)
  {
    RenderElement(GfxContext,
                  arg,
                  background->GetDeviceTexture(),
                  background_tile_color,
                  background_tile_colorify,
                  backlight_intensity * arg.alpha,
                  force_filter,
                  tile_transform);
  }

  edge_color = edge_color + ((background_tile_color - edge_color) * backlight_intensity);
  nux::Color edge_tile_colorify = background_tile_colorify;

  if (arg.colorify_background && !arg.keyboard_nav_hl)
  {
    // Mix edge_tile_colorify with plain white (1.0f).
    // Would be nicer to tweak value from HSV colorspace, instead.
    float mix_factor = (arg.system_item) ? 0.2f : 0.16f;

    edge_tile_colorify.red =   edge_tile_colorify.red   * (1.0f - mix_factor) + 1.0f * mix_factor;
    edge_tile_colorify.green = edge_tile_colorify.green * (1.0f - mix_factor) + 1.0f * mix_factor;
    edge_tile_colorify.blue =  edge_tile_colorify.blue  * (1.0f - mix_factor) + 1.0f * mix_factor;
  }

  RenderElement(GfxContext,
                arg,
                edge->GetDeviceTexture(),
                edge_color,
                edge_tile_colorify,
                arg.alpha * arg.alpha, // Dim edges of semi-transparent tiles
                force_filter,
                tile_transform);
  // end tile draw

  // draw icon
  RenderElement(GfxContext,
                arg,
                arg.icon->TextureForSize(image_size)->GetDeviceTexture(),
                nux::color::White,
                colorify,
                arg.alpha,
                false,
                arg.icon->GetTransform(ui::IconTextureSource::TRANSFORM_IMAGE, monitor));

  // draw overlay shine
  RenderElement(GfxContext,
                arg,
                shine->GetDeviceTexture(),
                nux::color::White,
                colorify,
                arg.alpha,
                force_filter,
                tile_transform);

  // draw glow
  if (glow_intensity > 0)
  {
    RenderElement(GfxContext,
                  arg,
                  glow->GetDeviceTexture(),
                  glow_color,
                  nux::color::White,
                  glow_intensity * arg.alpha,
                  force_filter,
                  arg.icon->GetTransform(ui::IconTextureSource::TRANSFORM_GLOW, monitor));
  }

  // draw shimmer
  if (arg.shimmer_progress > 0 && arg.shimmer_progress < 1.0f)
  {
    int x1 = owner_geo.x + owner_geo.width;
    int x2 = owner_geo.x + owner_geo.width;
    float shimmer_constant = 1.9f;

    x1 -= geo.width * arg.shimmer_progress * shimmer_constant;
    GfxContext.PushClippingRectangle(nux::Geometry(x1, geo.y, x2 - x1, geo.height));

    float fade_out = 1.0f - CLAMP(((x2 - x1) - geo.width) / (geo.width * (shimmer_constant - 1.0f)), 0.0f, 1.0f);

    RenderElement(GfxContext,
                  arg,
                  local::icon_glow[size]->GetDeviceTexture(),
                  arg.icon->GlowColor(),
                  nux::color::White,
                  fade_out * arg.alpha,
                  force_filter,
                  arg.icon->GetTransform(ui::IconTextureSource::TRANSFORM_GLOW, monitor));

    GfxContext.PopClippingRectangle();
  }

  // draw progress bar
  if (arg.progress_bias > -1.0f && arg.progress_bias < 1.0f)
  {
    if (local::offscreen_progress_texture->GetWidth() != icon_size ||
        local::offscreen_progress_texture->GetHeight() != icon_size)
    {
      local::offscreen_progress_texture = nux::GetGraphicsDisplay()->GetGpuDevice()
        ->CreateSystemCapableDeviceTexture(icon_size, icon_size, 1, nux::BITFMT_R8G8B8A8);
    }
    RenderProgressToTexture(GfxContext, local::offscreen_progress_texture, arg.progress, arg.progress_bias);

    RenderElement(GfxContext,
                  arg,
                  local::offscreen_progress_texture,
                  nux::color::White,
                  nux::color::White,
                  arg.alpha,
                  force_filter,
                  tile_transform);
  }

  if (arg.icon->Emblem())
  {
    RenderElement(GfxContext,
                  arg,
                  arg.icon->Emblem()->GetDeviceTexture(),
                  nux::color::White,
                  nux::color::White,
                  arg.alpha,
                  force_filter,
                  arg.icon->GetTransform(ui::IconTextureSource::TRANSFORM_EMBLEM, monitor));
  }

  // draw indicators
  RenderIndicators(GfxContext,
                   arg,
                   arg.running_arrow ? arg.window_indicators : 0,
                   arg.active_arrow ? 1 : 0,
                   arg.alpha,
                   geo);

  // draw superkey-shortcut label
  if (arg.draw_shortcut && arg.shortcut_label)
  {
    char shortcut = (char) arg.shortcut_label;

    if (local::label_map.find(shortcut) == local::label_map.end())
      local::label_map[shortcut] = RenderCharToTexture(shortcut, icon_size, icon_size);

    RenderElement(GfxContext,
                  arg,
                  local::label_map[shortcut]->GetDeviceTexture(),
                  nux::Color(0xFFFFFFFF),
                  nux::color::White,
                  arg.alpha,
                  false,
                  tile_transform);
  }
}

nux::BaseTexture* IconRenderer::RenderCharToTexture(const char label, int width, int height)
{
  nux::BaseTexture*     texture  = NULL;
  nux::CairoGraphics*   cg       = new nux::CairoGraphics(CAIRO_FORMAT_ARGB32,
                                                          width,
                                                          height);
  cairo_t*              cr       = cg->GetContext();
  PangoLayout*          layout   = NULL;
  PangoFontDescription* desc     = NULL;
  GtkSettings*          settings = gtk_settings_get_default();  // not ref'ed
  gchar*                fontName = NULL;

  double label_pos = double(icon_size / 3.0f);
  double text_size = double(icon_size / 4.0f);
  double label_x = label_pos;
  double label_y = label_pos;
  double label_w = label_pos;
  double label_h = label_pos;
  double label_r = 3.0f;

  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);
  cairo_scale(cr, 1.0f, 1.0f);
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  cg->DrawRoundedRectangle(cr, 1.0f, label_x, label_y, label_r, label_w, label_h);
  cairo_set_source_rgba(cr, 0.0f, 0.0f, 0.0f, 0.65f);
  cairo_fill(cr);

  layout = pango_cairo_create_layout(cr);
  g_object_get(settings, "gtk-font-name", &fontName, NULL);
  desc = pango_font_description_from_string(fontName);
  pango_font_description_set_absolute_size(desc, text_size * PANGO_SCALE);
  pango_layout_set_font_description(layout, desc);
  pango_layout_set_text(layout, &label, 1);

  PangoRectangle logRect;
  PangoRectangle inkRect;
  pango_layout_get_extents(layout, &inkRect, &logRect);

  // position and paint text
  cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 1.0f);
  double x = label_x - ((logRect.width / PANGO_SCALE) - label_w) / 2.0f;
  double y = label_y - ((logRect.height / PANGO_SCALE) - label_h) / 2.0f - 1;
  cairo_move_to(cr, x, y);
  pango_cairo_show_layout(cr, layout);

  nux::NBitmapData* bitmap = cg->GetBitmap();
  texture = nux::GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableTexture();
  texture->Update(bitmap);
  delete bitmap;
  delete cg;
  g_object_unref(layout);
  pango_font_description_free(desc);
  g_free(fontName);

  return texture;
}

void IconRenderer::RenderElement(nux::GraphicsEngine& GfxContext,
                                 RenderArg const& arg,
                                 nux::ObjectPtr<nux::IOpenGLBaseTexture> icon,
                                 nux::Color bkg_color,
                                 nux::Color colorify,
                                 float alpha,
                                 bool force_filter,
                                 std::vector<nux::Vector4>& xform_coords)
{
  if (icon.IsNull())
    return;

  if (nux::Abs(arg.x_rotation) < 0.01f &&
      nux::Abs(arg.y_rotation) < 0.01f &&
      nux::Abs(arg.z_rotation) < 0.01f &&
      !force_filter)
    icon->SetFiltering(GL_NEAREST, GL_NEAREST);
  else
    icon->SetFiltering(GL_LINEAR, GL_LINEAR);

  nux::Vector4 const& v0 = xform_coords[0];
  nux::Vector4 const& v1 = xform_coords[1];
  nux::Vector4 const& v2 = xform_coords[2];
  nux::Vector4 const& v3 = xform_coords[3];

  float s0, t0, s1, t1, s2, t2, s3, t3;

  if (icon->GetResourceType() == nux::RTTEXTURERECTANGLE)
  {
    s0 = 0.0f;
    t0 = 0.0f;
    s1 = 0.0f;
    t1 = icon->GetHeight();
    s2 = icon->GetWidth();
    t2 = t1;
    s3 = s2;
    t3 = 0.0f;
  }
  else
  {
    s0 = 0.0f;
    t0 = 0.0f;
    s1 = 0.0f;
    t1 = 1.0f;
    s2 = 1.0f;
    t2 = 1.0f;
    s3 = 1.0f;
    t3 = 0.0f;
  }

  float VtxBuffer[] =
  {
    // Perspective correct
    v0.x, v0.y, 0.0f, 1.0f,     s0 / v0.w, t0 / v0.w, 0.0f, 1.0f / v0.w,
    v1.x, v1.y, 0.0f, 1.0f,     s1 / v1.w, t1 / v1.w, 0.0f, 1.0f / v1.w,
#ifdef USE_GLES
    v3.x, v3.y, 0.0f, 1.0f,     s3 / v3.w, t3 / v3.w, 0.0f, 1.0f / v3.w,
    v2.x, v2.y, 0.0f, 1.0f,     s2 / v2.w, t2 / v2.w, 0.0f, 1.0f / v2.w,
#else
    v2.x, v2.y, 0.0f, 1.0f,     s2 / v2.w, t2 / v2.w, 0.0f, 1.0f / v2.w,
    v3.x, v3.y, 0.0f, 1.0f,     s3 / v3.w, t3 / v3.w, 0.0f, 1.0f / v3.w,
#endif
  };

  CHECKGL(glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0));
  CHECKGL(glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0));

  int VertexLocation = -1;
  int TextureCoord0Location = -1;
  int FragmentColor = 0;
  int ColorifyColor = 0;
  int DesatFactor = 0;

  if (nux::GetWindowThread()->GetGraphicsEngine().UsingGLSLCodePath())
  {
    local::shader_program_uv_persp_correction->Begin();

    int TextureObjectLocation   = local::shader_program_uv_persp_correction->GetUniformLocationARB("TextureObject0");
    VertexLocation          = local::shader_program_uv_persp_correction->GetAttributeLocation("iVertex");
    TextureCoord0Location   = local::shader_program_uv_persp_correction->GetAttributeLocation("iTexCoord0");
    FragmentColor           = local::shader_program_uv_persp_correction->GetUniformLocationARB("color0");
    ColorifyColor           = local::shader_program_uv_persp_correction->GetUniformLocationARB("colorify_color");
    DesatFactor             = local::shader_program_uv_persp_correction->GetUniformLocationARB("desat_factor");

    if (TextureObjectLocation != -1)
      CHECKGL(glUniform1iARB(TextureObjectLocation, 0));

    int VPMatrixLocation = local::shader_program_uv_persp_correction->GetUniformLocationARB("ViewProjectionMatrix");
    if (VPMatrixLocation != -1)
    {
      local::shader_program_uv_persp_correction->SetUniformLocMatrix4fv((GLint)VPMatrixLocation, 1, false, (GLfloat*) & (_stored_projection_matrix.m));
    }
  }
#ifndef USE_GLES
  else
  {
    local::asm_shader->Begin();

    VertexLocation        = nux::VTXATTRIB_POSITION;
    TextureCoord0Location = nux::VTXATTRIB_TEXCOORD0;

    nux::GetWindowThread()->GetGraphicsEngine().SetTexture(GL_TEXTURE0, icon);

    // Set the model-view matrix
    CHECKGL(glMatrixMode(GL_MODELVIEW));
    CHECKGL(glLoadMatrixf((float*) GfxContext.GetOpenGLModelViewMatrix().m));

    // Set the projection matrix
    CHECKGL(glMatrixMode(GL_PROJECTION));
    CHECKGL(glLoadMatrixf((float*) GfxContext.GetOpenGLProjectionMatrix().m));
  }
#endif

  CHECKGL(glEnableVertexAttribArrayARB(VertexLocation));
  CHECKGL(glVertexAttribPointerARB((GLuint)VertexLocation, 4, GL_FLOAT, GL_FALSE, 32, VtxBuffer));

  if (TextureCoord0Location != -1)
  {
    CHECKGL(glEnableVertexAttribArrayARB(TextureCoord0Location));
    CHECKGL(glVertexAttribPointerARB((GLuint)TextureCoord0Location, 4, GL_FLOAT, GL_FALSE, 32, VtxBuffer + 4));
  }

  nux::Color bg_color = bkg_color * alpha;

  if (nux::GetWindowThread()->GetGraphicsEngine().UsingGLSLCodePath())
  {
    CHECKGL(glUniform4fARB(FragmentColor, bg_color.red, bg_color.green, bg_color.blue, bg_color.alpha));
    CHECKGL(glUniform4fARB(ColorifyColor, colorify.red, colorify.green, colorify.blue, colorify.alpha));
    CHECKGL(glUniform4fARB(DesatFactor, arg.saturation, arg.saturation, arg.saturation, arg.saturation));

    nux::GetWindowThread()->GetGraphicsEngine().SetTexture(GL_TEXTURE0, icon);
#ifdef USE_GLES
    CHECKGL(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
#else
    CHECKGL(glDrawArrays(GL_QUADS, 0, 4));
#endif
  }
#ifndef USE_GLES
  else
  {
    CHECKGL(glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 0, bg_color.red, bg_color.green, bg_color.blue, bg_color.alpha));
    CHECKGL(glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 1, arg.saturation, arg.saturation, arg.saturation, arg.saturation));
    CHECKGL(glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 2, colorify.red, colorify.green, colorify.blue, colorify.alpha));

    nux::GetWindowThread()->GetGraphicsEngine().SetTexture(GL_TEXTURE0, icon);
    CHECKGL(glDrawArrays(GL_QUADS, 0, 4));
  }
#endif

  if (VertexLocation != -1)
    CHECKGL(glDisableVertexAttribArrayARB(VertexLocation));
  if (TextureCoord0Location != -1)
    CHECKGL(glDisableVertexAttribArrayARB(TextureCoord0Location));
//   if(VertexColorLocation != -1)
//     CHECKGL( glDisableVertexAttribArrayARB(VertexColorLocation) );

  if (nux::GetWindowThread()->GetGraphicsEngine().UsingGLSLCodePath())
  {
    local::shader_program_uv_persp_correction->End();
  }
  else
  {
    local::asm_shader->End();
  }
}

void IconRenderer::RenderIndicators(nux::GraphicsEngine& GfxContext,
                                    RenderArg const& arg,
                                    int running,
                                    int active,
                                    float alpha,
                                    nux::Geometry const& geo)
{
  int markerCenter = (int) arg.render_center.y;
  markerCenter -= (int)(arg.x_rotation / (2 * M_PI) * icon_size);


  if (running > 0)
  {
    int scale = 1;

    int markerX;
    if (pip_style == OUTSIDE_TILE)
    {
      markerX = geo.x;
    }
    else
    {
      auto bounds = arg.icon->GetTransform(ui::IconTextureSource::TRANSFORM_TILE, monitor);
      markerX = bounds[0].x + 2;
      scale = 2;
    }

    nux::TexCoordXForm texxform;
    nux::Color color = nux::color::LightGrey;

    if (arg.keyboard_nav_hl && pip_style == OVER_TILE)
      color = nux::color::Gray;

    if (arg.running_colored)
      color = nux::color::SkyBlue;

    color = color * alpha;

    nux::BaseTexture* texture;

    // markers are well outside screen bounds to start
    int markers [3] = {-100, -100, -100};

    if (!arg.running_on_viewport)
    {
      markers[0] = markerCenter;
      texture = local::arrow_empty_ltr;
    }
    else if (running == 1)
    {
      markers[0] = markerCenter;
      texture = local::arrow_ltr;
    }
    else if (running == 2)
    {
      markers[0] = markerCenter - 2 * scale;
      markers[1] = markerCenter + 2 * scale;
      texture = local::pip_ltr;
    }
    else
    {
      markers[0] = markerCenter - 4 * scale;
      markers[1] = markerCenter;
      markers[2] = markerCenter + 4 * scale;
      texture = local::pip_ltr;
    }


    for (int i = 0; i < 3; i++)
    {
      int center = markers[i];
      if (center == -100)
        break;
      
      GfxContext.QRP_1Tex(markerX,
                          center - ((texture->GetHeight() * scale) / 2) - 1,
                          (float) texture->GetWidth() * scale,
                          (float) texture->GetHeight() * scale,
                          texture->GetDeviceTexture(),
                          texxform,
                          color);
    }
  }

  if (active > 0)
  {
    nux::TexCoordXForm texxform;

    nux::Color color = nux::color::LightGrey * alpha;
    GfxContext.QRP_1Tex((geo.x + geo.width) - local::arrow_rtl->GetWidth(),
                        markerCenter - (local::arrow_rtl->GetHeight() / 2) - 1,
                        (float) local::arrow_rtl->GetWidth(),
                        (float) local::arrow_rtl->GetHeight(),
                        local::arrow_rtl->GetDeviceTexture(),
                        texxform,
                        color);
  }
}

void IconRenderer::RenderProgressToTexture(nux::GraphicsEngine& GfxContext,
                                           nux::ObjectPtr<nux::IOpenGLBaseTexture> texture,
                                           float progress_fill,
                                           float bias)
{
  int width = texture->GetWidth();
  int height = texture->GetHeight();

  int progress_width =  icon_size;
  int progress_height = local::progress_bar_trough->GetHeight();

  int fill_width = image_size - (icon_size - image_size);
  int fill_height = local::progress_bar_fill->GetHeight();

  int fill_offset = (progress_width - fill_width) / 2;

  // We need to perform a barn doors effect to acheive the slide in and out

  int left_edge = width / 2 - progress_width / 2;
  int right_edge = width / 2 + progress_width / 2;

  if (bias < 0.0f)
  {
    // pulls the right edge in
    right_edge -= (int)(-bias * (float) progress_width);
  }
  else if (bias > 0.0f)
  {
    // pulls the left edge in
    left_edge += (int)(bias * progress_width);
  }

  int fill_y = (height - fill_height) / 2;
  int progress_y = fill_y + (fill_height - progress_height) / 2;
  int half_size = (right_edge - left_edge) / 2;

  SetOffscreenRenderTarget(texture);

  // FIXME
  glClear(GL_COLOR_BUFFER_BIT);
  nux::TexCoordXForm texxform;

  fill_width *= progress_fill;

  // left door
  GfxContext.PushClippingRectangle(nux::Geometry(left_edge, 0, half_size, height));
  GfxContext.QRP_1Tex(left_edge, progress_y, progress_width, progress_height,
                      local::progress_bar_trough->GetDeviceTexture(), texxform,
                      nux::color::White);
  GfxContext.QRP_1Tex(left_edge + fill_offset, fill_y, fill_width, fill_height,
                      local::progress_bar_fill->GetDeviceTexture(), texxform,
                      nux::color::White);
  GfxContext.PopClippingRectangle();

  // right door
  GfxContext.PushClippingRectangle(nux::Geometry(left_edge + half_size, 0, half_size, height));
  GfxContext.QRP_1Tex(right_edge - progress_width, progress_y,
                      progress_width, progress_height,
                      local::progress_bar_trough->GetDeviceTexture(), texxform,
                      nux::color::White);
  GfxContext.QRP_1Tex(right_edge - progress_width + fill_offset, fill_y,
                      fill_width, fill_height,
                      local::progress_bar_fill->GetDeviceTexture(), texxform,
                      nux::color::White);

  GfxContext.PopClippingRectangle();

  RestoreSystemRenderTarget();
}

void IconRenderer::DestroyTextures()
{
  local::destroy_textures();
}

void IconRenderer::GetInverseScreenPerspectiveMatrix(nux::Matrix4& ViewMatrix, nux::Matrix4& PerspectiveMatrix,
                                                     int ViewportWidth,
                                                     int ViewportHeight,
                                                     float NearClipPlane,
                                                     float FarClipPlane,
                                                     float Fovy)
{
  /*
    Objective:
    Given a perspective matrix defined by (Fovy, AspectRatio, NearPlane, FarPlane), we want to compute
    the ModelView matrix that transform a quad defined by its top-left coord (0, 0) and its
    bottom-right coord (WindowWidth, WindowHeight) into a full screen quad that covers the entire viewport (one to one).
    Any quad that is facing the camera and whose 4 points are on the 4 guiding line of the view frustum (pyramid)
    will always cover the entire viewport one to one (when projected on the screen).
    So all we have to do is to define a quad with x:[-x_cs, x_cs] and y:[-y_cs, y_cs] and find the z distance in eye space (z_cs) so that
    the quad touches the 4 guiding lines of the view frustum.
    We consider a well centered projection view (no skewing, no oblique clipping plane, ...) and derive the following equations:
       x_cs = AspectRatio*y_cs
       y_cs/z_cs = tanf(Fovy/2) ==> z_cs = y_cs*1/tanf(Fovy/2) (this is the absolute value the quad depth value will be -z_cs since we are using OpenGL right hand coord system).

    The quad (in camera space) facing the camera and centered around the camera view axis is defined by the points (-x_cs, y_cs) (top-left)
    and the point (x_cs, -y_cs) (bottom-right). If we move that quad along the camera view axis and place it at a distance z_cs of the camera,
    then its 4 corners are each on the 4 lines of the view frustum.

      (-x_cs, y_cs)          Camera Space
                   ^
         __________|__________
        |          |          |
        |          |          |
        |          |(0, 0)    |
        |----------|----------|->
        |          |          |
        |          |          |
        |__________|__________|
                           (x_cs, -y_cs)

    The full-screen quad (in screen space) is defined by the point (0, 0) (top-left) and (WindowWidth, WindowHeight) (bottom-right).
    We can choose and arbitrary value y_cs and compute the z_cs position in camera space that will produce a quad in camera space that projects into
    the full-screen space.

      (0, 0)            Screen Space
         _____________________->
        |          |          |
        |          |          |
        |          |          |
        |----------|----------|
        |          |          |
        |          |          |
        |__________|__________|
        v                   (WindowWidth, WindowHeight)

    The model view matrix is the succession of transformation that convert the quad (0, 0, WindowWidth, WindowHeight) into
    the quad (-x_cs, y_cs, x_cs, -y_cs)

    Screen Space           Camera Space
         x        ---->    x_ = x*2*x_cs/WindowWidth - x_cs
         y        ---->    y_ = -y*2*y_cs/WindowHeight + y_cs
         z        ---->    z_ = A*z -y_cs*1/tanf(Fovy/2)
       where A is a coefficient that can attenuate the rate of change in depth when the quad moves along the camera axis

    If the following is the projection matrix:

    (a, 0,  0, 0)     a = 1/(AspectRatio*tan(Fovy/2))
    (0, b,  0, 0)     b = 1/tan(Fovy/2)
    (0, 0,  c, d)
    (0, 0, -1, 0)

    and the camera space vertex is (x_cs, y_cs, z_cs, w_cs) projects to the top left corner of the view port on the screen ((-1, 1) in clip space), then
      x_cs*a/(-z_cs) = -1; |        z_cs = x_cs*a           x_cs*a = -y_cs*b  ==> x_cs = y_cs*AspectRatio
                           |  ==>                     ==>
      y_cs*b/(-z_cs) = +1; |        z_cs = -y_cs*b          z_cs = -y_cs*1/tanf(Fovy/2)
  */


  float AspectRatio = (float)ViewportWidth / (float)ViewportHeight;
  float CameraToScreenDistance = -1.0f;
  float y_cs = -CameraToScreenDistance * tanf(0.5f * Fovy/* *M_PI/180.0f*/);
  float x_cs = y_cs * AspectRatio;

  ViewMatrix = nux::Matrix4::TRANSLATE(-x_cs, y_cs, CameraToScreenDistance) *
               nux::Matrix4::SCALE(2.0f * x_cs / ViewportWidth, -2.0f * y_cs / ViewportHeight, -2.0f * 3 * y_cs / ViewportHeight /* or -2.0f * x_cs/ViewportWidth*/);

  PerspectiveMatrix.Perspective(Fovy, AspectRatio, NearClipPlane, FarClipPlane);
}

void
IconRenderer::SetOffscreenRenderTarget(nux::ObjectPtr<nux::IOpenGLBaseTexture> texture)
{
  int width = texture->GetWidth();
  int height = texture->GetHeight();

  nux::GetGraphicsDisplay()->GetGpuDevice()->FormatFrameBufferObject(width, height, nux::BITFMT_R8G8B8A8);
  nux::GetGraphicsDisplay()->GetGpuDevice()->SetColorRenderTargetSurface(0, texture->GetSurfaceLevel(0));
  nux::GetGraphicsDisplay()->GetGpuDevice()->ActivateFrameBuffer();

  nux::GetGraphicsDisplay()->GetGraphicsEngine()->SetContext(0, 0, width, height);
  nux::GetGraphicsDisplay()->GetGraphicsEngine()->SetViewport(0, 0, width, height);
  nux::GetGraphicsDisplay()->GetGraphicsEngine()->Push2DWindow(width, height);
  nux::GetGraphicsDisplay()->GetGraphicsEngine()->EmptyClippingRegion();
}

void
IconRenderer::RestoreSystemRenderTarget()
{
  nux::GetWindowCompositor().RestoreRenderingSurface();
}


// The local namespace is purely for namespacing the file local variables below.
namespace local
{
namespace
{
void setup_shaders()
{
  if (nux::GetWindowThread()->GetGraphicsEngine().UsingGLSLCodePath())
  {
    shader_program_uv_persp_correction = nux::GetGraphicsDisplay()->GetGpuDevice()->CreateShaderProgram();
    shader_program_uv_persp_correction->LoadIShader(gPerspectiveCorrectShader.GetTCharPtr());
    shader_program_uv_persp_correction->Link();
  }
  else
  {
    asm_shader = nux::GetGraphicsDisplay()->GetGpuDevice()->CreateAsmShaderProgram();
    asm_shader->LoadVertexShader(TCHAR_TO_ANSI(*PerspectiveCorrectVtx));

    if ((nux::GetGraphicsDisplay()->GetGpuDevice()->SUPPORT_GL_ARB_TEXTURE_NON_POWER_OF_TWO() == false) &&
        (nux::GetGraphicsDisplay()->GetGpuDevice()->SUPPORT_GL_EXT_TEXTURE_RECTANGLE() ||
         nux::GetGraphicsDisplay()->GetGpuDevice()->SUPPORT_GL_ARB_TEXTURE_RECTANGLE()))
    {
      // No support for non power of two textures but support for rectangle textures
      asm_shader->LoadPixelShader(TCHAR_TO_ANSI(*PerspectiveCorrectTexRectFrg));
    }
    else
    {
      asm_shader->LoadPixelShader(TCHAR_TO_ANSI(*PerspectiveCorrectTexFrg));
    }

    asm_shader->Link();
  }
}


inline nux::BaseTexture* load_texture(const char* filename)
{
  return nux::CreateTexture2DFromFile(filename, -1, true);
}

void generate_textures(std::vector<nux::BaseTexture*>& icons, const char* big_file, const char* small_file)
{
  icons.resize(IconSize::LAST);
  icons[IconSize::BIG] = load_texture(big_file);
  icons[IconSize::SMALL] = load_texture(small_file);
}

void generate_textures()
{
  progress_bar_trough = load_texture(PKGDATADIR"/progress_bar_trough.png");
  progress_bar_fill = load_texture(PKGDATADIR"/progress_bar_fill.png");

  generate_textures(icon_background,
                    PKGDATADIR"/launcher_icon_back_150.png",
                    PKGDATADIR"/launcher_icon_back_54.png");
  generate_textures(icon_selected_background,
                    PKGDATADIR"/launcher_icon_selected_back_150.png",
                    PKGDATADIR"/launcher_icon_back_54.png");
  generate_textures(icon_edge,
                    PKGDATADIR"/launcher_icon_edge_150.png",
                    PKGDATADIR"/launcher_icon_edge_54.png");
  generate_textures(icon_glow,
                    PKGDATADIR"/launcher_icon_glow_200.png",
                    PKGDATADIR"/launcher_icon_glow_62.png");
  generate_textures(icon_shadow,
                    PKGDATADIR"/launcher_icon_shadow_200.png",
                    PKGDATADIR"/launcher_icon_shadow_62.png");
  generate_textures(icon_shine,
                    PKGDATADIR"/launcher_icon_shine_150.png",
                    PKGDATADIR"/launcher_icon_shine_54.png");

  squircle_base = load_texture(PKGDATADIR"/squircle_base_54.png");
  squircle_base_selected = load_texture(PKGDATADIR"/squircle_base_selected_54.png");
  squircle_edge = load_texture(PKGDATADIR"/squircle_edge_54.png");
  squircle_glow = load_texture(PKGDATADIR"/squircle_glow_62.png");
  squircle_shadow = load_texture(PKGDATADIR"/squircle_shadow_62.png");
  squircle_shine = load_texture(PKGDATADIR"/squircle_shine_54.png");

  pip_ltr = load_texture(PKGDATADIR"/launcher_pip_ltr.png");
  arrow_ltr = load_texture(PKGDATADIR"/launcher_arrow_ltr.png");
  arrow_empty_ltr = load_texture(PKGDATADIR"/launcher_arrow_outline_ltr.png");

  pip_rtl = load_texture(PKGDATADIR"/launcher_pip_rtl.png");
  arrow_rtl = load_texture(PKGDATADIR"/launcher_arrow_rtl.png");
  arrow_empty_rtl = load_texture(PKGDATADIR"/launcher_arrow_outline_rtl.png");

  offscreen_progress_texture = nux::GetGraphicsDisplay()->GetGpuDevice()
    ->CreateSystemCapableDeviceTexture(2, 2, 1, nux::BITFMT_R8G8B8A8);

  setup_shaders();
  textures_created = true;
}

void destroy_textures(std::vector<nux::BaseTexture*>& icons)
{
  icons[SMALL]->UnReference();
  icons[BIG]->UnReference();
  icons.clear();
}

void destroy_textures()
{
  if (!textures_created)
    return;

  progress_bar_trough->UnReference();
  progress_bar_fill->UnReference();
  pip_ltr->UnReference();
  pip_rtl->UnReference();
  arrow_ltr->UnReference();
  arrow_rtl->UnReference();
  arrow_empty_ltr->UnReference();
  arrow_empty_rtl->UnReference();

  destroy_textures(icon_background);
  destroy_textures(icon_selected_background);
  destroy_textures(icon_edge);
  destroy_textures(icon_glow);
  destroy_textures(icon_shadow);
  destroy_textures(icon_shine);

  squircle_base->UnReference();
  squircle_base_selected->UnReference();
  squircle_edge->UnReference();
  squircle_glow->UnReference();
  squircle_shadow->UnReference();
  squircle_shine->UnReference();

  for (auto it = label_map.begin(), end = label_map.end(); it != end; ++it)
    it->second->UnReference();
  label_map.clear();

  textures_created = false;
}

} // anon namespace
} // namespace local

} // namespace ui
} // namespace unity
