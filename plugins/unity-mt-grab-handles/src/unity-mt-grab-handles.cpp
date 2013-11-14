/*
 * Copyright (C) 2011 Canonical Ltd
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

#include "unity-mt-grab-handles.h"
#include <iostream>

COMPIZ_PLUGIN_20090315(unitymtgrabhandles, UnityMTGrabHandlesPluginVTable);

unsigned int unity::MT::MaximizedHorzMask = CompWindowStateMaximizedHorzMask;
unsigned int unity::MT::MaximizedVertMask = CompWindowStateMaximizedVertMask;
unsigned int unity::MT::MoveMask = CompWindowActionMoveMask;
unsigned int unity::MT::ResizeMask = CompWindowActionResizeMask;

void
unity::MT::X11TextureFactory::setActiveWrap (const GLTexture::List &t)
{
  mWrap = t;
}

unity::MT::Texture::Ptr
unity::MT::X11TextureFactory::create ()
{
  unity::MT::Texture::Ptr tp(static_cast<unity::MT::Texture*> (new unity::MT::X11Texture(mWrap)));
  return tp;
}

unity::MT::X11Texture::X11Texture (const GLTexture::List &t)
{
  mTexture = t;
}

const GLTexture::List &
unity::MT::X11Texture::get ()
{
  return mTexture;
}

unity::MT::X11ImplFactory::X11ImplFactory (Display *dpy) :
  mDpy (dpy)
{
}

unity::MT::GrabHandle::Impl *
unity::MT::X11ImplFactory::create (const GrabHandle::Ptr &handle)
{
  unity::MT::GrabHandle::Impl *impl = new X11GrabHandleImpl (mDpy, handle);
  return impl;
}

unity::MT::X11GrabHandleImpl::X11GrabHandleImpl (Display *dpy, const GrabHandle::Ptr &h) :
  mGrabHandle (h),
  mIpw (None),
  mDpy (dpy)
{

}

void
unity::MT::X11GrabHandleImpl::show ()
{
  if (mIpw)
  {
    XMapWindow (mDpy, mIpw);
    return;
  }

  XSetWindowAttributes xswa;

  xswa.override_redirect = TRUE;

  unity::MT::GrabHandle::Ptr gh = mGrabHandle.lock ();

  mIpw = XCreateWindow(mDpy,
                       DefaultRootWindow (mDpy),
                       -100, -100,
                       gh->width (),
                       gh->height (),
                       0,
                       CopyFromParent, InputOnly,
                       CopyFromParent, CWOverrideRedirect, &xswa);

  UnityMTGrabHandlesScreen::get(screen)->addHandleWindow(gh, mIpw);

  XMapWindow (mDpy, mIpw);
}

void
unity::MT::X11GrabHandleImpl::hide ()
{
  if (mIpw)
    XUnmapWindow (mDpy, mIpw);
}

void
unity::MT::X11GrabHandleImpl::lockPosition (int x,
                                            int y,
                                            unsigned int flags)
{
  XWindowChanges xwc;
  unsigned int   vm = 0;

  if (!mIpw)
    return;

  if (flags & unity::MT::PositionSet)
  {
    xwc.x = x;
    xwc.y = y;
    vm |= CWX | CWY;
  }

  unity::MT::GrabHandle::Ptr gh = mGrabHandle.lock ();

  gh->raise ();

  XConfigureWindow(screen->dpy(), mIpw, vm, &xwc);
  XSelectInput(screen->dpy(), mIpw, ButtonPressMask | ButtonReleaseMask);
}

unity::MT::X11GrabHandleImpl::~X11GrabHandleImpl ()
{
  if (mIpw)
  {
    UnityMTGrabHandlesScreen::get(screen)->removeHandleWindow(mIpw);

    XDestroyWindow(mDpy, mIpw);
  }
}

void
unity::MT::X11GrabHandleImpl::buttonPress (int x,
                                           int y,
                                           unsigned int button) const
{
  unity::MT::GrabHandle::Ptr gh = mGrabHandle.lock ();
  gh->requestMovement (x, y, button);
}

void
UnityMTGrabHandlesWindow::raiseGrabHandle (const std::shared_ptr <const unity::MT::GrabHandle> &h)
{
  UnityMTGrabHandlesScreen::get (screen)->raiseHandle (h, window->frame ());
}

void
UnityMTGrabHandlesWindow::requestMovement (int x,
                                           int y,
					   unsigned int direction,
					   unsigned int button)
{
  /* Send _NET_MOVERESIZE to root window so that a button-1
   * press on this window will start resizing the window around */
  XEvent     event;

  if (screen->getOption("raise_on_click"))
    window->updateAttributes(CompStackingUpdateModeAboveFullscreen);

  if (window->id() != screen->activeWindow())
    if (window->focus())
      window->moveInputFocusTo();

  event.xclient.type    = ClientMessage;
  event.xclient.display = screen->dpy ();

  event.xclient.serial      = 0;
  event.xclient.send_event  = true;

  event.xclient.window       = window->id();
  event.xclient.message_type = Atoms::wmMoveResize;
  event.xclient.format       = 32;

  event.xclient.data.l[0] = x;
  event.xclient.data.l[1] = y;
  event.xclient.data.l[2] = direction;
  event.xclient.data.l[3] = button;
  event.xclient.data.l[4] = 1;

  XSendEvent(screen->dpy(), screen->root(), false,
             SubstructureRedirectMask | SubstructureNotifyMask,
             &event);
}

