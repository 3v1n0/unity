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

#include "DecorationsPriv.h"

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
Atom _NET_FRAME_EXTENTS = 0;
}
}

Manager::Impl::Impl(decoration::Manager* parent, UnityScreen* uscreen)
  : uscreen_(uscreen)
  , active_window_(0)
{
  manager_ = parent;
  dpy = screen->dpy();
  atom::_NET_REQUEST_FRAME_EXTENTS = XInternAtom(dpy, "_NET_REQUEST_FRAME_EXTENTS", False);
  atom::_NET_FRAME_EXTENTS = XInternAtom(dpy, "_NET_FRAME_EXTENTS", False);
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
  screen->addSupportedAtomsSetEnabled(uscreen_, false);
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
  {
    win.second->UpdateDecorationPosition();
    win.first->cWindow->damageOutputExtents();
  }
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
  active_window_ = screen->activeWindow();
  switch (event->type)
  {
    case ClientMessage:
      if (event->xclient.message_type == atom::_NET_REQUEST_FRAME_EXTENTS)
      {
        UpdateWindow(event->xclient.window);
      }
      break;
  }

  return false;
}

bool Manager::Impl::HandleEventAfter(XEvent* event)
{
  if (screen->activeWindow() != active_window_)
  {
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
      if (event->xproperty.atom == Atoms::mwmHints)
      {
        UpdateWindow(event->xproperty.window);
      }
      break;
    case ConfigureNotify:
      UpdateWindow(event->xconfigure.window);
      break;
    default:
      if (screen->XShape() && event->type == screen->shapeEvent() + ShapeNotify)
      {
        auto window = reinterpret_cast<XShapeEvent*>(event)->window;
        if (!UpdateWindow(window))
        {
          auto const& win = GetWindowByFrame(window);

          if (win)
          {
            win->impl_->SyncXShapeWithFrameRegion();
          }
        }
      }
      break;
  }

  return false;
}

// Public APIs

Manager::Manager(UnityScreen *screen)
  : shadow_offset(nux::Point(1, 1))
  , active_shadow_color(nux::color::Black * 0.4)
  , active_shadow_radius(8)
  , inactive_shadow_color(nux::color::Black * 0.4)
  , inactive_shadow_radius(5)
  , impl_(new Impl(this, screen))
{}

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

} // decoration namespace
} // unity namespace
