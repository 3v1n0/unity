/*
 * Copyright (C) 2013 Canonical Ltd
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
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 *
 */

#ifndef OVERLAY_WINDOW_BUTTONS
#define OVERLAY_WINDOW_BUTTONS

#include "Nux/Nux.h"
#include "Nux/BaseWindow.h"

#include "WindowButtons.h"

namespace unity
{

class OverlayWindowButtons : public nux::BaseWindow, public debug::Introspectable
{
  NUX_DECLARE_OBJECT_TYPE(OverlayWindowButtons, nux::BaseWindow);
public:
  OverlayWindowButtons();

  void Show();
  void Hide();

  nux::Area* FindAreaUnderMouse(nux::Point const& mouse_position,
                                nux::NuxEventType event_type);

protected:
  void Draw(nux::GraphicsEngine& gfx_context, bool force_draw);

  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);
  
private:
  void UpdateGeometry();

  nux::ObjectPtr<WindowButtons> window_buttons_;
};

} // namespace unity

#endif // OVERLAY_WINDOW_BUTTONS
