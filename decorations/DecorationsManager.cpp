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
#include <NuxGraphics/CairoGraphics.h>
#include <UnityCore/DBusIndicators.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>

#include "WindowManager.h"

namespace unity
{
namespace decoration
{
Manager* manager_ = nullptr;

namespace atom
{
Atom _NET_REQUEST_FRAME_EXTENTS = 0;
Atom _NET_WM_VISIBLE_NAME = 0;
Atom _UNITY_GTK_BORDER_RADIUS = 0;
}

Manager::Impl::Impl(decoration::Manager* parent, menu::Manager::Ptr const& menu)
  : data_pool_(DataPool::Get())
  , menu_manager_(menu)
{
  if (!manager_)
    manager_ = parent;

  Display* dpy = screen->dpy();
  atom::_NET_REQUEST_FRAME_EXTENTS = XInternAtom(dpy, "_NET_REQUEST_FRAME_EXTENTS", False);
  atom::_NET_WM_VISIBLE_NAME = XInternAtom(dpy, "_NET_WM_VISIBLE_NAME", False);
  atom::_UNITY_GTK_BORDER_RADIUS = XInternAtom(dpy, "_UNITY_GTK_BORDER_RADIUS", False);

  auto rebuild_cb = sigc::mem_fun(this, &Impl::OnShadowOptionsChanged);
  manager_->active_shadow_color.changed.connect(sigc::hide(sigc::bind(rebuild_cb, true)));
  manager_->active_shadow_radius.changed.connect(sigc::hide(sigc::bind(rebuild_cb, true)));
  manager_->inactive_shadow_color.changed.connect(sigc::hide(sigc::bind(rebuild_cb, false)));
  manager_->inactive_shadow_radius.changed.connect(sigc::hide(sigc::bind(rebuild_cb, false)));
  manager_->shadow_offset.changed.connect(sigc::hide(sigc::mem_fun(this, &Impl::UpdateWindowsExtents)));
  menu_manager_->integrated_menus.changed.connect(sigc::hide(sigc::mem_fun(this, &Impl::SetupIntegratedMenus)));

  BuildInactiveShadowTexture();
  BuildActiveShadowTexture();
  SetupIntegratedMenus();
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

cu::PixmapTexture::Ptr Manager::Impl::BuildShapedShadowTexture(unsigned int radius, nux::Color const& color, DecorationsShape const& shape) {
  //Ideally it would be shape.getWidth + radius * 2 but Cairographics::BlurSurface isn't bounded by the radius
  //and we need to compensate by using a larger texture. Make sure to modify Window::Impl::ComputeShapedShadowQuad in
  //DecoratedWindow.cpp if you change the factor.
  int blur_margin_factor = 2;
  int img_width = shape.getWidth() + radius * 2 * blur_margin_factor;
  int img_height = shape.getHeight() + radius * 2 * blur_margin_factor;
  nux::CairoGraphics img(CAIRO_FORMAT_ARGB32, img_width, img_height);
  auto* img_ctx = img.GetInternalContext();

  for (int i=0; i<shape.getRectangleCount(); i++) {
    XRectangle rect = shape.getRectangle(i);
    cairo_rectangle(img_ctx, rect.x + radius * blur_margin_factor - shape.getXoffs(), rect.y + radius * blur_margin_factor - shape.getYoffs(), rect.width, rect.height);
    cairo_set_source_rgba(img_ctx, color.red, color.green, color.blue, color.alpha);
    cairo_fill(img_ctx);
  }
  img.BlurSurface(radius);

  cu::CairoContext shadow_ctx(img_width, img_height);
  cairo_set_source_surface(shadow_ctx, img.GetSurface(), 0, 0);
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

void Manager::Impl::SetupIntegratedMenus()
{
  if (!menu_manager_->integrated_menus())
  {
    UnsetAppMenu();
    menu_connections_.Clear();
    return;
  }

  menu_connections_.Add(menu_manager_->appmenu_added.connect(sigc::mem_fun(this, &Impl::SetupAppMenu)));
  menu_connections_.Add(menu_manager_->appmenu_removed.connect(sigc::mem_fun(this, &Impl::UnsetAppMenu)));
  menu_connections_.Add(menu_manager_->key_activate_entry.connect(sigc::mem_fun(this, &Impl::OnMenuKeyActivated)));
  menu_connections_.Add(menu_manager_->always_show_menus.changed.connect(sigc::hide(sigc::mem_fun(this, &Impl::SetupAppMenu))));

  SetupAppMenu();
}

bool Manager::Impl::OnMenuKeyActivated(std::string const& entry_id)
{
  // Lambda will crash with this, unless we don't use a workaround.
  return active_deco_win_ && active_deco_win_->impl_->ActivateMenu(entry_id);
}

void Manager::Impl::SetupAppMenu()
{
  auto const& appmenu = menu_manager_->AppMenu();

  if (!appmenu)
  {
    UnsetAppMenu();
    return;
  }

  for (auto const& win : windows_)
    win.second->impl_->SetupAppMenu();

  menu_connections_.Remove(appmenu_connection_);
  appmenu_connection_ = menu_connections_.Add(appmenu->updated_win.connect([this] (uint32_t xid) {
    if (Window::Ptr const& win = GetWindowByXid(xid))
      win->impl_->SetupAppMenu();
  }));
}

void Manager::Impl::UnsetAppMenu()
{
  menu_connections_.Remove(appmenu_connection_);

  for (auto const& win : windows_)
  {
    win.second->impl_->UnsetAppMenu();
    win.second->impl_->Damage();
  }
}

void Manager::Impl::UpdateWindowsExtents()
{
  for (auto const& win : windows_)
    win.second->impl_->RedrawDecorations();
}

bool Manager::Impl::UpdateWindow(::Window xid)
{
  auto const& win = GetWindowByXid(xid);

  if (win && !win->GetCompWindow()->hasUnmapReference())
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
  switch (event->type)
  {
    case ClientMessage:
      if (event->xclient.message_type == atom::_NET_REQUEST_FRAME_EXTENTS)
      {
        if (Window::Ptr const& win = GetWindowByXid(event->xclient.window))
          win->impl_->SendFrameExtents();
      }
      else if (event->xclient.message_type == Atoms::toolkitAction)
      {
        Atom msg = event->xclient.data.l[0];
        if (msg == Atoms::toolkitActionForceQuitDialog)
        {
          if (Window::Ptr const& win = GetWindowByXid(event->xclient.window))
          {
            Time time = event->xclient.data.l[1];
            bool show = event->xclient.data.l[2];
            win->impl_->ShowForceQuitDialog(show, time);
            return true;
          }
        }
      }
      break;
    case MotionNotify:
    case EnterNotify:
    case LeaveNotify:
    case ButtonPress:
    case ButtonRelease:
      if (HandleFrameEvent(event))
        return true;
    case FocusOut:
      if (event->xfocus.mode == NotifyGrab && !last_mouse_owner_.expired())
      {
        last_mouse_owner_.lock()->UngrabPointer();
        last_mouse_owner_.reset();
      }
      break;
  }

  return false;
}

bool Manager::Impl::HandleEventAfter(XEvent* event)
{
  switch (event->type)
  {
    case PropertyNotify:
    {
      if (event->xproperty.atom == Atoms::winActive)
      {
        if (active_deco_win_)
          active_deco_win_->impl_->active = false;

        auto active_xid = screen->activeWindow();
        auto const& new_active = GetWindowByXid(active_xid);
        active_deco_win_ = new_active;

        if (new_active)
          new_active->impl_->active = true;
      }
      else if (event->xproperty.atom == Atoms::mwmHints ||
               event->xproperty.atom == Atoms::wmAllowedActions)
      {
        if (Window::Ptr const& win = GetWindowByXid(event->xproperty.window))
          win->impl_->UpdateFrameActions();
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
      else if (event->xproperty.atom == atom::_UNITY_GTK_BORDER_RADIUS)
      {
        UpdateWindow(event->xproperty.window);
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
  if (WindowManager::Default().IsScaleActive())
    return false;

  auto const& win = GetWindowByFrame(event->xany.window);
  CompWindow* comp_window = win ? win->GetCompWindow() : nullptr;

  if (comp_window && comp_window->defaultViewport() != screen->vp())
    return false;

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
      input_mixer->MotionEvent(CompPoint(event->xmotion.x_root, event->xmotion.y_root), event->xmotion.time);
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
      input_mixer->ButtonDownEvent(CompPoint(event->xbutton.x_root, event->xbutton.y_root), event->xbutton.button, event->xbutton.time);
      if (input_mixer->GetMouseOwner())
        last_mouse_owner_ = input_mixer;
      break;
    }
    case ButtonRelease:
    {
      input_mixer->ButtonUpEvent(CompPoint(event->xbutton.x_root, event->xbutton.y_root), event->xbutton.button, event->xbutton.time);
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

Manager::Manager(menu::Manager::Ptr const& menu)
  : shadow_offset(Style::Get()->ShadowOffset())
  , active_shadow_color(Style::Get()->ActiveShadowColor())
  , active_shadow_radius(Style::Get()->ActiveShadowRadius())
  , inactive_shadow_color(Style::Get()->InactiveShadowColor())
  , inactive_shadow_radius(Style::Get()->InactiveShadowRadius())
  , impl_(new Impl(this, menu))
{}

Manager::~Manager()
{
  if (manager_ == this)
    manager_ = nullptr;
}

void Manager::AddSupportedAtoms(std::vector<Atom>& atoms) const
{
  atoms.push_back(atom::_UNITY_GTK_BORDER_RADIUS);
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
  .add("active_window", screen->activeWindow());
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
