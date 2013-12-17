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

Manager::Impl::Impl(decoration::Manager* parent)
  : active_window_(0)
  , enable_add_supported_atoms_(true)
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
  SetupButtonsTextures();
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

void Manager::Impl::SetupButtonsTextures()
{
  CompSize size;
  CompString plugin_name("unityshell");
  auto const& style = Style::Get();

  for (unsigned button = 0; button < window_buttons_.size(); ++button)
  {
    for (unsigned state = 0; state < window_buttons_[button].size(); ++state)
    {
      auto file = style->WindowButtonFile(WindowButtonType(button), WidgetState(state));
      auto const& tex_list = GLTexture::readImageToTexture(file, plugin_name, size);

      if (!tex_list.empty())
      {
        LOG_DEBUG(logger) << "Loading texture " << file;
        window_buttons_[button][state] = std::make_shared<cu::SimpleTexture>(tex_list);
      }
      else
      {
        LOG_WARN(logger) << "Impossible to load button texture " << file;
        // Generate cairo texture!
      }
    }
  }
}

cu::SimpleTexture::Ptr const& Manager::Impl::GetButtonTexture(WindowButtonType wbt, WidgetState ws) const
{
  return window_buttons_[unsigned(wbt)][unsigned(ws)];
}

void Manager::Impl::UpdateWindowsExtents()
{
  for (auto const& win : windows_)
  {
    win.second->UpdateDecorationPosition();
    win.second->impl_->cwin_->damageOutputExtents();
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
      if (event->xproperty.atom == Atoms::mwmHints)
        UpdateWindow(event->xproperty.window);
      break;
    case ConfigureNotify:
      UpdateWindow(event->xconfigure.window);
      break;
    case MotionNotify:
    case EnterNotify:
    case LeaveNotify:
    case ButtonPress:
    case ButtonRelease:
      if (HandleFrameEvent(event))
        return true;
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

bool Manager::Impl::HandleFrameEvent(XEvent* event)
{
  auto const& win = GetWindowByFrame(event->xany.window);

  if (!win)
    return false;

  auto const& input_mixer = win->impl_->input_mixer_;

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
      break;
    }
    case ButtonRelease:
    {
      input_mixer->ButtonUpEvent(CompPoint(event->xbutton.x_root, event->xbutton.y_root), event->xbutton.button);
      break;
    }
  }

  return true;
}

// Public APIs

Manager::Manager()
  : shadow_offset(nux::Point(1, 1))
  , active_shadow_color(nux::color::Black * 0.4)
  , active_shadow_radius(8)
  , inactive_shadow_color(nux::color::Black * 0.4)
  , inactive_shadow_radius(5)
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
  auto win = std::make_shared<Window>(cwin);
  impl_->windows_[cwin] = win;
  return win;
}

void Manager::UnHandleWindow(CompWindow* cwin)
{
  impl_->windows_.erase(cwin);
}

Window::Ptr Manager::GetWindowByXid(::Window xid)
{
  return impl_->GetWindowByXid(xid);
}

} // decoration namespace
} // unity namespace
