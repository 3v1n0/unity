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

#include <NuxCore/Object.h>
#include <Nux/Nux.h>
#include <Nux/WindowCompositor.h>

using namespace unity::ui;

namespace unity
{
namespace switcher
{

NUX_IMPLEMENT_OBJECT_TYPE(SwitcherView);

SwitcherView::SwitcherView(NUX_FILE_LINE_DECL)
  : View(NUX_FILE_LINE_PARAM)
  , target_sizes_set_(false)
  , redraw_handle_(0)
{
  icon_renderer_ = AbstractIconRenderer::Ptr(new IconRenderer());
  border_size = 50;
  flat_spacing = 10;
  icon_size = 128;
  minimum_spacing = 20;
  tile_size = 150;
  vertical_size = tile_size + 80;
  text_size = 15;
  animation_length = 250;
  spread_size = 3.5f;

  save_time_.tv_sec = 0;
  save_time_.tv_nsec = 0;

  render_targets_.clear ();

  background_texture_ = nux::CreateTexture2DFromFile(PKGDATADIR"/switcher_background.png", -1, true);

  text_view_ = new nux::StaticCairoText("Testing");
  text_view_->SinkReference();
  text_view_->SetGeometry(nux::Geometry(0, 0, 200, text_size));
  text_view_->SetTextColor(nux::color::White);
  text_view_->SetFont("Ubuntu Bold 10");
}

SwitcherView::~SwitcherView()
{
  text_view_->UnReference();
  if (redraw_handle_ > 0)
    g_source_remove(redraw_handle_);
}

WindowRenderTargetList SwitcherView::ExternalTargets ()
{
  WindowRenderTargetList result = render_targets_;
  return result;
}

static int
DeltaTTime(struct timespec const* x, struct timespec const* y)
{
  return ((x->tv_sec - y->tv_sec) * 1000) + ((x->tv_nsec - y->tv_nsec) / 1000000);
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

void SwitcherView::SaveLast ()
{
  saved_args_ = last_args_;
  saved_background_ = last_background_;
  clock_gettime(CLOCK_MONOTONIC, &save_time_);
}

void SwitcherView::OnDetailSelectionIndexChanged (int index)
{
  QueueDraw ();
}

void SwitcherView::OnDetailSelectionChanged (bool detail)
{
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

long SwitcherView::ProcessEvent(nux::IEvent& ievent, long TraverseInfo, long ProcessEventInfo)
{
  return TraverseInfo;
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

void SwitcherView::UpdateRenderTargets (RenderArg const& selection_arg, timespec const& current)
{
  std::vector<Window> xids = model_->DetailXids ();

  int width = 1;
  int height = 1;

  while (width * height < (int) xids.size ())
  {
    if (height < width)
      height++;
    else
      width++;
  }

  int block_width = (tile_size * spread_size) / width;
  int block_height = (tile_size * spread_size) / height;

  int x = 0;
  int y = 0;

  nux::Geometry absolute = GetAbsoluteGeometry ();
  int start_x = absolute.x + selection_arg.render_center.x - (tile_size * spread_size) / 2;
  int start_y = absolute.y + selection_arg.render_center.y - (tile_size * spread_size) / 2;

  int ms_since_change = DeltaTTime(&current, &save_time_);
  float progress = MIN (1.0f, (float) ms_since_change / (float) animation_length());

  int i = 0;
  for (Window window : xids)
  {
    RenderTargetData element;
    element.window = window;
    element.bounding = nux::Geometry (start_x + x * block_width, start_y + y * block_height, block_width, block_height);

    element.alpha = 0.75f * progress;
    if (i == model_->detail_selection_index)
      element.alpha = 1.0f * progress;

    render_targets_.push_back (element);

    ++x;

    if (x >= width)
    {
      x = 0;
      ++y;
    }

    ++i;
  }
}

std::list<RenderArg> SwitcherView::RenderArgsFlat(nux::Geometry& background_geo, int selection, timespec const& current)
{
  std::list<RenderArg> results;
  nux::Geometry base = GetGeometry();

  render_targets_.clear ();

  bool detail_selection = model_->detail_selection;

  if (detail_selection)
  {
    background_geo.y = base.y + base.height / 2 - (vertical_size / 2) * (spread_size * 0.75f);
    background_geo.height = vertical_size * (spread_size * 0.75f) + text_size;
  }
  else
  {
    background_geo.y = base.y + base.height / 2 - (vertical_size / 2);
    background_geo.height = vertical_size + text_size;  
  }

  if (model_)
  {

    int size = model_->Size();
    int padded_tile_size = tile_size + flat_spacing * 2;
    int max_width = base.width - border_size * 2;

    if (detail_selection)
      max_width -= padded_tile_size * (spread_size - 1.0f);

    int flat_width = size * padded_tile_size;

    int n_flat_icons = CLAMP((max_width - 30) / padded_tile_size - 1, 0, size);

    float x = 0;

    int overflow = flat_width - max_width;

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


    float partial_overflow = (float) overflow / MAX(1.0f, (float)(size - n_flat_icons - 1));
    float partial_overflow_scalar = (float)(padded_tile_size - partial_overflow) / (float)(padded_tile_size);

    int first_flat, last_flat;
    int half_fold_left = -1;
    int half_fold_right = -1;

    if (selection == 0)
    {
      first_flat = 0;
      last_flat = n_flat_icons;
    }
    else if (selection >= 1 && selection <= n_flat_icons - 1)
    {
      first_flat = 1;
      last_flat = n_flat_icons;

      half_fold_left = 0;
      half_fold_right = last_flat + 1;
    }
    else if (selection >= size - 2)
    {
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

      if (i == selection && detail_selection)
        scalar = spread_size;

      x += flat_spacing * scalar;

      x += (tile_size / 2) * scalar;

      if (should_flat)
        arg.render_center = nux::Point3((int) x, y, 0);
      else
        arg.render_center = nux::Point3(x, y, 0);

      x += (tile_size / 2 + flat_spacing) * scalar;

      arg.y_rotation = (1.0f - MIN (1.0f, scalar));

      if (!should_flat && overflow > 0)
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
        UpdateRenderTargets (arg, current);
      }

      results.push_back(arg);
      ++i;
    }

    int ms_since_change = DeltaTTime(&current, &save_time_);
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
  self->redraw_handle_ = 0;
  return FALSE;
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

  nux::ROPConfig ROP;
  ROP.Blend = false;
  ROP.SrcBlend = GL_ONE;
  ROP.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

  // clear region
  gPainter.PaintBackground(GfxContext, base);

  nux::Geometry background_geo;

  last_args_ = RenderArgsFlat(background_geo, model_->SelectionIndex(), current);
  last_background_ = background_geo;


  gPainter.PaintTextureShape(GfxContext, background_geo, background_texture_, 30, 30, 30, 30, false);

  int internal_offset = 21;
  nux::Geometry internal_clip(background_geo.x + internal_offset, background_geo.y, background_geo.width - internal_offset * 2, background_geo.height);
  GfxContext.PushClippingRectangle(internal_clip);

  icon_renderer_->PreprocessIcons(last_args_, base);

  GfxContext.GetRenderStates().SetPremultipliedBlend(nux::SRC_OVER);
  std::list<RenderArg>::iterator it;
  for (it = last_args_.begin(); it != last_args_.end(); ++it)
  {
    if (it->icon == model_->Selection())
      text_view_->SetBaseX(it->render_center.x - text_view_->GetBaseWidth() / 2);
    if (it->y_rotation < 0)
      icon_renderer_->RenderIcon(GfxContext, *it, base, base);
  }

  std::list<RenderArg>::reverse_iterator rit;
  for (rit = last_args_.rbegin(); rit != last_args_.rend(); ++rit)
  {
    if (rit->y_rotation >= 0)
      icon_renderer_->RenderIcon(GfxContext, *rit, base, base);
  }

  GfxContext.PopClippingRectangle();
  GfxContext.PopClippingRectangle();

  // probably evil
  gPainter.PopBackground();

  text_view_->SetBaseY(background_geo.y + background_geo.height - 45);
  text_view_->Draw(GfxContext, force_draw);

  int ms_since_change = DeltaTTime(&current, &save_time_);

  if (ms_since_change < animation_length)
    redraw_handle_ = g_timeout_add(0, &SwitcherView::OnDrawTimeout, this);
}



}
}
