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

#include "DecorationsPriv.h"

#include <core/atoms.h>
#include <NuxCore/Logger.h>
#include <NuxGraphics/CairoGraphics.h>
#include <X11/Xatom.h>
#include "WindowManager.h"

namespace unity
{
namespace decoration
{
Display* dpy = nullptr;
Manager* manager_ = nullptr;

namespace
{
DECLARE_LOGGER(logger, "unity.decoration.manager");

namespace atom
{
Atom _NET_REQUEST_FRAME_EXTENTS = 0;
Atom _NET_WM_VISIBLE_NAME = 0;
}
}

Manager::Impl::Impl(decoration::Manager* parent)
  : active_window_(0)
  , enable_add_supported_atoms_(true)
  , data_pool_(DataPool::Get())
{
  manager_ = parent;
  dpy = screen->dpy();
  atom::_NET_REQUEST_FRAME_EXTENTS = XInternAtom(dpy, "_NET_REQUEST_FRAME_EXTENTS", False);
  atom::_NET_WM_VISIBLE_NAME = XInternAtom(dpy, "_NET_WM_VISIBLE_NAME", False);
  screen->updateSupportedWmHints();

  auto rebuild_cb = sigc::mem_fun(this, &Impl::OnShadowOptionsChanged);
  manager_->active_shadow_color.changed.connect(sigc::hide(sigc::bind(rebuild_cb, true)));
  manager_->active_shadow_radius.changed.connect(sigc::hide(sigc::bind(rebuild_cb, true)));
  manager_->inactive_shadow_color.changed.connect(sigc::hide(sigc::bind(rebuild_cb, false)));
  manager_->inactive_shadow_radius.changed.connect(sigc::hide(sigc::bind(rebuild_cb, false)));
  manager_->shadow_offset.changed.connect(sigc::hide(sigc::mem_fun(this, &Impl::UpdateWindowsExtents)));

  BuildInactiveShadowTexture();
  BuildActiveShadowTexture();
}

Manager::Impl::~Impl()
{
  enable_add_supported_atoms_ = false;
  screen->updateSupportedWmHints();
}

cu::PixmapTexture::Ptr Manager::Impl::BuildShadowTexture(unsigned radius, nux::Color const& color)
{
  int tex_size = radius * 4;

  nux::CairoGraphics dummy(CAIRO_FORMAT_ARGB32, tex_size, tex_size);
  auto* dummy_ctx = dummy.GetInternalContext();
  cairo_rectangle(dummy_ctx, radius*2, radius*2, tex_size, tex_size);
  cairo_set_source_rgba(dummy_ctx, color.red, color.green, color.blue, color.alpha);
  cairo_fill(dummy_ctx);
  dummy.BlurSurface(radius);

  cu::CairoContext shadow_ctx(tex_size, tex_size);
  cairo_set_source_surface(shadow_ctx, dummy.GetSurface(), 0, 0);
  cairo_paint(shadow_ctx);
  return shadow_ctx;
}

void Manager::Impl::BuildActiveShadowTexture()
{
  active_shadow_pixmap_ = BuildShadowTexture(manager_->active_shadow_radius(), manager_->active_shadow_color());
}

void Manager::Impl::BuildInactiveShadowTexture()
{
  inactive_shadow_pixmap_ = BuildShadowTexture(manager_->inactive_shadow_radius(), manager_->inactive_shadow_color());
}

void Manager::Impl::OnShadowOptionsChanged(bool active)
{
  if (active)
    BuildActiveShadowTexture();
  else
    BuildInactiveShadowTexture();

  UpdateWindowsExtents();
}

void Manager::Impl::UpdateWindowsExtents()
{
  for (auto const& win : windows_)
    win.second->impl_->RedrawDecorations();
}

bool Manager::Impl::UpdateWindow(::Window xid)
{
  auto const& win = GetWindowByXid(xid);

  if (win && !win->impl_->win_->hasUnmapReference())
  {
    win->Update();
    return true;
  }

  return false;
}

Window::Ptr Manager::Impl::GetWindowByXid(::Window xid) const
{
  for (auto const& pair : windows_)
  {
    if (pair.first->id() == xid)
      return pair.second;
  }

  return nullptr;
}

Window::Ptr Manager::Impl::GetWindowByFrame(::Window xid) const
{
  auto it = framed_windows_.find(xid);
  if (it == framed_windows_.end())
    return nullptr;

  return it->second.lock();
}

bool Manager::Impl::HandleEventBefore(XEvent* event)
{
  active_window_ = screen->activeWindow();
  switch (event->type)
  {
    case ClientMessage:
      if (event->xclient.message_type == atom::_NET_REQUEST_FRAME_EXTENTS)
      {
        if (Window::Ptr const& win = GetWindowByXid(event->xclient.window))
          win->impl_->Decorate();
      }
      break;
    case MotionNotify:
    case EnterNotify:
    case LeaveNotify:
    case ButtonPress:
    case ButtonRelease:
      if (HandleFrameEvent(event))
        return true;
      break;
  }

  return false;
}

bool Manager::Impl::HandleEventAfter(XEvent* event)
{
  if (screen->activeWindow() != active_window_)
  {
    // Do this when _NET_ACTIVE_WINDOW changes on root!
    auto const& old_active = GetWindowByXid(active_window_);

    if (old_active)
      old_active->impl_->active = false;

    active_window_ = screen->activeWindow();

    auto const& new_active = GetWindowByXid(active_window_);

    if (new_active)
      new_active->impl_->active = true;
  }

  switch (event->type)
  {
    case PropertyNotify:
    {
      if (event->xproperty.atom == Atoms::mwmHints)
      {
        if (Window::Ptr const& win = GetWindowByXid(event->xproperty.window))
        {
          win->impl_->CleanupWindowControls();
          win->Update();
        }
      }
      else if (event->xproperty.atom == XA_WM_NAME ||
               event->xproperty.atom == Atoms::wmName ||
               event->xproperty.atom == atom::_NET_WM_VISIBLE_NAME)
      {
        if (Window::Ptr const& win = GetWindowByXid(event->xproperty.window))
        {
          auto& wm = WindowManager::Default();
          win->title = wm.GetStringProperty(event->xproperty.window, event->xproperty.atom);
        }
      }
      break;
    }
    case ConfigureNotify:
      UpdateWindow(event->xconfigure.window);
      break;
    default:
      if (screen->XShape() && event->type == screen->shapeEvent() + ShapeNotify)
      {
        auto window = reinterpret_cast<XShapeEvent*>(event)->window;
        if (!UpdateWindow(window))
        {
          if (Window::Ptr const& win = GetWindowByFrame(window))
            win->impl_->SyncXShapeWithFrameRegion();
        }
      }
      break;
  }

  return false;
}

bool Manager::Impl::HandleFrameEvent(XEvent* event)
{
  auto const& win = GetWindowByFrame(event->xany.window);

  // ButtonRelease events might happen also outside the frame window, in this
  // case we must unset the mouse owner, wherever the event happens.
  if (!win && event->type != ButtonRelease)
    return false;

  auto const& input_mixer = win ? win->impl_->input_mixer_ : last_mouse_owner_.lock();

  if (!input_mixer)
    return false;

  switch (event->type)
  {
    case MotionNotify:
    {
      input_mixer->MotionEvent(CompPoint(event->xmotion.x_root, event->xmotion.y_root));
      break;
    }
    case EnterNotify:
    {
      input_mixer->EnterEvent(CompPoint(event->xcrossing.x_root, event->xcrossing.y_root));
      break;
    }
    case LeaveNotify:
    {
      input_mixer->LeaveEvent(CompPoint(event->xcrossing.x_root, event->xcrossing.y_root));
      break;
    }
    case ButtonPress:
    {
      input_mixer->ButtonDownEvent(CompPoint(event->xbutton.x_root, event->xbutton.y_root), event->xbutton.button);
      if (input_mixer->GetMouseOwner())
        last_mouse_owner_ = input_mixer;
      break;
    }
    case ButtonRelease:
    {
      input_mixer->ButtonUpEvent(CompPoint(event->xbutton.x_root, event->xbutton.y_root), event->xbutton.button);
      last_mouse_owner_.reset();
      break;
    }
  }

  // This causes the Alt+Move not to work, we should probably return this value based on the actual handled state
  // return true;
  return false;
}

Window::Ptr Manager::Impl::HandleWindow(CompWindow* cwin)
{
  auto win = std::make_shared<Window>(cwin);
  auto* wimpl = win->impl_.get();

  std::weak_ptr<decoration::Window> weak_win(win);
  wimpl->framed.connect(sigc::bind(sigc::mem_fun(this, &Impl::OnWindowFrameChanged), weak_win));
  windows_[cwin] = win;

  if (wimpl->frame_)
    framed_windows_[wimpl->frame_] = win;

  return win;
}

void Manager::Impl::OnWindowFrameChanged(bool framed, ::Window frame, std::weak_ptr<decoration::Window> const& window)
{
  if (!framed || !frame)
    framed_windows_.erase(frame);
  else
    framed_windows_[frame] = window;
}

// Public APIs

Manager::Manager()
  : shadow_offset(Style::Get()->ShadowOffset())
  , active_shadow_color(Style::Get()->ActiveShadowColor())
  , active_shadow_radius(Style::Get()->ActiveShadowRadius())
  , inactive_shadow_color(Style::Get()->InactiveShadowColor())
  , inactive_shadow_radius(Style::Get()->InactiveShadowRadius())
  , impl_(new Impl(this))
{}

Manager::~Manager()
{}

void Manager::AddSupportedAtoms(std::vector<Atom>& atoms) const
{
  if (impl_->enable_add_supported_atoms_)
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

Window::Ptr Manager::HandleWindow(CompWindow* cwin)
{
  return impl_->HandleWindow(cwin);
}

void Manager::UnHandleWindow(CompWindow* cwin)
{
  impl_->windows_.erase(cwin);
}

Window::Ptr Manager::GetWindowByXid(::Window xid)
{
  return impl_->GetWindowByXid(xid);
}

std::string Manager::GetName() const
{
  return "DecorationsManager";
}

void Manager::AddProperties(debug::IntrospectionData& data)
{
  data.add("shadow_offset", shadow_offset())
  .add("active_shadow_color", active_shadow_color())
  .add("active_shadow_radius", active_shadow_radius())
  .add("inactive_shadow_color", inactive_shadow_color())
  .add("inactive_shadow_radius", inactive_shadow_radius())
  .add("active_window", impl_->active_window_);
}

debug::Introspectable::IntrospectableList Manager::GetIntrospectableChildren()
{
  IntrospectableList children;

  for (auto const& win : impl_->windows_)
    children.push_back(win.second.get());

  return children;
}


} // decoration namespace
} // unity namespace
