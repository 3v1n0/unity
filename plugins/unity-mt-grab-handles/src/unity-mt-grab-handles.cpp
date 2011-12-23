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

#define NUM_HANDLES 9
#define FADE_MSEC UnityMTGrabHandlesScreen::get (screen)->optionGetFadeDuration ()

COMPIZ_PLUGIN_20090315(unitymtgrabhandles, UnityMTGrabHandlesPluginVTable);

Unity::MT::GrabHandle::ImplFactory * Unity::MT::GrabHandle::ImplFactory::mDefault = NULL;

Unity::MT::GrabHandle::ImplFactory *
Unity::MT::GrabHandle::ImplFactory::Default()
{
  return mDefault;
}

void
unity::MT::GrabHandle::ImplFactory::SetDefault (ImplFactory *factory)
{
  if (mDefault)
  {
    delete mDefault;
    mDefault = NULL;
  }

  mDefault = factory;
}

unity::MT::X11ImplFactory::X11ImplFactory (Display *dpy) :
  mDpy (dpy)
{
}

Unity::MT::GrabHandle::Impl *
Unity::MT::X11ImplFactory::create (const GrabHandle::Ptr &handle)
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
unity::MT::GrabHandle::buttonPress (int x,
                                    int y,
                                    unsigned int button) const
{
  mImpl->buttonPress (x, y, button);
}

void
unity::MT::GrabHandle::requestMovement (int x,
                                        int y,
                                        unsigned int button) const
{
  unity::MT::GrabHandleGroup::Ptr ghg = mOwner.lock ();
  ghg->requestMovement (x, y, (maskHandles.find (mId))->second, button);
}

void
unity::MT::GrabHandle::show ()
{
  mImpl->show ();
}

void
unity::MT::GrabHandle::hide ()
{
  mImpl->hide ();
}

void
unity::MT::GrabHandle::raise () const
{
  unity::MT::GrabHandleGroup::Ptr ghg = mOwner.lock ();
  boost::shared_ptr <const unity::MT::GrabHandle> gh = shared_from_this ();
  ghg->raiseHandle (gh);
}

void
unity::MT::GrabHandle::reposition(int          x,
                                  int          y,
				  unsigned int flags)
{
  damage (mRect);

  if (flags & PositionSet)
  {
    mRect.x = x;
    mRect.y = y;
  }

  if (flags & PositionLock)
  {
    mImpl->lockPosition (x, y, flags);
  }

  damage (mRect);
}

void
unity::MT::GrabHandle::reposition(int x, int y, unsigned int flags) const
{
  if (flags & PositionLock)
  {
    mImpl->lockPosition (x, y, flags);
  }
}

unity::MT::TextureLayout
unity::MT::GrabHandle::layout()
{
  return TextureLayout(mTexture, mRect);
}

unity::MT::GrabHandle::GrabHandle(GLTexture::List *texture,
                                  unsigned int    width,
                                  unsigned int    height,
                                  const boost::shared_ptr <GrabHandleGroup> &owner,
				  unsigned int    id) :
  mOwner(owner),
  mTexture (texture),
  mId(id),
  mRect (0, 0, width, height),
  mImpl (NULL)
{
}

unity::MT::GrabHandle::Ptr
unity::MT::GrabHandle::create (GLTexture::List *texture, unsigned int width, unsigned int height,
                               const boost::shared_ptr <GrabHandleGroup> &owner,
                               unsigned int id)
{
  unity::MT::GrabHandle::Ptr p (new unity::MT::GrabHandle (texture, width, height, owner, id));
  p->mImpl = unity::MT::GrabHandle::ImplFactory::Default ()->create (p);

  return p;
}

unity::MT::GrabHandle::~GrabHandle()
{
  delete mImpl;
}

void
unity::MT::GrabHandleGroup::show(unsigned int handles)
{
  for(const unity::MT::GrabHandle::Ptr & handle : mHandles)
    if (handles & handle->id ())
      handle->show();

  mState = State::FADE_IN;
}