/* Super speed hack */
static bool
sortPointers(void *p1, void *p2)
{
  return (void*) p1 < (void*) p2;
}

void
UnityMTGrabHandlesScreen::raiseHandle (const std::shared_ptr <const unity::MT::GrabHandle> &h,
                                       Window                                                owner)
{
  for (const auto &pair : mInputHandles)
  {
    const unity::MT::GrabHandle::Ptr gh = pair.second.lock();
    if (*gh == *h)
    {
      unsigned int mask = CWSibling | CWStackMode;
      XWindowChanges xwc;

      xwc.stack_mode = Above;
      xwc.sibling = owner;

      XConfigureWindow (screen->dpy (), pair.first, mask, &xwc);
    }
  }
}

void
UnityMTGrabHandlesScreen::handleEvent(XEvent* event)
{
  CompWindow* w;

  w = NULL;

  switch (event->type)
  {
    case FocusIn:
    case FocusOut:
      if (event->xfocus.mode == NotifyUngrab)
      {
	for(CompWindow * w : screen->windows())
        {
          UnityMTGrabHandlesWindow* mtwindow = UnityMTGrabHandlesWindow::get(w);
          if (mtwindow->handleTimerActive())
            mtwindow->resetTimer();
        }
      }
      break;
    case ClientMessage:

      if (event->xclient.message_type == mCompResizeWindowAtom)
      {
        CompWindow* w = screen->findWindow(event->xclient.window);

        if (w)
        {
          CompRect r;
          UMTGH_WINDOW(w);

          r.setGeometry(event->xclient.data.l[0] - w->input().left,
                        event->xclient.data.l[1] - w->input().top,
                        event->xclient.data.l[2] + w->input().left + w->input().right,
                        event->xclient.data.l[3] + w->input().top + w->input().bottom);

          uw->relayout(r, false);
        }
      }

      break;

    case PropertyNotify:

      /* Stacking order of managed clients changed, check old
       * stacking order and ensure stacking of handles
       * that were changed in the stack */


      if (event->xproperty.atom == Atoms::clientListStacking)
      {
        CompWindowVector       invalidated(0);
        CompWindowVector       clients = screen->clientList(true);
        CompWindowVector       oldClients = mLastClientListStacking;
        CompWindowVector       clientListStacking = screen->clientList(true);
        /* Windows can be removed and added from the client list
         * here at the same time (eg hide/unhide launcher ... racy)
         * so we need to check if the client list contains the same
         * windows as it used to. Sort both lists and compare ... */

        std::sort(clients.begin(), clients.end(), sortPointers);
        std::sort(oldClients.begin(),
                  oldClients.end(), sortPointers);

        if (clients != mLastClientListStacking)
          invalidated = clients;
        else
        {
          CompWindowVector::const_iterator cit = clientListStacking.begin();
          CompWindowVector::const_iterator oit = mLastClientListStacking.begin();

          for (; cit != clientListStacking.end(); ++cit, oit++)
          {
            /* All clients from this point onwards in cit are invalidated
             * so splice the list to the end of the new client list
             * and update the stacking of handles there */
            if ((*cit)->id() != (*oit)->id())
            {
              invalidated.push_back((*cit));
            }
          }
        }

	for(CompWindow * w : invalidated)
	  UnityMTGrabHandlesWindow::get(w)->restackHandles();

        mLastClientListStacking = clients;
      }

      break;

    case ButtonPress:
    {

      if (event->xbutton.button != 1)
        break;

      auto it = mInputHandles.find(event->xbutton.window);

      if (it != mInputHandles.end())
      {
        const unity::MT::GrabHandle::Ptr gh = it->second.lock();
        if (gh)
          gh->buttonPress (event->xbutton.x_root,
                           event->xbutton.y_root,
                           event->xbutton.button);
      }

      break;
    }
    case ConfigureNotify:

      w = screen->findTopLevelWindow(event->xconfigure.window);

      if (w)
        UnityMTGrabHandlesWindow::get(w)->relayout(w->inputRect(), true);

      break;

    case MapNotify:
    {

      auto it = mInputHandles.find(event->xmap.window);

      if (it != mInputHandles.end())
      {
        const unity::MT::GrabHandle::Ptr gh = it->second.lock();
        if (gh)
          gh->reposition (0, 0, unity::MT::PositionLock);
      }

      break;
    }
    default:

      break;
  }

  screen->handleEvent(event);
}

