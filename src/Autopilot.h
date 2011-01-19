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
 * Authored by: Alex Launi <alex.launi@canonical.com>
 */

#include "Nux/Nux.h"
#include "NuxCore/Point.h"
namespace Autopilot
{
  class Mouse
  {
  public:
    enum Button {
      Left = 1,
      Right = 2,
      Middle = 3
    };

    Mouse ();
    ~Mouse ();
    void Press(Button button);
    void Release (Button button);
    void Click (Button button);
    void Move (nux::Point *point);
    void SetPosition (nux::Point *point);
    nux::Point *GetPosition ();    
  private:
    Display *_display;
  };
  
  class UnityTests
  {
  public:
    UnityTests ();
    static void Run ();

  private:
    void DragLauncher ();
    void DragLauncherIconAlongEdgeAndDrop ();
    void DragLauncherIconOutAndDrop ();
    void DragLauncherIconOutAndMove ();
    void ShowQuicklist ();
    void ShowTooltip ();
    void TestSetup ();
    nux::Point _initial;
    nux::Point _launcher;

    static Mouse *_mouse;
    static UnityTests *_tests;
  };
};
