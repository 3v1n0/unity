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

#include <UnityCore/GLibWrapper.h>
#include <UnityCore/ConnectionManager.h>
#include <NuxGraphics/CairoGraphics.h>
#include "unity-shared/CairoTexture.h"
#include "unity-shared/ThemeSettings.h"
#include "unity-shared/TextureCache.h"
#include "unity-shared/UnitySettings.h"
#include "unity-shared/WindowManager.h"
#include "GraphicsUtils.h"

#include <gtk/gtk.h>

#include <pango/pango.h>
#include <pango/pangocairo.h>

#include "IconRenderer.h"

namespace unity
{
namespace ui
{
namespace
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

const std::string gPerspectiveCorrectShader = TEXT(
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

const std::string PerspectiveCorrectVtx = TEXT(
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

const std::string PerspectiveCorrectTexFrg = TEXT(
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

const std::string PerspectiveCorrectTexRectFrg = TEXT(
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

const float edge_illumination_multiplier = 2.0f;
const float glow_multiplier = 2.3f;
const float fill_offset_ratio = 0.125f;
} // anonymous namespace

// The local namespace is purely for namespacing the file local variables below.
namespace local
{
namespace
{
enum IconSize
{
  SMALL = 0,
  BIG,
  SIZE,
};

const std::array<int, IconSize::SIZE> TILE_SIZES = { 54, 150 };
const std::array<int, IconSize::SIZE> GLOW_SIZES = { 62, 200 };
const std::array<int, IconSize::SIZE> MARKER_SIZES = { 19, 37 };

constexpr double count_scaling(double icon_size, bool switcher) { return icon_size / (TILE_SIZES[local::IconSize::SMALL] * (switcher ? 2.0 : 1.0)); }
} // anonymous namespace
} // local namespace

struct IconRenderer::TexturesPool
{
  static std::shared_ptr<TexturesPool> Get()
  {
    static std::shared_ptr<TexturesPool> instance(new TexturesPool());
    return instance;
  }

  nux::ObjectPtr<nux::IOpenGLBaseTexture> offscreen_progress_texture;
  nux::ObjectPtr<nux::IOpenGLShaderProgram> shader_program_uv_persp_correction;
#ifndef USE_GLES
  nux::ObjectPtr<nux::IOpenGLAsmShaderProgram> asm_shader;
#endif

  int VertexLocation;
  int VPMatrixLocation;
  int TextureCoord0Location;
  int FragmentColor;
  int ColorifyColor;
  int DesatFactor;

private:
  TexturesPool();

  inline void LoadTexture(BaseTexturePtr &texture_ptr, std::string const& filename)
  {
    texture_ptr.Adopt(nux::CreateTexture2DFromFile(filename.c_str(), -1, true));
  }

  inline void GenerateTextures(BaseTexturePtr (&texture)[local::IconSize::SIZE],
                               std::string const& big_file, std::string const& small_file)
  {
    LoadTexture(texture[local::IconSize::SMALL], small_file);
    LoadTexture(texture[local::IconSize::BIG], big_file);
  }

  void SetupShaders();
};

struct IconRenderer::LocalTextures
{
  LocalTextures(IconRenderer* parent)
    : parent_(parent)
    , textures_loaded_(false)
  {
    connections_.Add(TextureCache::GetDefault().themed_invalidated.connect([this] {
      if (textures_loaded_)
        ReloadIconSizedTextures(parent_->icon_size, parent_->image_size);
    }));

    auto clear_labels = sigc::hide(sigc::mem_fun(this, &LocalTextures::ClearLabels));
    connections_.Add(Settings::Instance().font_scaling.changed.connect(clear_labels));
    connections_.Add(WindowManager::Default().average_color.changed.connect(clear_labels));
  }

  void ReloadIconSizedTextures(int icon_size, int image_size)
  {
    using namespace local;
    IconSize tex_size = icon_size > 100 ? IconSize::BIG : IconSize::SMALL;
    auto const& tile_sufix = std::to_string(TILE_SIZES[tex_size]);

    int icon_glow_size = std::round(icon_size * (GLOW_SIZES[tex_size] / static_cast<double>(TILE_SIZES[tex_size])));
    auto const& glow_sufix = std::to_string(GLOW_SIZES[tex_size]);

    int marker_size = std::round(icon_size * (MARKER_SIZES[tex_size] / static_cast<double>(TILE_SIZES[tex_size])));
    auto const& marker_sufix = std::to_string(MARKER_SIZES[tex_size]);

    struct TextureData { BaseTexturePtr* tex_ptr; std::string name; int size; };
    std::vector<TextureData> texture_files = {
      {&icon_background, "launcher_icon_back_"+tile_sufix, icon_size},
      {&icon_selected_background, "launcher_icon_selected_back_"+tile_sufix, icon_size},
      {&icon_edge, "launcher_icon_edge_"+tile_sufix, icon_size},
      {&icon_glow, "launcher_icon_glow_"+glow_sufix, icon_glow_size},
      {&icon_shadow, "launcher_icon_shadow_"+glow_sufix, icon_glow_size},
      {&icon_shine, "launcher_icon_shine_"+tile_sufix, icon_size},
      {&arrow_ltr, "launcher_arrow_ltr_"+marker_sufix, marker_size},
      {&arrow_rtl, "launcher_arrow_rtl_"+marker_sufix, marker_size},
      {&arrow_btt, "launcher_arrow_btt_"+marker_sufix, marker_size},
      {&arrow_ttb, "launcher_arrow_ttb_"+marker_sufix, marker_size},
      {&arrow_empty_ltr, "launcher_arrow_outline_ltr_"+marker_sufix, marker_size},
      {&arrow_empty_btt, "launcher_arrow_outline_btt_"+marker_sufix, marker_size},
      {&pip_ltr, "launcher_pip_ltr_"+marker_sufix, marker_size},
      {&pip_btt, "launcher_pip_btt_"+marker_sufix, marker_size},
      {&progress_bar_trough, "progress_bar_trough", icon_size},
      {&progress_bar_fill, "progress_bar_fill", image_size - (icon_size - image_size)},
    };

    auto& cache = TextureCache::GetDefault();
    for (auto const& tex_data : texture_files)
      *tex_data.tex_ptr = cache.FindTexture(tex_data.name, tex_data.size, tex_data.size);

    textures_loaded_ = true;
  }

  nux::BaseTexture* RenderLabelTexture(char label, int icon_size, nux::Color const&);

  BaseTexturePtr const& GetLabelTexture(char label, int icon_size, nux::Color const& color)
  {
    labels_.push_back(TextureCache::GetDefault().FindTexture(std::string(1, label), icon_size, icon_size, [this, &color] (std::string const& label, int size, int) {
      return RenderLabelTexture(label[0], size, color);
    }));

    return labels_.back();
  }

  void ClearLabels()
  {
    labels_.clear();
  }

  BaseTexturePtr icon_background;
  BaseTexturePtr icon_selected_background;
  BaseTexturePtr icon_edge;
  BaseTexturePtr icon_glow;
  BaseTexturePtr icon_shadow;
  BaseTexturePtr icon_shine;
  BaseTexturePtr arrow_ltr;
  BaseTexturePtr arrow_rtl;
  BaseTexturePtr arrow_btt;
  BaseTexturePtr arrow_ttb;
  BaseTexturePtr arrow_empty_ltr;
  BaseTexturePtr arrow_empty_btt;
  BaseTexturePtr pip_ltr;
  BaseTexturePtr pip_btt;
  BaseTexturePtr progress_bar_trough;
  BaseTexturePtr progress_bar_fill;

private:
  IconRenderer* parent_;
  bool textures_loaded_;
  std::vector<BaseTexturePtr> labels_;
  connection::Manager connections_;
};

IconRenderer::IconRenderer()
  : icon_size(0)
  , image_size(0)
  , spacing(0)
  , textures_(TexturesPool::Get())
  , local_textures_(std::make_shared<LocalTextures>(this))
{
  pip_style = OUTSIDE_TILE;
}

void IconRenderer::SetTargetSize(int tile_size, int image_size_, int spacing_)
{
  if (icon_size != tile_size || image_size != image_size_)
  {
    icon_size = tile_size;
    image_size = image_size_;
    local_textures_->ReloadIconSizedTextures(icon_size, image_size);
    local_textures_->ClearLabels();
  }

  spacing = spacing_;
}

void IconRenderer::PreprocessIcons(std::list<RenderArg>& args, nux::Geometry const& geo)
{
  nux::Matrix4 ObjectMatrix;
  nux::Matrix4 ViewMatrix;
  nux::Matrix4 ProjectionMatrix;
  nux::Matrix4 ViewProjectionMatrix;

  stored_projection_matrix_ = nux::GetWindowThread()->GetGraphicsEngine().GetOpenGLModelViewProjectionMatrix();

  GetInverseScreenPerspectiveMatrix(ViewMatrix, ProjectionMatrix, geo.width, geo.height, 0.1f, 1000.0f, DEGTORAD(90));

  nux::Matrix4 const& PremultMatrix = ProjectionMatrix * ViewMatrix;
  int monitor = this->monitor();

  std::list<RenderArg>::iterator it;
  int i;
  for (it = args.begin(), i = 0; it != args.end(); ++it, ++i)
  {
    IconTextureSource* launcher_icon = it->icon;

    if (it->render_center == launcher_icon->LastRenderCenter(monitor) &&
        it->logical_center == launcher_icon->LastLogicalCenter(monitor) &&
        it->rotation == launcher_icon->LastRotation(monitor) &&
        it->skip == launcher_icon->WasSkipping(monitor) &&
        launcher_icon->Count() == launcher_icon->LastCount(monitor) &&
        (launcher_icon->Emblem() != nullptr) == launcher_icon->HadEmblem(monitor))
    {
      continue;
    }

    launcher_icon->RememberCenters(monitor, it->render_center, it->logical_center);
    launcher_icon->RememberRotation(monitor, it->rotation);
    launcher_icon->RememberSkip(monitor, it->skip);
    launcher_icon->RememberEmblem(monitor, launcher_icon->Emblem() != nullptr);
    launcher_icon->RememberCount(monitor, launcher_icon->Count());

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
                   nux::Matrix4::ROTATEX(it->rotation.x) *              // rotate the icon
                   nux::Matrix4::ROTATEY(it->rotation.y) *
                   nux::Matrix4::ROTATEZ(it->rotation.z) *
                   nux::Matrix4::TRANSLATE(-x - w / 2.0f, -y - h / 2.0f, -z); // Put the center the icon to (0, 0)

    ViewProjectionMatrix = PremultMatrix * ObjectMatrix;

    UpdateIconTransform(launcher_icon, ViewProjectionMatrix, geo, x, y, w, h, z, ui::IconTextureSource::TRANSFORM_TILE);

    w = image_size;
    h = image_size;
    x = it->render_center.x - icon_size / 2.0f + (icon_size - image_size) / 2.0f;
    y = it->render_center.y - icon_size / 2.0f + (icon_size - image_size) / 2.0f;
    z = it->render_center.z;

    UpdateIconTransform(launcher_icon, ViewProjectionMatrix, geo, x, y, w, h, z, ui::IconTextureSource::TRANSFORM_IMAGE);

    w = local_textures_->icon_glow->GetWidth();
    h = local_textures_->icon_glow->GetHeight();
    x = it->render_center.x - w / 2.0f;
    y = it->render_center.y - h / 2.0f;
    z = it->render_center.z;

    UpdateIconTransform(launcher_icon, ViewProjectionMatrix, geo, x, y, w, h, z, ui::IconTextureSource::TRANSFORM_GLOW);

    if (Settings::Instance().launcher_position() == LauncherPosition::LEFT)
    {
      w = geo.width + 2;
      h = icon_size + spacing;
      if (i == (int) args.size() - 1)
        h += 4;
    }
    else
    {
      h = geo.height + 2;
      w = icon_size + spacing;
      if (i == (int) args.size() - 1)
        w += 4;
    }
    x = it->logical_center.x - w / 2.0f;
    y = it->logical_center.y - h / 2.0f;
    z = it->logical_center.z;

    UpdateIconTransform(launcher_icon, ViewProjectionMatrix, geo, x, y, w, h, z, ui::IconTextureSource::TRANSFORM_HIT_AREA);

    float emb_w, emb_h;
    nux::BaseTexture* emblem = launcher_icon->Emblem();

    if (nux::BaseTexture* count_texture = launcher_icon->CountTexture(local::count_scaling(icon_size, pip_style != OUTSIDE_TILE)))
    {
      emblem = count_texture;
      emb_w = emblem->GetWidth();
      emb_h = emblem->GetHeight();
    }
    else if (emblem)
    {
      emb_w = std::round(emblem->GetWidth() * scale());
      emb_h = std::round(emblem->GetHeight() * scale());
    }

    if (emblem)
    {
      float w = icon_size;
      float h = icon_size;

      x = it->render_center.x + (icon_size * 0.50f - emb_w - icon_size * 0.05f); // puts right edge of emblem just over the edge of the launcher icon
      y = it->render_center.y - icon_size * 0.50f;     // y = top left corner position of emblem
      z = it->render_center.z;

      ObjectMatrix = nux::Matrix4::TRANSLATE(geo.width / 2.0f, geo.height / 2.0f, z) * // Translate the icon to the center of the viewport
                     nux::Matrix4::ROTATEX(it->rotation.x) *              // rotate the icon
                     nux::Matrix4::ROTATEY(it->rotation.y) *
                     nux::Matrix4::ROTATEZ(it->rotation.z) *
                     nux::Matrix4::TRANSLATE(-(it->render_center.x - w / 2.0f) - w / 2.0f, -(it->render_center.y - h / 2.0f) - h / 2.0f, -z); // Put the center the icon to (0, 0)

      ViewProjectionMatrix = PremultMatrix * ObjectMatrix;

      UpdateIconSectionTransform(launcher_icon, ViewProjectionMatrix, geo, x, y, emb_w, emb_h, z,
                                 it->render_center.x - w / 2.0f, it->render_center.y - h / 2.0f, w, h, ui::IconTextureSource::TRANSFORM_EMBLEM);
    }
  }
}

void IconRenderer::UpdateIconTransform(ui::IconTextureSource* icon, nux::Matrix4 const& ViewProjectionMatrix, nux::Geometry const& geo,
                                       float x, float y, float w, float h, float z, ui::IconTextureSource::TransformIndex index)
{
  UpdateIconSectionTransform(icon, ViewProjectionMatrix, geo, x, y, w, h, z, x, y, w, h, index);
}

void IconRenderer::UpdateIconSectionTransform(ui::IconTextureSource* icon, nux::Matrix4 const& ViewProjectionMatrix, nux::Geometry const& geo,
                                              float x, float y, float w, float h, float z, float xx, float yy, float ww, float hh, ui::IconTextureSource::TransformIndex index)
{
  nux::Vector4 v0(x,     y,     z, 1.0f);
  nux::Vector4 v1(x,     y + h, z, 1.0f);
  nux::Vector4 v2(x + w, y + h, z, 1.0f);
  nux::Vector4 v3(x + w, y,     z, 1.0f);

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
  if (arg.skip)
    return;

  // This check avoids a crash when the icon is not available on the system.
  auto* texture_for_size = arg.icon->TextureForSize(image_size);

  if (!texture_for_size)
    return;

  GfxContext.GetRenderStates().SetBlend(true);
  GfxContext.GetRenderStates().SetPremultipliedBlend(nux::SRC_OVER);
  GfxContext.GetRenderStates().SetColorMask(true, true, true, true);

  nux::Color background_tile_color = arg.icon->BackgroundColor();
  nux::Color glow_color = arg.icon->GlowColor();
  nux::Color edge_color = nux::color::White;
  nux::Color colorify = arg.colorify;
  nux::Color background_tile_colorify = arg.colorify;
  nux::Color edge_tile_colorify = nux::color::White;
  bool colorify_background = arg.colorify_background;
  float backlight_intensity = arg.backlight_intensity;
  float glow_intensity = arg.glow_intensity * glow_multiplier;
  float shadow_intensity = 0.6f;

  BaseTexturePtr background = local_textures_->icon_background;
  BaseTexturePtr const& edge = local_textures_->icon_edge;
  BaseTexturePtr const& glow = local_textures_->icon_glow;
  BaseTexturePtr const& shine = local_textures_->icon_shine;
  BaseTexturePtr const& shadow = local_textures_->icon_shadow;

  nux::Color shortcut_color = arg.colorify;

  bool force_filter = icon_size != background->GetWidth();

  if (backlight_intensity == 0.0f)
  {
    // Colorize tiles without backlight with the launcher background color
    colorify_background = true;
  }

  if (colorify_background)
  {
    // Extra tweaks for tiles that are colorized with the launcher background color
    background_tile_color = nux::color::White;
  }

  if (backlight_intensity > 0 && arg.draw_edge_only)
  {
    float edge_glow = backlight_intensity * edge_illumination_multiplier;
    if (edge_glow > glow_intensity)
    {
      glow_intensity = edge_glow;
    }
  }

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

    background = local_textures_->icon_selected_background;
  }
  else
  {
    colorify.red +=   (0.5f + 0.5f * arg.saturation) * (1.0f - colorify.red);
    colorify.blue +=  (0.5f + 0.5f * arg.saturation) * (1.0f - colorify.blue);
    colorify.green += (0.5f + 0.5f * arg.saturation) * (1.0f - colorify.green);

    if (colorify_background)
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
    glow_intensity = (arg.keyboard_nav_hl) ? glow_intensity : 0.0f ;
  }

  // draw shadow
  if (shadow_intensity > 0)
  {
    nux::Color shadow_color = background_tile_colorify * 0.3f;

    RenderElement(GfxContext,
                  arg,
                  shadow->GetDeviceTexture(),
                  nux::color::White,
                  shadow_color,
                  shadow_intensity * arg.alpha,
                  force_filter,
                  arg.icon->GetTransform(ui::IconTextureSource::TRANSFORM_GLOW, monitor));
  }

  auto const& tile_transform = arg.icon->GetTransform(ui::IconTextureSource::TRANSFORM_TILE, monitor);

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

    edge_color = edge_color + ((background_tile_color - edge_color) * backlight_intensity);
  }

  if (colorify_background && !arg.keyboard_nav_hl)
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
                texture_for_size->GetDeviceTexture(),
                nux::color::White,
                colorify,
                arg.alpha,
                force_filter,
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
                  local_textures_->icon_glow->GetDeviceTexture(),
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
    if (textures_->offscreen_progress_texture->GetWidth() != icon_size ||
        textures_->offscreen_progress_texture->GetHeight() != icon_size)
    {
      textures_->offscreen_progress_texture = nux::GetGraphicsDisplay()->GetGpuDevice()
        ->CreateSystemCapableDeviceTexture(icon_size, icon_size, 1, nux::BITFMT_R8G8B8A8);
    }
    RenderProgressToTexture(GfxContext, textures_->offscreen_progress_texture, arg.progress, arg.progress_bias);

    RenderElement(GfxContext,
                  arg,
                  textures_->offscreen_progress_texture,
                  nux::color::White,
                  nux::color::White,
                  arg.alpha,
                  force_filter,
                  tile_transform);
  }

  if (nux::BaseTexture* count_texture = arg.icon->CountTexture(local::count_scaling(icon_size, pip_style != OUTSIDE_TILE)))
  {
    RenderElement(GfxContext,
                  arg,
                  count_texture->GetDeviceTexture(),
                  nux::color::White,
                  nux::color::White,
                  arg.alpha,
                  force_filter,
                  arg.icon->GetTransform(ui::IconTextureSource::TRANSFORM_EMBLEM, monitor));
  }
  else if (arg.icon->Emblem())
  {
    RenderElement(GfxContext,
                  arg,
                  arg.icon->Emblem()->GetDeviceTexture(),
                  nux::color::White,
                  nux::color::White,
                  arg.alpha,
                  force_filter || scale != 1.0,
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
    char shortcut = static_cast<char>(arg.shortcut_label);
    auto const& label = local_textures_->GetLabelTexture(shortcut, icon_size, shortcut_color);

    RenderElement(GfxContext,
                  arg,
                  label->GetDeviceTexture(),
                  nux::Color(0xFFFFFFFF),
                  nux::color::White,
                  arg.alpha,
                  false,
                  tile_transform);
  }
}

nux::BaseTexture* IconRenderer::LocalTextures::RenderLabelTexture(char label, int icon_size, nux::Color const& bg_color)
{
  nux::CairoGraphics cg(CAIRO_FORMAT_ARGB32, icon_size, icon_size);
  cairo_t* cr = cg.GetInternalContext();
  glib::String font_name;

  const double label_ratio = 0.44f;
  const double label_size = icon_size * label_ratio;
  const double label_x = (icon_size - label_size) / 2.0f;
  const double label_y = (icon_size - label_size) / 2.0f;
  const double label_w = label_size;
  const double label_h = label_size;
  const double label_radius = 3.0f;

  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);
  cairo_scale(cr, 1.0f, 1.0f);
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  cg.DrawRoundedRectangle(cr, 1.0f, label_x, label_y, label_radius, label_w, label_h);
  cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.75f);
  cairo_fill_preserve(cr);
  cairo_set_source_rgba(cr, bg_color.red, bg_color.green, bg_color.blue, 0.20f);
  cairo_fill(cr);

