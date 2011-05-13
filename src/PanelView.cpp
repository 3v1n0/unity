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

#include "PanelView.h"
#include "PanelStyle.h"

#include "IndicatorObjectFactoryRemote.h"
#include "IndicatorObjectEntryProxy.h"
#include "PanelIndicatorObjectView.h"

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
   SetCompositionLayout (_layout);

   // Home button
   _home_button = new PanelHomeButton ();
   _layout->AddView (_home_button, 0, nux::eCenter, nux::eFull);
   AddChild (_home_button);

   _menu_view = new PanelMenuView ();
   _layout->AddView (_menu_view, 1, nux::eCenter, nux::eFull);
   AddChild (_menu_view);

   _tray = new PanelTray ();
   _layout->AddView (_tray, 0, nux::eCenter, nux::eFull);
   AddChild (_tray);

  _remote = new IndicatorObjectFactoryRemote ();
  _on_object_added_connection = _remote->OnObjectAdded.connect (sigc::mem_fun (this, &PanelView::OnObjectAdded));
  _on_menu_pointer_moved_connection = _remote->OnMenuPointerMoved.connect (sigc::mem_fun (this, &PanelView::OnMenuPointerMoved));
  _on_entry_activate_request_connection = _remote->OnEntryActivateRequest.connect (sigc::mem_fun (this, &PanelView::OnEntryActivateRequest));
  _on_entry_activated_connection = _remote->IndicatorObjectFactory::OnEntryActivated.connect (sigc::mem_fun (this, &PanelView::OnEntryActivated));
  _on_synced_connection = _remote->IndicatorObjectFactory::OnSynced.connect (sigc::mem_fun (this, &PanelView::OnSynced));
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

  delete _remote;
  delete _bg_layer;
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
  nux::Geometry geo = GetGeometry ();

  /* First add some properties from the backend */
  _remote->AddProperties (builder);

  /* Now some props from ourselves */
  g_variant_builder_add (builder, "{sv}", "x", g_variant_new_int32 (geo.x));
  g_variant_builder_add (builder, "{sv}", "y", g_variant_new_int32 (geo.y));
  g_variant_builder_add (builder, "{sv}", "width", g_variant_new_int32 (geo.width));
  g_variant_builder_add (builder, "{sv}", "height", g_variant_new_int32 (geo.height));
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

void
PanelView::ForceUpdateBackground ()
{
  std::list<Area *>::iterator it;

  _is_dirty = true;
  UpdateBackground ();

  std::list<Area *> my_children = _layout->GetChildren ();
  for (it = my_children.begin(); it != my_children.end(); it++)
  {
    PanelIndicatorObjectView *view = static_cast<PanelIndicatorObjectView *> (*it);

    view->QueueDraw ();
    if (view->_layout == NULL)
      continue;

    std::list<Area *>::iterator it2;

    std::list<Area *> its_children = view->_layout->GetChildren ();
    for (it2 = its_children.begin(); it2 != its_children.end(); it2++)
    {
      PanelIndicatorObjectEntryView *entry = static_cast<PanelIndicatorObjectEntryView *> (*it2);
      entry->QueueDraw ();
    }
  }
  _home_button->QueueDraw ();
  QueueDraw ();
}

//
// Signals
//
void
PanelView::OnObjectAdded (IndicatorObjectProxy *proxy)
{
  PanelIndicatorObjectView *view;
  
  // Appmenu is treated differently as it needs to expand
  // We could do this in a more special way, but who has the time for special?
  if (g_strstr_len (proxy->GetName ().c_str (), -1, "appmenu") != NULL)
  {
    view = _menu_view;
    _menu_view->SetProxy (proxy);
  }
  else
  {
    view = new PanelIndicatorObjectView (proxy);

    _layout->AddView (view, 0, nux::eCenter, nux::eFull);
    AddChild (view);
  }

  _layout->SetContentDistribution (nux::eStackLeft);
  
  ComputeChildLayout ();
  NeedRedraw ();
}

void
PanelView::OnMenuPointerMoved (int x, int y)
{
  nux::Geometry geo = GetAbsoluteGeometry ();
  nux::Geometry hgeo = _home_button->GetAbsoluteGeometry ();

  if (x <= (hgeo.x + hgeo.width))
    return; 
  
  if (x >= geo.x && x <= (geo.x + geo.width)
      && y >= geo.y && y <= (geo.y + geo.height))
  {
    std::list<Area *>::iterator it;

    std::list<Area *> my_children = _layout->GetChildren ();
    for (it = my_children.begin(); it != my_children.end(); it++)
    {
      PanelIndicatorObjectView *view = static_cast<PanelIndicatorObjectView *> (*it);
      
      if (view->_layout == NULL
          || (view == _menu_view && _menu_view->HasOurWindowFocused ()))
        continue;

      geo = view->GetAbsoluteGeometry ();
      if (x >= geo.x && x <= (geo.x + geo.width)
          && y >= geo.y && y <= (geo.y + geo.height))
      {
        std::list<Area *>::iterator it2;

        std::list<Area *> its_children = view->_layout->GetChildren ();
        for (it2 = its_children.begin(); it2 != its_children.end(); it2++)
        {
          PanelIndicatorObjectEntryView *entry = static_cast<PanelIndicatorObjectEntryView *> (*it2);

          geo = entry->GetAbsoluteGeometry ();
          if (x >= geo.x && x <= (geo.x + geo.width)
              && y >= geo.y && y <= (geo.y + geo.height))
          {
            entry->OnMouseDown (x, y, 0, 0);
            break;
          }
        }
        break;
      }
    }
  }
}