void
UnityMTGrabHandlesScreen::donePaint()
{
  if (mMoreAnimate)
  {
    for (const unity::MT::GrabHandleGroup::Ptr &handles : mGrabHandles)
    {
      if (handles->needsAnimate())
      {
          handles->forEachHandle ([this](const unity::MT::GrabHandle::Ptr &h)
				  {
				    h->damage (nux::Geometry (h->x (),
							      h->y (),
							      h->width (),
							      h->height ()));
				  });
      }
    }
  }

  cScreen->donePaint();
}

void
UnityMTGrabHandlesScreen::preparePaint(int msec)
{
  if (mMoreAnimate)
  {
    mMoreAnimate = false;

    for(const unity::MT::GrabHandleGroup::Ptr &handles : mGrabHandles)
    {
      mMoreAnimate |= handles->animate(msec);
    }
  }

  cScreen->preparePaint(msec);
}

bool
UnityMTGrabHandlesWindow::handleTimerActive()
{
  return mTimer.active ();
}

bool
UnityMTGrabHandlesWindow::allowHandles()
{
  /* Not on override redirect windows */
  if (window->overrideRedirect())
    return false;

  return true;
}

void
UnityMTGrabHandlesWindow::getOutputExtents(CompWindowExtents& output)
{
  auto f = [this] (const unity::MT::GrabHandle::Ptr &h)
  {
    output.left = std::max (window->borderRect().left() + h->width () / 2, static_cast <unsigned int> (output.left));
    output.right = std::max (window->borderRect().right()  + h->width () / 2, static_cast <unsigned int> (output.right));
    output.top = std::max (window->borderRect().top() + h->height () / 2, static_cast <unsigned int> (output.top));
    output.bottom = std::max (window->borderRect().bottom() + h->height () / 2, static_cast <unsigned int> (output.bottom));
  };

  if (mHandles)
  {
    /* Only care about the handle on the outside */
    mHandles->forEachHandle (f);
  }
  else
    window->getOutputExtents(output);

}

