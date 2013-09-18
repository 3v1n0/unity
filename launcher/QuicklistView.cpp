// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2010 Canonical Ltd
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
* Authored by: Jay Taoko <jay.taoko@canonical.com>
* Authored by: Mirco Müller <mirco.mueller@canonical.com
*/

#include <Nux/Nux.h>
#include <Nux/VLayout.h>
#include <Nux/HLayout.h>
#include <Nux/WindowThread.h>
#include <Nux/WindowCompositor.h>
#include <Nux/BaseWindow.h>
#include <Nux/Button.h>
#include <NuxGraphics/GraphicsEngine.h>
#include <Nux/TextureArea.h>
#include <NuxGraphics/CairoGraphics.h>
#include <UnityCore/Variant.h>

#include "unity-shared/CairoTexture.h"

#include "QuicklistView.h"
#include "QuicklistMenuItem.h"
#include "QuicklistMenuItemLabel.h"
#include "QuicklistMenuItemSeparator.h"
#include "QuicklistMenuItemCheckmark.h"
#include "QuicklistMenuItemRadio.h"

#include "unity-shared/Introspectable.h"
#include "unity-shared/UnitySettings.h"

#include "unity-shared/UBusWrapper.h"
#include "unity-shared/UBusMessages.h"
#include "unity-shared/DashStyle.h"

