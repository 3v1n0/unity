// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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
* Authored by: Marco Trevisan <marco.trevisan@canonical.com>
*/

#include <Nux/Nux.h>
#include <NuxCore/Logger.h>

#include "DecorationsManagerPriv.h"
#include "WindowManager.h"

namespace unity
{
namespace decoration
{

namespace
{
DECLARE_LOGGER(logger, "unity.decoration.manager");

const unsigned GRAB_BORDER = 10;

Display* dpy = nullptr;
Manager* manager_ = nullptr;

namespace atom
{
Atom _NET_REQUEST_FRAME_EXTENTS = 0;
Atom _NET_FRAME_EXTENTS = 0;
}
}

Manager::Impl::Impl(UnityScreen* uscreen)
  : uscreen_(uscreen)
  , cscreen_(uscreen_->screen)
  , active_window_(0)
{
  dpy = cscreen_->dpy();
  atom::_NET_REQUEST_FRAME_EXTENTS = XInternAtom(dpy, "_NET_REQUEST_FRAME_EXTENTS", False);
  atom::_NET_FRAME_EXTENTS = XInternAtom(dpy, "_NET_FRAME_EXTENTS", False);
  cscreen_->updateSupportedWmHints();
}

Manager::Impl::~Impl()
{
  cscreen_->addSupportedAtomsSetEnabled(uscreen_, false);
  cscreen_->updateSupportedWmHints();
}

bool Manager::Impl::UpdateWindow(::Window xid)
{
  auto const& win = GetWindowByXid(xid);

  if (win /* && !window->hasUnmapReference()*/)
  {
    win->Update();
    return true;
  }

  return false;
}

Window::Ptr Manager::Impl::GetWindowByXid(::Window xid)
{
  for (auto const& pair : windows_)
  {
    if (pair.first->window->id() == xid)
      return pair.second;
  }

  return nullptr;
}

Window::Ptr Manager::Impl::GetWindowByFrame(::Window xid)
{
  for (auto const& pair : windows_)
  {
    if (pair.second->impl_->frame_ == xid)
      return pair.second;
  }

  return nullptr;
}

bool Manager::Impl::HandleEventBefore(XEvent* event)
{
  active_window_ = cscreen_->activeWindow();
  switch (event->type)
  {
    case ClientMessage:
      if (event->xclient.message_type == atom::_NET_REQUEST_FRAME_EXTENTS)
        UpdateWindow(event->xclient.window);
      break;
  }

  return false;
}

bool Manager::Impl::HandleEventAfter(XEvent* event)
{
  if (cscreen_->activeWindow() != active_window_)
  {
    UpdateWindow(active_window_);
    active_window_ = cscreen_->activeWindow();
    UpdateWindow(active_window_);
  }

  switch (event->type)
  {
    case ClientMessage:
      if (event->xclient.message_type == atom::_NET_REQUEST_FRAME_EXTENTS)
      {
        // return true;
      }
      break;
    case EnterNotify:
      break;
    case LeaveNotify:
      break;
    case MotionNotify:
      break;
    case PropertyNotify:
      if (event->xproperty.atom == Atoms::mwmHints)
      {
        UpdateWindow(event->xproperty.window);
      }
      break;
    case ConfigureNotify:
      UpdateWindow(event->xconfigure.window);
      break;
    default:
      if (cscreen_->XShape() && event->type == cscreen_->shapeEvent() + ShapeNotify)
      {
        auto window = reinterpret_cast<XShapeEvent*>(event)->window;
        if (!UpdateWindow(window))
        {
          auto const& win = GetWindowByFrame(window);

          if (win)
            win->impl_->SyncXShapeWithFrameRegion();
        }
      }
      break;
  }

  return false;
}

Manager::Manager(UnityScreen *screen)
  : impl_(new Manager::Impl(screen))
{
  manager_ = this;
}

Manager::~Manager()
{}

void Manager::AddSupportedAtoms(std::vector<Atom>& atoms) const
{
  atoms.push_back(atom::_NET_REQUEST_FRAME_EXTENTS);
}

bool Manager::HandleEventBefore(XEvent* xevent)
{
  return impl_->HandleEventBefore(xevent);
}

bool Manager::HandleEventAfter(XEvent* xevent)
{
  return impl_->HandleEventAfter(xevent);
}

Window::Ptr Manager::HandleWindow(UnityWindow* uwin)
{
  auto win = std::make_shared<Window>(uwin);
  impl_->windows_[uwin] = win;
  return win;
}

void Manager::UnHandleWindow(UnityWindow* uwin)
{
  impl_->windows_.erase(uwin);
}

Window::Ptr Manager::GetWindowByXid(::Window xid)
{
  return impl_->GetWindowByXid(xid);
}


///////////////////


Window::Impl::Impl(Window* parent, UnityWindow* uwin)
  : parent_(parent)
  , uwin_(uwin)
  , frame_(0)
  , undecorated_(false)
{
  auto* window = uwin_->window;

  if (window->shaded() || window->isViewable())
  {
    window->resizeNotifySetEnabled(uwin_, false);
    Update();
    window->resizeNotifySetEnabled(uwin_, true);
  }
}

Window::Impl::~Impl()
{
  UnsetExtents();
  UnsetFrame();
}

void Window::Impl::Update()
{
  if (ShouldBeDecorated())
  {
    SetupExtents();
    UpdateFrame();
  }
  else/* if (frame_ || !undecorated_)*/
  {
    UnsetExtents();
    UnsetFrame();
  }
}

void Window::Impl::UnsetExtents()
{
  if (uwin_->window->destroyed() || uwin_->window->hasUnmapReference())
    return;

  CompWindowExtents empty(0, 0, 0, 0);
  uwin_->window->setWindowFrameExtents(&empty, &empty);
  uwin_->window->updateWindowOutputExtents();
}

void Window::Impl::SetupExtents()
{
  auto* window = uwin_->window;

  if (!window->hasUnmapReference())
  {
    CompWindowExtents border(5, 5, 28, 5);
    CompWindowExtents input(border);
    input.left += GRAB_BORDER;
    input.right += GRAB_BORDER;
    input.top += GRAB_BORDER;
    input.bottom += GRAB_BORDER;

    window->setWindowFrameExtents(&border, &input);
    uwin_->window->updateWindowOutputExtents();
  }

  uwin_->cWindow->damageOutputExtents();
}

void Window::Impl::UnsetFrame()
{
  if (!frame_)
    return;

  XDestroyWindow(dpy, frame_);
  frame_ = 0;
  frame_geo_.Set(0, 0, 0, 0);
}

void Window::Impl::UpdateFrame()
{
  auto* window = uwin_->window;
  auto const& input = window->input();
  auto const& server = window->serverGeometry();
  nux::Geometry frame_geo(0, 0,/*input.left, input.top,*/
                          server.widthIncBorders() + input.left + input.right,
                          server.heightIncBorders() + input.top  + input.bottom);

  if (window->shaded())
    frame_geo.height = input.top + input.bottom;

  if (!frame_)
  {
    /* Since we're reparenting windows here, we need to grab the server
     * which sucks, but its necessary */
    XGrabServer (screen->dpy ());

    std::cout << "WIndow frame " << window->frame () << " for " << window->id() << " " << WindowManager::Default().GetWindowName(window->id()) << std::endl;
    XSetWindowAttributes attr;
    attr.event_mask = StructureNotifyMask;
    // attr.event_mask = ButtonPressMask | EnterWindowMask | LeaveWindowMask | ExposureMask;
    attr.override_redirect = True;

    auto parent = window->frame(); // id()?
    frame_ = XCreateWindow(dpy, parent, frame_geo.x, frame_geo.y,
                           frame_geo.width, frame_geo.height, 0, CopyFromParent,
                           InputOnly, CopyFromParent, CWOverrideRedirect | CWEventMask, &attr);

    if (manager_->impl_->cscreen_->XShape())
      XShapeSelectInput(dpy, frame_, ShapeNotifyMask);

    XMapWindow(dpy, frame_);

    XGrabButton(dpy, AnyButton, AnyModifier, frame_, True,
                ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, GrabModeSync,
                GrabModeSync, None, None);

    XSelectInput(dpy, frame_, ButtonPressMask | ButtonReleaseMask | EnterWindowMask |
                              LeaveWindowMask | PointerMotionMask | ButtonMotionMask |
                              PropertyChangeMask /*| FocusChangeMask*/);

    XUngrabServer(dpy);
  }

  if (frame_geo_ != frame_geo)
  {
    XMoveResizeWindow(dpy, frame_, frame_geo.x, frame_geo.y, frame_geo.width, frame_geo.height);
    XLowerWindow(dpy, frame_);

    int i = 0;
    XRectangle rects[4];

    rects[i].x = 0;
    rects[i].y = 0;
    rects[i].width = frame_geo.width;
    rects[i].height = input.top;

    if (rects[i].width && rects[i].height)
        i++;

    rects[i].x = 0;
    rects[i].y = input.top;
    rects[i].width = input.left;
    rects[i].height = frame_geo.height - input.top - input.bottom;

    if (rects[i].width && rects[i].height)
        i++;

    rects[i].x = frame_geo.width - input.right;
    rects[i].y = input.top;
    rects[i].width = input.right;
    rects[i].height = frame_geo.height - input.top - input.bottom;

    if (rects[i].width && rects[i].height)
        i++;

    rects[i].x = 0;
    rects[i].y = frame_geo.height - input.bottom;
    rects[i].width = frame_geo.width;
    rects[i].height = input.bottom;

    if (rects[i].width && rects[i].height)
        i++;

    XShapeCombineRectangles(dpy, frame_, ShapeBounding, 0, 0, rects, i, ShapeSet, YXBanded);

    frame_geo_ = frame_geo;
    SyncXShapeWithFrameRegion();
  }
}

void Window::Impl::SyncXShapeWithFrameRegion()
{
  frame_region_ = CompRegion();

  int n = 0;
  int order = 0;
  XRectangle *rects = NULL;

  rects = XShapeGetRectangles(dpy, frame_, ShapeInput, &n, &order);
  if (!rects)
    return;

  for (int i = 0; i < n; ++i)
  {
    auto& rect = rects[i];
    frame_region_ += CompRegion(rect.x, rect.y, rect.width, rect.height);
  }

  XFree(rects);

  uwin_->window->updateFrameRegion();
}

bool Window::Impl::FullyDecorated() const
{
  auto* window = uwin_->window;

  if (!window->isViewable()) // Check me!
    return false;

  if ((window->state() & MAXIMIZE_STATE) == MAXIMIZE_STATE)
    return false;

  if (window->overrideRedirect())
    return false;

  if (window->wmType() & (CompWindowTypeDockMask | CompWindowTypeDesktopMask))
    return false;

  switch (window->type())
  {
    case CompWindowTypeDialogMask:
    case CompWindowTypeModalDialogMask:
    case CompWindowTypeUtilMask:
    case CompWindowTypeMenuMask:
    case CompWindowTypeNormalMask:
      if (window->mwmDecor() & (MwmDecorAll | MwmDecorTitle))
        return true;
  }

  return false;
}

bool Window::Impl::ShouldBeDecorated() const
{
  auto* window = uwin_->window;

  return (window->frame() || window->hasUnmapReference()) && FullyDecorated();
}

Window::Window(UnityWindow* uwin)
  : impl_(new Impl(this, uwin))
{}

Window::~Window()
{}

void Window::Update()
{
  impl_->Update();
}

void Window::UpdateFrameRegion(CompRegion* r)
{
  if (impl_->frame_region_.isEmpty())
    return;

  auto* window = impl_->uwin_->window;
  auto const& geo = window->geometry();
  auto const& input = window->input();

  *r += impl_->frame_region_.translated(geo.x() - input.left, geo.y() - input.top);
}

} // decoration namespace
} // unity namespace