bool
UnityMTGrabHandlesWindow::glDraw(const GLMatrix&            transform,
                                 const GLWindowPaintAttrib& attrib,
                                 const CompRegion&          region,
                                 unsigned int              mask)
{
  /* Draw the window on the bottom, we will be drawing the
   * handles on top */
  bool status = gWindow->glDraw(transform, attrib, region, mask);

  if (mHandles && mHandles->visible())
  {
    unsigned int allowedHandles = unity::MT::getLayoutForMask (window->state (), window->actions ());
    unsigned int handle = 0;

    for(unity::MT::TextureLayout layout : mHandles->layout (allowedHandles))
    {
      /* We want to set the geometry of the handle to the window
       * region */
      CompRegion reg = CompRegion(layout.second.x, layout.second.y, layout.second.width, layout.second.height);

      for(GLTexture * tex : static_cast<unity::MT::X11Texture*>(layout.first.get())->get())
      {
        GLTexture::MatrixList matl;
        GLTexture::Matrix     mat = tex->matrix();
        CompRegion        paintRegion(region);
        GLWindowPaintAttrib   wAttrib(attrib);

        /* We can reset the window geometry since it will be
         * re-added later */
        gWindow->vertexBuffer()->begin();

        /* Not sure what this does, but it is necessary
         * (adjusts for scale?) */
        mat.x0 -= mat.xx * reg.boundingRect().x1();
        mat.y0 -= mat.yy * reg.boundingRect().y1();

        matl.push_back(mat);

        if (mask & PAINT_WINDOW_TRANSFORMED_MASK)
          paintRegion = infiniteRegion;

        /* Now allow plugins to mess with the geometry of our
         * dim (so we get a nice render for things like
         * wobbly etc etc */
        gWindow->glAddGeometry(matl, reg, paintRegion);

	if (gWindow->vertexBuffer()->end())
	{
	  wAttrib.opacity = mHandles->opacity();

          glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
          /* Draw the dim texture with all of it's modified
           * geometry glory */
          gWindow->glDrawTexture(tex,
                                 transform, wAttrib,
                                 mask | PAINT_WINDOW_BLEND_MASK
                                 | PAINT_WINDOW_TRANSLUCENT_MASK |
                                 PAINT_WINDOW_TRANSFORMED_MASK);
          /* Texture rendering tear-down */
          glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        }
      }

      handle++;
    }
  }

  return status;
}

void
UnityMTGrabHandlesWindow::relayout(const CompRect& r, bool hard)
{
  if (mHandles)
    mHandles->relayout(nux::Geometry (r.x (), r.y (), r.width (), r.height ()), hard);
}

void
UnityMTGrabHandlesWindow::grabNotify(int x, int y, unsigned int state, unsigned int mask)
{
  window->grabNotify(x, y, state, mask);
}

void
UnityMTGrabHandlesWindow::moveNotify(int dx, int dy, bool immediate)
{
  if (mHandles)
    mHandles->relayout(nux::Geometry (window->inputRect ().x (), window->inputRect ().y (),
                                      window->inputRect ().width (), window->inputRect ().height ()), false);

  window->moveNotify(dx, dy, immediate);
}

void
UnityMTGrabHandlesWindow::ungrabNotify()
{
  window->ungrabNotify();
}

bool
UnityMTGrabHandlesWindow::handlesVisible()
{
  if (!mHandles)
    return false;

  return mHandles->visible();
}

void
UnityMTGrabHandlesWindow::hideHandles()
{
  if (mHandles)
    mHandles->hide();

  window->updateWindowOutputExtents();
  cWindow->damageOutputExtents();

  disableTimer();
}

bool
UnityMTGrabHandlesWindow::onHideTimeout()
{
  CompOption::Vector o (1);
  CompOption::Value  v;

  if (screen->grabbed())
    return true;

  v.set ((int) window->id ());

  o[0].setName ("window", CompOption::TypeInt);
  o[0].set (v);

  UnityMTGrabHandlesScreen::get (screen)->hideHandles (NULL, 0, o);
  return false;
}

