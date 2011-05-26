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
#include "NuxGraphics/XInputWindow.h"
#include "Nux/BaseWindow.h"
#include "Nux/WindowCompositor.h"

#include "PanelMenuView.h"
#include "PanelStyle.h"

#include "WindowManager.h"

#include "IndicatorObjectEntryProxy.h"
#include "Variant.h"

#include <gio/gdesktopappinfo.h>

#include "ubus-server.h"
#include "UBusMessages.h"

#include "UScreen.h"


static void on_active_window_changed (BamfMatcher   *matcher,
                                      BamfView      *old_view,
                                      BamfView      *new_view,
                                      PanelMenuView *self);

static void on_name_changed (BamfView*      bamf_view,
                             gchar*         old_name,
                             gchar*         new_name,
                             PanelMenuView* self);

PanelMenuView::PanelMenuView (int padding)
: _matcher (NULL),
  _title_layer (NULL),
  _util_cg (CAIRO_FORMAT_ARGB32, 1, 1),
  _gradient_texture (NULL),
  _title_tex (NULL),
  _is_inside (false),
  _is_maximized (false),
  _is_own_window (false),
  _last_active_view (NULL),
  _last_width (0),
  _last_height (0),
  _places_showing (false),
  _show_now_activated (false),
  _we_control_active (false),
  _monitor (0),
  _active_xid (0),
  _active_moved_id (0),
  _place_shown_interest (0),
  _place_hidden_interest (0)
{
  WindowManager *win_manager;

  _matcher = bamf_matcher_get_default ();
  _activate_window_changed_id = g_signal_connect (_matcher, "active-window-changed",
                                                  G_CALLBACK (on_active_window_changed), this);

  _menu_layout = new nux::HLayout ("", NUX_TRACKER_LOCATION);
  _menu_layout->SetParentObject (this);

  /* This is for our parent and for PanelView to read indicator entries, we
   * shouldn't touch this again
   */
  _layout = _menu_layout;

  _padding = padding;
  _name_changed_callback_instance = NULL;
  _name_changed_callback_id = 0;

  _window_buttons = new WindowButtons ();
  _window_buttons->NeedRedraw ();
  _on_winbutton_close_clicked_connection = _window_buttons->close_clicked.connect (sigc::mem_fun (this, &PanelMenuView::OnCloseClicked));
  _on_winbutton_minimize_clicked_connection = _window_buttons->minimize_clicked.connect (sigc::mem_fun (this, &PanelMenuView::OnMinimizeClicked));
  _on_winbutton_restore_clicked_connection = _window_buttons->restore_clicked.connect (sigc::mem_fun (this, &PanelMenuView::OnRestoreClicked));
  _on_winbutton_redraw_signal_connection = _window_buttons->redraw_signal.connect (sigc::mem_fun (this, &PanelMenuView::OnWindowButtonsRedraw));

  _panel_titlebar_grab_area = new PanelTitlebarGrabArea ();
  _panel_titlebar_grab_area->SinkReference ();
  _on_titlebargrab_mouse_down_connnection = _panel_titlebar_grab_area->mouse_down.connect (sigc::mem_fun (this, &PanelMenuView::OnMaximizedGrab));
  _on_titlebargrab_mouse_doubleleftclick_connnection = _panel_titlebar_grab_area->mouse_doubleleftclick.connect (sigc::mem_fun (this, &PanelMenuView::OnMouseDoubleClicked));
  _on_titlebargrab_mouse_middleclick_connnection = _panel_titlebar_grab_area->mouse_middleclick.connect (sigc::mem_fun (this, &PanelMenuView::OnMouseMiddleClicked));

  win_manager = WindowManager::Default ();

  _on_window_minimized_connection = win_manager->window_minimized.connect (sigc::mem_fun (this, &PanelMenuView::OnWindowMinimized));
  _on_window_unminimized_connection = win_manager->window_unminimized.connect (sigc::mem_fun (this, &PanelMenuView::OnWindowUnminimized));
  _on_window_initspread_connection = win_manager->initiate_spread.connect (sigc::mem_fun (this, &PanelMenuView::OnSpreadInitiate));
  _on_window_terminatespread_connection = win_manager->terminate_spread.connect (sigc::mem_fun (this, &PanelMenuView::OnSpreadTerminate));

  _on_window_maximized_connection = win_manager->window_maximized.connect (sigc::mem_fun (this, &PanelMenuView::OnWindowMaximized));
  _on_window_restored_connection = win_manager->window_restored.connect (sigc::mem_fun (this, &PanelMenuView::OnWindowRestored));
  _on_window_unmapped_connection = win_manager->window_unmapped.connect (sigc::mem_fun (this, &PanelMenuView::OnWindowUnmapped));
  _on_window_moved_connection = win_manager->window_moved.connect (sigc::mem_fun (this, &PanelMenuView::OnWindowMoved));

  _on_panelstyle_changed_connection = PanelStyle::GetDefault ()->changed.connect (sigc::mem_fun (this, &PanelMenuView::Refresh));

  // Register for all the interesting events
  UBusServer *ubus = ubus_server_get_default ();
  _place_shown_interest = ubus_server_register_interest (ubus, UBUS_PLACE_VIEW_SHOWN,
                                 (UBusCallback)PanelMenuView::OnPlaceViewShown,
                                 this);
  _place_hidden_interest = ubus_server_register_interest (ubus, UBUS_PLACE_VIEW_HIDDEN,
                                 (UBusCallback)PanelMenuView::OnPlaceViewHidden,
                                 this);

  Refresh ();
}

