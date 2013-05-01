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

  nux::Property<int> horizontal_spacing;
  nux::Property<int> vertical_spacing;
  nux::Property<int> padding;

  sigc::signal<void> selection_change;

  int GetSelectedIndex();
  virtual unsigned GetIndexAtPosition(int x, int y);

  virtual void Activate(LocalResult const& local_result, int index, ActivateType type);

  virtual void RenderResultTexture(ResultViewTexture::Ptr const& result_texture);

  virtual void GetResultDimensions(int& rows, int& columns);

protected:
  void MouseMove(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags);
  void MouseClick(int x, int y, unsigned long button_flags, unsigned long key_flags);
  void MouseDoubleClick(int x, int y, unsigned long button_flags, unsigned long key_flags);

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

  virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual long ComputeContentSize();

  virtual void UpdateRenderTextures();

  virtual void AddResult(Result const& result);
  virtual void RemoveResult(Result const& result);

  // This is overridden so we can include position of results.
  virtual debug::ResultWrapper* CreateResultWrapper(Result const& result, int index);
  virtual void UpdateResultWrapper(debug::ResultWrapper* wrapper, Result const& result, int index);

private:
  typedef std::tuple <int, int> ResultListBounds;
  ResultListBounds GetVisableResults();

  void DrawRow(nux::GraphicsEngine& GfxContext, ResultListBounds const& visible_bounds, int row_index, int y_position, nux::Geometry const& absolute_position);

  void QueueLazyLoad();
  void QueueResultsChanged();
  bool DoLazyLoad();

  int GetItemsPerRow();
  void SizeReallocate();
  std::tuple<int, int> GetResultPosition(LocalResult const& local_result);
  std::tuple<int, int> GetResultPosition(const unsigned int& index);

  unsigned mouse_over_index_;
  int active_index_;
  nux::Property<int> selected_index_;
  LocalResult focused_result_;

  LocalResult activated_result_;

  unsigned last_lazy_loaded_result_;
  bool all_results_preloaded_;
  int last_mouse_down_x_;
  int last_mouse_down_y_;
  LocalResult current_drag_result_;
  unsigned drag_index_;

  int recorded_dash_width_;
  int recorded_dash_height_;

  int mouse_last_x_;
  int mouse_last_y_;

  int extra_horizontal_spacing_;

  UBusManager ubus_;
  glib::Source::UniquePtr lazy_load_source_;
  glib::Source::UniquePtr results_changed_idle_;
  nux::Color background_color_;

  glib::Source::UniquePtr activate_timer_;
};

} // namespace dash
} // namespace unity

#endif // UNITYSHELL_RESULTVIEWGRID_H
