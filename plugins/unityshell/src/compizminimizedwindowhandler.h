// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authored By:
 * Sam Spilsbury <sam.spilsbury@canonical.com>
 */

#ifndef _COMPIZ_COMPIZMINIMIZEDWINDOWHANDLER_H
#define _COMPIZ_COMPIZMINIMIZEDWINDOWHANDLER_H

#include <core/core.h>
#include "minimizedwindowhandler.h"
#include "comptransientfor.h"

// Will be merged back into compiz
namespace compiz {

class PrivateCompizMinimizedWindowHandler
{
public:

  PrivateCompizMinimizedWindowHandler () {};

  CompWindow         *mWindow;
};

template <typename Screen, typename Window>
class CompizMinimizedWindowHandler:
    public MinimizedWindowHandler
{
public:

  CompizMinimizedWindowHandler (CompWindow *w);

  void setVisibility (bool visible);
  unsigned int getPaintMask ();

  void minimize ();
  void unminimize ();

  void updateFrameRegion (CompRegion &r);

  static void setFunctions (bool keepMinimized);
  static void handleCompizEvent (const char *, const char *, CompOption::Vector &);
  static void handleEvent (XEvent *event);
  static std::list<CompWindow *> minimizingWindows;

  typedef CompizMinimizedWindowHandler<Screen, Window> CompizMinimizedWindowHandler_complete;
  typedef boost::shared_ptr<CompizMinimizedWindowHandler_complete> Ptr;
protected:

  virtual std::vector<unsigned int> getTransients ();

private:

  PrivateCompizMinimizedWindowHandler *priv;
  static bool handleEvents;
};
}

template <typename Screen, typename Window>
CompWindowList compiz::CompizMinimizedWindowHandler<Screen, Window>::minimizingWindows;

template <typename Screen, typename Window>
bool compiz::CompizMinimizedWindowHandler<Screen, Window>::handleEvents = true;

template <typename Screen, typename Window>
compiz::CompizMinimizedWindowHandler<Screen, Window>::CompizMinimizedWindowHandler(CompWindow *w) :
  MinimizedWindowHandler (screen->dpy (), w->id ())
{
  priv = new PrivateCompizMinimizedWindowHandler ();

  priv->mWindow = w;

}

template <typename Screen, typename Window>
std::vector<unsigned int>
compiz::CompizMinimizedWindowHandler<Screen, Window>::getTransients ()
{
  std::vector <unsigned int> transients;

  for (CompWindow *w : screen->windows ())
  {
    compiz::CompTransientForReader *reader = new compiz::CompTransientForReader (w);

    if (reader->isTransientFor (priv->mWindow->id ()) ||
        reader->isGroupTransientFor (priv->mWindow->id ()))
      transients.push_back (w->id ());

    delete reader;
  }

  return transients;
}

template <typename Screen, typename Window>
void
compiz::CompizMinimizedWindowHandler<Screen, Window>::setVisibility (bool visible)
{
  MinimizedWindowHandler::setVisibility (visible, priv->mWindow->id ());

  CompositeWindow::get (priv->mWindow)->addDamage ();
  GLWindow::get (priv->mWindow)->glPaintSetEnabled (Window::get (priv->mWindow), !visible);
}

template <typename Screen, typename Window>
void
compiz::CompizMinimizedWindowHandler<Screen, Window>::minimize ()
{
  Atom          wmState = XInternAtom (screen->dpy (), "WM_STATE", 0);
  unsigned long data[2];

  std::vector<unsigned int> transients = getTransients ();

  handleEvents = true;
  priv->mWindow->windowNotify (CompWindowNotifyMinimize);
  priv->mWindow->changeState (priv->mWindow->state () | CompWindowStateHiddenMask);

  for (unsigned int &w : transients)
  {
    CompWindow *win = screen->findWindow (w);

    Window::get (win)->mMinimizeHandler = MinimizedWindowHandler::Ptr (new CompizMinimizedWindowHandler (win));
    Window::get (win)->minimize ();
  }

  priv->mWindow->windowNotify (CompWindowNotifyHide);
  setVisibility (false);

  data[0] = IconicState;
  data[1] = None;

  XChangeProperty (screen->dpy (), priv->mWindow->id (), wmState, wmState,
                   32, PropModeReplace, (unsigned char *) data, 2);

  priv->mWindow->changeState (priv->mWindow->state () | CompWindowStateHiddenMask);
  priv->mWindow->moveInputFocusToOtherWindow ();
}