void
UnityMTGrabHandlesWindow::resetTimer()
{
  mTimer.stop ();
  mTimer.setTimes (2000, 2200);
  mTimer.start ();
}

void
UnityMTGrabHandlesWindow::disableTimer()
{
  mTimer.stop ();
}

void
UnityMTGrabHandlesWindow::showHandles(bool use_timer)
{
  UMTGH_SCREEN (screen);

  if (!mHandles)
  {
    mHandles = unity::MT::GrabHandleGroup::create (this, us->textures ());
    us->addHandles(mHandles);
  }

  if (!mHandles->visible())
  {
    unsigned int showingMask = unity::MT::getLayoutForMask (window->state (), window->actions ());
    activate();
    mHandles->show(showingMask);
    mHandles->relayout(nux::Geometry (window->inputRect().x (),
                                      window->inputRect().y (),
                                      window->inputRect().width(),
                                      window->inputRect().height()), true);

    window->updateWindowOutputExtents();
    cWindow->damageOutputExtents();
  }

  if (use_timer)
    resetTimer();
  else
    disableTimer();
}

void
UnityMTGrabHandlesWindow::restackHandles()
{
  if (!mHandles)
    return;

  mHandles->forEachHandle ([this](const unity::MT::GrabHandle::Ptr &h)
                           { h->reposition (0, 0, unity::MT::PositionLock); });
}

void
UnityMTGrabHandlesScreen::addHandleWindow(const unity::MT::GrabHandle::Ptr &h, Window w)
{
  mInputHandles.insert(std::make_pair(w, h));
}

void
UnityMTGrabHandlesScreen::removeHandleWindow(Window w)
{
  mInputHandles.erase(w);
}

void
UnityMTGrabHandlesScreen::addHandles(const unity::MT::GrabHandleGroup::Ptr &handles)
{
  mGrabHandles.push_back(handles);
}

void
UnityMTGrabHandlesScreen::removeHandles(const unity::MT::GrabHandleGroup::Ptr &handles)
{
  mGrabHandles.remove(handles);

  mMoreAnimate = true;
}

bool
UnityMTGrabHandlesScreen::toggleHandles(CompAction*         action,
                                        CompAction::State  state,
                                        CompOption::Vector& options)
{
  CompWindow* w = screen->findWindow(CompOption::getIntOptionNamed(options,
                                                                   "window",
								  0));
  if (w)
  {
    UMTGH_WINDOW(w);

    if (!uw->allowHandles())
      return false;

    if (uw->handlesVisible())
      uw->hideHandles();
    else
      uw->showHandles(true);

    mMoreAnimate = true;
  }

  return true;
}

bool
UnityMTGrabHandlesScreen::showHandles(CompAction*         action,
                                      CompAction::State  state,
                                      CompOption::Vector& options)
{
  CompWindow* w = screen->findWindow(CompOption::getIntOptionNamed(options,
                                                                   "window",
                                                                   0));

  bool use_timer = CompOption::getBoolOptionNamed(options, "use-timer", true);

  if (w)
  {
    UMTGH_WINDOW(w);

    if (!uw->allowHandles())
      return false;

    uw->showHandles(use_timer);

    if (!uw->handlesVisible())
      mMoreAnimate = true;
  }

  return true;
}

bool
UnityMTGrabHandlesScreen::hideHandles(CompAction*         action,
                                      CompAction::State  state,
                                      CompOption::Vector& options)
{
  CompWindow* w = screen->findWindow(CompOption::getIntOptionNamed(options,
                                                                   "window",
                                                                   0));
  if (w)
  {
    UMTGH_WINDOW(w);

    if (!uw->allowHandles())
      return false;

    if (uw->handlesVisible())
    {
      uw->hideHandles();
      mMoreAnimate = true;
    }
  }

  return true;
}

