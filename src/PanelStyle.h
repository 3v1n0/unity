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
 * Authored by: Mirco Müller <mirco.mueller@canonical.com>
 */

#ifndef PANEL_STYLE_H
#define PANEL_STYLE_H

#include "Nux/Nux.h"

class PanelStyle {
  public:
    PanelStyle ();
    ~PanelStyle ();

    static PanelStyle* GetDefault ();

    void GetTextColor (nux::Color* color);
    void GetBackgroundTop (nux::Color* color);
    void GetBackgroundBottom (nux::Color* color);
    void GetTextShadow (nux::Color* color);

  private:
    static PanelStyle* _panelStyle;
    nux::Color  _text;
    nux::Color  _bgTop;
    nux::Color  _bgBottom;
    nux::Color  _textShadow;
};

#endif // PANEL_STYLE_H
