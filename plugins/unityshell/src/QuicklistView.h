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

#ifndef QUICKLISTVIEW_H
#define QUICKLISTVIEW_H

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>
#include <NuxGraphics/GraphicsEngine.h>
#include <Nux/TextureArea.h>
#include <NuxImage/CairoGraphics.h>

#include <pango/pango.h>
#include <pango/pangocairo.h>

#include "CairoBaseWindow.h"
#include "QuicklistMenuItem.h"

#include "Introspectable.h"

class QuicklistMenuItem;
class QuicklistMenuItemLabel;

class QuicklistView : public unity::CairoBaseWindow, public unity::debug::Introspectable
{
  NUX_DECLARE_OBJECT_TYPE(QuicklistView, unity::CairoBaseWindow);
public:
  QuicklistView();

  ~QuicklistView();

  void Draw(nux::GraphicsEngine& gfxContext,
            bool             forceDraw);

  void DrawContent(nux::GraphicsEngine& gfxContext,
                   bool             forceDraw);

  void SetText(nux::NString text);

  void RemoveAllMenuItem();

  void AddMenuItem(QuicklistMenuItem* item);

  void RenderQuicklistView();

  void ShowQuicklistWithTipAt(int anchor_tip_x, int anchor_tip_y);
  virtual void ShowWindow(bool b, bool StartModal = false);

  void Show();
  void Hide();

  int GetNumItems();
  QuicklistMenuItem* GetNthItems(int index);
  QuicklistMenuItemType GetNthType(int index);
  std::list<QuicklistMenuItem*> GetChildren();
  void DefaultToFirstItem();

  void TestMenuItems(DbusmenuMenuitem* root);

  // Introspection
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

  void EnableQuicklistForTesting(bool enable_testing);

  // Key navigation
  virtual bool InspectKeyEvent(unsigned int eventType,
                               unsigned int keysym,
                               const char* character);

  //Required for a11y
  QuicklistMenuItem* GetSelectedMenuItem();
  sigc::signal<void> selection_change;

private:
  void RecvCairoTextChanged(QuicklistMenuItem* item);
  void RecvCairoTextColorChanged(QuicklistMenuItem* item);
  void RecvItemMouseClick(QuicklistMenuItem* item, int x, int y);
  void RecvItemMouseRelease(QuicklistMenuItem* item, int x, int y);
  void RecvItemMouseEnter(QuicklistMenuItem* item);
  void RecvItemMouseLeave(QuicklistMenuItem* item);
  void RecvItemMouseDrag(QuicklistMenuItem* item, int x, int y);

  void RecvMouseDown(int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseUp(int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseClick(int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseMove(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseDrag(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseDownOutsideOfQuicklist(int x, int y, unsigned long button_flags, unsigned long key_flags);

  void RecvKeyPressed(unsigned long    eventType  ,   /*event type*/
                      unsigned long    keysym     ,   /*event keysym*/
                      unsigned long    state      ,   /*event state*/
                      const TCHAR*     character  ,   /*character*/
                      unsigned short   keyCount);

  void RecvStartFocus();
  void RecvEndFocus();

  void PreLayoutManagement();

  long PostLayoutManagement(long layoutResult);

  void PositionChildLayout(float offsetX, float offsetY);

  void LayoutWindowElements();

  void NotifyConfigurationChange(int width, int height);

  //! A convenience function to fill in the default quicklist with some random items.
  void FillInDefaultItems();

  void CancelItemsPrelightStatus();

  //! Check the mouse up event sent by an item. Detect the item where the mous is and emit the appropriate signal.
  void CheckAndEmitItemSignal(int x, int y);

  bool IsMenuItemSeperator(int index);

  //nux::CairoGraphics*   _cairo_graphics;
  int                   _anchorX;
  int                   _anchorY;
  nux::NString          _labelText;
  int                   _top_size; // size of the segment from point 13 to 14. See figure in ql_compute_full_mask_path.

  bool                  _mouse_down;

  //iIf true, suppress the Quicklist behaviour that is expected in Unity.
  // Keep the Quicklist on screen for testing and automation.
  bool                  _enable_quicklist_for_testing;

  float _anchor_width;
  float _anchor_height;
  float _corner_radius;
  float _padding;
  float _left_padding_correction;
  float _bottom_padding_correction_normal;
  float _bottom_padding_correction_single_item;
  float _offset_correction;
  nux::HLayout* _hlayout;
  nux::VLayout* _vlayout;
  nux::VLayout* _item_layout;
  nux::VLayout* _default_item_layout;
  nux::SpaceLayout* _left_space;  //!< Space from the left of the widget to the left of the text.
  nux::SpaceLayout* _right_space; //!< Space from the right of the text to the right of the widget.
  nux::SpaceLayout* _top_space;  //!< Space from the left of the widget to the left of the text.
  nux::SpaceLayout* _bottom_space; //!< Space from the right of the text to the right of the widget.

  bool _cairo_text_has_changed;
  void UpdateTexture();
  std::list<QuicklistMenuItem*> _item_list;
  std::list<QuicklistMenuItem*> _default_item_list;

  // used by keyboard/a11y-navigation
  int _current_item_index;
};

#endif // QUICKLISTVIEW_H

