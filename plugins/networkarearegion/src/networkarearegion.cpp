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

#include "networkarearegion.h"

COMPIZ_PLUGIN_20090315(networkarearegion, UnityNETWorkareaRegionPluginVTable);

void
UnityNETWorkareaRegionScreen::setProperty()
{
  CompRegion    sr;
  unsigned long* data;
  unsigned int  dataSize;
  unsigned int  offset = 0;

  sr = sr.united(CompRect(0, 0, screen->width(), screen->height()));

  foreach(CompWindow * w, screen->clientList())
  {
    if (!w->struts())
      continue;

    sr -= CompRect(w->struts()->left.x, w->struts()->left.y, w->struts()->left.width, w->struts()->left.height);
    sr -= CompRect(w->struts()->right.x, w->struts()->right.y, w->struts()->right.width, w->struts()->right.height);
    sr -= CompRect(w->struts()->top.x, w->struts()->top.y, w->struts()->top.width, w->struts()->top.height);
    sr -= CompRect(w->struts()->bottom.x, w->struts()->bottom.y, w->struts()->bottom.width, w->struts()->bottom.height);
  }

  dataSize = sr.rects().size()  * 4;
  data = new unsigned long[dataSize];

  foreach(const CompRect & r, sr.rects())
  {
    data[offset * 4 + 0] = r.x();
    data[offset * 4 + 1] = r.y();
    data[offset * 4 + 2] = r.width();
    data[offset * 4 + 3] = r.height();

    offset++;
  }

  XChangeProperty(screen->dpy(), screen->root(), mUnityNETWorkareaRegionAtom,
                  XA_CARDINAL, 32, PropModeReplace, (const unsigned char*) data, dataSize);

  delete[] data;
}

void
UnityNETWorkareaRegionScreen::outputChangeNotify()
{
  setProperty();
  screen->outputChangeNotify ();
}

void
UnityNETWorkareaRegionScreen::handleEvent(XEvent* event)
{
  screen->handleEvent(event);

  switch (event->type)
  {
    case PropertyNotify:

      if (event->xproperty.atom == (Atom) Atoms::wmStrut ||
	  event->xproperty.atom == (Atom) Atoms::wmStrutPartial)
      {
        CompWindow* w = screen->findWindow(event->xproperty.window);

        if (w)
        {
          if (w->struts())
          {
            UnityNETWorkareaRegionWindow* unwmh = UnityNETWorkareaRegionWindow::get(w);

            w->moveNotifySetEnabled(unwmh, true);
            w->resizeNotifySetEnabled(unwmh, true);

            /* The struts got updated, so we need to set the property again */
            setProperty();
          }
        }
      }
  }
}

void
UnityNETWorkareaRegionWindow::moveNotify(int dx, int dy, bool immediate)
{
  UnityNETWorkareaRegionScreen::get(screen)->setProperty();
  window->moveNotify(dx, dy, immediate);
}

void
UnityNETWorkareaRegionWindow::resizeNotify(int dx, int dy, int dwidth, int dheight)
{
  UnityNETWorkareaRegionScreen::get(screen)->setProperty();
  window->resizeNotify(dx, dy, dwidth, dheight);
}

void
UnityNETWorkareaRegionScreen::addSupportedAtoms(std::vector<Atom> &atoms)
{
  atoms.push_back(mUnityNETWorkareaRegionAtom);

  screen->addSupportedAtoms(atoms);
}

UnityNETWorkareaRegionScreen::UnityNETWorkareaRegionScreen(CompScreen* s) :
  PluginClassHandler <UnityNETWorkareaRegionScreen, CompScreen> (s),
  mUnityNETWorkareaRegionAtom(XInternAtom(screen->dpy(), "_UNITY_NET_WORKAREA_REGION", 0))
{
  ScreenInterface::setHandler(screen);
  screen->updateSupportedWmHints();
}

UnityNETWorkareaRegionScreen::~UnityNETWorkareaRegionScreen()
{
  /* Delete the property and the bit saying we support it */
  screen->addSupportedAtomsSetEnabled(this, false);
  screen->updateSupportedWmHints();

  XDeleteProperty(screen->dpy(), screen->root(), mUnityNETWorkareaRegionAtom);
}


UnityNETWorkareaRegionWindow::UnityNETWorkareaRegionWindow(CompWindow* w) :
  PluginClassHandler <UnityNETWorkareaRegionWindow, CompWindow> (w),
  window(w)
{
  if (w->struts())
  {
    UnityNETWorkareaRegionScreen::get(screen)->setProperty();
    WindowInterface::setHandler(w, true);
  }
  else
    WindowInterface::setHandler(w, false);
}

UnityNETWorkareaRegionWindow::~UnityNETWorkareaRegionWindow()
{
  if (window->struts())
    UnityNETWorkareaRegionScreen::get(screen)->setProperty();
}

bool
UnityNETWorkareaRegionPluginVTable::init()
{
  if (!CompPlugin::checkPluginABI("core", CORE_ABIVERSION))
    return false;

  return true;
}
