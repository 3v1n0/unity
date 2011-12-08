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

#include "SwitcherView.h"
#include "IconRenderer.h"
#include "LayoutSystem.h"

#include "TimeUtil.h"

#include <UnityCore/Variant.h>

#include <NuxCore/Object.h>
#include <Nux/Nux.h>
#include <Nux/WindowCompositor.h>

namespace unity
{
using namespace ui;
using launcher::AbstractLauncherIcon;

namespace switcher
{

NUX_IMPLEMENT_OBJECT_TYPE(SwitcherView);

SwitcherView::SwitcherView(NUX_FILE_LINE_DECL)
  : View(NUX_FILE_LINE_PARAM)
  , target_sizes_set_(false)
  , redraw_handle_(0)
{
  icon_renderer_ = AbstractIconRenderer::Ptr(new IconRenderer());
  icon_renderer_->pip_style = OVER_TILE;

  layout_system_ = LayoutSystem::Ptr (new LayoutSystem ());
  border_size = 50;
  flat_spacing = 10;
  icon_size = 128;
  minimum_spacing = 10;
  tile_size = 150;
  vertical_size = tile_size + 80;
  text_size = 15;
  animation_length = 250;
  spread_size = 3.5f;
  render_boxes = false;

  animation_draw_ = false;

  save_time_.tv_sec = 0;
  save_time_.tv_nsec = 0;

  render_targets_.clear ();

  background_top_ = nux::CreateTexture2DFromFile(PKGDATADIR"/switcher_top.png", -1, true);
  background_left_ = nux::CreateTexture2DFromFile(PKGDATADIR"/switcher_left.png", -1, true);
  background_corner_ = nux::CreateTexture2DFromFile(PKGDATADIR"/switcher_corner.png", -1, true);
  rounding_texture_ = nux::CreateTexture2DFromFile(PKGDATADIR"/switcher_round_rect.png", -1, true);

  text_view_ = new nux::StaticCairoText("Testing");
  text_view_->SinkReference();
  text_view_->SetMaximumWidth ((int) (tile_size * spread_size));
  text_view_->SetLines(1);
  text_view_->SetTextColor(nux::color::White);
  text_view_->SetFont("Ubuntu Bold 10");

  icon_size.changed.connect (sigc::mem_fun (this, &SwitcherView::OnIconSizeChanged));
  tile_size.changed.connect (sigc::mem_fun (this, &SwitcherView::OnTileSizeChanged));

  bg_effect_helper_.owner = this;
}

SwitcherView::~SwitcherView()
{
  background_top_->UnReference();
  background_left_->UnReference();
  background_corner_->UnReference();
  rounding_texture_->UnReference();
  text_view_->UnReference();
  if (redraw_handle_ > 0)
    g_source_remove(redraw_handle_);
}

std::string SwitcherView::GetName() const
{
  return "SwitcherView";
}

void SwitcherView::AddProperties(GVariantBuilder* builder)
{
  unity::variant::BuilderWrapper(builder)
  .add("render-boxes", render_boxes)
  .add("border-size", border_size)
  .add("flat-spacing", flat_spacing)
  .add("icon-size", icon_size)
  .add("minimum-spacing", minimum_spacing)
  .add("tile-size", tile_size)
  .add("vertical-size", vertical_size)
  .add("text-size", text_size)
  .add("animation-length", animation_length)
  .add("spread-size", (float)spread_size);
}

void
SwitcherView::SetupBackground()
{
  bg_effect_helper_.enabled = true;
}

LayoutWindowList SwitcherView::ExternalTargets ()
{
  LayoutWindowList result = render_targets_;
  return result;
}

void SwitcherView::SetModel(SwitcherModel::Ptr model)
{
  model_ = model;
  model->selection_changed.connect(sigc::mem_fun(this, &SwitcherView::OnSelectionChanged));
  model->detail_selection.changed.connect (sigc::mem_fun (this, &SwitcherView::OnDetailSelectionChanged));
  model->detail_selection_index.changed.connect (sigc::mem_fun (this, &SwitcherView::OnDetailSelectionIndexChanged));

  if (model->Selection())
    text_view_->SetText(model->Selection()->tooltip_text().c_str());
}

void SwitcherView::OnIconSizeChanged (int size)
{
  icon_renderer_->SetTargetSize(tile_size, icon_size, 10);
}

void SwitcherView::OnTileSizeChanged (int size)
{
  icon_renderer_->SetTargetSize(tile_size, icon_size, 10);
  vertical_size = tile_size + 80;
}

void SwitcherView::SaveLast ()
{
  saved_args_ = last_args_;
  saved_background_ = last_background_;
  clock_gettime(CLOCK_MONOTONIC, &save_time_);
}

void SwitcherView::OnDetailSelectionIndexChanged (unsigned int index)
{
  if (model_->detail_selection)
  {
    Window detail_window = model_->DetailSelectionWindow();
    text_view_->SetText(model_->Selection()->NameForWindow (detail_window));
  }
  QueueDraw ();
}

void SwitcherView::OnDetailSelectionChanged (bool detail)
{

  if (detail)
  {
    Window detail_window = model_->DetailSelectionWindow();
    text_view_->SetText(model_->Selection()->NameForWindow (detail_window));
  }
  else
  {
    text_view_->SetText(model_->Selection()->tooltip_text().c_str());
  }
  SaveLast ();
  QueueDraw ();
}

void SwitcherView::OnSelectionChanged(AbstractLauncherIcon* selection)
{
  if (selection)
    text_view_->SetText(selection->tooltip_text().c_str());
  SaveLast ();
  QueueDraw();
}

SwitcherModel::Ptr SwitcherView::GetModel()
{
  return model_;
}

void SwitcherView::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  return;
}

RenderArg SwitcherView::CreateBaseArgForIcon(AbstractLauncherIcon* icon)
{
  RenderArg arg;
  arg.icon = icon;
  arg.alpha = 0.95f;

  // tells the renderer to render arrows by number
  arg.running_on_viewport = true;

  arg.window_indicators = icon->RelatedWindows();
  if (arg.window_indicators > 1)
    arg.running_arrow = true;
  else
    arg.window_indicators = 0;

  if (icon == model_->Selection())
  {
    arg.keyboard_nav_hl = true;
    arg.backlight_intensity = 1.0f;
  }
  else
  {
    arg.backlight_intensity = 0.7f;
  }

  return arg;
}

RenderArg SwitcherView::InterpolateRenderArgs(RenderArg const& start, RenderArg const& end, float progress)
{
  // easing
  progress = -pow(progress - 1.0f, 2) + 1;
  
  RenderArg result = end;

  result.x_rotation = start.x_rotation + (end.x_rotation - start.x_rotation) * progress;
  result.y_rotation = start.y_rotation + (end.y_rotation - start.y_rotation) * progress;
  result.z_rotation = start.z_rotation + (end.z_rotation - start.z_rotation) * progress;

  result.render_center.x = start.render_center.x + (end.render_center.x - start.render_center.x) * progress;
  result.render_center.y = start.render_center.y + (end.render_center.y - start.render_center.y) * progress;
  result.render_center.z = start.render_center.z + (end.render_center.z - start.render_center.z) * progress;

  result.logical_center = result.render_center;

  return result;
}

nux::Geometry SwitcherView::InterpolateBackground (nux::Geometry const& start, nux::Geometry const& end, float progress)
{
  progress = -pow(progress - 1.0f, 2) + 1;

  nux::Geometry result;

  result.x = start.x + (end.x - start.x) * progress;
  result.y = start.y + (end.y - start.y) * progress;
  result.width = start.width + (end.width - start.width) * progress;
  result.height = start.height + (end.height - start.height) * progress;

  return result;
}

nux::Geometry SwitcherView::UpdateRenderTargets (nux::Point const& center, timespec const& current)
{
  std::vector<Window> xids = model_->DetailXids ();

  int ms_since_change = TimeUtil::TimeDelta(&current, &save_time_);
  float progress = MIN (1.0f, (float) ms_since_change / (float) animation_length());

  for (Window window : xids)
  {
    LayoutWindow::Ptr layout_window (new LayoutWindow (window));

    if (window == model_->DetailSelectionWindow ())
      layout_window->alpha = 1.0f * progress;
    else
      layout_window->alpha = 0.9f * progress;
    
    render_targets_.push_back (layout_window);
  }

  nux::Geometry max_bounds;

  nux::Geometry absolute = GetAbsoluteGeometry ();
  nux::Size spread_size = SpreadSize();
  max_bounds.x = absolute.x + center.x - spread_size.width / 2;
  max_bounds.y = absolute.y + center.y - spread_size.height / 2;
  max_bounds.width = spread_size.width;
  max_bounds.height = spread_size.height;

  nux::Geometry final_bounds;
  layout_system_->LayoutWindows (render_targets_, max_bounds, final_bounds);

  return final_bounds;
}

void SwitcherView::OffsetRenderTargets (int x, int y)
{
  for (LayoutWindow::Ptr target : render_targets_)
  {
    target->result.x += x;
    target->result.y += y;
  }
}

nux::Size SwitcherView::SpreadSize()
{
  nux::Geometry base = GetGeometry();
  nux::Size result (base.width - border_size * 2, base.height - border_size * 2);
  
  int width_padding = std::max(model_->Size() - 1, 0) * minimum_spacing + tile_size;
  int height_padding = text_size;

  result.width -= width_padding;
  result.height -= height_padding;
  
  return result;
}

void SwitcherView::GetFlatIconPositions (int n_flat_icons, 
                                         int size, 
                                         int selection, 
                                         int &first_flat, 
                                         int &last_flat, 
                                         int &half_fold_left, 
                                         int &half_fold_right)
{
  half_fold_left = -1;
  half_fold_right = -1;

  if (n_flat_icons == 0)
  {
    first_flat = selection + 1;
    last_flat = selection;
  }
  else if (n_flat_icons == 1)
  {
    if (selection == 0)
    {
      // selection is first item
      first_flat = 0;
      last_flat = n_flat_icons;
    }
    else if (selection >= size - 2)
    {
      // selection is in ending area where all icons at end should flatten
      first_flat = size - n_flat_icons - 1;
      last_flat = size - 1;
    }
    else
    {
      first_flat = selection;
      last_flat = selection;

      half_fold_left = first_flat - 1;
      half_fold_right = last_flat + 1;
    }
  }
  else
  {
    if (selection == 0)
    {
      // selection is first item
      first_flat = 0;
      last_flat = n_flat_icons;
    }
    else if (selection >= 1 && selection <= n_flat_icons - 1)
    {
      // selection is in "beginning" area before flat section starts moving
      // first item should half fold still
      first_flat = 1;
      last_flat = n_flat_icons;

      half_fold_left = 0;
      half_fold_right = last_flat + 1;
    }
    else if (selection >= size - 2)
    {
      // selection is in ending area where all icons at end should flatten
      first_flat = size - n_flat_icons - 1;
      last_flat = size - 1;
    }
    else
    {
      first_flat = selection - n_flat_icons + 2;
      last_flat = selection + 1;

      half_fold_left = first_flat - 1;
      half_fold_right = last_flat + 1;
    }
  }
}

std::list<RenderArg> SwitcherView::RenderArgsFlat(nux::Geometry& background_geo, int selection, timespec const& current)
{
  std::list<RenderArg> results;
  nux::Geometry base = GetGeometry();

  render_targets_.clear ();

  bool detail_selection = model_->detail_selection;

  background_geo.y = base.y + base.height / 2 - (vertical_size / 2);
  background_geo.height = vertical_size + text_size;  


  if (model_)
  {

    int size = model_->Size();
    int padded_tile_size = tile_size + flat_spacing * 2;
    int max_width = base.width - border_size * 2;

    nux::Geometry spread_bounds;
    int spread_padded_width = 0;
    if (detail_selection)
    {
      spread_bounds = UpdateRenderTargets (nux::Point (0, 0), current);
      // remove extra space consumed by spread
      spread_padded_width = spread_bounds.width + 100;
      max_width -= spread_padded_width - tile_size;

      int expansion = MAX (0, spread_bounds.height - icon_size);
      background_geo.y -= expansion / 2;
      background_geo.height += expansion;
    }

    int flat_width = size * padded_tile_size;

    int n_flat_icons = CLAMP((max_width - size * minimum_spacing) / padded_tile_size - 1, 0, size);

    int overflow = flat_width - max_width;
    float x = 0;
    if (overflow < 0)
    {
      background_geo.x = base.x - overflow / 2;
      background_geo.width = base.width + overflow;

      x -= overflow / 2;
      overflow = 0;
    }
    else
    {
      background_geo.x = base.x;
      background_geo.width = base.width;
    }

    int non_flat_icons = std::max (1.0f, (float)size - n_flat_icons - 1);
    float partial_overflow = (float) overflow / (float)non_flat_icons;
    float partial_overflow_scalar = (float)(padded_tile_size - partial_overflow) / (float)(padded_tile_size);

    int first_flat, last_flat;
    int half_fold_left;
    int half_fold_right;

    GetFlatIconPositions (n_flat_icons, size, selection, first_flat, last_flat, half_fold_left, half_fold_right); 

    SwitcherModel::iterator it;
    int i = 0;
    int y = base.y + base.height / 2;
    x += border_size;
    for (it = model_->begin(); it != model_->end(); ++it)
    {
      RenderArg arg = CreateBaseArgForIcon(*it);

      float scalar = partial_overflow_scalar;

      bool should_flat = false;

      if (i >= first_flat && i <= last_flat)
      {
        scalar = 1.0f;
        should_flat = true;
      }
      else if (i == half_fold_left || i == half_fold_right)
      {
        scalar += (1.0f - scalar) * 0.5f;
      }

      int half_size = tile_size / 2;
      if (i == selection && detail_selection)
      {
        half_size = spread_padded_width / 2;
        scalar = 1.0f;
      }

      x += (half_size + flat_spacing) * scalar;

      if (should_flat)
        arg.render_center = nux::Point3((int) x, y, 0);
      else
        arg.render_center = nux::Point3(x, y, 0);

      x += (half_size + flat_spacing) * scalar;

      arg.y_rotation = (1.0f - MIN (1.0f, scalar));

      if (!should_flat && overflow > 0 && i != selection)
      {
        if (i > last_flat)
        {
          arg.render_center.x -= 20;
        }
        else
        {
          arg.render_center.x += 20;
          arg.y_rotation = -arg.y_rotation;
        }
      }

      arg.render_center.z = abs(80.0f * arg.y_rotation);
      arg.logical_center = arg.render_center;

      if (i == selection && detail_selection)
      {
        arg.skip = true;
        OffsetRenderTargets (arg.render_center.x, arg.render_center.y);
      }

      results.push_back(arg);
      ++i;
    }

    int ms_since_change = TimeUtil::TimeDelta(&current, &save_time_);
    if (saved_args_.size () == results.size () && ms_since_change < animation_length)
    {
      float progress = (float) ms_since_change / (float) animation_length();

      std::list<RenderArg> end = results;
      results.clear();

      std::list<RenderArg>::iterator start_it, end_it;
      for (start_it = saved_args_.begin(), end_it = end.begin(); start_it != saved_args_.end(); ++start_it, ++end_it)
      {
        results.push_back(InterpolateRenderArgs(*start_it, *end_it, progress));
      }

      background_geo = InterpolateBackground (saved_background_, background_geo, progress);
    }
  }

  return results;
}

gboolean SwitcherView::OnDrawTimeout(gpointer data)
{
  SwitcherView* self = static_cast<SwitcherView*>(data);

  self->QueueDraw();
  self->animation_draw_ = true;
  self->redraw_handle_ = 0;
  return FALSE;
}

void SwitcherView::DrawBackground(nux::GraphicsEngine& GfxContext, nux::Geometry const& geo)
{
  int border = 30;

  GfxContext.GetRenderStates().SetBlend (TRUE, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  nux::TexCoordXForm texxform;
  texxform.SetTexCoordType (nux::TexCoordXForm::OFFSET_COORD);
  texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);

  // Draw TOP-LEFT CORNER
  texxform.u0 = 0;
  texxform.v0 = 0;
  texxform.u1 = border;
  texxform.v1 = border;
  GfxContext.QRP_1Tex (geo.x, geo.y, 
                       border, border, background_corner_->GetDeviceTexture(), texxform, nux::color::White);
  
  // Draw TOP-RIGHT CORNER
  texxform.u0 = 0;
  texxform.v0 = 0;
  texxform.u1 = border;
  texxform.v1 = border;
  texxform.flip_u_coord = true;
  texxform.flip_v_coord = false;
  GfxContext.QRP_1Tex (geo.x + geo.width - border, geo.y, 
                       border, border, background_corner_->GetDeviceTexture(), texxform, nux::color::White);
  
  // Draw BOTTOM-LEFT CORNER
  texxform.u0 = 0;
  texxform.v0 = 0;
  texxform.u1 = border;
  texxform.v1 = border;
  texxform.flip_u_coord = false;
  texxform.flip_v_coord = true;
  GfxContext.QRP_1Tex (geo.x, geo.y + geo.height - border, 
                       border, border, background_corner_->GetDeviceTexture(), texxform, nux::color::White);
  
  // Draw BOTTOM-RIGHT CORNER
  texxform.u0 = 0;
  texxform.v0 = 0;
  texxform.u1 = border;
  texxform.v1 = border;
  texxform.flip_u_coord = true;
  texxform.flip_v_coord = true;
  GfxContext.QRP_1Tex (geo.x + geo.width - border, geo.y + geo.height - border, 
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
  GfxContext.QRP_1Tex (geo.x + border, geo.y, geo.width - border - border, border, background_top_->GetDeviceTexture(), texxform, nux::color::White);
  
  // Draw BOTTOM BORDER
  texxform.u0 = 0;
  texxform.v0 = 0;
  texxform.u1 = top_width;
  texxform.v1 = top_height;
  texxform.flip_u_coord = false;
  texxform.flip_v_coord = true;
  GfxContext.QRP_1Tex (geo.x + border, geo.y + geo.height - border, geo.width - border - border, border, background_top_->GetDeviceTexture(), texxform, nux::color::White);
  

  int left_width = background_left_->GetWidth();
  int left_height = background_left_->GetHeight();

  // Draw LEFT BORDER
  texxform.u0 = 0;
  texxform.v0 = 0;
  texxform.u1 = left_width;
  texxform.v1 = left_height;
  texxform.flip_u_coord = false;
  texxform.flip_v_coord = false;
  GfxContext.QRP_1Tex (geo.x, geo.y + border, border, geo.height - border - border, background_left_->GetDeviceTexture(), texxform, nux::color::White);
  
  // Draw RIGHT BORDER
  texxform.u0 = 0;
  texxform.v0 = 0;
  texxform.u1 = left_width;
  texxform.v1 = left_height;
  texxform.flip_u_coord = true;
  texxform.flip_v_coord = false;
  GfxContext.QRP_1Tex (geo.x + geo.width - border, geo.y + border, border, geo.height - border - border, background_left_->GetDeviceTexture(), texxform, nux::color::White);

  GfxContext.GetRenderStates().SetBlend (FALSE);
}

void SwitcherView::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  timespec current;
  clock_gettime(CLOCK_MONOTONIC, &current);

  if (!target_sizes_set_)
  {
    icon_renderer_->SetTargetSize(tile_size, icon_size, 10);
    target_sizes_set_ = true;
  }

  nux::Geometry base = GetGeometry();
  GfxContext.PushClippingRectangle(base);

  // clear region
  gPainter.PaintBackground(GfxContext, base);

  nux::Geometry background_geo;

  last_args_ = RenderArgsFlat(background_geo, model_->SelectionIndex(), current);
  last_background_ = background_geo;

  // magic constant comes from texture contents (distance to cleared area)
  const int internal_offset = 20;
  nux::Geometry internal_clip(background_geo.x + internal_offset, 
                              background_geo.y + internal_offset, 
                              background_geo.width - internal_offset * 2, 
                              background_geo.height - internal_offset * 2);
  GfxContext.PushClippingRectangle(internal_clip);


  nux::Geometry geo_absolute = GetAbsoluteGeometry ();
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
  gPainter.PushDrawColorLayer (GfxContext, internal_clip, background_color, false, rop);

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
  
  icon_renderer_->PreprocessIcons(last_args_, base);

  GfxContext.GetRenderStates().SetPremultipliedBlend(nux::SRC_OVER);
  std::list<RenderArg>::iterator it;
  for (it = last_args_.begin(); it != last_args_.end(); ++it)
  {
    if (it->icon == model_->Selection())
    {
      int view_width = text_view_->GetBaseWidth();
      int start_x = it->render_center.x - view_width / 2;

      internal_clip.Expand (-10, -10);
      if (start_x < internal_clip.x)
        start_x = internal_clip.x;
      else if (start_x + view_width > internal_clip.x + internal_clip.width)
        start_x = (internal_clip.x + internal_clip.width) - view_width;

      text_view_->SetBaseX(start_x);
    }
    if (it->y_rotation < 0)
      icon_renderer_->RenderIcon(GfxContext, *it, base, base);
  }

  std::list<RenderArg>::reverse_iterator rit;
  for (rit = last_args_.rbegin(); rit != last_args_.rend(); ++rit)
  {
    if (rit->y_rotation >= 0)
      icon_renderer_->RenderIcon(GfxContext, *rit, base, base);
  }

  if (render_boxes)
  {
    float val = 0.1f;
    for (LayoutWindow::Ptr layout : ExternalTargets())
    {
      gPainter.Paint2DQuadColor(GfxContext, layout->result, nux::Color(val, val, val ,val));
      val += 0.1f;

      if (val > 1)
        val = 0.1f;
    }

    for (rit = last_args_.rbegin(); rit != last_args_.rend(); ++rit)
    {
      nux::Geometry tmp (rit->render_center.x - 1, rit->render_center.y - 1, 2, 2);
      gPainter.Paint2DQuadColor(GfxContext, tmp, nux::color::Red);
    }
  }

  // render orange box that will encirlce active item(s)
  for (LayoutWindow::Ptr window : ExternalTargets())
  {
    if (window->alpha >= 1.0f)
    {
      nux::Geometry orange_box = window->result;
      orange_box.Expand(5, 5);
      orange_box.x -= geo_absolute.x;
      orange_box.y -= geo_absolute.y;

      gPainter.PaintTextureShape(GfxContext, orange_box, rounding_texture_, 6, 6, 6, 6, false);
    }
  }

  GfxContext.PopClippingRectangle();
  GfxContext.PopClippingRectangle();

  DrawBackground(GfxContext, background_geo);

  text_view_->SetBaseY(background_geo.y + background_geo.height - 45);
  text_view_->Draw(GfxContext, force_draw);

  int ms_since_change = TimeUtil::TimeDelta(&current, &save_time_);

  if (ms_since_change < animation_length && redraw_handle_ == 0)
    redraw_handle_ = g_idle_add_full (G_PRIORITY_DEFAULT, &SwitcherView::OnDrawTimeout, this, NULL);

  animation_draw_ = false;
}



}
}
