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

#include "unity-shared/CairoTexture.h"

#include "QuicklistView.h"
#include "QuicklistMenuItem.h"
#include "QuicklistMenuItemLabel.h"
#include "QuicklistMenuItemSeparator.h"
#include "QuicklistMenuItemCheckmark.h"
#include "QuicklistMenuItemRadio.h"

#include "unity-shared/Introspectable.h"
#include "unity-shared/PanelStyle.h"
#include "unity-shared/DecorationStyle.h"
#include "unity-shared/UnitySettings.h"
#include "unity-shared/UScreen.h"

#include "unity-shared/UBusWrapper.h"
#include "unity-shared/UBusMessages.h"
#include "unity-shared/DashStyle.h"

namespace unity
{
namespace
{
  const RawPixel ANCHOR_WIDTH  = 10_em;
  const RawPixel TOP_SIZE      =  4_em;

  const RawPixel ANCHOR_HEIGHT = 18_em;
  const RawPixel CORNER_RADIUS =  4_em;
  const RawPixel MAX_HEIGHT  = 1000_em;
  const RawPixel MAX_WIDTH   = 1000_em;

  const RawPixel LEFT_PADDING_CORRECTION = -1_em;
  const RawPixel OFFSET_CORRECTION = -1_em;
}

NUX_IMPLEMENT_OBJECT_TYPE(QuicklistView);

QuicklistView::QuicklistView(int monitor)
  : CairoBaseWindow(monitor)
  , _anchorX(0)
  , _anchorY(0)
  , _labelText("QuicklistView 1234567890")
  , _top_size(TOP_SIZE)
  , _padding(decoration::Style::Get()->ActiveShadowRadius())
  , _mouse_down(false)
  , _enable_quicklist_for_testing(false)
  , _restore_input_focus(false)
  , _cairo_text_has_changed(true)
  , _current_item_index(-1)
{
  SetGeometry(nux::Geometry(0, 0, 1, 1));

  int width = 0;
  int height = 0;
  // when launcher is on the left, the anchor is on the left of the menuitem, and
  // when launcher is on the bottom, the anchor is on the bottom of the menuitem.
  if (Settings::Instance().launcher_position == LauncherPosition::LEFT)
    width = ANCHOR_WIDTH;
  else
    height = ANCHOR_WIDTH;
  _left_space = new nux::SpaceLayout(RawPixel(_padding + width + CORNER_RADIUS + LEFT_PADDING_CORRECTION).CP(cv_),
                                     RawPixel(_padding + width + CORNER_RADIUS + LEFT_PADDING_CORRECTION).CP(cv_),
                                     1, MAX_HEIGHT.CP(cv_));

  _right_space = new nux::SpaceLayout(_padding.CP(cv_) + CORNER_RADIUS.CP(cv_),
                                      _padding.CP(cv_) + CORNER_RADIUS.CP(cv_),
                                      1, MAX_HEIGHT.CP(cv_));

  _top_space = new nux::SpaceLayout(1, MAX_WIDTH.CP(cv_),
                                    _padding.CP(cv_) + CORNER_RADIUS.CP(cv_),
                                    _padding.CP(cv_) + CORNER_RADIUS.CP(cv_));

  _bottom_space = new nux::SpaceLayout(1, MAX_WIDTH.CP(cv_),
                                       _padding.CP(cv_) + height + CORNER_RADIUS.CP(cv_),
                                       _padding.CP(cv_) + height + CORNER_RADIUS.CP(cv_));

  _vlayout = new nux::VLayout(TEXT(""), NUX_TRACKER_LOCATION);
  _vlayout->AddLayout(_top_space, 0);

  _item_layout     = new nux::VLayout(TEXT(""), NUX_TRACKER_LOCATION);
  _vlayout->AddLayout(_item_layout, 0);

  _vlayout->AddLayout(_bottom_space, 0);
  _vlayout->SetMinimumWidth(RawPixel(140).CP(cv_));

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

int QuicklistView::CalculateX() const
{
  int x = 0;
  if (Settings::Instance().launcher_position() == LauncherPosition::LEFT)
    x = _anchorX - _padding.CP(cv_);
  else
  {
    int size = 0;
    int max = GetBaseWidth() - ANCHOR_HEIGHT.CP(cv_) - 2 * CORNER_RADIUS.CP(cv_) - 2 * _padding.CP(cv_);
    if (_top_size.CP(cv_) > max)
    {
      size = max;
    }
    else if (_top_size.CP(cv_) > 0)
    {
      size = _top_size.CP(cv_);
    }
    x = _anchorX - (ANCHOR_HEIGHT.CP(cv_) / 2) - size - CORNER_RADIUS.CP(cv_) - _padding.CP(cv_);
  }

  return x;
}

int QuicklistView::CalculateY() const
{
  int y = 0;
  if (Settings::Instance().launcher_position() == LauncherPosition::LEFT)
    y = _anchorY - (ANCHOR_HEIGHT.CP(cv_) / 2) - _top_size.CP(cv_) - CORNER_RADIUS.CP(cv_) - _padding.CP(cv_);
  else
    y = _anchorY - GetBaseHeight() + _padding.CP(cv_);
  return y;
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

      if (Settings::Instance().launcher_position() == LauncherPosition::BOTTOM)
      {
        PromptHide();
        UBusManager::SendMessage(UBUS_QUICKLIST_END_KEY_NAV);
        UBusManager::SendMessage(UBUS_LAUNCHER_PREV_KEY_NAV);
        UBusManager::SendMessage(UBUS_LAUNCHER_OPEN_QUICKLIST);
      }
      else
      {
        HideAndEndQuicklistNav();
      }

      break;

      // right (close quicklist, go back to launcher key-nav)
    case NUX_VK_RIGHT:
    case NUX_KP_RIGHT:
      if (Settings::Instance().launcher_position() == LauncherPosition::BOTTOM)
      {
        PromptHide();
        UBusManager::SendMessage(UBUS_QUICKLIST_END_KEY_NAV);
        UBusManager::SendMessage(UBUS_LAUNCHER_NEXT_KEY_NAV);
        UBusManager::SendMessage(UBUS_LAUNCHER_OPEN_QUICKLIST);
      }

      break;

      // esc (close quicklist, exit key-nav)
    case NUX_VK_ESCAPE:
      Hide();
      // inform UnityScreen we leave key-nav completely
      UBusManager::SendMessage(UBUS_LAUNCHER_END_KEY_NAV, glib::Variant(_restore_input_focus));
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

void
QuicklistView::EnableQuicklistForTesting(bool enable_testing)
{
  _enable_quicklist_for_testing = enable_testing;
}

void QuicklistView::SetQuicklistPosition(int tip_x, int tip_y)
{
  _anchorX = tip_x;
  _anchorY = tip_y;

  if (!_enable_quicklist_for_testing)
  {
    if (!_item_list.empty())
    {
      auto* us = UScreen::GetDefault();
      int ql_monitor = us->GetMonitorAtPosition(_anchorX, _anchorY);
      auto const& ql_monitor_geo = us->GetMonitorGeometry(ql_monitor);
      auto launcher_position = Settings::Instance().launcher_position();

      if (launcher_position == LauncherPosition::LEFT)
      {
        int offscreen_size = GetBaseY() + GetBaseHeight() - (ql_monitor_geo.y + ql_monitor_geo.height);
        if (offscreen_size > 0)
          _top_size = offscreen_size + TOP_SIZE;
        else
          _top_size = TOP_SIZE;
      }
      else
      {
        int offscreen_size_left = ql_monitor_geo.x - (_anchorX - GetBaseWidth() / 2);
        int offscreen_size_right = _anchorX + GetBaseWidth()/2 - (ql_monitor_geo.x + ql_monitor_geo.width);
        int half_size = (GetBaseWidth() / 2) - _padding.CP(cv_) - CORNER_RADIUS.CP(cv_) - (ANCHOR_HEIGHT.CP(cv_) / 2);

        if (offscreen_size_left > 0)
          _top_size = half_size - offscreen_size_left;
        else if (offscreen_size_right > 0)
          _top_size = half_size + offscreen_size_right;
        else
          _top_size = half_size;
      }

      SetXY(CalculateX(), CalculateY());
    }
    else
    {
      _top_size = 0;
      SetXY(CalculateX(), CalculateY());
    }
  }
}

void QuicklistView::ShowQuicklistWithTipAt(int x, int y, bool restore_input_focus)
{
  SetQuicklistPosition(x, y);
  Show(restore_input_focus);
}

void QuicklistView::Show(bool restore_input_focus)
{
  if (!IsVisible())
  {
    _restore_input_focus = restore_input_focus;
    CairoBaseWindow::Show();
    GrabPointer();
    GrabKeyboard();
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
    CairoBaseWindow::Hide();

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
  CairoBaseWindow::Draw(gfxContext, forceDraw);

  nux::Geometry base(GetGeometry());
  base.x = 0;
  base.y = 0;

  gfxContext.PushClippingRectangle(base);

  for (auto const& item : _item_list)
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

  for (auto const& item : _item_list)
  {
    // Make sure item is in layout if it should be
    if (!item->GetVisible())
    {
      _item_layout->RemoveChildObject(item.GetPointer());
      continue;
    }
    else if (!item->GetParentObject())
    {
      _item_layout->AddView(item.GetPointer(), 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
    }

    nux::Size const& text_extents = item->GetTextExtents();
    MaxItemWidth = std::max(MaxItemWidth, text_extents.width);
    TotalItemHeight += text_extents.height;
  }

  int rotated_anchor_height = 0;
  if (Settings::Instance().launcher_position() == LauncherPosition::BOTTOM)
    rotated_anchor_height = ANCHOR_WIDTH;

  if (TotalItemHeight < ANCHOR_HEIGHT.CP(cv_))
  {
    int b = (ANCHOR_HEIGHT.CP(cv_) - TotalItemHeight) / 2 + _padding.CP(cv_) + CORNER_RADIUS.CP(cv_) + rotated_anchor_height;
    int t = b + OFFSET_CORRECTION.CP(cv_) - rotated_anchor_height;

    _top_space->SetMinimumHeight(t);
    _top_space->SetMaximumHeight(t);

    _bottom_space->SetMinimumHeight(b);
    _bottom_space->SetMaximumHeight(b);
  }
  else
  {
    int b = _padding.CP(cv_) + CORNER_RADIUS.CP(cv_) + rotated_anchor_height;
    int t = b + OFFSET_CORRECTION.CP(cv_) - rotated_anchor_height;

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

  int width = 0;
  if (Settings::Instance().launcher_position() == LauncherPosition::LEFT)
    width = ANCHOR_WIDTH;
  int x = RawPixel(_padding + width + CORNER_RADIUS + OFFSET_CORRECTION).CP(cv_);
  int y = _top_space->GetMinimumHeight();

  for (auto const& item : _item_list)
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

  for (auto const& item : _item_list)
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
  for (auto const& item : _item_list)
  {
    if (!item->GetVisible())
      continue;

    geo = item->GetGeometry();
    geo.width = _item_layout->GetBaseWidth();

    if (geo.IsPointInside(x, y))
    {
      // An action is performed: send the signal back to the application
      ActivateItem(item.GetPointer());
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
  for (auto const& item : _item_list)
    item->Select(false);
}

void QuicklistView::RecvItemMouseDrag(QuicklistMenuItem* item, int x, int y)
{
  nux::Geometry geo;
  for (auto const& it : _item_list)
  {
    int item_index = GetItemIndex(it.GetPointer());

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

nux::Area* QuicklistView::FindAreaUnderMouse(const nux::Point& mouse_position, nux::NuxEventType event_type)
{
  auto launcher_position = Settings::Instance().launcher_position();
  if ((launcher_position == LauncherPosition::LEFT && (mouse_position.x > _anchorX)) ||
      (launcher_position == LauncherPosition::BOTTOM && (mouse_position.y < _anchorY)))
  {
    return (CairoBaseWindow::FindAreaUnderMouse(mouse_position, event_type));
  }

  return nullptr;
}

void QuicklistView::RemoveAllMenuItem()
{
  _item_layout->Clear();
  _item_list.clear();
  _cairo_text_has_changed = true;
  QueueRelayout();
}

void QuicklistView::AddMenuItem(QuicklistMenuItem* item)
{
  if (!item)
    return;

  item->sigTextChanged.connect(sigc::mem_fun(this, &QuicklistView::RecvCairoTextChanged));
  item->sigColorChanged.connect(sigc::mem_fun(this, &QuicklistView::RecvCairoTextColorChanged));
  item->sigMouseClick.connect(sigc::mem_fun(this, &QuicklistView::RecvItemMouseClick));
  item->sigMouseReleased.connect(sigc::mem_fun(this, &QuicklistView::RecvItemMouseRelease));
  item->sigMouseEnter.connect(sigc::mem_fun(this, &QuicklistView::RecvItemMouseEnter));
  item->sigMouseLeave.connect(sigc::mem_fun(this, &QuicklistView::RecvItemMouseLeave));
  item->sigMouseDrag.connect(sigc::mem_fun(this, &QuicklistView::RecvItemMouseDrag));
  item->SetScale(cv_->DPIScale());

  _item_list.push_back(QuicklistMenuItem::Ptr(item));

  _cairo_text_has_changed = true;
  QueueRelayout();
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
    for (auto const& item : _item_list)
    {
      if (i++ == index)
        return item.GetPointer();
    }
  }

  return nullptr;
}

int QuicklistView::GetItemIndex(QuicklistMenuItem* item)
{
  int index = -1;

  for (auto const& it : _item_list)
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

std::list<QuicklistMenuItem::Ptr> QuicklistView::GetChildren()
{
  return _item_list;
}

void QuicklistView::SelectFirstItem()
{
  SelectItem(0);
}

void ql_tint_dot_hl(cairo_t* cr,
                    gfloat  scale,
                    gint    width,
                    gint    height,
                    gfloat  hl_x,
                    gfloat  hl_y,
                    gfloat  hl_size,
                    nux::Color const& tint_color,
                    nux::Color const& hl_color,
                    nux::Color const& dot_color)
{
  cairo_pattern_t* dots_pattern = NULL;
  cairo_pattern_t* hl_pattern   = NULL;

  // create context for dot-pattern
  nux::CairoGraphics dots_surf(CAIRO_FORMAT_ARGB32, 4 * scale, 4 * scale);
  cairo_surface_set_device_scale(dots_surf.GetSurface(), scale, scale);
  cairo_t* dots_cr = dots_surf.GetInternalContext();

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
                        tint_color.red,
                        tint_color.green,
                        tint_color.blue,
                        tint_color.alpha);
  cairo_fill_preserve(cr);

  // create pattern in dot-context
  cairo_set_operator(dots_cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(dots_cr);
  cairo_scale(dots_cr, 1.0f, 1.0f);
  cairo_set_operator(dots_cr, CAIRO_OPERATOR_OVER);
  cairo_set_source_rgba(dots_cr,
                        dot_color.red,
                        dot_color.green,
                        dot_color.blue,
                        dot_color.alpha);
  cairo_rectangle(dots_cr, 0.0f, 0.0f, 1.0f, 1.0f);
  cairo_fill(dots_cr);
  cairo_rectangle(dots_cr, 2.0f, 2.0f, 1.0f, 1.0f);
  cairo_fill(dots_cr);
  dots_pattern = cairo_pattern_create_for_surface(dots_surf.GetSurface());

  // fill path of normal context with dot-pattern
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  cairo_set_source(cr, dots_pattern);
  cairo_pattern_set_extend(dots_pattern, CAIRO_EXTEND_REPEAT);
  cairo_fill_preserve(cr);
  cairo_pattern_destroy(dots_pattern);

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
                                    hl_color.red,
                                    hl_color.green,
                                    hl_color.blue,
                                    hl_color.alpha);
  cairo_pattern_add_color_stop_rgba(hl_pattern, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f);
  cairo_set_source(cr, hl_pattern);
  cairo_fill(cr);
  cairo_pattern_destroy(hl_pattern);
}

void ql_setup(cairo_surface_t** surf,
              cairo_t**         cr,
              gboolean          outline,
              gboolean          negative)
{
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
                               gfloat   width,
                               gfloat   height,
                               gint     upper_size,
                               gfloat   radius,
                               guint    pad)
{
  //  On the right of the icon:   On the top of the icon:
  //     0  1        2  3            0  1           2  3
  //     +--+--------+--+            +--+-----------+--+
  //     |              |            |                 |
  //     + 14           + 4       14 +                 + 4
  //     |              |            |                 |
  //     |              |            |                 |
  //     |              |            |                 |
  //     + 13           |            |                 |
  //    /               |            |                 |
  //   /                |            |                 |
  //  + 12              |            |                 |
  //   \                |            |                 |
  //    \               |            |                 |
  //  11 +              |            |                 |
  //     |              |         13 +                 + 5
  //     |              |            |     10    8     |
  //     |              |         12 +--+--+     +--+--+ 6
  //  10 +              + 5             11  \   /   7
  //     |              |                    \ /
  //     +--+--------+--+ 6                   +
  //     9  8        7                        9


  gfloat padding  = pad;
  int ZEROPOINT5 = 0.0f;
  auto launcher_position = Settings::Instance().launcher_position();

  //gint dynamic_size = height - 2*radius - 2*padding - anchor_height;
  //gint upper_dynamic_size = upper_size;
  //gint lower_dynamic_size = dynamic_size - upper_dynamic_size;

  int size = 0;
  if (launcher_position == LauncherPosition::LEFT)
    size = height;
  else
    size = width;

  gfloat HeightToAnchor = ((gfloat) size - 2.0f * radius - anchor_height - 2 * padding) / 2.0f;
  if (HeightToAnchor < 0.0f)
  {
    g_warning("Anchor-height and corner-radius a higher than whole texture!");
    return;
  }

  if (upper_size >= 0)
  {
    if (upper_size > size - 2.0f * radius - anchor_height - 2 * padding)
    {
      //g_warning ("[_compute_full_mask_path] incorrect upper_size value");
      HeightToAnchor = 0;
    }
    else
    {
      HeightToAnchor = size - 2.0f * radius - anchor_height - 2 * padding - upper_size;
    }
  }
  else
  {
    if (launcher_position == LauncherPosition::LEFT)
      HeightToAnchor = (size - 2.0f * radius - anchor_height - 2 * padding) / 2.0f;
    else
      HeightToAnchor = size - 2.0f * radius - anchor_height - 2 * padding;
  }

  cairo_translate(cr, -0.5f, -0.5f);

  // create path
  if (launcher_position == LauncherPosition::LEFT)
  {
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
  }
  else
  {
    cairo_move_to(cr, padding + radius + ZEROPOINT5, padding + ZEROPOINT5);  // Point 1
    cairo_line_to(cr, width - padding - radius, padding + ZEROPOINT5);    // Point 2
    cairo_arc(cr,
              width  - padding - radius + ZEROPOINT5,
              padding + radius + ZEROPOINT5,
              radius,
              -90.0f * G_PI / 180.0f,
              0.0f * G_PI / 180.0f);   // Point 4
    cairo_line_to(cr,
                  (gdouble) width - padding + ZEROPOINT5,
                  (gdouble) height - radius - anchor_width - padding + ZEROPOINT5); // Point 5
    cairo_arc(cr,
              (gdouble) width - padding - radius + ZEROPOINT5,
              (gdouble) height - padding - anchor_width - radius + ZEROPOINT5,
              radius,
              0.0f * G_PI / 180.0f,
              90.0f * G_PI / 180.0f);  // Point 7
    cairo_line_to(cr,
                  (gdouble) width - padding - radius - HeightToAnchor + ZEROPOINT5,
                  height - padding - anchor_width + ZEROPOINT5);   // Point 8
    cairo_line_to(cr,
                  (gdouble) width - padding - radius - HeightToAnchor - anchor_height / 2.0f + ZEROPOINT5,
                  height - padding + ZEROPOINT5); // Point 9
    cairo_line_to(cr,
                  (gdouble) width - padding - radius - HeightToAnchor - anchor_height + ZEROPOINT5,
                  height - padding - anchor_width + ZEROPOINT5);  // Point 10
    cairo_arc(cr,
              padding + radius + ZEROPOINT5,
              (gdouble) height - padding - anchor_width - radius,
              radius,
              90.0f * G_PI / 180.0f,
              180.0f * G_PI / 180.0f); // Point 11
    cairo_line_to(cr,
                  padding + ZEROPOINT5,
                  (gdouble) height - padding -anchor_width - radius + ZEROPOINT5); // Point 13
    cairo_line_to(cr, padding + ZEROPOINT5, padding + radius  + ZEROPOINT5);   // Point 14
    cairo_arc(cr,
              padding + radius + ZEROPOINT5,
              padding + radius + ZEROPOINT5,
              radius,
              180.0f * G_PI / 180.0f,
              270.0f * G_PI / 180.0f);
  }
  cairo_close_path(cr);
}

void ql_compute_mask(cairo_t* cr)
{
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_fill_preserve(cr);
}

void ql_compute_outline(cairo_t* cr,
                        gfloat   line_width,
                        nux::Color const& line_color,
                        gfloat   size)
{
  cairo_pattern_t* pattern = NULL;
  float            x       = 0.0f;
  float            y       = 0.0f;
  float            offset  = 2.5f * ANCHOR_WIDTH / size;

  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

  pattern = cairo_pattern_create_linear(x, y, size, y);
  cairo_pattern_add_color_stop_rgba(pattern, 0.0f,
                                    line_color.red,
                                    line_color.green,
                                    line_color.blue,
                                    line_color.alpha);
  cairo_pattern_add_color_stop_rgba(pattern, offset,
                                    line_color.red,
                                    line_color.green,
                                    line_color.blue,
                                    line_color.alpha);
  cairo_pattern_add_color_stop_rgba(pattern, 1.1f * offset,
                                    line_color.red * 0.65f,
                                    line_color.green * 0.65f,
                                    line_color.blue * 0.65f,
                                    line_color.alpha);
  cairo_pattern_add_color_stop_rgba(pattern, 1.0f,
                                    line_color.red * 0.65f,
                                    line_color.green * 0.65f,
                                    line_color.blue * 0.65f,
                                    line_color.alpha);
  cairo_set_source(cr, pattern);
  cairo_set_line_width(cr, line_width);
  cairo_stroke(cr);
  cairo_pattern_destroy(pattern);
}

void ql_draw(cairo_t* cr,
             gboolean outline,
             gfloat   line_width,
             nux::Color const& color,
             gboolean negative,
             gboolean stroke)
{
  // prepare drawing
  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

  // actually draw the mask
  if (outline)
  {
    cairo_set_line_width(cr, line_width);
    cairo_set_source_rgba(cr, color.red, color.green, color.blue, color.alpha);
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
                 nux::Color const& color,
                 gboolean  negative,
                 gboolean  stroke)
{
  // prepare drawing
  cairo_set_operator(*cr, CAIRO_OPERATOR_SOURCE);

  // actually draw the mask
  if (outline)
  {
    cairo_set_line_width(*cr, line_width);
    cairo_set_source_rgba(*cr, color.red, color.green, color.blue, color.alpha);
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
  gfloat  width,
  gfloat  height,
  gfloat  anchor_width,
  gfloat  anchor_height,
  gint    upper_size,
  gfloat  corner_radius,
  guint   blur_coeff,
  nux::Color const& rgba_shadow,
  gfloat  line_width,
  gint    padding_size,
  nux::Color const& rgba_line)
{
  ql_setup(&surf, &cr, TRUE, FALSE);
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
  gfloat   width,
  gfloat   height,
  gfloat   radius,
  gfloat   anchor_width,
  gfloat   anchor_height,
  gint     upper_size,
  gboolean negative,
  gboolean outline,
  gfloat   line_width,
  gint     padding_size,
  nux::Color const&  rgba)
{
  ql_setup(&surf, &cr, outline, negative);
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

  SetQuicklistPosition(_anchorX, _anchorY);

  RawPixel size_above_anchor = _item_list.empty() ? RawPixel(-1) : _top_size;
  int width = GetBaseWidth();
  int height = GetBaseHeight();

  auto const& deco_style = decoration::Style::Get();
  float dpi_scale = cv_->DPIScale();
  float blur_coef = std::round(deco_style->ActiveShadowRadius() * dpi_scale / 2.0f);

  nux::CairoGraphics cairo_bg(CAIRO_FORMAT_ARGB32, width, height);
  nux::CairoGraphics cairo_mask(CAIRO_FORMAT_ARGB32, width, height);
  nux::CairoGraphics cairo_outline(CAIRO_FORMAT_ARGB32, width, height);

  cairo_surface_set_device_scale(cairo_bg.GetSurface(), dpi_scale, dpi_scale);
  cairo_surface_set_device_scale(cairo_mask.GetSurface(), dpi_scale, dpi_scale);
  cairo_surface_set_device_scale(cairo_outline.GetSurface(), dpi_scale, dpi_scale);

  cairo_t* cr_bg      = cairo_bg.GetInternalContext();
  cairo_t* cr_mask    = cairo_mask.GetInternalContext();
  cairo_t* cr_outline = cairo_outline.GetInternalContext();

  nux::Color tint_color(0.0f, 0.0f, 0.0f, HasBlurredBackground() ? 0.60f : 1.0f);
  nux::Color hl_color(1.0f, 1.0f, 1.0f, 0.35f);
  nux::Color dot_color(1.0f, 1.0f, 1.0f, 0.03f);
  nux::Color shadow_color(deco_style->ActiveShadowColor());
  nux::Color outline_color(1.0f, 1.0f, 1.0f, 0.40f);
  nux::Color mask_color(1.0f, 1.0f, 1.0f, 1.00f);

  ql_tint_dot_hl(cr_bg,
                 dpi_scale,
                 width / dpi_scale,
                 height / dpi_scale,
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
    width / dpi_scale,
    height / dpi_scale,
    ANCHOR_WIDTH,
    ANCHOR_HEIGHT,
    size_above_anchor,
    CORNER_RADIUS,
    blur_coef,
    shadow_color,
    1.0f * dpi_scale,
    _padding,
    outline_color);

  ql_compute_full_mask(
    cr_mask,
    cairo_mask.GetSurface(),
    width / dpi_scale,
    height / dpi_scale,
    CORNER_RADIUS,             // radius,
    ANCHOR_WIDTH,              // anchor_width,
    ANCHOR_HEIGHT,             // anchor_height,
    size_above_anchor,         // upper_size,
    true,                      // negative,
    false,                     // outline,
    1.0,                       // line_width,
    _padding,                  // padding_size,
    mask_color);

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

void QuicklistView::AddProperties(debug::IntrospectionData& introspection)
{
  introspection
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
  debug::Introspectable::IntrospectableList list(_item_list.size());

  for (auto const& item: _item_list)
    list.push_back(item.GetPointer());

  return list;
}

} // NAMESPACE