  glib::Object<PangoLayout> layout(pango_cairo_create_layout(cr));
  auto const& font = theme::Settings::Get()->font();
  std::shared_ptr<PangoFontDescription> desc(pango_font_description_from_string(font.c_str()),
                                             pango_font_description_free);
  const double text_ratio = 0.75;
  int text_size = pango_units_from_double(label_size * text_ratio * Settings::Instance().font_scaling());
  pango_font_description_set_absolute_size(desc.get(), text_size);
  pango_layout_set_font_description(layout, desc.get());
  pango_layout_set_text(layout, &label, 1);

  PangoRectangle ink_rect;
  pango_layout_get_pixel_extents(layout, &ink_rect, nullptr);

  // position and paint text
  cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 1.0f);
  double x = label_x - std::round((ink_rect.width - label_w) / 2.0f) - ink_rect.x;
  double y = label_y - std::round((ink_rect.height - label_h) / 2.0f) - ink_rect.y;
  cairo_move_to(cr, x, y);
  pango_cairo_show_layout(cr, layout);

  return texture_from_cairo_graphics(cg);
}

void IconRenderer::RenderElement(nux::GraphicsEngine& GfxContext,
                                 RenderArg const& arg,
                                 nux::ObjectPtr<nux::IOpenGLBaseTexture> const& icon,
                                 nux::Color const& bkg_color,
                                 nux::Color const& colorify,
                                 float alpha,
                                 bool force_filter,
                                 std::vector<nux::Vector4> const& xform_coords)
{
  if (icon.IsNull())
    return;

  if (std::abs(arg.rotation.x) < 0.01f &&
      std::abs(arg.rotation.y) < 0.01f &&
      std::abs(arg.rotation.z) < 0.01f &&
      !force_filter)
  {
    icon->SetFiltering(GL_NEAREST, GL_NEAREST);
  }
  else
  {
    icon->SetFiltering(GL_LINEAR, GL_LINEAR);
  }

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
    textures_->shader_program_uv_persp_correction->Begin();

    VertexLocation = textures_->VertexLocation;
    TextureCoord0Location = textures_->TextureCoord0Location;
    FragmentColor = textures_->FragmentColor;
    ColorifyColor = textures_->ColorifyColor;
    DesatFactor = textures_->DesatFactor;

    if (textures_->VPMatrixLocation != -1)
    {
      textures_->shader_program_uv_persp_correction->SetUniformLocMatrix4fv((GLint)textures_->VPMatrixLocation, 1, false, (GLfloat*) & (stored_projection_matrix_.m));
    }
  }
