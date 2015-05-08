// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2015 Canonical Ltd
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
 * Authored by: Luke Yelavich <luke.yelavich@canonical.com>
 */

#ifndef EXPANDERVIEW_H
#define EXPANDERVIEW_H

#include <Nux/Nux.h>

namespace unity
{

class ExpanderView : public nux::View
{
  NUX_DECLARE_OBJECT_TYPE(ExpanderView, nux::View);
public:
  ExpanderView(NUX_FILE_LINE_PROTO);

  nux::Property<bool> expanded;
  nux::Property<std::string> label;
protected:
  void Draw(nux::GraphicsEngine& graphics_engine, bool force_draw);
  void DrawContent(nux::GraphicsEngine& graphics_engine, bool force_draw);

  bool AcceptKeyNavFocus();

  nux::Area* FindAreaUnderMouse(const nux::Point& mouse_position, nux::NuxEventType event_type);
};

}

#endif
