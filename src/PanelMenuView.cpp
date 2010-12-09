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

#include "Nux/Nux.h"
#include "Nux/HLayout.h"
#include "Nux/VLayout.h"
#include "Nux/Button.h"
#include "Nux/PushButton.h"

#include "NuxGraphics/GLThread.h"
#include "Nux/BaseWindow.h"
#include "Nux/WindowCompositor.h"

#include "PanelMenuView.h"

#include "IndicatorObjectEntryProxy.h"

#include <glib.h>

PanelMenuView::PanelMenuView ()
{
  _title_layout = new nux::HLayout ("", NUX_TRACKER_LOCATION);
  _title_layout->Reference ();
  nux::PushButton *but = new nux::PushButton ("Hello");
  _title_layout->AddView (but, 1, nux::eCenter, nux::eFull);
   
  _menu_layout = new nux::HLayout ("", NUX_TRACKER_LOCATION);
  _menu_layout->Reference ();

  _layout = _menu_layout;
  //SetCompositionLayout (_title_layout);
  
  but->OnMouseDown.connect (sigc::mem_fun  (this, &PanelMenuView::RecvButtonMouseDown));
  but->OnMouseUp.connect   (sigc::mem_fun  (this, &PanelMenuView::RecvButtonMouseUp));
//   but->OnMouseClick.connect(sigc::mem_fun  (this, &PanelMenuView::RecvMouseClick));
  but->OnMouseEnter.connect(sigc::mem_fun  (this, &PanelMenuView::RecvButtonMouseEnter));
  but->OnMouseLeave.connect(sigc::mem_fun  (this, &PanelMenuView::RecvButtonMouseLeave));
  but->OnMouseMove.connect (sigc::mem_fun  (this, &PanelMenuView::RecvButtonMouseMove));
//   but->OnMouseDrag.connect (sigc::mem_fun  (this, &PanelMenuView::RecvMouseDrag));  
}

PanelMenuView::~PanelMenuView ()
{
  // Unrefrence the layouts.
  _title_layout->UnReference ();
  _menu_layout->UnReference ();
}

void
PanelMenuView::SetProxy (IndicatorObjectProxy *proxy)
{
  _proxy = proxy;
  printf ("IndicatorAdded: %s\n", _proxy->GetName ().c_str ());

  _proxy->OnEntryAdded.connect (sigc::mem_fun (this, &PanelMenuView::OnEntryAdded));
  _proxy->OnEntryMoved.connect (sigc::mem_fun (this, &PanelMenuView::OnEntryMoved));
  _proxy->OnEntryRemoved.connect (sigc::mem_fun (this, &PanelMenuView::OnEntryRemoved));

  OnMouseEnter.connect (sigc::mem_fun (this, &PanelMenuView::OnMouseEnterRecv));
  OnMouseLeave.connect (sigc::mem_fun (this, &PanelMenuView::OnMouseLeaveRecv));
}

long
PanelMenuView::ProcessEvent (nux::IEvent &ievent, long TraverseInfo, long ProcessEventInfo)
{
  long ret = TraverseInfo;
  nux::Geometry geo = GetGeometry ();
  
  if (!geo.IsPointInside (ievent.e_x, ievent.e_y))
  {
    if (_layout != _menu_layout)
    {
      printf ("Mouse Out\n");
      // Let the _title_layout process the event before switching. The button inside
      // the _title_layout will recognize a "Mouse Leave" event
      ret = _layout->ProcessEvent (ievent, ret, ProcessEventInfo);
      
      _layout = _menu_layout;
      _layout->NeedRedraw ();
      NeedRedraw ();
      return ret;
    }
  }
  else
  {
    if (_layout != _title_layout)
    {
      printf ("Mouse In\n");
      _layout = _title_layout;
      
      _layout->NeedRedraw ();
      NeedRedraw ();
    }
  }
  
  ret = _layout->ProcessEvent (ievent, ret, ProcessEventInfo);

  //OnEvent (ievent, ret, ProcessEventInfo);
  return ret;
}

void PanelMenuView::PreLayoutManagement ()
{
  View::PreLayoutManagement ();
}

