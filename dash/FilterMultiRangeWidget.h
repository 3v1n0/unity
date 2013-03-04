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

#ifndef UNITYSHELL_FILTERMULTIRANGE_H
#define UNITYSHELL_FILTERMULTIRANGE_H

#include <Nux/Nux.h>
#include <Nux/HLayout.h>
#include <Nux/VLayout.h>
#include <UnityCore/MultiRangeFilter.h>

#include "FilterAllButton.h"
#include "FilterBasicButton.h"
#include "FilterExpanderLabel.h"

namespace unity
{
namespace dash
{

class FilterMultiRangeButton;

class FilterMultiRangeWidget : public FilterExpanderLabel
{
  NUX_DECLARE_OBJECT_TYPE(FilterMultiRangeWidget, FilterExpanderLabel);
  typedef nux::ObjectPtr<FilterMultiRangeButton> FilterMultiRangeButtonPtr;
public:
  FilterMultiRangeWidget(NUX_FILE_LINE_PROTO);

  void SetFilter(Filter::Ptr const& filter);
  std::string GetFilterType();

protected:
  void InitTheme();

  nux::Area* FindAreaUnderMouse(const nux::Point& mouse_position, nux::NuxEventType event_type);

  void RecvMouseMove(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseUp(int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseDown(int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseDrag(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags);

  void ClearRedirectedRenderChildArea();

private:
  void OnAllActivated(nux::View* view);
  void OnOptionAdded(dash::FilterOption::Ptr const& new_filter);
  void OnOptionRemoved(dash::FilterOption::Ptr const& removed_filter);
  void OnActiveChanged(bool value);

  void UpdateMouseFocus(nux::Point const& abs_cursor_position);
  virtual void Click(FilterMultiRangeButtonPtr const& button);

  bool CheckDrag();

  nux::HLayout* layout_;
  FilterAllButton* all_button_;

  std::vector<FilterMultiRangeButtonPtr> buttons_;
  MultiRangeFilter::Ptr filter_;

  FilterMultiRangeButtonPtr mouse_down_button_;
  FilterMultiRangeButtonPtr mouse_down_left_active_button_;
  FilterMultiRangeButtonPtr mouse_down_right_active_button_;
  bool dragging_;

  friend class TestFilterMultiRangeWidget;
};

} // unityshell dash
} // unityshell unity

#endif // UNITYSHELL_FILTERMULTIRANGE_H