#ifndef USE_GLES
  else
  {
    textures_->asm_shader->Begin();

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
    textures_->shader_program_uv_persp_correction->End();
  }
  else
  {
#ifndef USE_GLES
    textures_->asm_shader->End();
#endif
  }
}

void IconRenderer::RenderIndicators(nux::GraphicsEngine& GfxContext,
                                    RenderArg const& arg,
                                    int running,
                                    int active,
                                    float alpha,
                                    nux::Geometry const& geo)
{
  int markerCenter = 0;
  bool switcher_mode = (pip_style != OUTSIDE_TILE);
  bool left_markers = (switcher_mode || Settings::Instance().launcher_position() == LauncherPosition::LEFT);

  if (left_markers)
  {
    markerCenter = (int) arg.render_center.y;
    markerCenter -= (int)(arg.rotation.x / (2 * M_PI) * icon_size);
  }
  else
  {
    markerCenter = (int) arg.render_center.x;

    if (pip_style == OUTSIDE_TILE)
      markerCenter += (int)(arg.rotation.y / (2 * M_PI) * icon_size);
  }

  if (running > 0)
  {
    int markerX = 0;
    if (left_markers)
    {
      if (pip_style == OUTSIDE_TILE)
      {
        markerX = geo.x;
      }
      else
      {
        auto const& bounds = arg.icon->GetTransform(ui::IconTextureSource::TRANSFORM_TILE, monitor);
        markerX = bounds[0].x + 1;
      }
    }
    nux::TexCoordXForm texxform;
    nux::Color color = nux::color::LightGrey;

    if (arg.keyboard_nav_hl && pip_style == OVER_TILE)
      color = nux::color::Gray;

    if (arg.running_colored)
      color = nux::color::SkyBlue;

    color = color * alpha;

    BaseTexturePtr texture;

    // markers are well outside screen bounds to start
    int markers [3] = {-100, -100, -100};

    if (!arg.running_on_viewport)
    {
      markers[0] = markerCenter;
      if (left_markers)
        texture = local_textures_->arrow_empty_ltr;
      else
        texture = local_textures_->arrow_empty_btt;
    }
    else if (running == 1)
    {
      markers[0] = markerCenter;
      if (left_markers)
        texture = local_textures_->arrow_ltr;
      else
        texture = local_textures_->arrow_btt;
    }
    else if (running == 2)
    {
      int texture_size = 0;
      if (left_markers)
      {
        texture = local_textures_->pip_ltr;
        texture_size = texture->GetHeight();
      }
      else
      {
        texture = local_textures_->pip_btt;
        texture_size = texture->GetWidth();
      }

      double default_tex_size = local::MARKER_SIZES[local::IconSize::SMALL];
      int offset = std::max(1.0, std::round(2.0 * texture_size / default_tex_size));
      markers[0] = markerCenter - offset;
      markers[1] = markerCenter + offset;
    }
    else
    {
      int texture_size = 0;
      if (left_markers)
      {
        texture = local_textures_->pip_ltr;
        texture_size = texture->GetHeight();
      }
      else
      {
        texture = local_textures_->pip_btt;
        texture_size = texture->GetWidth();
      }

      double default_tex_size = local::MARKER_SIZES[local::IconSize::SMALL];
      int offset = std::max(1.0, std::round(4.0 * texture_size / default_tex_size));
      markers[0] = markerCenter - offset;
      markers[1] = markerCenter;
      markers[2] = markerCenter + offset;
    }

    int markerY = 0;
    if (!left_markers)
    {
      if (pip_style == OUTSIDE_TILE)
      {
        markerY = (geo.y + geo.height) - texture->GetHeight();
      }
      else
      {
        auto const& bounds = arg.icon->GetTransform(ui::IconTextureSource::TRANSFORM_TILE, monitor);

        markerY = (bounds[2].y - (texture->GetHeight()*scale));
      }
    }

    for (int i = 0; i < 3; i++)
    {
      int center = markers[i];
      if (center == -100)
        break;

      if (left_markers)
        markerY = center - std::round(texture->GetHeight() / 2.0f);
      else
        markerX = center - std::round(texture->GetWidth() / 2.0f);

      GfxContext.QRP_1Tex(markerX,
                          markerY,
                          texture->GetWidth(),
                          texture->GetHeight(),
                          texture->GetDeviceTexture(),
                          texxform,
                          color);
    }
  }

  if (active > 0)
  {
    nux::TexCoordXForm texxform;

    nux::Color color = nux::color::LightGrey * alpha;
    if (left_markers)
    {
      auto const& arrow_rtl = local_textures_->arrow_rtl;
      GfxContext.QRP_1Tex((geo.x + geo.width) - arrow_rtl->GetWidth(),
                          markerCenter - std::round(arrow_rtl->GetHeight() / 2.0f),
                          arrow_rtl->GetWidth(),
                          arrow_rtl->GetHeight(),
                          arrow_rtl->GetDeviceTexture(),
                          texxform,
                          color);
    }
    else
    {
      auto const& arrow_ttb = local_textures_->arrow_ttb;
      GfxContext.QRP_1Tex(markerCenter - std::round(arrow_ttb->GetWidth() / 2.0f),
                          geo.y,
                          arrow_ttb->GetWidth(),
                          arrow_ttb->GetHeight(),
                          arrow_ttb->GetDeviceTexture(),
                          texxform,
                          color);
    }
  }
}