void
unity::MT::GrabHandleGroup::hide()
{
  for(const unity::MT::GrabHandle::Ptr & handle : mHandles)
    handle->hide();

  mState = State::FADE_OUT;
}

void
unity::MT::GrabHandleGroup::raiseHandle(const boost::shared_ptr <const unity::MT::GrabHandle> &h)
{
  mOwner->raiseGrabHandle (h);
}

bool
unity::MT::GrabHandleGroup::animate(unsigned int msec)
{
  mMoreAnimate = false;

  switch (mState)
  {
    case State::FADE_IN:

      mOpacity += ((float) msec / (float) FADE_MSEC) * OPAQUE;

      if (mOpacity >= OPAQUE)
      {
        mOpacity = OPAQUE;
        mState = State::NONE;
      }
      break;
    case State::FADE_OUT:
      mOpacity -= ((float) msec / (float) FADE_MSEC) * OPAQUE;

      if (mOpacity <= 0)
      {
        mOpacity = 0;
        mState = State::NONE;
      }
      break;
    default:
      break;
  }

  mMoreAnimate = mState != State::NONE;

  return mMoreAnimate;
}

int
unity::MT::GrabHandleGroup::opacity()
{
  return mOpacity;
}

bool
unity::MT::GrabHandleGroup::visible()
{
  return mOpacity > 0.0f;
}

bool
unity::MT::GrabHandleGroup::needsAnimate()
{
  return mMoreAnimate;
}

void
unity::MT::GrabHandleGroup::relayout(const nux::Geometry& rect, bool hard)
{
  /* Each grab handle at each vertex, eg:
   *
   * 1 - topleft
   * 2 - top
   * 3 - topright
   * 4 - right
   * 5 - bottom-right
   * 6 - bottom
   * 7 - bottom-left
   * 8 - left
   */

  const float pos[9][2] =
  {
    {0.0f, 0.0f}, {0.5f, 0.0f}, {1.0f, 0.0f},
    {1.0f, 0.5f}, {1.0f, 1.0f},
    {0.5f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.5f},
    {0.5f, 0.5f} /* middle */
  };

  for (unsigned int i = 0; i < NUM_HANDLES; i++)
  {
    unity::MT::GrabHandle::Ptr & handle = mHandles.at(i);
    CompPoint p(rect.x + rect.width * pos[i][0] -
                handle->width () / 2,
                rect.y + rect.height * pos[i][1] -
                handle->height () / 2);

    handle->reposition (p.x (), p.y (), unity::MT::PositionSet | (hard ? unity::MT::PositionLock : 0));
  }
}

void
UnityMTGrabHandlesWindow::raiseGrabHandle (const boost::shared_ptr <const unity::MT::GrabHandle> &h)
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

unity::MT::GrabHandleGroup::GrabHandleGroup(GrabHandleWindow *owner,
                                            std::vector <unity::MT::TextureSize>  &textures) :
  mState(State::NONE),
  mOpacity(0.0f),
  mMoreAnimate(false),
  mOwner(owner)
{
}

unity::MT::GrabHandleGroup::Ptr
unity::MT::GrabHandleGroup::create (GrabHandleWindow *owner,
                                    std::vector<unity::MT::TextureSize> &textures)
{
    unity::MT::GrabHandleGroup::Ptr p = unity::MT::GrabHandleGroup::Ptr (new unity::MT::GrabHandleGroup (owner, textures));
    for (unsigned int i = 0; i < NUM_HANDLES; i++)
      p->mHandles.push_back(unity::MT::GrabHandle::create (textures.at(i).first,
                                                           textures.at(i).second.width,
                                                           textures.at(i).second.height,
                                                           p,
                                                           handlesMask.find (i)->second));
    return p;
}

unity::MT::GrabHandleGroup::~GrabHandleGroup()
{
  for (unity::MT::GrabHandle::Ptr & handle : mHandles)
    handle->damage (nux::Geometry (handle->x (),
                                   handle->y (),
                                   handle->width (),
                                   handle->height ()));
}

