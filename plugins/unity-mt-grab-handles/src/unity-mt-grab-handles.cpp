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

void
Unity::MT::GrabHandle::reposition(CompPoint* p, bool hard)
{
  XWindowChanges xwc;
  unsigned int   vm = 0;

  UMTGH_SCREEN(screen);

  us->cScreen->damageRegion((CompRect&) *this);

  if (p)
  {
    setX(p->x());
    setY(p->y());

    xwc.x = x();
    xwc.y = y();

    vm |= (CWX | CWY);
  }

  vm |= (CWStackMode | CWSibling);

  xwc.stack_mode = Above;
  xwc.sibling = mOwner;

  if (hard)
  {
    XConfigureWindow(screen->dpy(), mIpw, vm, &xwc);
    XSelectInput(screen->dpy(), mIpw, ButtonPressMask | ButtonReleaseMask);
  }

  us->cScreen->damageRegion((CompRect&) *this);
}

void
Unity::MT::GrabHandle::hide()
{
  if (mIpw)
    XUnmapWindow(screen->dpy(), mIpw);
}

void
Unity::MT::GrabHandle::show()
{
  if (!mIpw)
  {
    XSetWindowAttributes xswa;

    xswa.override_redirect = TRUE;

    mIpw = XCreateWindow(screen->dpy(),
                         screen->root(),
                         -100, -100,
                         mTexture->second.width(),
                         mTexture->second.height(),
                         0,
                         CopyFromParent, InputOnly,
                         CopyFromParent, CWOverrideRedirect, &xswa);

    UnityMTGrabHandlesScreen::get(screen)->addHandleWindow(this, mIpw);

    reposition(NULL, true);
  }

  XMapWindow(screen->dpy(), mIpw);
}

Unity::MT::TextureLayout
Unity::MT::GrabHandle::layout()
{
  return TextureLayout(&mTexture->first, (CompRect*) this);
}

Unity::MT::GrabHandle::GrabHandle(TextureSize* t, Window owner, unsigned int id) :
  mIpw(0),
  mOwner(owner),
  mTexture(t),
  mId(id)
{
  setX(0);
  setY(0);
  setSize(t->second);
}

Unity::MT::GrabHandle::~GrabHandle()
{
  if (mIpw)
  {
    UnityMTGrabHandlesScreen::get(screen)->removeHandleWindow(mIpw);

    XDestroyWindow(screen->dpy(), mIpw);
  }
}

void
Unity::MT::GrabHandle::handleButtonPress(XButtonEvent* be)
{
  /* Send _NET_MOVERESIZE to root window so that a button-1
   * press on this window will start resizing the window around */
  XEvent     event;
  CompWindow* w;
  w = screen->findTopLevelWindow(mOwner, false);

  if (!w)
    return;

  if (screen->getOption("raise_on_click"))
    w->updateAttributes(CompStackingUpdateModeAboveFullscreen);

  if (w->id() != screen->activeWindow())
    if (w->focus())
      w->moveInputFocusTo();

  event.xclient.type    = ClientMessage;
  event.xclient.display = screen->dpy();

  event.xclient.serial    = 0;
  event.xclient.send_event    = true;

  /* FIXME: Need to call findWindow since we need to send the
   * _NET_WM_MOVERESIZE request to the client not the frame
   * which we are tracking. That's a bit shitty */
  event.xclient.window      = w->id();
  event.xclient.message_type      = Atoms::wmMoveResize;
  event.xclient.format      = 32;

  event.xclient.data.l[0] = be->x_root;
  event.xclient.data.l[1] = be->y_root;
  event.xclient.data.l[2] = mId;
  event.xclient.data.l[3] = be->button;
  event.xclient.data.l[4] = 1;

  XSendEvent(screen->dpy(), screen->root(), false,
             SubstructureRedirectMask | SubstructureNotifyMask,
             &event);
}

void
Unity::MT::GrabHandleGroup::show()
{
  foreach(Unity::MT::GrabHandle & handle, *this)
  handle.show();

  mState = FADE_IN;
}

