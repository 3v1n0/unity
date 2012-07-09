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
 * Authored by: Sam Spilsbury <sam.spilsbury@canonical.com>
 */

#include <core/core.h>
#include <core/pluginclasshandler.h>

#include <core/atoms.h>
#include <X11/Xatom.h>

#include "networkarearegion_options.h"

class UnityNETWorkareaRegionScreen :
  public PluginClassHandler <UnityNETWorkareaRegionScreen, CompScreen>,
  public ScreenInterface,
  public NetworkarearegionOptions
{
public:
  UnityNETWorkareaRegionScreen(CompScreen*);
  ~UnityNETWorkareaRegionScreen();

  void
  outputChangeNotify();

  void
  setProperty();

  void
  handleEvent(XEvent* event);

  void
  addSupportedAtoms(std::vector<Atom> &atoms);

private:

  Atom mUnityNETWorkareaRegionAtom;
};

class UnityNETWorkareaRegionWindow :
  public PluginClassHandler <UnityNETWorkareaRegionWindow, CompWindow>,
  public WindowInterface
{
public:

  UnityNETWorkareaRegionWindow(CompWindow*);
  ~UnityNETWorkareaRegionWindow();

  CompWindow* window;

  void
  moveNotify(int dx, int dy, bool immediate);

  void
  resizeNotify(int dx, int dy, int dwidth, int dheight);
};

class UnityNETWorkareaRegionPluginVTable :
  public CompPlugin::VTableForScreenAndWindow <UnityNETWorkareaRegionScreen, UnityNETWorkareaRegionWindow>
{
public:
  bool init();
};