PanelMenuView::~PanelMenuView ()
{
  
  _on_winbutton_close_clicked_connection.disconnect ();
  _on_winbutton_minimize_clicked_connection.disconnect ();
  _on_winbutton_restore_clicked_connection.disconnect ();
  _on_winbutton_redraw_signal_connection.disconnect ();
  _on_titlebargrab_mouse_down_connnection.disconnect ();
  _on_titlebargrab_mouse_doubleleftclick_connnection.disconnect ();
  _on_titlebargrab_mouse_middleclick_connnection.disconnect ();
  _on_window_minimized_connection.disconnect ();
  _on_window_unminimized_connection.disconnect ();
  _on_window_initspread_connection.disconnect ();
  _on_window_terminatespread_connection.disconnect ();
  _on_window_maximized_connection.disconnect ();
  _on_window_restored_connection.disconnect ();
  _on_window_unmapped_connection.disconnect ();
  _on_window_moved_connection.disconnect ();  
  _on_panelstyle_changed_connection.disconnect ();
  
  if (_name_changed_callback_id)
      g_signal_handler_disconnect (_name_changed_callback_instance,
                                   _name_changed_callback_id);
  if (_activate_window_changed_id)
      g_signal_handler_disconnect (_matcher,
                                   _activate_window_changed_id);
  if (_active_moved_id)
    g_source_remove (_active_moved_id);                                   
  
  if (_title_layer)
    delete _title_layer;
  if (_title_tex)
    _title_tex->UnReference ();

  _menu_layout->UnReference ();
  _window_buttons->UnReference ();
  _panel_titlebar_grab_area->UnReference ();

  UBusServer* ubus = ubus_server_get_default ();
  if (_place_shown_interest != 0)
    ubus_server_unregister_interest (ubus, _place_shown_interest);

  if (_place_hidden_interest != 0)
    ubus_server_unregister_interest (ubus, _place_hidden_interest);
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

  _proxy->OnEntryAdded.connect (sigc::mem_fun (this, &PanelMenuView::OnEntryAdded));
  _proxy->OnEntryMoved.connect (sigc::mem_fun (this, &PanelMenuView::OnEntryMoved));
  _proxy->OnEntryRemoved.connect (sigc::mem_fun (this, &PanelMenuView::OnEntryRemoved));
}

