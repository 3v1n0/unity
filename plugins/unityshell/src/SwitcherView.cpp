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

namespace unity {
namespace switcher {

NUX_IMPLEMENT_OBJECT_TYPE (SwitcherView);

SwitcherView::SwitcherView(NUX_FILE_LINE_DECL) 
: View (NUX_FILE_LINE_PARAM)
, target_sizes_set_ (false)
, redraw_handle_ (0)
{
  icon_renderer_ = AbstractIconRenderer::Ptr (new IconRenderer ());
  border_size = 50;
  flat_spacing = 10;
  icon_size = 128;
  minimum_spacing = 20;
  tile_size = 170;
  vertical_size = 250;
  text_size = 15;
  animation_length = 250;

  background_texture_ = nux::CreateTexture2DFromFile (PKGDATADIR"/switcher_background.png", -1, true);

  text_view_ = new nux::StaticCairoText ("Testing");
  text_view_->SinkReference ();
  text_view_->SetGeometry (nux::Geometry (0, 0, 200, text_size));
  text_view_->SetTextColor (nux::color::White);
  text_view_->SetFont ("Ubuntu Bold 10");
}

SwitcherView::~SwitcherView()
{
  text_view_->UnReference ();
  if (redraw_handle_ > 0)
    g_source_remove (redraw_handle_);
}

static int
DeltaTTime (struct timespec const *x, struct timespec const *y)
{
  return ((x->tv_sec - y->tv_sec) * 1000) + ((x->tv_nsec - y->tv_nsec) / 1000000);
}

void SwitcherView::SetModel (SwitcherModel::Ptr model)
{
  model_ = model;
  model->selection_changed.connect (sigc::mem_fun (this, &SwitcherView::OnSelectionChanged));

  if (model->Selection ())
    text_view_->SetText (model->Selection ()->tooltip_text().c_str ());
}

void SwitcherView::OnSelectionChanged (AbstractLauncherIcon *selection)
{
  if (selection)
    text_view_->SetText (selection->tooltip_text().c_str ());
  QueueDraw ();
}

SwitcherModel::Ptr SwitcherView::GetModel ()
{
  return model_;
}

long SwitcherView::ProcessEvent (nux::IEvent &ievent, long TraverseInfo, long ProcessEventInfo)
{
  return TraverseInfo;
}

void SwitcherView::Draw (nux::GraphicsEngine& GfxContext, bool force_draw)
{
  return;
}

RenderArg SwitcherView::CreateBaseArgForIcon (AbstractLauncherIcon *icon)
{
  RenderArg arg;
  arg.icon = icon;
  arg.alpha = 0.95f;

  if (icon == model_->Selection ())
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

RenderArg SwitcherView::InterpolateRenderArgs (RenderArg const& start, RenderArg const& end, float progress)
{
  progress = -pow(progress-1.0f,2)+1;
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

std::list<RenderArg> SwitcherView::RenderArgsFlat (nux::Geometry& background_geo, int selection, timespec const& current)
{
  std::list<RenderArg> results;
  nux::Geometry base = GetGeometry ();

  background_geo.y = base.y + base.height / 2 - (vertical_size / 2);
  background_geo.height = vertical_size + text_size;


  if (model_)
  {
    int size = model_->Size ();
    int max_width = base.width - border_size * 2;
    int padded_tile_size = tile_size + flat_spacing * 2;
    int flat_width = size * padded_tile_size;

    int n_flat_icons = CLAMP ((max_width - 30) / padded_tile_size - 1, 0, size);
    
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


    float partial_overflow = (float) overflow / MAX (1.0f, (float) (size - n_flat_icons - 1));
    float partial_overflow_scalar = (float) (padded_tile_size - partial_overflow) / (float) (padded_tile_size);

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
    for (it = model_->begin (); it != model_->end (); ++it)
    {
      RenderArg arg = CreateBaseArgForIcon (*it);

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

      x += flat_spacing * scalar;
      
      x += (tile_size / 2) * scalar;

      if (should_flat)
        arg.render_center = nux::Point3 ((int) x, y, 0);
      else
        arg.render_center = nux::Point3 (x, y, 0);

      x += (tile_size / 2 + flat_spacing) * scalar;

      arg.y_rotation = (1.0f - scalar);
      
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

      arg.render_center.z = abs (80.0f * arg.y_rotation);

      arg.logical_center = arg.render_center;
      results.push_back (arg);
      ++i;
    }

    timespec last_change_time = model_->SelectionChangeTime ();
    int ms_since_change = DeltaTTime (&current, &last_change_time);
    if (overflow > 0 && selection == model_->SelectionIndex () && selection != model_->LastSelectionIndex () && ms_since_change < animation_length)
    {
      float progress = (float) ms_since_change / (float) animation_length();

      nux::Geometry last_geo;
      std::list<RenderArg> start = RenderArgsFlat (last_geo, model_->LastSelectionIndex (), current);
      std::list<RenderArg> end = results;

      results.clear ();

      std::list<RenderArg>::iterator start_it, end_it;
      for (start_it = start.begin (), end_it = end.begin (); start_it != start.end (); ++start_it, ++end_it)
      {
        results.push_back (InterpolateRenderArgs (*start_it, *end_it, progress));
      }
    }
  }

  return results;
}

std::list<RenderArg> SwitcherView::RenderArgsMechanical (nux::Geometry& background_geo, AbstractLauncherIcon *selection, timespec const& current)
{
  std::list<RenderArg> results;
  nux::Geometry base = GetGeometry ();

  background_geo.y = base.y + base.height / 2 - (vertical_size / 2);
  background_geo.height = vertical_size + text_size;

  if (model_)
  {
    int size = model_->Size ();
    int max_width = base.width - border_size * 2;
    int padded_tile_size = tile_size + flat_spacing * 2;
    int flat_width = size * padded_tile_size;
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

    float partial_overflow = (float) overflow / (float) (model_->Size () - 1);
    float partial_overflow_scalar = (float) (padded_tile_size - partial_overflow) / (float) (padded_tile_size); 

    SwitcherModel::iterator it;
    int i = 0;
    int y = base.y + base.height / 2;
    x += border_size;
    bool seen_selected = false;
    for (it = model_->begin (); it != model_->end (); ++it)
    {
      RenderArg arg = CreateBaseArgForIcon (*it);

      float scalar = partial_overflow_scalar;
      bool is_selection = arg.icon == selection;
      if (is_selection)
      {
        seen_selected = true;
        scalar = 1.0f;
      }

      x += flat_spacing * scalar;
      
      x += (tile_size / 2) * scalar;

      if (is_selection)
        arg.render_center = nux::Point3 ((int) x, y, 0);
      else
        arg.render_center = nux::Point3 (x, y, 0);

      x += (tile_size / 2 + flat_spacing) * scalar;

      arg.y_rotation = (1.0f - scalar) * 0.75f;
      
      if (!is_selection && overflow > 0)
      {
        if (seen_selected)
        {
          arg.render_center.x -= 20;
        }
        else
        {
          arg.render_center.x += 20;
          arg.y_rotation = -arg.y_rotation;
        }
      }

      arg.render_center.z = abs (80.0f * arg.y_rotation);

      arg.logical_center = arg.render_center;
      results.push_back (arg);
      ++i;
    }

    timespec last_change_time = model_->SelectionChangeTime ();
    int ms_since_change = DeltaTTime (&current, &last_change_time);
    if (overflow > 0 && selection == model_->Selection () && selection != model_->LastSelection () && ms_since_change < animation_length)
    {
      float progress = (float) ms_since_change / (float) animation_length();

      nux::Geometry last_geo;
      std::list<RenderArg> start = RenderArgsMechanical (last_geo, model_->LastSelection (), current);
      std::list<RenderArg> end = results;

      results.clear ();

      std::list<RenderArg>::iterator start_it, end_it;
      for (start_it = start.begin (), end_it = end.begin (); start_it != start.end (); ++start_it, ++end_it)
      {
        results.push_back (InterpolateRenderArgs (*start_it, *end_it, progress));
      }
    }
  }

  return results;
}

gboolean SwitcherView::OnDrawTimeout (gpointer data)
{
  SwitcherView *self = static_cast<SwitcherView *> (data);

  self->QueueDraw ();
  self->redraw_handle_ = 0;
  return FALSE;
}

void SwitcherView::DrawContent (nux::GraphicsEngine &GfxContext, bool force_draw)
{
  timespec current;
  clock_gettime (CLOCK_MONOTONIC, &current);

  if (!target_sizes_set_)
  {
    icon_renderer_->SetTargetSize (tile_size, icon_size, 10);
    target_sizes_set_ = true;
  }

  nux::Geometry base = GetGeometry ();
  GfxContext.PushClippingRectangle (base);

  nux::ROPConfig ROP;
  ROP.Blend = false;
  ROP.SrcBlend = GL_ONE;
  ROP.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

  // clear region
  gPainter.PaintBackground (GfxContext, base);

  nux::Geometry background_geo;

  std::list<RenderArg> args = RenderArgsFlat (background_geo, model_->SelectionIndex (), current);

  //nux::Geometry background_geo (base.x, base.y + base.height / 2 - 150, base.width, 300);
  gPainter.PaintTextureShape (GfxContext, background_geo, background_texture_, 30, 30, 30, 30, false);

  int internal_offset = 21;
  nux::Geometry internal_clip (background_geo.x + internal_offset, background_geo.y, background_geo.width - internal_offset * 2, background_geo.height);
  GfxContext.PushClippingRectangle (internal_clip);

  icon_renderer_->PreprocessIcons (args, base);

  GfxContext.GetRenderStates ().SetPremultipliedBlend (nux::SRC_OVER);
  std::list<RenderArg>::iterator it;
  for (it = args.begin (); it != args.end (); ++it)
  {
    if (it->icon == model_->Selection ())
      text_view_->SetBaseX (it->render_center.x - text_view_->GetBaseWidth () / 2);
    if (it->y_rotation < 0)
      icon_renderer_->RenderIcon (GfxContext, *it, base, base);
  }

  std::list<RenderArg>::reverse_iterator rit;
  for (rit = args.rbegin (); rit != args.rend (); ++rit)
  {
    if (rit->y_rotation >= 0)
      icon_renderer_->RenderIcon (GfxContext, *rit, base, base);
  }

  GfxContext.PopClippingRectangle();
  GfxContext.PopClippingRectangle();
  
  // probably evil
  gPainter.PopBackground ();

  text_view_->SetBaseY (background_geo.y + background_geo.height - 45);
  text_view_->Draw (GfxContext, force_draw);

  timespec last_change_time = model_->SelectionChangeTime ();
  int ms_since_change = DeltaTTime (&current, &last_change_time);

  if (ms_since_change < animation_length)
    redraw_handle_ = g_timeout_add (0, &SwitcherView::OnDrawTimeout, this);
}



}
}