void
Unity::MT::GrabHandleGroup::hide()
{
  foreach(Unity::MT::GrabHandle & handle, *this)
  handle.hide();

  mState = FADE_OUT;
}

bool
Unity::MT::GrabHandleGroup::animate(unsigned int msec)
{
  mMoreAnimate = false;

  switch (mState)
  {
    case FADE_IN:

      mOpacity += ((float) msec / (float) FADE_MSEC) * OPAQUE;

      if (mOpacity >= OPAQUE)
      {
        mOpacity = OPAQUE;
        mState = NONE;
      }
      break;
    case FADE_OUT:
      mOpacity -= ((float) msec / (float) FADE_MSEC) * OPAQUE;

      if (mOpacity <= 0)
      {
        mOpacity = 0;
        mState = NONE;
      }
      break;
    default:
      break;
  }

  mMoreAnimate = mState != NONE;

  return mMoreAnimate;
}

int
Unity::MT::GrabHandleGroup::opacity()
{
  return mOpacity;
}

bool
Unity::MT::GrabHandleGroup::visible()
{
  return mOpacity > 0.0f;
}

bool
Unity::MT::GrabHandleGroup::needsAnimate()
{
  return mMoreAnimate;
}

void
Unity::MT::GrabHandleGroup::relayout(const CompRect& rect, bool hard)
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
    Unity::MT::GrabHandle& handle = at(i);
    CompPoint p(rect.x() + rect.width() * pos[i][0] -
                handle.width() / 2,
                rect.y() + rect.height() * pos[i][1] -
                handle.height() / 2);

    handle.reposition(&p, hard);
  }
}

Unity::MT::GrabHandleGroup::GrabHandleGroup(Window owner) :
  mState(NONE),
  mOpacity(0.0f),
  mMoreAnimate(false)
{
  UMTGH_SCREEN(screen);

  for (unsigned int i = 0; i < NUM_HANDLES; i++)
    push_back(Unity::MT::GrabHandle(&us->textures().at(i), owner, i));
}

Unity::MT::GrabHandleGroup::~GrabHandleGroup()
{
  UMTGH_SCREEN(screen);

  foreach(Unity::MT::GrabHandle & handle, *this)
  us->cScreen->damageRegion((CompRect&) handle);
}

std::vector <Unity::MT::TextureLayout>
Unity::MT::GrabHandleGroup::layout()
{
  std::vector <Unity::MT::TextureLayout> layout;

  foreach(Unity::MT::GrabHandle & handle, *this)
  layout.push_back(handle.layout());

  return layout;
}

/* Super speed hack */
static bool
sortPointers(const CompWindow* p1, const CompWindow* p2)
{
  return (void*) p1 < (void*) p2;
}