void
unity::MT::GrabHandleGroup::requestMovement (int x,
                                             int y,
					     unsigned int direction,
					     unsigned int button)
{
  mOwner->requestMovement (x, y, direction, button);
}

unsigned int
unity::MT::getLayoutForMask (unsigned int state,
                             unsigned int actions)
{
  unsigned int allHandles = 0;
  for (unsigned int i = 0; i < NUM_HANDLES; i++)
  {
    allHandles |= (1 << i);
  }

  struct _skipInfo
  {
    /* All must match in order for skipping to apply */
    unsigned int state; /* Match if in state */
    unsigned int notstate; /* Match if not in state */
    unsigned int actions; /* Match if in actions */
    unsigned int notactions; /* Match if not in actions */
    unsigned int allowOnly;
  };

  const unsigned int numSkipInfo = 5;
  const struct _skipInfo skip[5] =
  {
    /* Vertically maximized, don't care
     * about left or right handles, or
     * the movement handle */
    {
      CompWindowStateMaximizedVertMask,
      CompWindowStateMaximizedHorzMask,
      0, ~0,
      LeftHandle | RightHandle | MiddleHandle
    },
    /* Horizontally maximized, don't care
     * about top or bottom handles, or
     * the movement handle */
    {
      CompWindowStateMaximizedHorzMask,
      CompWindowStateMaximizedVertMask,
      0, ~0,
      TopHandle | BottomHandle | MiddleHandle
    },
    /* Maximized, don't care about the movement
    * handle */
    {
      CompWindowStateMaximizedVertMask | CompWindowStateMaximizedHorzMask,
      0, 0, ~0,
      MiddleHandle
    },
    /* Immovable, don't show move handle */
    {
      0,
      ~0,
      ~0, CompWindowActionMoveMask,
      TopLeftHandle | TopHandle | TopRightHandle |
      LeftHandle | RightHandle |
      BottomLeftHandle | BottomHandle | BottomRightHandle
    },
    /* Not resizable, don't show resize handle */
    {
      0,
      ~0,
      ~0, CompWindowActionResizeMask,
      MiddleHandle
    },
  };

  for (unsigned int j = 0; j < numSkipInfo; j++)
  {
    const bool exactState = skip[j].state && skip[j].state != static_cast <unsigned int> (~0);
    const bool exactActions = skip[j].actions && skip[j].actions != static_cast <unsigned int> (~0);

    bool stateMatch = false;
    bool actionMatch = false;

    if (exactState)
      stateMatch = (skip[j].state & state) == skip[j].state;
    else
      stateMatch = skip[j].state & state;

    stateMatch &= !(state & skip[j].notstate);

    if (exactActions)
      actionMatch = (skip[j].actions & actions) == skip[j].actions;
    else
      actionMatch = skip[j].actions & actions;

    actionMatch &= !(actions & skip[j].notactions);

    if (stateMatch || actionMatch)
      allHandles &= skip[j].allowOnly;  
  }

  return allHandles;
}

std::vector <unity::MT::TextureLayout>
unity::MT::GrabHandleGroup::layout(unsigned int handles)
{
  std::vector <unity::MT::TextureLayout> layout;

  for(const unity::MT::GrabHandle::Ptr & handle : mHandles)
    if (handle->id () & handles)
      layout.push_back (handle->layout ());

  return layout;
}

void
unity::MT::GrabHandleGroup::forEachHandle (const std::function <void (const unity::MT::GrabHandle::Ptr &)> &f)
{
  for (unity::MT::GrabHandle::Ptr &h : mHandles)
    f (h);
}

/* Super speed hack */
static bool
sortPointers(void *p1, void *p2)
{
  return (void*) p1 < (void*) p2;
}

