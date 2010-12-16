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
#include <glib.h>
#include <pango/pangocairo.h>
#include <gtk/gtk.h>

#include "Nux/Nux.h"
#include "Nux/HLayout.h"
#include "Nux/VLayout.h"
#include <Nux/TextureArea.h>

#include "NuxGraphics/GLThread.h"
#include "Nux/BaseWindow.h"
#include "Nux/WindowCompositor.h"

#include "PanelMenuView.h"

#include "WindowManager.h"

#include "IndicatorObjectEntryProxy.h"

#include <gio/gdesktopappinfo.h>

#define PADDING 12
#define BUTTONS_WIDTH 72

static void on_active_window_changed (BamfMatcher   *matcher,
                                      BamfView      *old_view,
                                      BamfView      *new_view,
                                      PanelMenuView *self);


PanelMenuView::PanelMenuView ()
: _matcher (NULL),
  _title_layer (NULL),
  _util_cg (CAIRO_FORMAT_ARGB32, 1, 1),
  _is_inside (false),
  _is_maximized (false),
  _last_active_view (NULL)
{  
  _matcher = bamf_matcher_get_default ();
  g_signal_connect (_matcher, "active-window-changed",
                    G_CALLBACK (on_active_window_changed), this);

  _menu_layout = new nux::HLayout ("", NUX_TRACKER_LOCATION);

  /* This is for our parent and for PanelView to read indicator entries, we
   * shouldn't touch this again
   */
  _layout = _menu_layout;

  _window_buttons = new WindowButtons ();
  _window_buttons->NeedRedraw ();
  _window_buttons->close_clicked.connect (sigc::mem_fun (this, &PanelMenuView::OnCloseClicked));
  _window_buttons->minimize_clicked.connect (sigc::mem_fun (this, &PanelMenuView::OnMinimizeClicked));
  _window_buttons->restore_clicked.connect (sigc::mem_fun (this, &PanelMenuView::OnRestoreClicked));

  WindowManager::Default ()->window_maximized.connect (sigc::mem_fun (this,
                                                                      &PanelMenuView::OnWindowMaximized));
  WindowManager::Default ()->window_restored.connect (sigc::mem_fun (this,
                                                                     &PanelMenuView::OnWindowRestored));

  Refresh ();
}

PanelMenuView::~PanelMenuView ()
{
  if (_title_layer)
    delete _title_layer;
  _menu_layout->UnReference ();
  _window_buttons->UnReference ();
}

void
PanelMenuView::FullRedraw ()
{
  _menu_layout->NeedRedraw ();
  _window_buttons->NeedRedraw ();
  NeedRedraw ();
}

void
PanelMenuView::SetProxy (IndicatorObjectProxy *proxy)
{
  _proxy = proxy;
  printf ("IndicatorAdded: %s\n", _proxy->GetName ().c_str ());

  _proxy->OnEntryAdded.connect (sigc::mem_fun (this, &PanelMenuView::OnEntryAdded));
  _proxy->OnEntryMoved.connect (sigc::mem_fun (this, &PanelMenuView::OnEntryMoved));
  _proxy->OnEntryRemoved.connect (sigc::mem_fun (this, &PanelMenuView::OnEntryRemoved));
}

long
PanelMenuView::ProcessEvent (nux::IEvent &ievent, long TraverseInfo, long ProcessEventInfo)
{
  long ret = TraverseInfo;
  nux::Geometry geo = GetGeometry ();

  if (geo.IsPointInside (ievent.e_x, ievent.e_y))
  {
    if (_is_inside != true)
    {
      _is_inside = true;
      FullRedraw ();
    }
  }
  else
  {
    if (_is_inside != false)
    {
      _is_inside = false;
      FullRedraw ();
    }
  }

  ret = _window_buttons->ProcessEvent (ievent, ret, ProcessEventInfo);
  ret = _menu_layout->ProcessEvent (ievent, ret, ProcessEventInfo);

  return ret;
}

long PanelMenuView::PostLayoutManagement (long LayoutResult)
{
  long res = View::PostLayoutManagement (LayoutResult);
  int w = _window_buttons->GetContentWidth ();
  
  nux::Geometry geo = GetGeometry ();

  _window_buttons->SetGeometry (geo.x + PADDING, geo.y, w, geo.height);
  _window_buttons->ComputeLayout2 ();
  
  /* Explicitly set the size and position of the widgets */
  geo.x += PADDING + w + PADDING;
  geo.width -= PADDING + w + PADDING;

  _menu_layout->SetGeometry (geo.x, geo.y, geo.width, geo.height);
  _menu_layout->ComputeLayout2();

  
  Refresh ();
  
  return res;
}

