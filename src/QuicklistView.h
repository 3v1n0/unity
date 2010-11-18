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

#include "Nux/Nux.h"
#include "Nux/BaseWindow.h"
#include "NuxGraphics/GraphicsEngine.h"
#include "Nux/TextureArea.h"
#include "NuxImage/CairoGraphics.h"
#include "StaticCairoText.h"

#include <pango/pango.h>
#include <pango/pangocairo.h>

#if defined(NUX_OS_LINUX)
#include <X11/Xlib.h>
#endif

#define ANCHOR_WIDTH         10.0f
#define ANCHOR_HEIGHT        18.0f
#define HIGH_LIGHT_Y         -30.0f
#define HIGH_LIGHT_MIN_WIDTH 200.0f
#define RADIUS               5.0f
#define BLUR_INTENSITY       8
#define LINE_WIDTH           1.0f
#define PADDING_SIZE         1
#define H_MARGIN             30
#define V_MARGIN             4
#define FONT_FACE            "Ubuntu 13"


class VLayout;
class HLayout;
class SpaceLayout;
class QuicklistMenuItem;

class QuicklistView : public nux::BaseWindow
{
  NUX_DECLARE_OBJECT_TYPE (QuicklistView, nux::BaseWindow);
public:
  QuicklistView ();

  ~QuicklistView ();

  long ProcessEvent (nux::IEvent& iEvent,
    long    traverseInfo,
    long    processEventInfo);

  void Draw (nux::GraphicsEngine& gfxContext,
    bool             forceDraw);

  void DrawContent (nux::GraphicsEngine& gfxContext,
    bool             forceDraw);

  void SetText (nux::NString text);
  
  void RemoveAllMenuItem ();
  void AddMenuItem (nux::NString str);
  void RenderQuicklistView ();

  void ShowQuicklistWithTipAt (int anchor_tip_x, int anchor_tip_y);
  virtual void ShowWindow (bool b, bool StartModal = false);
  
  int GetNumItems ();
  QuicklistMenuItem* GetNthItems (int index);
  QuicklistMenuItem* GetNthType  (int index);
  std::list<QuicklistMenuItem*> GetChildren ();
  
private:
  void RecvCairoTextChanged (nux::StaticCairoText& cairo_text);
  void RecvCairoTextColorChanged (nux::StaticCairoText& cairo_text);
  
  void RecvMouseDown (int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseUp (int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseClick (int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseMove (int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseDrag (int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseDownOutsideOfQuicklist (int x, int y, unsigned long button_flags, unsigned long key_flags);

  void PreLayoutManagement ();

  long PostLayoutManagement (long layoutResult);

  void PositionChildLayout (float offsetX, float offsetY);

  void LayoutWindowElements ();

  void NotifyConfigurationChange (int width, int height);

  //nux::CairoGraphics*   _cairo_graphics;
  int                   _anchorX;
  int                   _anchorY;
  nux::NString          _labelText;
  int                   _dpiX;
  int                   _dpiY;
  int                   _top_size; // size of the segment from point 13 to 14. See figure in ql_compute_full_mask_path.

  cairo_font_options_t* _fontOpts;

  nux::BaseTexture*     _texture_bg;
  nux::BaseTexture*     _texture_mask;
  nux::BaseTexture*     _texture_outline;

  float _anchor_width;
  float _anchor_height;
  float _corner_radius;
  float _padding;
  nux::HLayout *_hlayout;
  nux::VLayout *_vlayout;
  nux::VLayout *_item_layout;
  nux::VLayout *_default_item_layout;
  nux::SpaceLayout *_left_space;  //!< Space from the left of the widget to the left of the text.
  nux::SpaceLayout *_right_space; //!< Space from the right of the text to the right of the widget.
  nux::SpaceLayout *_top_space;  //!< Space from the left of the widget to the left of the text.
  nux::SpaceLayout *_bottom_space; //!< Space from the right of the text to the right of the widget.

  bool _cairo_text_has_changed;
  void UpdateTexture ();
  std::list<nux::StaticCairoText*> _item_list;
  std::list<nux::StaticCairoText*> _default_item_list;
};

#endif // QUICKLISTVIEW_H

