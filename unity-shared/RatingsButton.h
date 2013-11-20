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
 **
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 *
 */

#ifndef UNITYSHELL_RATINGSBUTTONWIDGET_H
#define UNITYSHELL_RATINGSBUTTONWIDGET_H

#include <memory>

#include <Nux/Nux.h>
#include <Nux/ToggleButton.h>
#include "unity-shared/Introspectable.h"

namespace unity
{

class RatingsButton : public unity::debug::Introspectable, public nux::ToggleButton
{
public:
  RatingsButton(int star_size, int star_gap, NUX_FILE_LINE_PROTO);
  virtual ~RatingsButton();

  void SetEditable(bool editable);
  virtual void SetRating(float rating);
  virtual float GetRating() const;

protected:
  virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);

  // Key-nav
  virtual bool AcceptKeyNavFocus();
  virtual bool InspectKeyEvent(unsigned int eventType, unsigned int keysym, const char* character);

  // Introspectable methods
  std::string GetName() const;
  void AddProperties(debug::IntrospectionData&);

private:
  void OnKeyDown(unsigned long event_type, unsigned long event_keysym,
                 unsigned long event_state, const TCHAR* character,
                 unsigned short key_repeat_count);

  void RecvMouseUp(int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseDrag(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseMove(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags);
  void UpdateRatingToMouse(int x);


protected:
  bool editable_;
  float rating_;
  int focused_star_;
  int star_size_;
  int star_gap_;
};

} // namespace unity

#endif // UNITYSHELL_RATINGSBUTTONWIDGET_H