template <typename Screen, typename Window>
void
compiz::CompizMinimizedWindowHandler<Screen, Window>::updateFrameRegion (CompRegion &r)
{
  unsigned int oldUpdateFrameRegionIndex;
  r -= infiniteRegion;

  /* Ensure no other plugins can touch this frame region */
  oldUpdateFrameRegionIndex = priv->mWindow->updateFrameRegionGetCurrentIndex ();
  priv->mWindow->updateFrameRegionSetCurrentIndex (MAXSHORT);
  priv->mWindow->updateFrameRegion (r);
  priv->mWindow->updateFrameRegionSetCurrentIndex (oldUpdateFrameRegionIndex);
}

template <typename Screen, typename Window>
void
compiz::CompizMinimizedWindowHandler<Screen, Window>::unminimize ()
{
  Atom          wmState = XInternAtom (screen->dpy (), "WM_STATE", 0);
  unsigned long data[2];

  std::vector<unsigned int> transients = getTransients ();

  priv->mWindow->windowNotify (CompWindowNotifyUnminimize);
  priv->mWindow->changeState (priv->mWindow->state () & ~CompWindowStateHiddenMask);
  priv->mWindow->windowNotify (CompWindowNotifyShow);

  for (unsigned int &w : transients)
  {
    CompWindow *win = screen->findWindow (w);

    if (Window::get (win)->mMinimizeHandler)
      Window::get (win)->mMinimizeHandler->unminimize ();

    Window::get (win)->mMinimizeHandler.reset ();
  }

  setVisibility (true);

  priv->mWindow->changeState (priv->mWindow->state () & ~CompWindowStateHiddenMask);

  data[0] = NormalState;
  data[1] = None;

  XChangeProperty (screen->dpy (), priv->mWindow->id (), wmState, wmState,
                   32, PropModeReplace, (unsigned char *) data, 2);
}

template <typename Screen, typename Window>
unsigned int
compiz::CompizMinimizedWindowHandler<Screen, Window>::getPaintMask ()
{
  bool doMask = true;

  for (CompWindow *w : minimizingWindows)
  {
    if (w->id () == priv->mWindow->id ())
      doMask = false;
    break;
  }

  if (doMask)
    return PAINT_WINDOW_NO_CORE_INSTANCE_MASK;

  return 0;
}

template <typename Screen, typename Window>
void
compiz::CompizMinimizedWindowHandler<Screen, Window>::handleCompizEvent (const char *pluginName,
                                                                         const char *eventName,
                                                                         CompOption::Vector &o)
{
  if (!handleEvents)
    return;

  if (strncmp (pluginName, "animation", 9) == 0 &&
      strncmp (eventName, "window_animation", 16) == 0)
  {
    if (CompOption::getStringOptionNamed (o, "type", "") == "minimize")
    {
      CompWindow *w = screen->findWindow (CompOption::getIntOptionNamed (
                                            o, "window", 0));
      if (w)
      {
        if (CompOption::getBoolOptionNamed (o, "active", false))
          minimizingWindows.push_back (w);
        else
          minimizingWindows.remove (w);
      }
    }
  }

  if (!CompOption::getBoolOptionNamed (o, "active", false) &&
      minimizingWindows.empty ())
  {
    handleEvents = false;
  }
}

template <typename Screen, typename Window>
void
compiz::CompizMinimizedWindowHandler<Screen, Window>::handleEvent (XEvent *event)
{
  /* Ignore sent events from the InputRemover */
  if (screen->XShape () && event->type ==
      screen->shapeEvent () + ShapeNotify &&
      !event->xany.send_event)
  {
    CompWindow *w = screen->findWindow (((XShapeEvent *) event)->window);

    if (w)
    {
      typedef compiz::CompizMinimizedWindowHandler<Screen, Window> plugin_handler;
      Window *pw = Window::get (w);
      plugin_handler::Ptr compizMinimizeHandler =
        boost::dynamic_pointer_cast <plugin_handler> (pw->mMinimizeHandler);
      /* Restore and re-save input shape and remove */
      if (compizMinimizeHandler)
      {
        compizMinimizeHandler->setVisibility (true);
        compizMinimizeHandler->setVisibility (false);
      }
    }
  }
}

template <typename Screen, typename Window>
void
compiz::CompizMinimizedWindowHandler<Screen, Window>::setFunctions (bool keepMinimized)
{
  for (CompWindow *w : screen->windows ())
  {
    bool m = w->minimized ();

    if (m)
      w->unminimize ();
    w->minimizeSetEnabled (Window::get (w), keepMinimized);
    w->unminimizeSetEnabled (Window::get (w), keepMinimized);
    w->minimizedSetEnabled (Window::get (w), keepMinimized);
    if (m)
      Window::get (w)->window->minimize ();
  }
}

#endif