void
UnityMTGrabHandlesScreen::handleEvent(XEvent* event)
{
  Unity::MT::GrabHandle* handle;
  std::map <Window, Unity::MT::GrabHandle*>::iterator it;
  CompWindow* w, *oldPrev, *oldNext;

  w = oldPrev = oldNext = NULL;

  switch (event->type)
  {
    case FocusIn:
    case FocusOut:
      if (event->xfocus.mode == NotifyUngrab)
      {
        foreach(CompWindow * w, screen->windows())
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

        foreach(CompWindow * w, invalidated)
        UnityMTGrabHandlesWindow::get(w)->restackHandles();

        mLastClientListStacking = clients;
      }

      break;

    case ButtonPress:

      if (event->xbutton.button != 1)
        break;

      it = mInputHandles.find(event->xbutton.window);

      if (it != mInputHandles.end())
      {
        handle = it->second;
        if (handle)
          handle->handleButtonPress((XButtonEvent*) event);
      }

      break;
    case ConfigureNotify:

      w = screen->findTopLevelWindow(event->xconfigure.window);

      if (w)
        UnityMTGrabHandlesWindow::get(w)->relayout(w->inputRect(), true);

      break;

    case MapNotify:

      it = mInputHandles.find(event->xmap.window);

      if (it != mInputHandles.end())
      {
        if (it->second)
          it->second->reposition(NULL, true);
      }

      break;
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
    foreach(Unity::MT::GrabHandleGroup * handles, mGrabHandles)
    {
      if (handles->needsAnimate())
      {
        foreach(Unity::MT::GrabHandle & handle, *handles)
        cScreen->damageRegion((CompRect&) handle);
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

    foreach(Unity::MT::GrabHandleGroup * handles, mGrabHandles)
    {
      mMoreAnimate |= handles->animate(msec);
    }
  }

  cScreen->preparePaint(msec);
}

bool
UnityMTGrabHandlesWindow::handleTimerActive()
{
  return _timer_handle != 0;
}

bool
UnityMTGrabHandlesWindow::allowHandles()
{
  /* Not on windows we can't move or resize */
  if (!(window->actions() & CompWindowActionResizeMask))
    return false;

  if (!(window->actions() & CompWindowActionMoveMask))
    return false;

  /* Not on override redirect windows */
  if (window->overrideRedirect())
    return false;

  return true;
}

void
UnityMTGrabHandlesWindow::getOutputExtents(CompWindowExtents& output)
{
  if (mHandles)
  {
    /* Only care about the handle on the outside */
    output.left   = MAX(output.left,   window->borderRect().left() + mHandles->at(0).width() / 2);
    output.right  = MAX(output.right,  window->borderRect().right() + mHandles->at(0).width() / 2);
    output.top    = MAX(output.top,    window->borderRect().top() + mHandles->at(0).height() / 2);
    output.bottom = MAX(output.bottom, window->borderRect().bottom() + mHandles->at(0).height() / 2);
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

  UMTGH_SCREEN(screen);

  if (mHandles && mHandles->visible())
  {
    unsigned int handle = 0;

    foreach(Unity::MT::TextureLayout layout, mHandles->layout())
    {
      /* We want to set the geometry of the dim to the window
       * region */
      CompRegion reg = CompRegion(*layout.second);

      struct _skipInfo
      {
        unsigned int vstate;
        unsigned int hstate;
        unsigned int handles[9];
      };

      const struct _skipInfo skip[3] =
      {
        {
          CompWindowStateMaximizedVertMask,
          CompWindowStateMaximizedVertMask,
          { 1, 1, 1, 0, 1, 1, 1, 0, 0 }
        },
        {
          CompWindowStateMaximizedHorzMask,
          CompWindowStateMaximizedHorzMask,
          {1, 0, 1, 1, 1, 0, 1, 1, 0 }
        },
        {
          CompWindowStateMaximizedVertMask,
          CompWindowStateMaximizedHorzMask,
          {1, 1, 1, 1, 1, 1, 1, 1, 1 }
        }
      };

      foreach(GLTexture * tex, *layout.first)
      {
        GLTexture::MatrixList matl;
        GLTexture::Matrix     mat = tex->matrix();
        CompRegion        paintRegion(region);
        bool          skipHandle = false;

        for (unsigned int j = 0; j < 3; j++)
        {
          if (skip[j].vstate & window->state() &&
              skip[j].hstate & window->state())
          {
            if (skip[j].handles[handle])
            {
              skipHandle = true;
              break;
            }
          }
        }

        if (skipHandle)
          break;

        /* We can reset the window geometry since it will be
         * re-added later */
        gWindow->geometry().reset();

        /* Scale the handles */
        mat.xx *= 1;
        mat.yy *= 1;

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
    mHandles->relayout(r, hard);
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
    mHandles->relayout((const CompRect&) window->inputRect(), false);

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

gboolean
UnityMTGrabHandlesWindow::onHideTimeout(gpointer data)
{
  UnityMTGrabHandlesWindow* self = static_cast<UnityMTGrabHandlesWindow*>(data);

  if (screen->grabbed())
    return true;

  // hack
  self->hideHandles();
  self->_mt_screen->mMoreAnimate = true;
  self->_timer_handle = 0;
  return false;
}

void
UnityMTGrabHandlesWindow::resetTimer()
{
  if (_timer_handle)
    g_source_remove(_timer_handle);

  _timer_handle = g_timeout_add(2000, &UnityMTGrabHandlesWindow::onHideTimeout, this);
}

void
UnityMTGrabHandlesWindow::disableTimer()
{
  if (_timer_handle)
    g_source_remove(_timer_handle);
}

void
UnityMTGrabHandlesWindow::showHandles(bool use_timer)
{
  if (!mHandles)
  {
    mHandles = new Unity::MT::GrabHandleGroup(window->frame());
    UnityMTGrabHandlesScreen::get(screen)->addHandles(mHandles);
  }

  if (!mHandles->visible())
  {
    activate();
    mHandles->show();
    mHandles->relayout(window->inputRect(), true);

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

  foreach(Unity::MT::GrabHandle & handle, *mHandles)
  handle.reposition(NULL, true);
}

void
UnityMTGrabHandlesScreen::addHandleWindow(Unity::MT::GrabHandle* h, Window w)
{
  mInputHandles.insert(std::pair <Window, Unity::MT::GrabHandle*> (w, h));
}

void
UnityMTGrabHandlesScreen::removeHandleWindow(Window w)
{
  mInputHandles.erase(w);
}

void
UnityMTGrabHandlesScreen::addHandles(Unity::MT::GrabHandleGroup* handles)
{
  mGrabHandles.push_back(handles);
}

void
UnityMTGrabHandlesScreen::removeHandles(Unity::MT::GrabHandleGroup* handles)
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
  ScreenInterface::setHandler(s);
  CompositeScreenInterface::setHandler(cScreen);
  GLScreenInterface::setHandler(gScreen);

  mHandleTextures.resize(NUM_HANDLES);

  for (unsigned int i = 0; i < NUM_HANDLES; i++)
  {
    CompString fname = "handle-";
    CompString pname("unitymtgrabhandles");

    fname = compPrintf("%s%i.png", fname.c_str(), i);
    mHandleTextures.at(i).first =
      GLTexture::readImageToTexture(fname, pname,
                                    mHandleTextures.at(i).second);
  }

  optionSetToggleHandlesKeyInitiate(boost::bind(&UnityMTGrabHandlesScreen::toggleHandles, this, _1, _2, _3));
  optionSetShowHandlesKeyInitiate(boost::bind(&UnityMTGrabHandlesScreen::showHandles, this, _1, _2, _3));
  optionSetHideHandlesKeyInitiate(boost::bind(&UnityMTGrabHandlesScreen::hideHandles, this, _1, _2, _3));
}

UnityMTGrabHandlesScreen::~UnityMTGrabHandlesScreen()
{
  while (mGrabHandles.size())
  {
    Unity::MT::GrabHandleGroup* handles = mGrabHandles.back();
    delete handles;
    mGrabHandles.pop_back();
  }

  mHandleTextures.clear();
}

UnityMTGrabHandlesWindow::UnityMTGrabHandlesWindow(CompWindow* w) :
  PluginClassHandler <UnityMTGrabHandlesWindow, CompWindow> (w),
  window(w),
  cWindow(CompositeWindow::get(w)),
  gWindow(GLWindow::get(w)),
  mHandles(NULL)
{
  WindowInterface::setHandler(window);
  CompositeWindowInterface::setHandler(cWindow);
  GLWindowInterface::setHandler(gWindow);

  // hack
  _mt_screen = UnityMTGrabHandlesScreen::get(screen);
  _timer_handle = 0;
}

UnityMTGrabHandlesWindow::~UnityMTGrabHandlesWindow()
{
  if (_timer_handle)
    g_source_remove(_timer_handle);

  if (mHandles)
  {
    UnityMTGrabHandlesScreen::get(screen)->removeHandles(mHandles);
    delete mHandles;

    mHandles = NULL;
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