void
PanelMenuView::Draw (nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry geo = GetGeometry ();
  
  GfxContext.PushClippingRectangle (geo);

  /* "Clear" out the background */
  nux::ROPConfig rop; 
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
 
  nux::ColorLayer layer (nux::Color (0x00000000), true, rop);
  gPainter.PushDrawLayer (GfxContext, GetGeometry (), &layer);

  if (_is_inside || _last_active_view)
    geo.width = PADDING + _window_buttons->GetContentWidth ();

  if (_is_maximized)
  {
    if (!_is_inside)
      gPainter.PushDrawLayer (GfxContext, GetGeometry (), _title_layer);
  }
  else
    gPainter.PushDrawLayer (GfxContext, GetGeometry (), _title_layer);

  gPainter.PopBackground ();
 
  GfxContext.PopClippingRectangle();
}

void
PanelMenuView::DrawContent (nux::GraphicsEngine &GfxContext, bool force_draw)
{
  nux::Geometry geo = GetGeometry ();

  GfxContext.PushClippingRectangle (geo);

  if (_is_inside || _last_active_view)
  {
    _layout->ProcessDraw (GfxContext, force_draw);
  }

  if (_is_maximized)
    _window_buttons->ProcessDraw (GfxContext, force_draw);

  GfxContext.PopClippingRectangle();
}

gchar *
PanelMenuView::GetActiveViewName ()
{
  gchar *label = NULL;

  if (_is_maximized)
  {
    BamfWindow *window = bamf_matcher_get_active_window (_matcher);

    if (BAMF_IS_WINDOW (window))
      label = g_strdup (bamf_view_get_name (BAMF_VIEW (window)));
  }

  if (!label)
  {
    BamfApplication *app = bamf_matcher_get_active_application (_matcher);
    if (BAMF_IS_APPLICATION (app))
    {
      const gchar     *filename;

      filename = bamf_application_get_desktop_file (app);

      if (filename && g_strcmp0 (filename, "") != 0)
      {
        GDesktopAppInfo *info;
    
        info = g_desktop_app_info_new_from_filename (bamf_application_get_desktop_file (app));
    
        if (info)
        {
          label = g_strdup (g_app_info_get_display_name (G_APP_INFO (info)));
          g_object_unref (info);
        }
        else
        {
          g_warning ("Unable to get GDesktopAppInfo for %s",
                     bamf_application_get_desktop_file (app));
        }
      }

      if (label == NULL)
      {
        BamfView *active_view;

        active_view = (BamfView *)bamf_matcher_get_active_window (_matcher);
        if (BAMF_IS_VIEW (active_view))
          label = g_strdup (bamf_view_get_name (active_view));
        else
          label = g_strdup ("");
      }
    }
    else
    {
      label = g_strdup (" ");
    }
  }

  return label;
}