long
PanelMenuView::ProcessEvent (nux::IEvent &ievent, long TraverseInfo, long ProcessEventInfo)
{
  long ret = TraverseInfo;
  nux::Geometry geo = GetAbsoluteGeometry ();

  if (!_we_control_active)
    return _panel_titlebar_grab_area->OnEvent (ievent, ret, ProcessEventInfo);

  if (geo.IsPointInside (ievent.e_x, ievent.e_y))
  {
    if (_is_inside != true)
    {
      if (_is_grabbed)
        _is_grabbed = false;
      else
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
  
  if (_is_maximized)
  {
    if (_window_buttons)
      ret = _window_buttons->ProcessEvent (ievent, ret, ProcessEventInfo);
    if (_panel_titlebar_grab_area)
      ret = _panel_titlebar_grab_area->OnEvent (ievent, ret, ProcessEventInfo);
  }
  ret = _panel_titlebar_grab_area->OnEvent (ievent, ret, ProcessEventInfo);

  if (!_is_own_window)
    ret = _menu_layout->ProcessEvent (ievent, ret, ProcessEventInfo);

  return ret;
}

long PanelMenuView::PostLayoutManagement (long LayoutResult)
{
  long res = View::PostLayoutManagement (LayoutResult);
  int old_window_buttons_w, new_window_buttons_w;
  int old_menu_area_w, new_menu_area_w;
  
  nux::Geometry geo = GetGeometry ();

  old_window_buttons_w = _window_buttons->GetContentWidth ();
  _window_buttons->SetGeometry (geo.x + _padding, geo.y, old_window_buttons_w, geo.height);
  _window_buttons->ComputeLayout2 ();
  new_window_buttons_w = _window_buttons->GetContentWidth ();

  /* Explicitly set the size and position of the widgets */
  geo.x += _padding + new_window_buttons_w + _padding;
  geo.width -= _padding + new_window_buttons_w + _padding;

  old_menu_area_w = _menu_layout->GetContentWidth ();
  _menu_layout->SetGeometry (geo.x, geo.y, old_menu_area_w, geo.height);
  _menu_layout->ComputeLayout2();
  new_menu_area_w = _menu_layout->GetContentWidth ();

  geo.x += new_menu_area_w;
  geo.width -= new_menu_area_w;

  _panel_titlebar_grab_area->SetGeometry (geo.x, geo.y, geo.width, geo.height);
  
  if (_is_inside)
    NeedRedraw ();
  
  return res;
}

void
PanelMenuView::Draw (nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry geo = GetGeometry ();
  int button_width = _padding + _window_buttons->GetContentWidth () + _padding;
  float factor = 4;
  button_width /= factor;

  if (geo.width != _last_width || geo.height != _last_height)
  {
    _last_width = geo.width;
    _last_height = geo.height;
    Refresh ();
  }
    
  GfxContext.PushClippingRectangle (geo);

  /* "Clear" out the background */
  nux::ROPConfig rop; 
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
 
  nux::ColorLayer layer (nux::Color (0x00000000), true, rop);
  gPainter.PushDrawLayer (GfxContext, GetGeometry (), &layer);

  if (_is_own_window || _places_showing || !_we_control_active)
  {

  }
  else if (_is_maximized)
  {
    if (!_is_inside && !_last_active_view && !_show_now_activated)
      gPainter.PushDrawLayer (GfxContext, GetGeometry (), _title_layer);
  }
  else
  {
    bool have_valid_entries = false;
    std::vector<PanelIndicatorObjectEntryView *>::iterator it, eit = _entries.end ();

    for (it = _entries.begin (); it != eit; ++it)
    {
      IndicatorObjectEntryProxy *proxy = (*it)->_proxy;

      if (proxy->icon_visible || proxy->label_visible)
        have_valid_entries = true;
    }

    if ((_is_inside || _last_active_view || _show_now_activated) && have_valid_entries)
    {
      if (_gradient_texture == NULL)
      {
        nux::NTextureData texture_data (nux::BITFMT_R8G8B8A8, geo.width, 1, 1);
        nux::ImageSurface surface = texture_data.GetSurface (0);
        nux::SURFACE_LOCKED_RECT lockrect;
        BYTE *dest;
        int num_row;
            
       _gradient_texture = nux::GetThreadGLDeviceFactory ()->CreateSystemCapableDeviceTexture (texture_data.GetWidth (), texture_data.GetHeight (), 1, texture_data.GetFormat ());

        _gradient_texture->LockRect (0, &lockrect, 0);

        dest = (BYTE *) lockrect.pBits;
        num_row = surface.GetBlockHeight ();

        for (int y = 0; y < num_row; y++)
        {
          for (int x = 0; x < geo.width; x++)
          {
            BYTE a;
            if (x < button_width * (factor - 1))
            {
              a = 0xff;
            }
            else if (x < button_width * factor)
            {
              a = 255 - 255 * (((float)x-(button_width * (factor -1)))/(float)(button_width));
            }
            else
            {
              a = 0x00;
            }

            *(dest + y * lockrect.Pitch + 4*x + 0) = (223 * a) / 255; //red
            *(dest + y * lockrect.Pitch + 4*x + 1) = (219 * a) / 255; //green
            *(dest + y * lockrect.Pitch + 4*x + 2) = (210 * a) / 255; //blue
            *(dest + y * lockrect.Pitch + 4*x + 3) = a;
          }
        }
        _gradient_texture->UnlockRect (0);
      }
      guint alpha = 0, src = 0, dest = 0;

      GfxContext.GetRenderStates ().GetBlend (alpha, src, dest);
      GfxContext.GetRenderStates ().SetBlend (true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

      nux::TexCoordXForm texxform0;
      nux::TexCoordXForm texxform1;

      // Modulate the checkboard and the gradient texture
      GfxContext.QRP_2TexMod(geo.x, geo.y,
                             geo.width, geo.height,
                             _gradient_texture, texxform0,
                             nux::color::White,
                             _title_tex->GetDeviceTexture (),
                             texxform1,
                             nux::color::White);

      GfxContext.GetRenderStates ().SetBlend (alpha, src, dest);
      // The previous blend is too aggressive on the texture and therefore there
      // is a slight loss of clarity. This fixes that
      geo.width = button_width * (factor - 1);
      gPainter.PushDrawLayer (GfxContext, geo, _title_layer);
      geo = GetGeometry ();  
    }
    else
    {
      if (_title_layer)
        gPainter.PushDrawLayer (GfxContext,
                                geo,
                                _title_layer);
    }
  }

  gPainter.PopBackground ();
 
  GfxContext.PopClippingRectangle();
}

void
PanelMenuView::DrawContent (nux::GraphicsEngine &GfxContext, bool force_draw)
{
  nux::Geometry geo = GetGeometry ();

  GfxContext.PushClippingRectangle (geo);

  if (!_is_own_window && !_places_showing && _we_control_active)
  {
    if (_is_inside || _last_active_view || _show_now_activated)
    {
      _layout->ProcessDraw (GfxContext, force_draw);
    }

    if (_is_maximized)
    {
      _window_buttons->ProcessDraw (GfxContext, true);
    }
  }

  GfxContext.PopClippingRectangle();
}

gchar *
PanelMenuView::GetActiveViewName ()
{
  gchar         *label = NULL;
  BamfWindow    *window;

  _is_own_window = false;
  
  window = bamf_matcher_get_active_window (_matcher);
  if (BAMF_IS_WINDOW (window))
  {
    std::list<Window> our_xids = nux::XInputWindow::NativeHandleList ();
    std::list<Window>::iterator it;

    it = std::find (our_xids.begin (), our_xids.end (), bamf_window_get_xid (BAMF_WINDOW (window)));
    if (it != our_xids.end ())
      _is_own_window = true;
  }

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

  // We can get into a race that causes the geometry to be wrong as there hasn't been a layout
  // cycle before the first callback. This is to protect from that.
  if (geo.width > _monitor_geo.width)
    return;
  
  char                 *label = GetActiveViewName ();
  PangoLayout          *layout = NULL;
  PangoFontDescription *desc = NULL;
  GtkSettings          *settings = gtk_settings_get_default ();
  cairo_t              *cr;
  cairo_pattern_t      *linpat;
  char                 *font_description = NULL;
  GdkScreen            *screen = gdk_screen_get_default ();
  int                   dpi = 0;
  const int             text_margin = 30;

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
    pango_font_description_set_weight (desc, PANGO_WEIGHT_BOLD);

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

  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);

  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

  x = _padding;
  y = 0;

  if (_is_maximized)
    x += _window_buttons->GetContentWidth () + _padding + _padding;

  if (label)
  {
    nux::Color const& col = PanelStyle::GetDefault ()->GetTextColor();
    float red = col.red;
    float green = col.green;
    float blue = col.blue;

    pango_cairo_update_layout (cr, layout);

    y += (height - text_height)/2;
    double startalpha = 1.0 - ((double)text_margin/(double)width);

    // Once again for the homies that could
    if (x+text_width >= width-1)
    {
        linpat = cairo_pattern_create_linear (x, y, width-1, y+text_height);
        cairo_pattern_add_color_stop_rgb (linpat, 0, red, green, blue);
        cairo_pattern_add_color_stop_rgb (linpat, startalpha, red, green, blue);
        cairo_pattern_add_color_stop_rgba (linpat, 1, 0, 0.0, 0.0, 0);
        cairo_set_source(cr, linpat);
        cairo_pattern_destroy(linpat);
    }
    else
    {
      cairo_set_source_rgb (cr, red, green, blue);
    }
    cairo_move_to (cr, x, y);
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
                                        nux::color::White,
                                        true,
                                        rop);

  if (_title_tex)
    _title_tex->UnReference ();

  _title_tex = texture2D;

  g_free (label);
}

/* The entry was refreshed - so relayout our panel */
void
PanelMenuView::OnEntryRefreshed (PanelIndicatorObjectEntryView *view)
{
  QueueRelayout ();
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

  Refresh ();
  FullRedraw ();
}

void
PanelMenuView::OnEntryAdded (IndicatorObjectEntryProxy *proxy)
{
  PanelIndicatorObjectEntryView *view = new PanelIndicatorObjectEntryView (proxy, 6);
  view->active_changed.connect (sigc::mem_fun (this, &PanelMenuView::OnActiveChanged));
  view->refreshed.connect (sigc::mem_fun (this, &PanelMenuView::OnEntryRefreshed));
  proxy->show_now_changed.connect (sigc::mem_fun (this, &PanelMenuView::UpdateShowNow));
  _menu_layout->AddView (view, 0, nux::eCenter, nux::eFull);
  _menu_layout->SetContentDistribution (nux::eStackLeft);

  _entries.push_back (view);

  AddChild (view);

  QueueRelayout ();
  QueueDraw ();
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

  QueueRelayout ();
  QueueDraw ();
}

void
PanelMenuView::AllMenusClosed ()
{
  _is_inside = false;
  _last_active_view = false;

  FullRedraw ();
}

void
PanelMenuView::OnNameChanged (gchar* new_name, gchar* old_name)
{
  Refresh ();
  FullRedraw ();
}

void
PanelMenuView::OnActiveWindowChanged (BamfView *old_view,
                                      BamfView *new_view)
{
  _show_now_activated = false;
  _is_maximized = false;
  _active_xid = 0;
  if (_active_moved_id)
    g_source_remove (_active_moved_id);
  _active_moved_id = 0;

  if (BAMF_IS_WINDOW (new_view))
  {
    BamfWindow *window = BAMF_WINDOW (new_view);
    guint32 xid = _active_xid = bamf_window_get_xid (window);
    _is_maximized = WindowManager::Default ()->IsWindowMaximized (xid);
    nux::Geometry geo = WindowManager::Default ()->GetWindowGeometry (xid);

    if (bamf_window_get_window_type (window) == BAMF_WINDOW_DESKTOP)
      _we_control_active = true;
    else
      _we_control_active = UScreen::GetDefault ()->GetMonitorGeometry (_monitor).IsPointInside (geo.x + (geo.width/2), geo.y);

    if (_decor_map.find (xid) == _decor_map.end ())
    {
      _decor_map[xid] = true;
      
      // if we've just started tracking this window and it is maximized, let's 
      // make sure it's undecorated just in case it slipped by us earlier 
      // (I'm looking at you, Chromium!)
      if (_is_maximized)
      {
        WindowManager::Default ()->Undecorate (xid);
        _maximized_set.insert (xid);
      }
    }

    // first see if we need to remove and old callback
    if (_name_changed_callback_id != 0)
      g_signal_handler_disconnect (_name_changed_callback_instance,
                                   _name_changed_callback_id);

    // register callback for new view and store handler-id
    _name_changed_callback_instance = G_OBJECT (new_view);
    _name_changed_callback_id = g_signal_connect (_name_changed_callback_instance,
                                                  "name-changed",
                                                  (GCallback) on_name_changed,
                                                  this);
  }

  Refresh ();
  FullRedraw ();
}

void
PanelMenuView::OnSpreadInitiate ()
{
  /*foreach (guint32 &xid, windows)
  {
    if (WindowManager::Default ()->IsWindowMaximized (xid))
      WindowManager::Default ()->Decorate (xid);
  }*/
}

void
PanelMenuView::OnSpreadTerminate ()
{
  /*foreach (guint32 &xid, windows)
  {
    if (WindowManager::Default ()->IsWindowMaximized (xid))
      WindowManager::Default ()->Undecorate (xid);
  }*/
}

void
PanelMenuView::OnWindowMinimized (guint32 xid)
{
  
  if (WindowManager::Default ()->IsWindowMaximized (xid))
  {
    WindowManager::Default ()->Decorate (xid);
    _maximized_set.erase (xid);
  }
}

void
PanelMenuView::OnWindowUnminimized (guint32 xid)
{  
  if (WindowManager::Default ()->IsWindowMaximized (xid))
  {
    WindowManager::Default ()->Undecorate (xid);
    _maximized_set.insert (xid);
  }
}

void
PanelMenuView::OnWindowUnmapped (guint32 xid)
{
  _decor_map.erase (xid);
  _maximized_set.erase (xid);
}

void
PanelMenuView::OnWindowMaximized (guint xid)
{
  BamfWindow *window;

  window = bamf_matcher_get_active_window (_matcher);
  if (BAMF_IS_WINDOW (window) && bamf_window_get_xid (window) == xid)
  {
    _is_maximized = true;
  }
  
  // We could probably just check if a key is available, but who wants to do that
  if (_decor_map.find (xid) == _decor_map.end ())
    _decor_map[xid] = WindowManager::Default ()->IsWindowDecorated (xid);
  
  if (_decor_map[xid])
  {
    WindowManager::Default ()->Undecorate (xid);
  }

  _maximized_set.insert (xid);

  Refresh ();
  FullRedraw ();
}

void
PanelMenuView::OnWindowRestored (guint xid)
{
  BamfWindow *window;
  
  window = bamf_matcher_get_active_window (_matcher);
  if (BAMF_IS_WINDOW (window) && bamf_window_get_xid (window) == xid)
  {
    _is_maximized = false;
  }

  if (_decor_map[xid])
  {
    WindowManager::Default ()->Decorate (xid);
  }
  
  _maximized_set.erase (xid);

  Refresh ();
  FullRedraw ();
}

gboolean
PanelMenuView::UpdateActiveWindowPosition (PanelMenuView *self)
{
  nux::Geometry geo = WindowManager::Default ()->GetWindowGeometry (self->_active_xid);

  self->_we_control_active = UScreen::GetDefault ()->GetMonitorGeometry (self->_monitor).IsPointInside (geo.x + (geo.width/2), geo.y);

  self->_active_moved_id = 0;

  self->QueueDraw ();

  return FALSE;
}

void
PanelMenuView::OnWindowMoved (guint xid)
{
  if (_active_xid == xid)
  {
    if (_active_moved_id)
      g_source_remove (_active_moved_id);
    
    _active_moved_id = g_timeout_add (250, (GSourceFunc)PanelMenuView::UpdateActiveWindowPosition, this);
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

void
PanelMenuView::OnWindowButtonsRedraw ()
{
  FullRedraw ();
}

guint32
PanelMenuView::GetMaximizedWindow ()
{
  guint32 window_xid = 0;
  nux::Geometry monitor =  UScreen::GetDefault ()->GetMonitorGeometry (_monitor);
  
  // Find the front-most of the maximized windows we are controlling
  foreach (guint32 xid, _maximized_set)
  {
    // We can safely assume only the front-most is visible
    if (WindowManager::Default ()->IsWindowOnCurrentDesktop (xid)
        && !WindowManager::Default ()->IsWindowObscured (xid))
    {
      nux::Geometry geo = WindowManager::Default ()->GetWindowGeometry (xid);
      if (monitor.IsPointInside (geo.x + (geo.width/2), geo.y))
      {
        window_xid = xid;
        break;
      }
    }
  }
  return window_xid;
}

void
PanelMenuView::OnMaximizedGrab (int x, int y)
{
  guint32 window_xid = GetMaximizedWindow ();
  
  if (window_xid != 0)
  {
    WindowManager::Default ()->Activate (window_xid);
    _is_inside = true;
    _is_grabbed = true;
    Refresh ();
    FullRedraw ();
    WindowManager::Default ()->StartMove (window_xid, x, y);
  }
}

void
PanelMenuView::OnMouseDoubleClicked ()
{
  guint32 window_xid = GetMaximizedWindow ();
  
  if (window_xid != 0)
  {
    WindowManager::Default ()->Restore (window_xid);
  }
}

void
PanelMenuView::OnMouseMiddleClicked ()
{
  guint32 window_xid = GetMaximizedWindow ();
  
  if (window_xid != 0)
  {
    WindowManager::Default ()->Lower (window_xid);
  } 
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
  unity::variant::BuilderWrapper(builder).add(GetGeometry());
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

static void
on_name_changed (BamfView*      bamf_view,
                 gchar*         old_name,
                 gchar*         new_name,
                 PanelMenuView* self)
{
  self->OnNameChanged (new_name, old_name);
}

void
PanelMenuView::OnPlaceViewShown (GVariant *data, PanelMenuView *self)
{
  self->_places_showing = true;
  self->QueueDraw ();
}

void
PanelMenuView::OnPlaceViewHidden (GVariant *data, PanelMenuView *self)
{
  self->_places_showing = false;
  self->QueueDraw ();
}

void
PanelMenuView::UpdateShowNow (bool ignore)
{
  std::vector<PanelIndicatorObjectEntryView *>::iterator it;
  _show_now_activated = false;
  
  for (it = _entries.begin(); it != _entries.end(); it++)
  {
    PanelIndicatorObjectEntryView *view = static_cast<PanelIndicatorObjectEntryView *> (*it);
    if (view->GetShowNow ())
      _show_now_activated = true;

  }
  QueueDraw ();
}

void
PanelMenuView::SetMonitor (int monitor)
{
  _monitor = monitor;
  _monitor_geo = UScreen::GetDefault ()->GetMonitorGeometry (_monitor);
}

bool
PanelMenuView::GetControlsActive ()
{
  return _we_control_active;
}

bool
PanelMenuView::HasOurWindowFocused ()
{
  return _is_own_window;
}