void IconRenderer::RenderProgressToTexture(nux::GraphicsEngine& GfxContext,
                                           nux::ObjectPtr<nux::IOpenGLBaseTexture> const& texture,
                                           float progress_fill,
                                           float bias)
{
  int width = texture->GetWidth();
  int height = texture->GetHeight();

  int progress_width = local_textures_->progress_bar_trough->GetHeight();
  int progress_height = local_textures_->progress_bar_trough->GetWidth();

  int fill_width = image_size - (icon_size - image_size);
  int fill_height = local_textures_->progress_bar_fill->GetHeight();

  int fill_offset = static_cast<float>(image_size) * fill_offset_ratio;

  // We need to perform a barn doors effect to acheive the slide in and out

  int left_edge = width / 2 - progress_width / 2;
  int right_edge = width / 2 + progress_width / 2;

  if (bias < 0.0f)
  {
    // pulls the right edge in
    right_edge -= -bias * static_cast<float>(progress_width);
  }
  else if (bias > 0.0f)
  {
    // pulls the left edge in
    left_edge += bias * static_cast<float>(progress_width);
  }

  int fill_y = (height - fill_height) / 2;
  int progress_y = fill_y + (fill_height - progress_height) / 2;
  int half_size = (right_edge - left_edge) / 2;

  unity::graphics::PushOffscreenRenderTarget(texture);

  // FIXME
  glClear(GL_COLOR_BUFFER_BIT);
  nux::TexCoordXForm texxform;

  fill_width *= progress_fill;

  // left door
  GfxContext.PushClippingRectangle(nux::Geometry(left_edge, 0, half_size, height));
  GfxContext.QRP_1Tex(left_edge, progress_y, progress_width, progress_height,
                      local_textures_->progress_bar_trough->GetDeviceTexture(), texxform,
                      nux::color::White);
  GfxContext.QRP_1Tex(left_edge + fill_offset, fill_y, fill_width, fill_height,
                      local_textures_->progress_bar_fill->GetDeviceTexture(), texxform,
                      nux::color::White);
  GfxContext.PopClippingRectangle();

  // right door
  GfxContext.PushClippingRectangle(nux::Geometry(left_edge + half_size, 0, half_size, height));
  GfxContext.QRP_1Tex(right_edge - progress_width, progress_y,
                      progress_width, progress_height,
                      local_textures_->progress_bar_trough->GetDeviceTexture(), texxform,
                      nux::color::White);
  GfxContext.QRP_1Tex(right_edge - progress_width + fill_offset, fill_y,
                      fill_width, fill_height,
                      local_textures_->progress_bar_fill->GetDeviceTexture(), texxform,
                      nux::color::White);

  GfxContext.PopClippingRectangle();

  unity::graphics::PopOffscreenRenderTarget();
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

  if (Settings::Instance().launcher_position() == LauncherPosition::LEFT)
    ViewMatrix = nux::Matrix4::TRANSLATE(-x_cs, y_cs, CameraToScreenDistance) *
                 nux::Matrix4::SCALE(2.0f * x_cs / ViewportWidth, -2.0f * y_cs / ViewportHeight, -2.0f * 3 * y_cs / ViewportHeight /* or -2.0f * x_cs/ViewportWidth*/);
  else
    ViewMatrix = nux::Matrix4::TRANSLATE(-x_cs, y_cs, CameraToScreenDistance) *
                 nux::Matrix4::SCALE(2.0f * x_cs / ViewportWidth, -2.0f * y_cs / ViewportHeight, -2.0f * x_cs / ViewportWidth);

  PerspectiveMatrix.Perspective(Fovy, AspectRatio, NearClipPlane, FarClipPlane);
}

