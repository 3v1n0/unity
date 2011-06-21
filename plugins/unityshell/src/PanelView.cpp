// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>
#include <Nux/HLayout.h>
#include <Nux/Layout.h>
#include <Nux/WindowCompositor.h>

#include <NuxImage/CairoGraphics.h>
#include <NuxImage/ImageSurface.h>

#include <NuxGraphics/GLThread.h>
#include <NuxGraphics/RenderingPipe.h>

#include <glib.h>

#include "PanelStyle.h"
#include "PanelIndicatorObjectView.h"
#include <UnityCore/UnityCore.h>

#include "PanelView.h"


namespace unity {

NUX_IMPLEMENT_OBJECT_TYPE (PanelView);

PanelView::PanelView (NUX_FILE_LINE_DECL)
:   View (NUX_FILE_LINE_PARAM),
  _is_dirty (true),
  _opacity (1.0f),
  _is_primary (false),
  _monitor (0)
{
  _needs_geo_sync = false;
  _style = new PanelStyle ();
  _on_panel_style_changed_connection = _style->changed.connect (sigc::mem_fun (this, &PanelView::ForceUpdateBackground));

  _bg_layer = new nux::ColorLayer (nux::Color (0xff595853), true);

  _layout = new nux::HLayout ("", NUX_TRACKER_LOCATION);

   // Home button - not an indicator view
   _home_button = new PanelHomeButton();
   _layout->AddView(_home_button, 0, nux::eCenter, nux::eFull);
   AddChild(_home_button);

   _menu_view = new PanelMenuView ();
   AddPanelView(_menu_view, 1);

   SetCompositionLayout (_layout);
   
   // Pannel tray shouldn't be an indicator view
   _tray = new PanelTray ();
   _layout->AddView(_tray, 0, nux::eCenter, nux::eFull);
   AddChild(_tray);

   _remote = indicator::DBusIndicators::Ptr(new indicator::DBusIndicators());
  _on_object_added_connection = _remote->on_object_added.connect(sigc::mem_fun(this, &PanelView::OnObjectAdded));
  _on_menu_pointer_moved_connection = _remote->on_menu_pointer_moved.connect(sigc::mem_fun(this, &PanelView::OnMenuPointerMoved));
  _on_entry_activate_request_connection = _remote->on_entry_activate_request.connect(sigc::mem_fun(this, &PanelView::OnEntryActivateRequest));
  _on_entry_activated_connection = _remote->on_entry_activated.connect(sigc::mem_fun(this, &PanelView::OnEntryActivated));
  _on_synced_connection = _remote->on_synced.connect(sigc::mem_fun(this, &PanelView::OnSynced));
}

PanelView::~PanelView ()
{
  _on_panel_style_changed_connection.disconnect ();
  _on_object_added_connection.disconnect ();
  _on_menu_pointer_moved_connection.disconnect ();
  _on_entry_activate_request_connection.disconnect ();
  _on_entry_activated_connection.disconnect ();
  _on_synced_connection.disconnect ();

  _style->UnReference ();

  delete _bg_layer;
}

void PanelView::AddPanelView(PanelIndicatorObjectView* child,
                             unsigned int stretchFactor)
{
  _layout->AddView(child, stretchFactor, nux::eCenter, nux::eFull);
  AddChild(child);
  children_.push_back(child);
}

const gchar* PanelView::GetName ()
{
	return "Panel";
}

const gchar *
PanelView::GetChildsName ()
{
  return "indicators";
}

void PanelView::AddProperties (GVariantBuilder *builder)
{
  variant::BuilderWrapper(builder)
    .add("backend", "remote")
    .add("service-name", _remote->name())
    .add("service-unique-name", _remote->owner_name())
    .add("using-local-service", _remote->using_local_service())
    .add(GetGeometry());
}

long
PanelView::ProcessEvent (nux::IEvent &ievent, long TraverseInfo, long ProcessEventInfo)
{
  long ret = TraverseInfo;
  ret = _layout->ProcessEvent (ievent, ret, ProcessEventInfo);
  return ret;
}

void
PanelView::Draw (nux::GraphicsEngine& GfxContext, bool force_draw)
{
  UpdateBackground ();

  GfxContext.PushClippingRectangle (GetGeometry() );

  gPainter.PushDrawLayer (GfxContext, GetGeometry (), _bg_layer);

  gPainter.PopBackground ();

  GfxContext.PopClippingRectangle ();

  if (_needs_geo_sync && _menu_view->GetControlsActive ())
  {
    SyncGeometries ();
    _needs_geo_sync = false;
  }
}

void
PanelView::DrawContent (nux::GraphicsEngine &GfxContext, bool force_draw)
{
  GfxContext.PushClippingRectangle (GetGeometry() );

  gPainter.PushLayer (GfxContext, GetGeometry (), _bg_layer);
  
  _layout->ProcessDraw (GfxContext, force_draw);

  gPainter.PopBackground ();
  GfxContext.PopClippingRectangle();
}

void
PanelView::PreLayoutManagement ()
{
  nux::View::PreLayoutManagement ();
}

long
PanelView::PostLayoutManagement (long LayoutResult)
{
  return nux::View::PostLayoutManagement (LayoutResult);
}

void
PanelView::UpdateBackground ()
{
  nux::Geometry geo = GetGeometry ();

  if (geo.width == _last_width && geo.height == _last_height && !_is_dirty)
    return;

  _last_width = geo.width;
  _last_height = geo.height;
  _is_dirty = false;

  GdkPixbuf *pixbuf = _style->GetBackground (geo.width, geo.height);
  nux::BaseTexture * texture2D = nux::CreateTexture2DFromPixbuf (pixbuf, true);
  g_object_unref (pixbuf);
  nux::TexCoordXForm texxform;
  texxform.SetTexCoordType (nux::TexCoordXForm::OFFSET_COORD);
  texxform.SetWrap (nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
  if (_bg_layer)
    delete _bg_layer;

  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
  nux::Color col = nux::color::White;
  col.alpha = _opacity;

  _bg_layer = new nux::TextureLayer (texture2D->GetDeviceTexture(),
                                     texxform,
                                     col,
                                     true,
                                     rop);
  texture2D->UnReference ();

  NeedRedraw ();
}

void PanelView::ForceUpdateBackground()
{
  _is_dirty = true;
  UpdateBackground ();

  for (Children::iterator i = children_.begin(), end = children_.end(); i != end; ++i)
  {
    (*i)->QueueDraw();
  }
  // The home button isn't an indicator view.
  _home_button->QueueDraw();
  _tray->QueueDraw();
  QueueDraw ();
}

//
// Signals
//
void PanelView::OnObjectAdded(indicator::Indicator::Ptr const& proxy)
{
  // Appmenu is treated differently as it needs to expand
  // We could do this in a more special way, but who has the time for special?
  if (proxy->name().find("appmenu") != std::string::npos)
  {
    _menu_view->SetProxy(proxy);
  }
  else
  {
    PanelIndicatorObjectView* view = new PanelIndicatorObjectView(proxy);
    AddPanelView(view, 0);
  }

  _layout->SetContentDistribution (nux::eStackLeft);

  ComputeChildLayout ();
  NeedRedraw ();
}

void PanelView::OnMenuPointerMoved(int x, int y)
{
  nux::Geometry geo = GetAbsoluteGeometry ();
  nux::Geometry hgeo = _home_button->GetAbsoluteGeometry ();

  if (x <= (hgeo.x + hgeo.width))
    return;

  if (geo.IsPointInside(x, y))
  {
    for (Children::iterator i = children_.begin(), end = children_.end(); i != end; ++i)
    {
      PanelIndicatorObjectView* view = *i;

      if (view == _menu_view && _menu_view->HasOurWindowFocused())
        continue;

      geo = view->GetAbsoluteGeometry();
      if (geo.IsPointInside(x, y))
      {
        view->OnPointerMoved(x, y);
        break;
      }
    }
  }
}

void PanelView::OnEntryActivateRequest(std::string const& entry_id)
{
  if (!_menu_view->GetControlsActive ())
    return;

  bool activated = false;
  for (Children::iterator i = children_.begin(), end = children_.end();
       i != end && !activated; ++i)
  {
    PanelIndicatorObjectView* view = *i;
    activated = view->ActivateEntry(entry_id);
  }
}

void PanelView::OnEntryActivated(std::string const& entry_id)
{
  if (entry_id == "")
    _menu_view->AllMenusClosed ();
}

void PanelView::OnSynced()
{
  _needs_geo_sync = true;
}

//
// Useful Public Methods
//
PanelHomeButton* PanelView::GetHomeButton()
{
  return _home_button;
}

void PanelView::StartFirstMenuShow()
{
}

void PanelView::EndFirstMenuShow()
{
  if (!_menu_view->GetControlsActive())
    return;

  bool activated = false;
  for (Children::iterator i = children_.begin(), end = children_.end();
       i != end && !activated; ++i)
  {
    PanelIndicatorObjectView* view = *i;
    activated = view->ActivateIfSensitive();
  }
}

void
PanelView::SetOpacity (float opacity)
{
  if (_opacity == opacity)
    return;

  _opacity = opacity;

  _home_button->SetOpacity (opacity);
  ForceUpdateBackground ();
}

bool
PanelView::GetPrimary ()
{
  return _is_primary;
}

void
PanelView::SetPrimary (bool primary)
{
  _is_primary = primary;

  _home_button->SetVisible (primary);
}

void PanelView::SyncGeometries()
{
  indicator::EntryLocationMap locations;
  for (Children::iterator i = children_.begin(), end = children_.end(); i != end; ++i)
  {
    (*i)->GetGeometryForSync(locations);
  }
  _remote->SyncGeometries(GetName(), locations);
}

void
PanelView::SetMonitor (int monitor)
{
  _monitor = monitor;
  _menu_view->SetMonitor (monitor);
}

} // namespace unity
