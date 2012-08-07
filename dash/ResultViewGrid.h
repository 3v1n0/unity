// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright 2011 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 *
 */

#ifndef UNITYSHELL_RESULTVIEWGRID_H
#define UNITYSHELL_RESULTVIEWGRID_H

#include <UnityCore/Categories.h>
#include <UnityCore/GLibSource.h>

#include "ResultView.h"
#include "unity-shared/UBusWrapper.h"

namespace unity
{
namespace dash
{

// will order its elements in to a grid that expands horizontally before vertically
class ResultViewGrid : public ResultView
{
public:
  NUX_DECLARE_OBJECT_TYPE(ResultViewGrid, ResultView);

  ResultViewGrid(NUX_FILE_LINE_DECL);

  void SetModelRenderer(ResultRenderer* renderer);
  void AddResult(Result& result);
  void RemoveResult(Result& result);

  nux::Property<int> horizontal_spacing;
  nux::Property<int> vertical_spacing;
  nux::Property<int> padding;

  sigc::signal<void> selection_change;

  int GetSelectedIndex();
  virtual uint GetIndexAtPosition(int x, int y);

protected:
  void MouseMove(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags);
  void MouseClick(int x, int y, unsigned long button_flags, unsigned long key_flags);

  virtual bool                    DndSourceDragBegin();
  virtual nux::NBitmapData*       DndSourceGetDragImage();
  virtual std::list<const char*>  DndSourceGetDragTypes();
  virtual const char*             DndSourceGetDataForType(const char* type, int* size, int* format);
  virtual void                    DndSourceDragFinished(nux::DndAction result);

  virtual bool InspectKeyEvent(unsigned int eventType, unsigned int keysym, const char* character);
  virtual bool AcceptKeyNavFocus();
  virtual nux::Area* KeyNavIteration(nux::KeyNavDirection direction);
  virtual void OnKeyNavFocusChange(nux::Area* area, bool has_focus, nux::KeyNavDirection direction);
  void OnKeyDown(unsigned long event_type, unsigned long event_keysym, unsigned long event_state, const TCHAR* character, unsigned short key_repeat_count);

  virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);;
  virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual long ComputeContentSize();

private:
  typedef std::tuple <int, int> ResultListBounds;
  ResultListBounds GetVisableResults();

  void QueueLazyLoad();
  void QueueViewChanged();
  bool DoLazyLoad();

  int GetItemsPerRow();
  void SizeReallocate();
  std::tuple<int, int> GetResultPosition(const std::string& uri);
  std::tuple<int, int> GetResultPosition(const unsigned int& index);

  uint mouse_over_index_;
  int active_index_;
  nux::Property<int> selected_index_;
  std::string focused_uri_;

  int last_lazy_loaded_result_;
  int last_mouse_down_x_;
  int last_mouse_down_y_;
  std::string current_drag_uri_;
  std::string current_drag_icon_name_;

  int recorded_dash_width_;
  int recorded_dash_height_;

  int mouse_last_x_;
  int mouse_last_y_;

  int extra_horizontal_spacing_;

  UBusManager ubus_;
  glib::Source::UniquePtr lazy_load_source_;
  glib::Source::UniquePtr view_changed_idle_;
};

} // namespace dash
} // namespace unity

#endif // UNITYSHELL_RESULTVIEWGRID_H