void
UnityMTGrabHandlesScreen::optionChanged (CompOption *option,
                                         UnitymtgrabhandlesOptions::Options num)
{
  if (num == UnitymtgrabhandlesOptions::FadeDuration)
  {
    unity::MT::FADE_MSEC = optionGetFadeDuration ();
  }
}

UnityMTGrabHandlesScreen::UnityMTGrabHandlesScreen(CompScreen* s) :
  PluginClassHandler <UnityMTGrabHandlesScreen, CompScreen> (s),
  cScreen(CompositeScreen::get(s)),
  gScreen(GLScreen::get(s)),
  mGrabHandles(0),
  mHandleTextures(0),
  mLastClientListStacking(screen->clientList(true)),
  mCompResizeWindowAtom(XInternAtom(screen->dpy(),
                                    "_COMPIZ_RESIZE_NOTIFY", 0)),
  mMoreAnimate(false)
{
  unity::MT::GrabHandle::ImplFactory::SetDefault (new unity::MT::X11ImplFactory (screen->dpy ()));
  unity::MT::Texture::Factory::SetDefault (new unity::MT::X11TextureFactory ());

  ScreenInterface::setHandler(s);
  CompositeScreenInterface::setHandler(cScreen);
  GLScreenInterface::setHandler(gScreen);

  mHandleTextures.resize(unity::MT::NUM_HANDLES);

  for (unsigned int i = 0; i < unity::MT::NUM_HANDLES; i++)
  {
    CompString fname = "handle-";
    CompString pname("unitymtgrabhandles");
    CompSize   size;

    fname = compPrintf("%s%i.png", fname.c_str(), i);
    GLTexture::List t = GLTexture::readImageToTexture(fname, pname,
                                                      size);

    (static_cast<unity::MT::X11TextureFactory*>(unity::MT::Texture::Factory::Default().get())->setActiveWrap(t));

    mHandleTextures.at(i).first = unity::MT::Texture::Factory::Default ()->create ();
    mHandleTextures.at (i).second.width = size.width ();
    mHandleTextures.at (i).second.height = size.height ();
  }

  unity::MT::FADE_MSEC = optionGetFadeDuration ();

  optionSetToggleHandlesKeyInitiate(boost::bind(&UnityMTGrabHandlesScreen::toggleHandles, this, _1, _2, _3));
  optionSetShowHandlesKeyInitiate(boost::bind(&UnityMTGrabHandlesScreen::showHandles, this, _1, _2, _3));
  optionSetHideHandlesKeyInitiate(boost::bind(&UnityMTGrabHandlesScreen::hideHandles, this, _1, _2, _3));
  optionSetFadeDurationNotify(boost::bind(&UnityMTGrabHandlesScreen::optionChanged, this, _1, _2));
}

UnityMTGrabHandlesScreen::~UnityMTGrabHandlesScreen()
{
  mGrabHandles.clear ();
}

UnityMTGrabHandlesWindow::UnityMTGrabHandlesWindow(CompWindow* w) :
  PluginClassHandler <UnityMTGrabHandlesWindow, CompWindow> (w),
  window(w),
  cWindow(CompositeWindow::get(w)),
  gWindow(GLWindow::get(w)),
  mHandles()
{
  WindowInterface::setHandler(window);
  CompositeWindowInterface::setHandler(cWindow);
  GLWindowInterface::setHandler(gWindow);

  mTimer.setCallback (boost::bind (&UnityMTGrabHandlesWindow::onHideTimeout, this));
}

UnityMTGrabHandlesWindow::~UnityMTGrabHandlesWindow()
{
  mTimer.stop ();

  if (mHandles)
  {
    UnityMTGrabHandlesScreen::get(screen)->removeHandles(mHandles);
  }
}

bool
UnityMTGrabHandlesPluginVTable::init()
{
  if (!CompPlugin::checkPluginABI("core", CORE_ABIVERSION) ||
      !CompPlugin::checkPluginABI("composite", COMPIZ_COMPOSITE_ABI) ||
      !CompPlugin::checkPluginABI("opengl", COMPIZ_OPENGL_ABI))
    return false;

  return true;
}