IconRenderer::TexturesPool::TexturesPool()
  : offscreen_progress_texture(nux::GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableDeviceTexture(2, 2, 1, nux::BITFMT_R8G8B8A8))
  , VertexLocation(-1)
  , VPMatrixLocation(0)
  , TextureCoord0Location(-1)
  , FragmentColor(0)
  , ColorifyColor(0)
  , DesatFactor(0)
{
  SetupShaders();
}

void IconRenderer::TexturesPool::SetupShaders()
{
  if (nux::GetWindowThread()->GetGraphicsEngine().UsingGLSLCodePath())
  {
    shader_program_uv_persp_correction = nux::GetGraphicsDisplay()->GetGpuDevice()->CreateShaderProgram();
    shader_program_uv_persp_correction->LoadIShader(gPerspectiveCorrectShader.c_str());
    shader_program_uv_persp_correction->Link();

    shader_program_uv_persp_correction->Begin();

    int TextureObjectLocation = shader_program_uv_persp_correction->GetUniformLocationARB("TextureObject0");
    VertexLocation            = shader_program_uv_persp_correction->GetAttributeLocation("iVertex");
    TextureCoord0Location     = shader_program_uv_persp_correction->GetAttributeLocation("iTexCoord0");
    FragmentColor             = shader_program_uv_persp_correction->GetUniformLocationARB("color0");
    ColorifyColor             = shader_program_uv_persp_correction->GetUniformLocationARB("colorify_color");
    DesatFactor               = shader_program_uv_persp_correction->GetUniformLocationARB("desat_factor");

    if (TextureObjectLocation != -1)
      CHECKGL(glUniform1iARB(TextureObjectLocation, 0));

    VPMatrixLocation = shader_program_uv_persp_correction->GetUniformLocationARB("ViewProjectionMatrix");

    shader_program_uv_persp_correction->End();
  }
  else
  {
#ifndef USE_GLES
    asm_shader = nux::GetGraphicsDisplay()->GetGpuDevice()->CreateAsmShaderProgram();
    asm_shader->LoadVertexShader(TCHAR_TO_ANSI(PerspectiveCorrectVtx.c_str()));

    if ((nux::GetGraphicsDisplay()->GetGpuDevice()->SUPPORT_GL_ARB_TEXTURE_NON_POWER_OF_TWO() == false) &&
        (nux::GetGraphicsDisplay()->GetGpuDevice()->SUPPORT_GL_EXT_TEXTURE_RECTANGLE() ||
         nux::GetGraphicsDisplay()->GetGpuDevice()->SUPPORT_GL_ARB_TEXTURE_RECTANGLE()))
    {
      // No support for non power of two textures but support for rectangle textures
      asm_shader->LoadPixelShader(TCHAR_TO_ANSI(PerspectiveCorrectTexRectFrg.c_str()));
    }
    else
    {
      asm_shader->LoadPixelShader(TCHAR_TO_ANSI(PerspectiveCorrectTexFrg.c_str()));
    }

    asm_shader->Link();
#endif
  }
}

} // namespace ui
} // namespace unity