void
PanelView::OnEntryActivateRequest (const char *entry_id)
{
  std::list<Area *>::iterator it;

  if (!_menu_view->GetControlsActive ())
    return;

  std::list<Area *> my_children = _layout->GetChildren ();
  for (it = my_children.begin(); it != my_children.end(); it++)
  {
    PanelIndicatorObjectView *view = static_cast<PanelIndicatorObjectView *> (*it);

    if (view->_layout == NULL)
      continue;

    std::list<Area *>::iterator it2;

    std::list<Area *> its_children = view->_layout->GetChildren ();
    for (it2 = its_children.begin(); it2 != its_children.end(); it2++)
    {
      PanelIndicatorObjectEntryView *entry = static_cast<PanelIndicatorObjectEntryView *> (*it2);

      if (g_strcmp0 (entry->GetName (), entry_id) == 0)
      {
        g_debug ("%s: Activating: %s", G_STRFUNC, entry_id);
        entry->Activate ();
        break;
      }
    }
  }
}

void
PanelView::OnEntryActivated (const char *entry_id)
{
  if (g_strcmp0 (entry_id, "") == 0)
    _menu_view->AllMenusClosed ();
}

void
PanelView::OnSynced ()
{
  _needs_geo_sync = true;
}

//
// Useful Public Methods
//
PanelHomeButton * 
PanelView::GetHomeButton ()
{
  return _home_button;
}

void
PanelView::StartFirstMenuShow ()
{

}

void
PanelView::EndFirstMenuShow ()
{
  std::list<Area *>::iterator it;

  if (!_menu_view->GetControlsActive ())
    return;

  std::list<Area *> my_children = _layout->GetChildren ();
  for (it = my_children.begin(); it != my_children.end(); it++)
  {
    PanelIndicatorObjectView *view = static_cast<PanelIndicatorObjectView *> (*it);

    if (view->_layout == NULL
        || (view == _menu_view && _menu_view->HasOurWindowFocused ()))       
      continue;

    std::list<Area *>::iterator it2;

    std::list<Area *> its_children = view->_layout->GetChildren ();
    for (it2 = its_children.begin(); it2 != its_children.end(); it2++)
    {
      PanelIndicatorObjectEntryView *entry = static_cast<PanelIndicatorObjectEntryView *> (*it2);
      IndicatorObjectEntryProxy *proxy = entry->_proxy;

      if (proxy != NULL && !proxy->label_sensitive && !proxy->icon_sensitive)
        continue;

      entry->Activate ();
      return;
    }
  }
}

Window
PanelView::GetTrayWindow ()
{
  return _tray->GetTrayWindow ();
}

void
PanelView::SetOpacity (float opacity)
{
  if (_opacity == opacity)
    return;

  _opacity = opacity;
  
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

void
PanelView::SyncGeometries ()
{
  GVariantBuilder b;
  GDBusProxy     *bus_proxy;
  GVariant       *method_args;
  std::list<Area *>::iterator it;

  g_variant_builder_init (&b, G_VARIANT_TYPE ("(a(ssiiii))"));
  g_variant_builder_open (&b, G_VARIANT_TYPE ("a(ssiiii)"));

  std::list<Area *> my_children = _layout->GetChildren ();
  for (it = my_children.begin(); it != my_children.end(); it++)
  {
    PanelIndicatorObjectView *view = static_cast<PanelIndicatorObjectView *> (*it);

    if (view->_layout == NULL)
      continue;

    std::list<Area *>::iterator it2;

    std::list<Area *> its_children = view->_layout->GetChildren ();
    for (it2 = its_children.begin (); it2 != its_children.end (); it2++)
    {
      nux::Geometry geo;
      PanelIndicatorObjectEntryView *entry = static_cast<PanelIndicatorObjectEntryView *> (*it2);

      if (entry == NULL)
        continue;

      geo = entry->GetAbsoluteGeometry ();
      g_variant_builder_add (&b, "(ssiiii)",
			     GetName (),
			     entry->_proxy->GetId (),
			     geo.x,
			     geo.y,
			     geo.GetWidth (),
			     geo.GetHeight ());
    }
  }

  g_variant_builder_close (&b);
  method_args = g_variant_builder_end (&b);

  // Send geometries to the panel service
  bus_proxy =_remote->GetRemoteProxy ();
  if (bus_proxy != NULL)
  {
    g_dbus_proxy_call (bus_proxy, "SyncGeometries", method_args,
                       G_DBUS_CALL_FLAGS_NONE,
                       -1,
                       NULL,
                       NULL,
                       NULL);
  }
}

void
PanelView::SetMonitor (int monitor)
{
  _monitor = monitor;
  _menu_view->SetMonitor (monitor);
}

