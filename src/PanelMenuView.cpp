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

#include "NuxGraphics/GLThread.h"
#include "Nux/BaseWindow.h"
#include "Nux/WindowCompositor.h"

#include "PanelMenuView.h"

#include "IndicatorObjectEntryProxy.h"

#include <glib.h>

PanelMenuView::PanelMenuView ()
{
  _title_layout = new nux::HLayout ("", NUX_TRACKER_LOCATION);
  nux::Button *but = new nux::Button ("Hello");
  _title_layout->AddView (but, 1, nux::eCenter, nux::eFull);
   
  _menu_layout = new nux::HLayout ("", NUX_TRACKER_LOCATION);

  _layout = _menu_layout;
  SetCompositionLayout (_layout);
}

PanelMenuView::~PanelMenuView ()
{
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
  ret = _layout->ProcessEvent (ievent, ret, ProcessEventInfo);

  return ret;
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
  _layout->Draw ();
}

void
PanelMenuView::OnMouseEnterRecv (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  g_debug ("%s", G_STRFUNC);
}

void
PanelMenuView::OnMouseLeaveRecv (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  g_debug ("%s", G_STRFUNC);
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
