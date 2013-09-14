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

#ifndef UNITYSHELL_FILTERRATINGSBUTTONWIDGET_H
#define UNITYSHELL_FILTERRATINGSBUTTONWIDGET_H

#include <memory>

#include <Nux/Nux.h>
#include <Nux/ToggleButton.h>
#include <Nux/CairoWrapper.h>
#include <UnityCore/RatingsFilter.h>

namespace unity
{
namespace dash
{

class FilterRatingsButton : public nux::ToggleButton
{
  NUX_DECLARE_OBJECT_TYPE(FilterRatingsButton, nux::ToggleButton);
public:
  FilterRatingsButton(NUX_FILE_LINE_PROTO);
  virtual ~FilterRatingsButton();

  void SetFilter(Filter::Ptr const& filter);
  RatingsFilter::Ptr GetFilter();
  std::string GetFilterType();

protected:
  virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);

  // Key-nav
  virtual bool AcceptKeyNavFocus();
  virtual bool InspectKeyEvent(unsigned int eventType, unsigned int keysym, const char* character);

private:
  void OnKeyDown(unsigned long event_type, unsigned long event_keysym,
                 unsigned long event_state, const TCHAR* character,
                 unsigned short key_repeat_count);

  void RecvMouseUp(int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseDrag(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseMove(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags);
  void OnRatingsChanged(int rating);

  dash::RatingsFilter::Ptr filter_;
  int focused_star_;

};

} // namespace dash
} // namespace unity

#endif // UNITYSHELL_FILTERRATINGSBUTTONWIDGET_H