long PanelMenuView::PostLayoutManagement (long LayoutResult)
{
  long res = View::PostLayoutManagement (LayoutResult);
  // The size of this widget has been computed. Get its geometry;
  nux::Geometry geo = GetGeometry ();
  // geo.x and geo.y represent the position of the top left corner of this widget on the screen
  // geo.width and geo.height represent the position of the width and size of this widget on the screen

  
  // Here, we explicitely set the size of the layouts to the size and position of the panel view.
  _menu_layout->SetGeometry (geo.x, geo.y, geo.width, geo.height);
  _menu_layout->ComputeLayout2();
  _title_layout->SetGeometry (geo.x, geo.y, geo.width, geo.height);
  _title_layout->ComputeLayout2();
  
  return res;
}

void
PanelMenuView::Draw (nux::GraphicsEngine& GfxContext, bool force_draw)
{

}

void
PanelMenuView::DrawContent (nux::GraphicsEngine &GfxContext, bool force_draw)
{
  GfxContext.PushClippingRectangle (GetGeometry() );
  _layout->ProcessDraw (GfxContext, force_draw);
  GfxContext.PopClippingRectangle();
}

void
PanelMenuView::DrawLayout ()
{
  //_layout->Draw ();
}

void
PanelMenuView::OnMouseEnterRecv (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  //g_debug ("%s", G_STRFUNC);
  //SetCompositionLayout (_menu_layout);
  //_layout = _menu_layout;
  //this->ComputeChildLayout ();
  NeedRedraw ();
}

void
PanelMenuView::OnMouseLeaveRecv (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  //g_debug ("%s", G_STRFUNC);
  //SetCompositionLayout (_title_layout);
  //_layout = _title_layout;
  //this->ComputeChildLayout ();
  //NeedRedraw ();
}

void
PanelMenuView::OnEntryAdded (IndicatorObjectEntryProxy *proxy)
{
  PanelIndicatorObjectEntryView *view = new PanelIndicatorObjectEntryView (proxy);
  _menu_layout->AddView (view, 0, nux::eCenter, nux::eFull);
  _menu_layout->SetContentDistribution (nux::eStackLeft);

  _entries.push_back (view);

  AddChild (view);

  this->ComputeChildLayout ();
  NeedRedraw ();  
}

void
PanelMenuView::OnEntryMoved (IndicatorObjectEntryProxy *proxy)
{
  printf ("ERROR: Moving IndicatorObjectEntry not supported\n");
}

void
PanelMenuView::OnEntryRemoved(IndicatorObjectEntryProxy *proxy)
{
  std::vector<PanelIndicatorObjectEntryView *>::iterator it;
  
  for (it = _entries.begin(); it != _entries.end(); it++)
  {
    PanelIndicatorObjectEntryView *view = static_cast<PanelIndicatorObjectEntryView *> (*it);
    if (view->_proxy == proxy)
      {
        RemoveChild (view);
        _entries.erase (it);
        _menu_layout->RemoveChildObject (view);

        break;
      }
  }

  this->ComputeChildLayout (); 
  NeedRedraw ();
}

const gchar *
PanelMenuView::GetName ()
{
  return "MenuView";
}

const gchar *
PanelMenuView::GetChildsName ()
{
  return "entries";
}

void
PanelMenuView::AddProperties (GVariantBuilder *builder)
{
  nux::Geometry geo = GetGeometry ();

  /* Now some props from ourselves */
  g_variant_builder_add (builder, "{sv}", "x", g_variant_new_int32 (geo.x));
  g_variant_builder_add (builder, "{sv}", "y", g_variant_new_int32 (geo.y));
  g_variant_builder_add (builder, "{sv}", "width", g_variant_new_int32 (geo.width));
  g_variant_builder_add (builder, "{sv}", "height", g_variant_new_int32 (geo.height));
}

void PanelMenuView::RecvButtonMouseDown (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
    // The button is not connected to the PanelMenuView in anyway. So we explicitely call a redraw on the 
    // PanelMenuView so it render itself and the button that is in the current layout.
    printf ("Button Down\n");
    NeedRedraw ();
}

void PanelMenuView::RecvButtonMouseUp (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
    // The button is not connected to the PanelMenuView in anyway. So we explicitely call a redraw on the 
    // PanelMenuView so it render itself and the button that is in the current layout.
    printf ("Button Up\n");
    NeedRedraw ();
}

void PanelMenuView::RecvButtonMouseEnter (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  printf ("Button Enter\n");
}

void PanelMenuView::RecvButtonMouseLeave (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  printf ("Button Leave\n");
}

void PanelMenuView::RecvButtonMouseMove (int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags)
{
  //printf ("Button Move\n");
}

