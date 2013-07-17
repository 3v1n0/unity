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
#include "MultiMonitor.h"
#include "SwitcherView.h"
#include "unity-shared/IconRenderer.h"
#include "unity-shared/TimeUtil.h"

#include <Nux/Nux.h>
#include <UnityCore/Variant.h>

namespace unity
{
using namespace ui;
using launcher::AbstractLauncherIcon;

namespace switcher
{

namespace
{
  const unsigned int VERTICAL_PADDING = 45;
  const unsigned int SPREAD_OFFSET = 100;
}

NUX_IMPLEMENT_OBJECT_TYPE(SwitcherView);

SwitcherView::SwitcherView()
  : render_boxes(false)
  , animate(true)
  , border_size(50)
  , flat_spacing(20)
  , icon_size(128)
  , minimum_spacing(10)
  , tile_size(150)
  , vertical_size(tile_size + VERTICAL_PADDING * 2)
  , text_size(15)
  , animation_length(250)
  , monitor(-1)
  , spread_size(3.5f)
  , icon_renderer_(std::make_shared<IconRenderer>())
  , text_view_(new StaticCairoText(""))
  , last_icon_selected_(-1)
  , last_detail_icon_selected_(-1)
  , target_sizes_set_(false)
{
  icon_renderer_->pip_style = OVER_TILE;
  icon_renderer_->monitor = max_num_monitors;

  text_view_->SetMaximumWidth(tile_size * spread_size);
  text_view_->SetLines(1);
  text_view_->SetTextColor(nux::color::White);
  text_view_->SetFont("Ubuntu Bold 10");

  icon_size.changed.connect (sigc::mem_fun (this, &SwitcherView::OnIconSizeChanged));
  tile_size.changed.connect (sigc::mem_fun (this, &SwitcherView::OnTileSizeChanged));

  mouse_move.connect(sigc::mem_fun(this, &SwitcherView::RecvMouseMove));
  mouse_up.connect  (sigc::mem_fun(this, &SwitcherView::RecvMouseUp));

  CaptureMouseDownAnyWhereElse(true);
  ResetTimer();

  animate.changed.connect([this] (bool enabled) {
    if (enabled)
    {
      SaveTime();
      QueueDraw();
    }
    else
    {
      ResetTimer();
    }
  });
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
  .add("spread-size", (float)spread_size)
  .add("label", text_view_->GetText())
  .add("spread_offset", SPREAD_OFFSET)
  .add("label_visible", text_view_->IsVisible());
}

debug::Introspectable::IntrospectableList SwitcherView::GetIntrospectableChildren()
{
  introspection_results_.clear();

  if (model_->detail_selection)
  {
    for (auto target : render_targets_)
    {
      // FIXME When a LayoutWindow is introspectable, it no longer renders :(
      //introspection_results_.push_back(target.get());
    }
  }
  else if (!last_args_.empty())
  {
    for (auto& args : last_args_)
    {
      introspection_results_.push_back(&args);
    }
  }

  return introspection_results_;
}

LayoutWindow::Vector SwitcherView::ExternalTargets ()
{
  return render_targets_;
}

void SwitcherView::SetModel(SwitcherModel::Ptr model)
{
  model_ = model;
  model->selection_changed.connect(sigc::mem_fun(this, &SwitcherView::OnSelectionChanged));
  model->detail_selection.changed.connect (sigc::mem_fun (this, &SwitcherView::OnDetailSelectionChanged));
  model->detail_selection_index.changed.connect (sigc::mem_fun (this, &SwitcherView::OnDetailSelectionIndexChanged));

  last_icon_selected_ = -1;

  if (!model->Selection())
    return;

  text_view_->SetVisible(!model->detail_selection);

  if (!model->detail_selection)
    text_view_->SetText(model->Selection()->tooltip_text());
}

void SwitcherView::OnIconSizeChanged (int size)
{
  icon_renderer_->SetTargetSize(tile_size, icon_size, 10);
}

void SwitcherView::OnTileSizeChanged (int size)
{
  icon_renderer_->SetTargetSize(tile_size, icon_size, 10);
  vertical_size = tile_size + VERTICAL_PADDING * 2;
}

void SwitcherView::SaveTime()
{
  clock_gettime(CLOCK_MONOTONIC, &save_time_);
}

void SwitcherView::ResetTimer()
{
  save_time_.tv_sec = 0;
  save_time_.tv_nsec = 0;
}

void SwitcherView::SaveLast()
{
  saved_args_ = last_args_;
  saved_background_ = last_background_;

  if (animate())
    SaveTime();
}

void SwitcherView::OnDetailSelectionIndexChanged(unsigned int index)
{
  QueueDraw ();
}

void SwitcherView::OnDetailSelectionChanged(bool detail)
{
  text_view_->SetVisible(!detail);

  last_detail_icon_selected_ = -1;

  if (!detail)
  {
    text_view_->SetText(model_->Selection()->tooltip_text());
    render_targets_.clear();
  }

  SaveLast();
  QueueDraw();
}

void SwitcherView::OnSelectionChanged(AbstractLauncherIcon::Ptr const& selection)
{
  if (selection)
    text_view_->SetText(selection->tooltip_text());

  SaveLast();
  QueueDraw();
}

void SwitcherView::RecvMouseMove(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags)
{
  if (model_->detail_selection)
  {
    HandleDetailMouseMove(x, y);
  }
  else
  {
    HandleMouseMove(x, y);
  }
}

void SwitcherView::HandleDetailMouseMove(int x, int y)
{
  int detail_icon_index = DetailIconIdexAt(x, y);

  if (detail_icon_index >= 0 && detail_icon_index != last_detail_icon_selected_)
  {
    model_->detail_selection_index = detail_icon_index;
    last_detail_icon_selected_ = detail_icon_index;
  }
}

void SwitcherView::HandleMouseMove(int x, int y)
{
  int icon_index = IconIndexAt(x, y);

  if (icon_index >= 0 && icon_index != last_icon_selected_)
  {
    if (icon_index != model_->SelectionIndex())
    {
      model_->Select(icon_index);
    }

    mouse_moving_over_icon.emit(icon_index);
    last_icon_selected_ = icon_index;
  }
}

void SwitcherView::RecvMouseUp(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  int button = nux::GetEventButton(button_flags);

  if (model_->detail_selection)
  {
    HandleDetailMouseUp(x, y, button);
  }
  else
  {
    HandleMouseUp(x, y, button);
  }
}

void SwitcherView::HandleDetailMouseUp(int x, int y, int button)
{
  int detail_icon_index = DetailIconIdexAt(x, y);

  if (button == 1)
  {
    if (detail_icon_index >= 0)
    {
      model_->detail_selection_index = detail_icon_index;
      hide_request.emit(true);
    }
  }
  else if (button == 3)
  {
    model_->detail_selection = false;
  }
}

void SwitcherView::HandleMouseUp(int x, int y, int button)
{
  int icon_index = IconIndexAt(x,y);

  if (button == 1)
  {
    if (icon_index >= 0)
    {
      model_->Select(icon_index);
      hide_request.emit(true);
    }
  }
  else if (button == 3)
  {
    right_clicked_icon.emit(icon_index);
  }
}

SwitcherModel::Ptr SwitcherView::GetModel()
{
  return model_;
}

RenderArg SwitcherView::CreateBaseArgForIcon(AbstractLauncherIcon::Ptr const& icon)
{
  RenderArg arg;
  arg.icon = icon.GetPointer();
  arg.alpha = 0.95f;

  // tells the renderer to render arrows by number
  arg.running_on_viewport = true;

  arg.window_indicators = icon->WindowsForMonitor(monitor).size();
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

  result.rotation.x = start.rotation.x + (end.rotation.x - start.rotation.x) * progress;
  result.rotation.y = start.rotation.y + (end.rotation.y - start.rotation.y) * progress;
  result.rotation.z = start.rotation.z + (end.rotation.z - start.rotation.z) * progress;

  result.render_center.x = start.render_center.x + (end.render_center.x - start.render_center.x) * progress;
  result.render_center.y = start.render_center.y + (end.render_center.y - start.render_center.y) * progress;
  result.render_center.z = start.render_center.z + (end.render_center.z - start.render_center.z) * progress;

  result.logical_center = result.render_center;

  return result;
}

nux::Geometry SwitcherView::InterpolateBackground(nux::Geometry const& start, nux::Geometry const& end, float progress)
{
  progress = -pow(progress - 1.0f, 2) + 1;

  nux::Geometry result;

  result.x = start.x + (end.x - start.x) * progress;
  result.y = start.y + (end.y - start.y) * progress;
  result.width = start.width + (end.width - start.width) * progress;
  result.height = start.height + (end.height - start.height) * progress;

  return result;
}

nux::Geometry SwitcherView::UpdateRenderTargets(float progress)
{
  std::vector<Window> const& xids = model_->DetailXids();
  render_targets_.clear();

  for (Window window : xids)
  {
    auto layout_window = std::make_shared<LayoutWindow>(window);
    bool selected = (window == model_->DetailSelectionWindow());
    layout_window->selected = selected;
    layout_window->alpha = (selected ? 1.0f : 0.9f) * progress;

    render_targets_.push_back(layout_window);
  }

  nux::Geometry max_bounds = GetAbsoluteGeometry();
  nux::Size const& spread_size = SpreadSize();
  max_bounds.x -= spread_size.width / 2;
  max_bounds.y -= spread_size.height / 2;
  max_bounds.width = spread_size.width;
  max_bounds.height = spread_size.height;

  nux::Geometry layout_geo;
  layout_system_.LayoutWindows(render_targets_, max_bounds, layout_geo);
  model_->SetRowSizes(layout_system_.GetRowSizes(render_targets_, max_bounds));

  return layout_geo;
}

void SwitcherView::ResizeRenderTargets(nux::Geometry const& layout_geo, float progress)
{
  if (progress >= 1.0f)
    return;

  // Animate the windows thumbnail sizes to make them grow with the switcher
  float to_finish = 1.0f - progress;
  nux::Point layout_abs_center((layout_geo.x + layout_geo.width/2.0f) * to_finish,
                               (layout_geo.y + layout_geo.height/2.0f) * to_finish);

  for (LayoutWindow::Ptr const& win : render_targets_)
  {
    auto final_geo = win->result;
    win->result = final_geo * progress;
    win->result.x += layout_abs_center.x;
    win->result.y += layout_abs_center.y;
  }
}

void SwitcherView::OffsetRenderTargets(int x, int y)
{
  for (LayoutWindow::Ptr const& target : render_targets_)
  {
    target->result.x += x;
    target->result.y += y;
  }
}

nux::Size SwitcherView::SpreadSize()
{
  nux::Geometry const& base = GetGeometry();
  nux::Size result(base.width - border_size * 2, base.height - border_size * 2);

  int width_padding = std::max(model_->Size() - 1, 0) * minimum_spacing + tile_size;
  result.width -= width_padding;

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

std::list<RenderArg> SwitcherView::RenderArgsFlat(nux::Geometry& background_geo, int selection, float progress)
{
  std::list<RenderArg> results;
  nux::Geometry const& base = GetGeometry();

  bool detail_selection = model_->detail_selection;

  background_geo.y = base.y + base.height / 2 - (vertical_size / 2);
  background_geo.height = vertical_size;

  if (text_view_->IsVisible())
    background_geo.height += text_size;

  if (model_)
  {
    int size = model_->Size();
    int padded_tile_size = tile_size + flat_spacing * 2;
    int max_width = base.width - border_size * 2;

    int spread_padded_width = 0;
    if (detail_selection)
    {
      nux::Geometry const& spread_bounds = UpdateRenderTargets(progress);
      ResizeRenderTargets(spread_bounds, progress);
      // remove extra space consumed by spread
      spread_padded_width = spread_bounds.width + SPREAD_OFFSET;
      max_width -= spread_padded_width - tile_size;

      int expansion = std::max(0, spread_bounds.height - icon_size);
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

    int i = 0;
    int y = base.y + base.height / 2;
    x += border_size;
    for (auto const& icon : *model_)
    {
      RenderArg arg = CreateBaseArgForIcon(icon);

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

      arg.render_center = nux::Point3(should_flat ? std::floor(x) : x, y, 0);

      x += (half_size + flat_spacing) * scalar;

      arg.rotation.y = (1.0f - std::min<float>(1.0f, scalar));

      if (!should_flat && overflow > 0 && i != selection)
      {
        if (i > last_flat)
        {
          arg.render_center.x -= 20;
        }
        else
        {
          arg.render_center.x += 20;
          arg.rotation.y = -arg.rotation.y;
        }
      }

      arg.render_center.z = abs(80.0f * arg.rotation.y);
      arg.logical_center = arg.render_center;

      if (i == selection && detail_selection)
      {
        arg.skip = true;
        OffsetRenderTargets(arg.render_center.x, arg.render_center.y);
      }

      results.push_back(arg);
      ++i;
    }

    if (saved_args_.size () == results.size () && progress < 1.0f)
    {
      std::list<RenderArg> end = results;
      results.clear();

      std::list<RenderArg>::iterator start_it, end_it;
      for (start_it = saved_args_.begin(), end_it = end.begin(); start_it != saved_args_.end(); ++start_it, ++end_it)
      {
        results.push_back(InterpolateRenderArgs(*start_it, *end_it, progress));
      }

      background_geo = InterpolateBackground(saved_background_, background_geo, progress);
    }
  }

  return results;
}

double SwitcherView::GetCurrentProgress()
{
  clock_gettime(CLOCK_MONOTONIC, &current_);
  DeltaTime ms_since_change = TimeUtil::TimeDelta(&current_, &save_time_);
  return std::min<double>(1.0f, ms_since_change / static_cast<double>(animation_length()));
}

void SwitcherView::PreDraw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  double progress = GetCurrentProgress();

  if (!target_sizes_set_)
  {
    icon_renderer_->SetTargetSize(tile_size, icon_size, 10);
    target_sizes_set_ = true;
  }

  nux::Geometry background_geo;
  last_args_ = RenderArgsFlat(background_geo, model_->SelectionIndex(), progress);
  last_background_ = background_geo;

  icon_renderer_->PreprocessIcons(last_args_, GetGeometry());
}

nux::Geometry SwitcherView::GetBackgroundGeometry()
{
  return last_background_;
}

void SwitcherView::DrawOverlay(nux::GraphicsEngine& GfxContext, bool force_draw, nux::Geometry const& clip)
{
  nux::Geometry const& base = GetGeometry();
  nux::Geometry internal_clip = clip;

  GfxContext.GetRenderStates().SetPremultipliedBlend(nux::SRC_OVER);


  for (auto const& arg : last_args_)
  {
    if (text_view_->IsVisible() && model_->Selection() == arg.icon)
    {
      int view_width = text_view_->GetBaseWidth();
      int start_x = arg.render_center.x - view_width / 2;

      internal_clip.Expand (-10, -10);
      if (start_x < internal_clip.x)
        start_x = internal_clip.x;
      else if (start_x + view_width > internal_clip.x + internal_clip.width)
        start_x = (internal_clip.x + internal_clip.width) - view_width;

      text_view_->SetBaseX(start_x);
    }

    if (arg.rotation.y < 0)
      icon_renderer_->RenderIcon(GfxContext, arg, base, base);
  }

  for (auto rit = last_args_.rbegin(); rit != last_args_.rend(); ++rit)
  {
    if (rit->rotation.y >= 0)
      icon_renderer_->RenderIcon(GfxContext, *rit, base, base);
  }

  if (render_boxes)
  {
    float val = 0.1f;
    for (LayoutWindow::Ptr const& layout : ExternalTargets())
    {
      gPainter.Paint2DQuadColor(GfxContext, layout->result, nux::Color(val, val, val ,val));
      val += 0.1f;

      if (val > 1)
        val = 0.1f;
    }

    for (auto rit = last_args_.rbegin(); rit != last_args_.rend(); ++rit)
    {
      nux::Geometry tmp (rit->render_center.x - 1, rit->render_center.y - 1, 2, 2);
      gPainter.Paint2DQuadColor(GfxContext, tmp, nux::color::Red);
    }
  }

  if (text_view_->IsVisible())
  {
    nux::GetPainter().PushPaintLayerStack();
    text_view_->SetBaseY(last_background_.y + last_background_.height - 45);
    text_view_->Draw(GfxContext, force_draw);
    nux::GetPainter().PopPaintLayerStack();
  }

  DeltaTime ms_since_change = TimeUtil::TimeDelta(&current_, &save_time_);

  if (ms_since_change < animation_length && !redraw_idle_)
  {
    redraw_idle_.reset(new glib::Idle([this] () {
      QueueDraw();
      redraw_idle_.reset();
      return false;
    }, glib::Source::Priority::DEFAULT));
  }
}

int SwitcherView::IconIndexAt(int x, int y) const
{
  int half_size = icon_size.Get() / 2 + 10;
  int icon_index = -1;

  // Taking icon rotation into consideration will make selection more
  // accurate when there are many icons present and the user clicks/taps
  // on icons close to edges. But manual testing has shown that the current
  // implementation is enough.

  int i = 0;
  for (auto const& arg : last_args_)
  {
    if (x < (arg.logical_center.x - half_size)
        || x > (arg.logical_center.x + half_size))
    {
      ++i;
    }
    else if (y < (arg.logical_center.y - half_size)
          || y > (arg.logical_center.y + half_size))
    {
      ++i;
    }
    else
    {
      icon_index = i;
      break;
    }
  }

  return icon_index;
}

int SwitcherView::DetailIconIdexAt(int x, int y) const
{
  int index = -1;

  for (unsigned int i = 0; i < render_targets_.size(); ++i)
  {
    if (render_targets_[i]->result.IsPointInside(x + SPREAD_OFFSET, y + SPREAD_OFFSET))
      return i;
  }

  return index;
}

}
}