void
PanelMenuView::Refresh ()
{
  nux::Geometry         geo = GetGeometry ();
  char                 *label = GetActiveViewName ();
  PangoLayout          *layout = NULL;
  PangoFontDescription *desc = NULL;
  GtkSettings          *settings = gtk_settings_get_default ();
  cairo_t              *cr;
  char                 *font_description = NULL;
  GdkScreen            *screen = gdk_screen_get_default ();
  int                   dpi = 0;

  int  x = 0;
  int  y = 0;
  int  width = geo.width;
  int  height = geo.height;
  int  text_width = 0;
  int  text_height = 0;

  if (label)
  {
    PangoContext *cxt;
    PangoRectangle log_rect;

    cr = _util_cg.GetContext ();

    g_object_get (settings,
                  "gtk-font-name", &font_description,
                  "gtk-xft-dpi", &dpi,
                  NULL);
    desc = pango_font_description_from_string (font_description);
    pango_font_description_set_weight (desc, PANGO_WEIGHT_NORMAL);

    layout = pango_cairo_create_layout (cr);
    pango_layout_set_font_description (layout, desc);
    pango_layout_set_text (layout, label, -1);
    
    cxt = pango_layout_get_context (layout);
    pango_cairo_context_set_font_options (cxt, gdk_screen_get_font_options (screen));
    pango_cairo_context_set_resolution (cxt, (float)dpi/(float)PANGO_SCALE);
    pango_layout_context_changed (layout);

    pango_layout_get_extents (layout, NULL, &log_rect);
    text_width = log_rect.width / PANGO_SCALE;
    text_height = log_rect.height / PANGO_SCALE;

    pango_font_description_free (desc);
    g_free (font_description);
    cairo_destroy (cr);
  }

  nux::CairoGraphics cairo_graphics(CAIRO_FORMAT_ARGB32, width, height);
  cr = cairo_graphics.GetContext();
  cairo_set_line_width (cr, 1);

  x = PADDING;
  y = 0;

  if (_is_maximized)
    x += _window_buttons->GetContentWidth () + PADDING;

  if (label)
  {
    pango_cairo_update_layout (cr, layout);

    // Once for the homies that couldn't be here
    cairo_set_source_rgb (cr, 50/255.0f, 50/255.0f, 45/255.0f);
    cairo_move_to (cr, x, ((height - text_height)/2)-1);
    pango_cairo_show_layout (cr, layout);
    cairo_stroke (cr);

    // Once again for the homies that could
    cairo_set_source_rgba (cr, 223/255.0f, 219/255.0f, 210/255.0f, 1.0f);
    cairo_move_to (cr, x, (height - text_height)/2);
    pango_cairo_show_layout (cr, layout);
    cairo_stroke (cr);
  }

  cairo_destroy (cr);
  if (layout)
    g_object_unref (layout);

  nux::NBitmapData* bitmap =  cairo_graphics.GetBitmap();

  // The Texture is created with a reference count of 1. 
  nux::BaseTexture* texture2D = nux::GetThreadGLDeviceFactory ()->CreateSystemCapableTexture ();
  texture2D->Update(bitmap);
  delete bitmap;

  if (_title_layer)
    delete _title_layer;
  
  nux::TexCoordXForm texxform;
  texxform.SetTexCoordType (nux::TexCoordXForm::OFFSET_COORD);
  texxform.SetWrap (nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
  
  nux::ROPConfig rop; 
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
  _title_layer = new nux::TextureLayer (texture2D->GetDeviceTexture(),
                                        texxform,
                                        nux::Color::White,
                                        false, 
                                        rop);

    
  texture2D->UnReference ();
  g_free (label);
}

void
PanelMenuView::OnActiveChanged (PanelIndicatorObjectEntryView *view,
                                bool                           is_active)
{
  if (is_active)
    _last_active_view = view;
  else
  {
    if (_last_active_view == view)
    {
      _last_active_view = NULL;
    }
  }
  FullRedraw ();
}

void
PanelMenuView::OnEntryAdded (IndicatorObjectEntryProxy *proxy)
{
  PanelIndicatorObjectEntryView *view = new PanelIndicatorObjectEntryView (proxy);
  view->active_changed.connect (sigc::mem_fun (this, &PanelMenuView::OnActiveChanged));
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

void
PanelMenuView::AllMenusClosed ()
{
  _is_inside = false;
  _last_active_view = false;

  FullRedraw ();
}

void
PanelMenuView::OnActiveWindowChanged (BamfView *old_view,
                                      BamfView *new_view)
{
  _is_maximized = false;

  if (BAMF_IS_WINDOW (new_view))
  {
    BamfWindow *window = BAMF_WINDOW (new_view);
    _is_maximized = WindowManager::Default ()->IsWindowMaximized (bamf_window_get_xid (window));
  }

  Refresh ();
  FullRedraw ();
}

void
PanelMenuView::OnWindowMaximized (guint xid)
{
  BamfWindow *window;

  window = bamf_matcher_get_active_window (_matcher);
  if (BAMF_IS_WINDOW (window) && bamf_window_get_xid (window) == xid)
  {
    _is_maximized = true;

    Refresh ();
    FullRedraw ();
  }
}

void
PanelMenuView::OnWindowRestored (guint xid)
{
  BamfWindow *window;
  
  window = bamf_matcher_get_active_window (_matcher);
  if (BAMF_IS_WINDOW (window) && bamf_window_get_xid (window) == xid)
  {
    _is_maximized = false;

    Refresh ();
    FullRedraw ();
  }
}

void
PanelMenuView::OnCloseClicked ()
{
  BamfWindow *window;

  window = bamf_matcher_get_active_window (_matcher);
  if (BAMF_IS_WINDOW (window))
    WindowManager::Default ()->Close (bamf_window_get_xid (window));
}

void
PanelMenuView::OnMinimizeClicked ()
{
  BamfWindow *window;

  window = bamf_matcher_get_active_window (_matcher);
  if (BAMF_IS_WINDOW (window))
    WindowManager::Default ()->Minimize (bamf_window_get_xid (window));
}

void
PanelMenuView::OnRestoreClicked ()
{
  BamfWindow *window;

  window = bamf_matcher_get_active_window (_matcher);
  if (BAMF_IS_WINDOW (window))
    WindowManager::Default ()->Restore (bamf_window_get_xid (window));
}


// Introspectable
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

/*
 * C code for callbacks
 */
static void
on_active_window_changed (BamfMatcher   *matcher,
                          BamfView      *old_view,
                          BamfView      *new_view,
                          PanelMenuView *self)
{
  self->OnActiveWindowChanged (old_view, new_view);
}