namespace unity
{
namespace
{
  const int ANCHOR_WIDTH = 10.0f;
  const int TOP_SIZE = 4;
}

NUX_IMPLEMENT_OBJECT_TYPE(QuicklistView);

QuicklistView::QuicklistView()
  : _anchorX(0)
  , _anchorY(0)
  , _labelText("QuicklistView 1234567890")
  , _top_size(TOP_SIZE)
  , _mouse_down(false)
  , _enable_quicklist_for_testing(false)
  , _anchor_width(10)
  , _anchor_height(18)
  , _corner_radius(4)
  , _padding(13)
  , _left_padding_correction(-1)
  , _offset_correction(-1)
  , _cairo_text_has_changed(true)
  , _current_item_index(-1)
{
  SetGeometry(nux::Geometry(0, 0, 1, 1));

  _left_space = new nux::SpaceLayout(_padding +
                                     _anchor_width +
                                     _corner_radius +
                                     _left_padding_correction,
                                     _padding +
                                     _anchor_width +
                                     _corner_radius +
                                     _left_padding_correction,
                                     1, 1000);
  _right_space = new nux::SpaceLayout(_padding + _corner_radius,
                                      _padding + _corner_radius,
                                      1, 1000);
  _top_space = new nux::SpaceLayout(1, 1000,
                                    _padding + _corner_radius,
                                    _padding + _corner_radius);
  _bottom_space = new nux::SpaceLayout(1, 1000,
                                       _padding + _corner_radius,
                                       _padding + _corner_radius);

  _vlayout = new nux::VLayout(TEXT(""), NUX_TRACKER_LOCATION);
  _vlayout->AddLayout(_top_space, 0);

  _item_layout     = new nux::VLayout(TEXT(""), NUX_TRACKER_LOCATION);
  _vlayout->AddLayout(_item_layout, 0);

  _vlayout->AddLayout(_bottom_space, 0);
  _vlayout->SetMinimumWidth(140);

  _hlayout = new nux::HLayout(TEXT(""), NUX_TRACKER_LOCATION);
  _hlayout->AddLayout(_left_space, 0);
  _hlayout->AddLayout(_vlayout, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
  _hlayout->AddLayout(_right_space, 0);

  SetWindowSizeMatchLayout(true);
  SetLayout(_hlayout);

  mouse_down_outside_pointer_grab_area.connect(sigc::mem_fun(this, &QuicklistView::RecvMouseDownOutsideOfQuicklist));
  mouse_down.connect(sigc::mem_fun(this, &QuicklistView::RecvMouseDown));
  mouse_up.connect(sigc::mem_fun(this, &QuicklistView::RecvMouseUp));
  mouse_click.connect(sigc::mem_fun(this, &QuicklistView::RecvMouseClick));
  mouse_move.connect(sigc::mem_fun(this, &QuicklistView::RecvMouseMove));
  mouse_drag.connect(sigc::mem_fun(this, &QuicklistView::RecvMouseDrag));
  key_down.connect(sigc::mem_fun(this, &QuicklistView::RecvKeyPressed));
  begin_key_focus.connect(sigc::mem_fun(this, &QuicklistView::RecvStartFocus));
  end_key_focus.connect(sigc::mem_fun(this, &QuicklistView::RecvEndFocus));

  SetAcceptKeyNavFocus(true);
}

void
QuicklistView::RecvStartFocus()
{
  PushToFront();
}

void
QuicklistView::RecvEndFocus()
{
}

void
QuicklistView::SelectItem(int index)
{
  CancelItemsPrelightStatus();
  int target_item = -1;

  if (IsMenuItemSelectable(index))
  {
    QuicklistMenuItem* menu_item = GetNthItems(index);

    if (menu_item)
    {
      target_item = index;
      menu_item->Select();
    }
  }

  if (_current_item_index != target_item)
  {
    _current_item_index = target_item;
    selection_change.emit();
    QueueDraw();
  }
}

bool
QuicklistView::IsMenuItemSelectable(int index)
{
  QuicklistMenuItem* menu_item = nullptr;

  if (index < 0)
    return false;

  menu_item = GetNthItems(index);
  if (!menu_item)
    return false;

  return menu_item->GetSelectable();
}

void
QuicklistView::RecvKeyPressed(unsigned long    eventType,
                              unsigned long    key_sym,
                              unsigned long    key_state,
                              const char*      character,
                              unsigned short   keyCount)
{
  switch (key_sym)
  {
      // home or page up (highlight the first menu-hitem)
    case NUX_VK_PAGE_UP:
    case NUX_VK_HOME:
    {
      int num_items = GetNumItems();
      int target_index = -1;

      do
      {
        ++target_index;
      }
      while (!IsMenuItemSelectable(target_index) && target_index < num_items);

      if (target_index < num_items)
        SelectItem(target_index);

      break;
    }
      // end or page down (highlight the last menu-hitem)
    case NUX_VK_PAGE_DOWN:
    case NUX_VK_END:
    {
      int target_index = GetNumItems();

      do
      {
        --target_index;
      }
      while (!IsMenuItemSelectable(target_index) && target_index >= 0);

      if (target_index >= 0)
        SelectItem(target_index);

      break;
    }
      // up (highlight previous menu-item)
    case NUX_VK_UP:
    case NUX_KP_UP:
    {
      int target_index = _current_item_index;
      bool loop_back = false;

      if (target_index <= 0)
        target_index = GetNumItems();

      do
      {
        --target_index;

        // If the first item is not selectable, we must loop from the last one
        if (!loop_back && target_index == 0 && !IsMenuItemSelectable(target_index))
        {
          loop_back = true;
          target_index = GetNumItems() - 1;
        }
      }
      while (!IsMenuItemSelectable(target_index) && target_index >= 0);

      if (target_index >= 0)
        SelectItem(target_index);

      break;
    }

      // down (highlight next menu-item)
    case NUX_VK_DOWN:
    case NUX_KP_DOWN:
    {
      int target_index = _current_item_index;
      int num_items = GetNumItems();
      bool loop_back = false;

      if (target_index >= num_items - 1)
        target_index = -1;

      do
      {
        ++target_index;

        // If the last item is not selectable, we must loop from the first one
        if (!loop_back && target_index == num_items - 1 && !IsMenuItemSelectable(target_index))
        {
          loop_back = true;
          target_index = 0;
        }
      }
      while (!IsMenuItemSelectable(target_index) && target_index < num_items);

      if (target_index < num_items)
        SelectItem(target_index);

      break;
    }

      // left (close quicklist, go back to laucher key-nav)
    case NUX_VK_LEFT:
    case NUX_KP_LEFT:
      HideAndEndQuicklistNav();
      break;

      // esc (close quicklist, exit key-nav)
    case NUX_VK_ESCAPE:
      Hide();
      // inform UnityScreen we leave key-nav completely
      UBusManager::SendMessage(UBUS_LAUNCHER_END_KEY_NAV);
      break;

      // <SPACE>, <RETURN> (activate selected menu-item)
    case NUX_VK_SPACE:
    case NUX_VK_ENTER:
    case NUX_KP_ENTER:
      if (IsMenuItemSelectable(_current_item_index))
      {
        ActivateItem(GetNthItems(_current_item_index));
        Hide();
      }
      break;

    default:
      break;
  }
}

QuicklistView::~QuicklistView()
{
  for (auto item : _item_list)
  {
    // Remove from introspection
    RemoveChild(item);
    item->UnReference();
  }

  _item_list.clear();
}

void
QuicklistView::EnableQuicklistForTesting(bool enable_testing)
{
  _enable_quicklist_for_testing = enable_testing;
}

void QuicklistView::ShowQuicklistWithTipAt(int anchor_tip_x, int anchor_tip_y)
{
  _anchorX = anchor_tip_x;
  _anchorY = anchor_tip_y;

  if (!_enable_quicklist_for_testing)
  {
    if (!_item_list.empty())
    {
      int offscreen_size = GetBaseY() +
                           GetBaseHeight() -
                           nux::GetWindowThread()->GetGraphicsDisplay().GetWindowHeight();

      if (offscreen_size > 0)
        _top_size = offscreen_size + TOP_SIZE;
      else
        _top_size = TOP_SIZE;

      int x = _anchorX - _padding;
      int y = _anchorY - _anchor_height / 2 - _top_size - _corner_radius - _padding;

      SetBaseX(x);
      SetBaseY(y);
    }
    else
    {
      _top_size = 0;
      int x = _anchorX - _padding;
      int y = _anchorY - _anchor_height / 2 - _top_size - _corner_radius - _padding;

      SetBaseX(x);
      SetBaseY(y);
    }
  }

  Show();
}

void QuicklistView::ShowWindow(bool b, bool start_modal)
{
  unity::CairoBaseWindow::ShowWindow(b, start_modal);
}

void QuicklistView::Show()
{
  if (!IsVisible())
  {
    // FIXME: ShowWindow shouldn't need to be called first
    ShowWindow(true);
    PushToFront();
    //EnableInputWindow (true, "quicklist", false, true);
    //SetInputFocus ();
    GrabPointer();
    GrabKeyboard();
    QueueDraw();
  }
}

void QuicklistView::Hide()
{
  if (IsVisible() && !_enable_quicklist_for_testing)
  {
    CancelItemsPrelightStatus();
    CaptureMouseDownAnyWhereElse(false);
    UnGrabPointer();
    UnGrabKeyboard();
    //EnableInputWindow (false);
    ShowWindow(false);

    if (_current_item_index != -1)
    {
      selection_change.emit();
      _current_item_index = -1;
    }
  }
}

void QuicklistView::HideAndEndQuicklistNav()
{
  Hide();
  // inform Launcher we switch back to Launcher key-nav
  UBusManager::SendMessage(UBUS_QUICKLIST_END_KEY_NAV);
}

void QuicklistView::Draw(nux::GraphicsEngine& gfxContext, bool forceDraw)
{
  _compute_blur_bkg = true;
  CairoBaseWindow::Draw(gfxContext, forceDraw);

  nux::Geometry base(GetGeometry());
  base.x = 0;
  base.y = 0;

  gfxContext.PushClippingRectangle(base);

  for (auto item : _item_list)
  {
    if (item->GetVisible())
      item->ProcessDraw(gfxContext, forceDraw);
  }

  gfxContext.PopClippingRectangle();
}

void QuicklistView::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw)
{}

void QuicklistView::PreLayoutManagement()
{
  int MaxItemWidth = 0;
  int TotalItemHeight = 0;

  for (auto item : _item_list)
  {
    // Make sure item is in layout if it should be
    if (!item->GetVisible())
    {
      _item_layout->RemoveChildObject(item);
      continue;
    }
    else if (!item->GetParentObject())
    {
      _item_layout->AddView(item, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
    }

    nux::Size const& text_extents = item->GetTextExtents();
    MaxItemWidth = std::max(MaxItemWidth, text_extents.width);
    TotalItemHeight += text_extents.height;
  }

  if (TotalItemHeight < _anchor_height)
  {
    int b = (_anchor_height - TotalItemHeight) / 2 + _padding + _corner_radius;
    int t = b + _offset_correction;

    _top_space->SetMinimumHeight(t);
    _top_space->SetMaximumHeight(t);

    _bottom_space->SetMinimumHeight(b);
    _bottom_space->SetMaximumHeight(b);
  }
  else
  {
    int b = _padding + _corner_radius;
    int t = b + _offset_correction;

    _top_space->SetMinimumHeight(t);
    _top_space->SetMaximumHeight(t);

    _bottom_space->SetMinimumHeight(b);
    _bottom_space->SetMaximumHeight(b);
  }

  _item_layout->SetMinimumWidth(MaxItemWidth);

  CairoBaseWindow::PreLayoutManagement();
}

long QuicklistView::PostLayoutManagement(long LayoutResult)
{
  long result = CairoBaseWindow::PostLayoutManagement(LayoutResult);

  UpdateTexture();

  int x = _padding + _anchor_width + _corner_radius + _offset_correction;
  int y = _top_space->GetMinimumHeight();

  for (auto item : _item_list)
  {
    if (!item->GetVisible())
      continue;

    item->SetBaseX(x);
    item->SetBaseY(y);

    y += item->GetBaseHeight();
  }

  // We must correct the width of line separators. The rendering of the separator can be smaller than the width of the
  // quicklist. The reason for that is, the quicklist width is determined by the largest entry it contains. That size is
  // only after MaxItemWidth is computed in QuicklistView::PreLayoutManagement.
  // The setting of the separator width is done here after the Layout cycle for this widget is over. The width of the separator
  // has bee set correctly during the layout cycle, but the cairo rendering still need to be adjusted.
  unsigned separator_width = _item_layout->GetBaseWidth();

  for (auto item : _item_list)
  {
    if (item->GetVisible() && item->GetCairoSurfaceWidth() != separator_width)
    {
      // Compute textures of the item.
      item->UpdateTexture();
    }
  }

  return result;
}

void QuicklistView::RecvCairoTextChanged(QuicklistMenuItem* cairo_text)
{
  _cairo_text_has_changed = true;
}

void QuicklistView::RecvCairoTextColorChanged(QuicklistMenuItem* cairo_text)
{
  NeedRedraw();
}

void QuicklistView::RecvItemMouseClick(QuicklistMenuItem* item, int x, int y)
{
  _mouse_down = false;
  if (IsVisible() && item->GetEnabled())
  {
    // Check if the mouse was released over an item and emit the signal
    CheckAndEmitItemSignal(x + item->GetBaseX(), y + item->GetBaseY());

    Hide();
  }
}

void QuicklistView::CheckAndEmitItemSignal(int x, int y)
{
  nux::Geometry geo;
  for (auto item : _item_list)
  {
    if (!item->GetVisible())
      continue;

    geo = item->GetGeometry();
    geo.width = _item_layout->GetBaseWidth();

    if (geo.IsPointInside(x, y))
    {
      // An action is performed: send the signal back to the application
      ActivateItem(item);
    }
  }
}

void QuicklistView::ActivateItem(QuicklistMenuItem* item)
{
  if (!item)
    return;

  item->Activate();
}

void QuicklistView::RecvItemMouseRelease(QuicklistMenuItem* item, int x, int y)
{
  _mouse_down = false;


  if (IsVisible() && item->GetEnabled())
  {
    // Check if the mouse was released over an item and emit the signal
    CheckAndEmitItemSignal(x + item->GetBaseX(), y + item->GetBaseY());

    Hide();
  }
}

void QuicklistView::CancelItemsPrelightStatus()
{
  for (auto item : _item_list)
  {
    item->Select(false);
  }
}

void QuicklistView::RecvItemMouseDrag(QuicklistMenuItem* item, int x, int y)
{
  nux::Geometry geo;
  for (auto it : _item_list)
  {
    int item_index = GetItemIndex(it);

    if (!IsMenuItemSelectable(item_index))
      continue;

    geo = it->GetGeometry();
    geo.width = _item_layout->GetBaseWidth();

    if (geo.IsPointInside(x + item->GetBaseX(), y + item->GetBaseY()))
    {
      SelectItem(item_index);
    }
  }
}

void QuicklistView::RecvItemMouseEnter(QuicklistMenuItem* item)
{
  int item_index = GetItemIndex(item);

  SelectItem(item_index);
}

void QuicklistView::RecvItemMouseLeave(QuicklistMenuItem* item)
{
  int item_index = GetItemIndex(item);

  if (item_index < 0 || item_index == _current_item_index)
    SelectItem(-1);
}

void QuicklistView::RecvMouseDown(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
//     if (IsVisible ())
//     {
//       CaptureMouseDownAnyWhereElse (false);
//       UnGrabPointer ();
//       EnableInputWindow (false);
//       ShowWindow (false);
//     }
}

void QuicklistView::RecvMouseUp(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  // Check if the mouse was released over an item and emit the signal
  CheckAndEmitItemSignal(x, y);
}

void QuicklistView::RecvMouseClick(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  if (IsVisible())
  {
    Hide();
  }
}

void QuicklistView::RecvMouseMove(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags)
{

}

void QuicklistView::RecvMouseDrag(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags)
{

}

void QuicklistView::RecvMouseDownOutsideOfQuicklist(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  Hide();
}

void QuicklistView::RemoveAllMenuItem()
{
  for (auto item : _item_list)
  {
    // Remove from introspection
    RemoveChild(item);
    item->UnReference();
  }


  _item_list.clear();

  _item_layout->Clear();
  _cairo_text_has_changed = true;
  nux::GetWindowThread()->QueueObjectLayout(this);
}

void QuicklistView::AddMenuItem(QuicklistMenuItem* item)
{
  if (item == 0)
    return;

  item->sigTextChanged.connect(sigc::mem_fun(this, &QuicklistView::RecvCairoTextChanged));
  item->sigColorChanged.connect(sigc::mem_fun(this, &QuicklistView::RecvCairoTextColorChanged));
  item->sigMouseClick.connect(sigc::mem_fun(this, &QuicklistView::RecvItemMouseClick));
  item->sigMouseReleased.connect(sigc::mem_fun(this, &QuicklistView::RecvItemMouseRelease));
  item->sigMouseEnter.connect(sigc::mem_fun(this, &QuicklistView::RecvItemMouseEnter));
  item->sigMouseLeave.connect(sigc::mem_fun(this, &QuicklistView::RecvItemMouseLeave));
  item->sigMouseDrag.connect(sigc::mem_fun(this, &QuicklistView::RecvItemMouseDrag));

  _item_list.push_back(item);
  item->Reference();
  // Add to introspection
  AddChild(item);

  _cairo_text_has_changed = true;
  nux::GetWindowThread()->QueueObjectLayout(this);
  NeedRedraw();
}

void QuicklistView::RenderQuicklistView()
{

}

int QuicklistView::GetNumItems()
{
  return _item_list.size();
}

QuicklistMenuItem* QuicklistView::GetNthItems(int index)
{
  if (index < (int)_item_list.size())
  {
    int i = 0;
    for (auto item : _item_list)
    {
      if (i++ == index)
        return item;
    }
  }

  return nullptr;
}

int QuicklistView::GetItemIndex(QuicklistMenuItem* item)
{
  int index = -1;

  for (auto it : _item_list)
  {
    ++index;

    if (it == item)
      return index;
  }

  return index;
}

QuicklistMenuItemType QuicklistView::GetNthType(int index)
{
  QuicklistMenuItem* item = GetNthItems(index);
  if (item)
    return item->GetItemType();

  return QuicklistMenuItemType::UNKNOWN;
}

std::list<QuicklistMenuItem*> QuicklistView::GetChildren()
{
  return _item_list;
}

void QuicklistView::SelectFirstItem()
{
  SelectItem(0);
}

void ql_tint_dot_hl(cairo_t* cr,
                    gint    width,
                    gint    height,
                    gfloat  hl_x,
                    gfloat  hl_y,
                    gfloat  hl_size,
                    gfloat* rgba_tint,
                    gfloat* rgba_hl,
                    gfloat* rgba_dot)
{
  cairo_surface_t* dots_surf    = NULL;
  cairo_t*         dots_cr      = NULL;
  cairo_pattern_t* dots_pattern = NULL;
  cairo_pattern_t* hl_pattern   = NULL;

  // create context for dot-pattern
  dots_surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 4, 4);
  dots_cr = cairo_create(dots_surf);

  // clear normal context
  cairo_scale(cr, 1.0f, 1.0f);
  cairo_set_source_rgba(cr, 0.0f, 0.0f, 0.0f, 0.0f);
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  // prepare drawing for normal context
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

  // create path in normal context
  cairo_rectangle(cr, 0.0f, 0.0f, (gdouble) width, (gdouble) height);

  // fill path of normal context with tint
  cairo_set_source_rgba(cr,
                        rgba_tint[0],
                        rgba_tint[1],
                        rgba_tint[2],
                        rgba_tint[3]);
  cairo_fill_preserve(cr);

  // create pattern in dot-context
  cairo_set_operator(dots_cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(dots_cr);
  cairo_scale(dots_cr, 1.0f, 1.0f);
  cairo_set_operator(dots_cr, CAIRO_OPERATOR_OVER);
  cairo_set_source_rgba(dots_cr,
                        rgba_dot[0],
                        rgba_dot[1],
                        rgba_dot[2],
                        rgba_dot[3]);
  cairo_rectangle(dots_cr, 0.0f, 0.0f, 1.0f, 1.0f);
  cairo_fill(dots_cr);
  cairo_rectangle(dots_cr, 2.0f, 2.0f, 1.0f, 1.0f);
  cairo_fill(dots_cr);
  dots_pattern = cairo_pattern_create_for_surface(dots_surf);

  // fill path of normal context with dot-pattern
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  cairo_set_source(cr, dots_pattern);
  cairo_pattern_set_extend(dots_pattern, CAIRO_EXTEND_REPEAT);
  cairo_fill_preserve(cr);
  cairo_pattern_destroy(dots_pattern);
  cairo_surface_destroy(dots_surf);
  cairo_destroy(dots_cr);

  // draw highlight
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  hl_pattern = cairo_pattern_create_radial(hl_x,
                                           hl_y,
                                           0.0f,
                                           hl_x,
                                           hl_y,
                                           hl_size);
  cairo_pattern_add_color_stop_rgba(hl_pattern,
                                    0.0f,
                                    rgba_hl[0],
                                    rgba_hl[1],
                                    rgba_hl[2],
                                    rgba_hl[3]);
  cairo_pattern_add_color_stop_rgba(hl_pattern, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f);
  cairo_set_source(cr, hl_pattern);
  cairo_fill(cr);
  cairo_pattern_destroy(hl_pattern);
}

void ql_setup(cairo_surface_t** surf,
              cairo_t**         cr,
              gboolean          outline,
              gint              width,
              gint              height,
              gboolean          negative)
{
//     // create context
//     if (outline)
//       *surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
//     else
//       *surf = cairo_image_surface_create (CAIRO_FORMAT_A8, width, height);
//     *cr = cairo_create (*surf);

  // clear context
  cairo_scale(*cr, 1.0f, 1.0f);
  if (outline)
  {
    cairo_set_source_rgba(*cr, 0.0f, 0.0f, 0.0f, 0.0f);
    cairo_set_operator(*cr, CAIRO_OPERATOR_CLEAR);
  }
  else
  {
    cairo_set_operator(*cr, CAIRO_OPERATOR_OVER);
    if (negative)
      cairo_set_source_rgba(*cr, 0.0f, 0.0f, 0.0f, 0.0f);
    else
      cairo_set_source_rgba(*cr, 1.0f, 1.0f, 1.0f, 1.0f);
  }
  cairo_paint(*cr);
}

void ql_compute_full_mask_path(cairo_t* cr,
                               gfloat   anchor_width,
                               gfloat   anchor_height,
                               gint     width,
                               gint     height,
                               gint     upper_size,
                               gfloat   radius,
                               guint    pad)
{
  //     0  1        2  3
  //     +--+--------+--+
  //     |              |
  //     + 14           + 4
  //     |              |
  //     |              |
  //     |              |
  //     + 13           |
  //    /               |
  //   /                |
  //  + 12              |
  //   \                |
  //    \               |
  //  11 +              |
  //     |              |
  //     |              |
  //     |              |
  //  10 +              + 5
  //     |              |
  //     +--+--------+--+ 6
  //     9  8        7


  gfloat padding  = pad;
  int ZEROPOINT5 = 0.0f;

  gfloat HeightToAnchor = ((gfloat) height - 2.0f * radius - anchor_height - 2 * padding) / 2.0f;
  if (HeightToAnchor < 0.0f)
  {
    g_warning("Anchor-height and corner-radius a higher than whole texture!");
    return;
  }

  //gint dynamic_size = height - 2*radius - 2*padding - anchor_height;
  //gint upper_dynamic_size = upper_size;
  //gint lower_dynamic_size = dynamic_size - upper_dynamic_size;

  if (upper_size >= 0)
  {
    if (upper_size > height - 2.0f * radius - anchor_height - 2 * padding)
    {
      //g_warning ("[_compute_full_mask_path] incorrect upper_size value");
      HeightToAnchor = 0;
    }
    else
    {
      HeightToAnchor = height - 2.0f * radius - anchor_height - 2 * padding - upper_size;
    }
  }
  else
  {
    HeightToAnchor = (height - 2.0f * radius - anchor_height - 2 * padding) / 2.0f;
  }

  cairo_translate(cr, -0.5f, -0.5f);

  // create path
  cairo_move_to(cr, padding + anchor_width + radius + ZEROPOINT5, padding + ZEROPOINT5);  // Point 1
  cairo_line_to(cr, width - padding - radius, padding + ZEROPOINT5);    // Point 2
  cairo_arc(cr,
            width  - padding - radius + ZEROPOINT5,
            padding + radius + ZEROPOINT5,
            radius,
            -90.0f * G_PI / 180.0f,
            0.0f * G_PI / 180.0f);   // Point 4
  cairo_line_to(cr,
                (gdouble) width - padding + ZEROPOINT5,
                (gdouble) height - radius - padding + ZEROPOINT5); // Point 5
  cairo_arc(cr,
            (gdouble) width - padding - radius + ZEROPOINT5,
            (gdouble) height - padding - radius + ZEROPOINT5,
            radius,
            0.0f * G_PI / 180.0f,
            90.0f * G_PI / 180.0f);  // Point 7
  cairo_line_to(cr,
                anchor_width + padding + radius + ZEROPOINT5,
                (gdouble) height - padding + ZEROPOINT5); // Point 8

  cairo_arc(cr,
            anchor_width + padding + radius + ZEROPOINT5,
            (gdouble) height - padding - radius,
            radius,
            90.0f * G_PI / 180.0f,
            180.0f * G_PI / 180.0f); // Point 10

  cairo_line_to(cr,
                padding + anchor_width + ZEROPOINT5,
                (gdouble) height - padding - radius - HeightToAnchor + ZEROPOINT5);   // Point 11
  cairo_line_to(cr,
                padding + ZEROPOINT5,
                (gdouble) height - padding - radius - HeightToAnchor - anchor_height / 2.0f + ZEROPOINT5); // Point 12
  cairo_line_to(cr,
                padding + anchor_width + ZEROPOINT5,
                (gdouble) height - padding - radius - HeightToAnchor - anchor_height + ZEROPOINT5);  // Point 13

  cairo_line_to(cr, padding + anchor_width + ZEROPOINT5, padding + radius  + ZEROPOINT5);   // Point 14
  cairo_arc(cr,
            padding + anchor_width + radius + ZEROPOINT5,
            padding + radius + ZEROPOINT5,
            radius,
            180.0f * G_PI / 180.0f,
            270.0f * G_PI / 180.0f);

  cairo_close_path(cr);
}

void ql_compute_mask(cairo_t* cr)
{
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_fill_preserve(cr);
}

void ql_compute_outline(cairo_t* cr,
                        gfloat   line_width,
                        gfloat*  rgba_line,
                        gfloat   size)
{
  cairo_pattern_t* pattern = NULL;
  float            x       = 0.0f;
  float            y       = 0.0f;
  float            offset  = 2.5f * ANCHOR_WIDTH / size;

  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

  pattern = cairo_pattern_create_linear(x, y, size, y);
  cairo_pattern_add_color_stop_rgba(pattern, 0.0f,
                                    rgba_line[0],
                                    rgba_line[1],
                                    rgba_line[2],
                                    rgba_line[3]);
  cairo_pattern_add_color_stop_rgba(pattern, offset,
                                    rgba_line[0],
                                    rgba_line[1],
                                    rgba_line[2],
                                    rgba_line[3]);
  cairo_pattern_add_color_stop_rgba(pattern, 1.1f * offset,
                                    rgba_line[0] * 0.65f,
                                    rgba_line[1] * 0.65f,
                                    rgba_line[2] * 0.65f,
                                    rgba_line[3]);
  cairo_pattern_add_color_stop_rgba(pattern, 1.0f,
                                    rgba_line[0] * 0.65f,
                                    rgba_line[1] * 0.65f,
                                    rgba_line[2] * 0.65f,
                                    rgba_line[3]);
  cairo_set_source(cr, pattern);
  cairo_set_line_width(cr, line_width);
  cairo_stroke(cr);
  cairo_pattern_destroy(pattern);
}

void ql_draw(cairo_t* cr,
             gboolean outline,
             gfloat   line_width,
             gfloat*  rgba,
             gboolean negative,
             gboolean stroke)
{
  // prepare drawing
  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

  // actually draw the mask
  if (outline)
  {
    cairo_set_line_width(cr, line_width);
    cairo_set_source_rgba(cr, rgba[0], rgba[1], rgba[2], rgba[3]);
  }
  else
  {
    if (negative)
      cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 1.0f);
    else
      cairo_set_source_rgba(cr, 0.0f, 0.0f, 0.0f, 0.0f);
  }

  // stroke or fill?
  if (stroke)
    cairo_stroke_preserve(cr);
  else
    cairo_fill_preserve(cr);
}

void ql_finalize(cairo_t** cr,
                 gboolean  outline,
                 gfloat    line_width,
                 gfloat*   rgba,
                 gboolean  negative,
                 gboolean  stroke)
{
  // prepare drawing
  cairo_set_operator(*cr, CAIRO_OPERATOR_SOURCE);

  // actually draw the mask
  if (outline)
  {
    cairo_set_line_width(*cr, line_width);
    cairo_set_source_rgba(*cr, rgba[0], rgba[1], rgba[2], rgba[3]);
  }
  else
  {
    if (negative)
      cairo_set_source_rgba(*cr, 1.0f, 1.0f, 1.0f, 1.0f);
    else
      cairo_set_source_rgba(*cr, 0.0f, 0.0f, 0.0f, 0.0f);
  }

  // stroke or fill?
  if (stroke)
    cairo_stroke(*cr);
  else
    cairo_fill(*cr);
}

void
ql_compute_full_outline_shadow(
  cairo_t* cr,
  cairo_surface_t* surf,
  gint    width,
  gint    height,
  gfloat  anchor_width,
  gfloat  anchor_height,
  gint    upper_size,
  gfloat  corner_radius,
  guint   blur_coeff,
  gfloat* rgba_shadow,
  gfloat  line_width,
  gint    padding_size,
  gfloat* rgba_line)
{
  ql_setup(&surf, &cr, TRUE, width, height, FALSE);
  ql_compute_full_mask_path(cr,
                            anchor_width,
                            anchor_height,
                            width,
                            height,
                            upper_size,
                            corner_radius,
                            padding_size);

  ql_draw(cr, TRUE, line_width, rgba_shadow, FALSE, FALSE);
  nux::CairoGraphics dummy(CAIRO_FORMAT_A1, 1, 1);
  dummy.BlurSurface(blur_coeff, surf);
  ql_compute_mask(cr);
  ql_compute_outline(cr, line_width, rgba_line, width);
}

void ql_compute_full_mask(
  cairo_t* cr,
  cairo_surface_t* surf,
  gint     width,
  gint     height,
  gfloat   radius,
  guint    shadow_radius,
  gfloat   anchor_width,
  gfloat   anchor_height,
  gint     upper_size,
  gboolean negative,
  gboolean outline,
  gfloat   line_width,
  gint     padding_size,
  gfloat*  rgba)
{
  ql_setup(&surf, &cr, outline, width, height, negative);
  ql_compute_full_mask_path(cr,
                            anchor_width,
                            anchor_height,
                            width,
                            height,
                            upper_size,
                            radius,
                            padding_size);
  ql_finalize(&cr, outline, line_width, rgba, negative, outline);
}

void QuicklistView::UpdateTexture()
{
  if (!_cairo_text_has_changed)
    return;

  int size_above_anchor = -1; // equal to size below
  int width = GetBaseWidth();
  int height = GetBaseHeight();

  if (!_enable_quicklist_for_testing)
  {
    if (!_item_list.empty())
    {
      int offscreen_size = GetBaseY() +
                           height -
                           nux::GetWindowThread()->GetGraphicsDisplay().GetWindowHeight();

      if (offscreen_size > 0)
        _top_size = offscreen_size + TOP_SIZE;
      else
        _top_size = TOP_SIZE;

      size_above_anchor = _top_size;
      int x = _anchorX - _padding;
      int y = _anchorY - _anchor_height / 2 - _top_size - _corner_radius - _padding;

      SetBaseX(x);
      SetBaseY(y);
    }
    else
    {
      _top_size = 0;
      size_above_anchor = -1;
      int x = _anchorX - _padding;
      int y = _anchorY - _anchor_height / 2 - _top_size - _corner_radius - _padding;

      SetBaseX(x);
      SetBaseY(y);
    }
  }

  float blur_coef         = 6.0f;

  nux::CairoGraphics cairo_bg(CAIRO_FORMAT_ARGB32, width, height);
  nux::CairoGraphics cairo_mask(CAIRO_FORMAT_ARGB32, width, height);
  nux::CairoGraphics cairo_outline(CAIRO_FORMAT_ARGB32, width, height);

  cairo_t* cr_bg      = cairo_bg.GetContext();
  cairo_t* cr_mask    = cairo_mask.GetContext();
  cairo_t* cr_outline = cairo_outline.GetContext();

  float   tint_color[4]    = {0.0f, 0.0f, 0.0f, 0.60f};
  float   hl_color[4]      = {1.0f, 1.0f, 1.0f, 0.35f};
  float   dot_color[4]     = {1.0f, 1.0f, 1.0f, 0.03f};
  float   shadow_color[4]  = {0.0f, 0.0f, 0.0f, 1.00f};
  float   outline_color[4] = {1.0f, 1.0f, 1.0f, 0.40f};
  float   mask_color[4]    = {1.0f, 1.0f, 1.0f, 1.00f};
  
  if (Settings::Instance().GetLowGfxMode())
  {
    float alpha_value = 1.0f;
    
    tint_color[3] = alpha_value;
    hl_color[3] = 0.2f;
    dot_color[3] = 0.0f;
    shadow_color[3] = alpha_value;
    outline_color[3] = alpha_value;
    mask_color[3] = alpha_value;
  }

  ql_tint_dot_hl(cr_bg,
                 width,
                 height,
                 width / 2.0f,
                 0,
                 nux::Max<float>(width / 1.6f, height / 1.6f),
                 tint_color,
                 hl_color,
                 dot_color);

  ql_compute_full_outline_shadow
  (
    cr_outline,
    cairo_outline.GetSurface(),
    width,
    height,
    _anchor_width,
    _anchor_height,
    size_above_anchor,
    _corner_radius,
    blur_coef,
    shadow_color,
    1.0f,
    _padding,
    outline_color);

  ql_compute_full_mask(
    cr_mask,
    cairo_mask.GetSurface(),
    width,
    height,
    _corner_radius,  // radius,
    16,             // shadow_radius,
    _anchor_width,   // anchor_width,
    _anchor_height,  // anchor_height,
    size_above_anchor,             // upper_size,
    true,           // negative,
    false,          // outline,
    1.0,            // line_width,
    _padding,        // padding_size,
    mask_color);

  cairo_destroy(cr_bg);
  cairo_destroy(cr_outline);
  cairo_destroy(cr_mask);

  texture_bg_ = texture_ptr_from_cairo_graphics(cairo_bg);
  texture_mask_ = texture_ptr_from_cairo_graphics(cairo_mask);
  texture_outline_ = texture_ptr_from_cairo_graphics(cairo_outline);

  _cairo_text_has_changed = false;

  // Request a redraw, so this area will be added to Compiz list of dirty areas.
  NeedRedraw();
}

void QuicklistView::PositionChildLayout(float offsetX, float offsetY)
{
}

void QuicklistView::LayoutWindowElements()
{
}

void QuicklistView::NotifyConfigurationChange(int width, int height)
{
}

void QuicklistView::SetText(std::string const& text)
{
  if (_labelText == text)
    return;

  _labelText = text;
  UpdateTexture();
}

// Introspection

std::string QuicklistView::GetName() const
{
  return "Quicklist";
}

void QuicklistView::AddProperties(GVariantBuilder* builder)
{
  variant::BuilderWrapper(builder)
    .add(GetAbsoluteGeometry())
    .add("base_x", GetBaseX())
    .add("base_y", GetBaseY())
    .add("base", nux::Point(GetBaseX(), GetBaseY()))
    .add("active", IsVisible());
}

//
// Key navigation
//
bool
QuicklistView::InspectKeyEvent(unsigned int eventType,
                               unsigned int keysym,
                               const char* character)
{
  // The Quicklist accepts all key inputs.
  return true;
}

QuicklistMenuItem*
QuicklistView::GetSelectedMenuItem()
{
  return GetNthItems(_current_item_index);
}

debug::Introspectable::IntrospectableList QuicklistView::GetIntrospectableChildren()
{
  _introspectable_children.clear();
  for (auto item: _item_list)
  {
    _introspectable_children.push_back(item);
  }
  return _introspectable_children;
}

} // NAMESPACE