void
UnityMTGrabHandlesScreen::raiseHandle (const boost::shared_ptr <const unity::MT::GrabHandle> &h,
                                       Window                                                owner)
{
  for (const auto &pair : mInputHandles)
  {
    if (*pair.second == *h)
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
  CompWindow* w, *oldPrev, *oldNext;

  w = oldPrev = oldNext = NULL;

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

          for (; cit != clientListStacking.end(); cit++, oit++)
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
	if (it->second)
          it->second->buttonPress (event->xbutton.x_root,
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
        if (it->second)
          it->second->reposition (0, 0, unity::MT::PositionLock);
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
          handles->forEachHandle ([&](const unity::MT::GrabHandle::Ptr &h)
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
  auto f = [&] (const unity::MT::GrabHandle::Ptr &h)
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
                                 GLFragment::Attrib&      fragment,
                                 const CompRegion&          region,
                                 unsigned int              mask)
{
  /* Draw the window on the bottom, we will be drawing the
   * handles on top */
  bool status = gWindow->glDraw(transform, fragment, region, mask);

  if (mHandles && mHandles->visible())
  {
    unsigned int allowedHandles = unity::MT::getLayoutForMask (window->state (), window->actions ());
    unsigned int handle = 0;

    UMTGH_SCREEN (screen);

    for(unity::MT::TextureLayout layout : mHandles->layout (allowedHandles))
    {
      /* We want to set the geometry of the handle to the window
       * region */
      CompRegion reg = CompRegion(layout.second.x, layout.second.y, layout.second.width, layout.second.height);

      for(GLTexture * tex : *layout.first)
      {
        GLTexture::MatrixList matl;
        GLTexture::Matrix     mat = tex->matrix();
        CompRegion        paintRegion(region);

        /* We can reset the window geometry since it will be
         * re-added later */
        gWindow->geometry().reset();

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

        /* Did it succeed? */
        if (gWindow->geometry().vertices)
        {
          fragment.setOpacity(mHandles->opacity());
          /* Texture rendering set-up */
          us->gScreen->setTexEnvMode(GL_MODULATE);
          glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
          /* Draw the dim texture with all of it's modified
           * geometry glory */
          gWindow->glDrawTexture(tex, fragment, mask | PAINT_WINDOW_BLEND_MASK
                                 | PAINT_WINDOW_TRANSLUCENT_MASK |
                                 PAINT_WINDOW_TRANSFORMED_MASK);
          /* Texture rendering tear-down */
          glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
          us->gScreen->setTexEnvMode(GL_REPLACE);
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

  mHandles->forEachHandle ([&](const unity::MT::GrabHandle::Ptr &h)
                           { h->reposition (0, 0, unity::MT::PositionLock); });
}

void
UnityMTGrabHandlesScreen::addHandleWindow(const unity::MT::GrabHandle::Ptr &h, Window w)
{
  mInputHandles.insert(std::pair <Window, const unity::MT::GrabHandle::Ptr> (w, h));
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

  ScreenInterface::setHandler(s);
  CompositeScreenInterface::setHandler(cScreen);
  GLScreenInterface::setHandler(gScreen);

  mHandleTextures.resize(NUM_HANDLES);

  for (unsigned int i = 0; i < NUM_HANDLES; i++)
  {
    CompString fname = "handle-";
    CompString pname("unitymtgrabhandles");
    CompSize   size;

    fname = compPrintf("%s%i.png", fname.c_str(), i);
    mHandleTextures.at(i).first =
      new GLTexture::List (GLTexture::readImageToTexture(fname, pname,
                                                         size));

    mHandleTextures.at (i).second.width = size.width ();
    mHandleTextures.at (i).second.height = size.height ();
  }

  optionSetToggleHandlesKeyInitiate(boost::bind(&UnityMTGrabHandlesScreen::toggleHandles, this, _1, _2, _3));
  optionSetShowHandlesKeyInitiate(boost::bind(&UnityMTGrabHandlesScreen::showHandles, this, _1, _2, _3));
  optionSetHideHandlesKeyInitiate(boost::bind(&UnityMTGrabHandlesScreen::hideHandles, this, _1, _2, _3));
}

UnityMTGrabHandlesScreen::~UnityMTGrabHandlesScreen()
{
  mGrabHandles.clear ();
  for (auto it : mHandleTextures)
    if (it.first)
      delete it.first;
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
