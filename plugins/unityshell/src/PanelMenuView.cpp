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
#include <glib.h>
#include <pango/pangocairo.h>
#include <gtk/gtk.h>
#include <X11/cursorfont.h>

#include <Nux/Nux.h>
#include <Nux/HLayout.h>
#include <Nux/VLayout.h>
#include <Nux/TextureArea.h>

#include <NuxGraphics/GLThread.h>
#include <NuxGraphics/XInputWindow.h>
#include <Nux/BaseWindow.h>

#include "CairoTexture.h"
#include "PanelMenuView.h"
#include "PanelStyle.h"
#include <UnityCore/Variant.h>

#include <gio/gdesktopappinfo.h>
#include <gconf/gconf-client.h>

#include "DashSettings.h"
#include "ubus-server.h"
#include "UBusMessages.h"

#include "UScreen.h"

#define WINDOW_TITLE_FONT_KEY "/apps/metacity/general/titlebar_font"

namespace unity
{

static void on_active_window_changed(BamfMatcher*   matcher,
                                     BamfView*      old_view,
                                     BamfView*      new_view,
                                     PanelMenuView* self);

static void on_name_changed(BamfView*      bamf_view,
                            gchar*         old_name,
                            gchar*         new_name,
                            PanelMenuView* self);

PanelMenuView::PanelMenuView(int padding)
  : _matcher(NULL),
    _title_layer(NULL),
    _util_cg(CAIRO_FORMAT_ARGB32, 1, 1),
    _gradient_texture(NULL),
    _title_tex(NULL),
    _is_inside(false),
    _is_maximized(false),
    _is_own_window(false),
    _last_active_view(NULL),
    _last_width(0),
    _last_height(0),
    _places_showing(false),
    _show_now_activated(false),
    _we_control_active(false),
    _monitor(0),
    _active_xid(0),
    _active_moved_id(0),
    _place_shown_interest(0),
    _place_hidden_interest(0)
{
  WindowManager* win_manager;

  _matcher = bamf_matcher_get_default();
  _activate_window_changed_id = g_signal_connect(_matcher, "active-window-changed",
                                                 G_CALLBACK(on_active_window_changed), this);

  // TODO: kill _menu_layout - should just use the _layout defined
  // in the base class.
  _menu_layout = new nux::HLayout("", NUX_TRACKER_LOCATION);
  _menu_layout->SetParentObject(this);

  /* This is for our parent and for PanelView to read indicator entries, we
   * shouldn't touch this again
   */
  layout_ = _menu_layout;

  _padding = padding;
  _name_changed_callback_instance = NULL;
  _name_changed_callback_id = 0;

  _window_buttons = new WindowButtons();
  _window_buttons->SetParentObject(this);
  _window_buttons->NeedRedraw();

  _window_buttons->close_clicked.connect(sigc::mem_fun(this, &PanelMenuView::OnCloseClicked));
  _window_buttons->minimize_clicked.connect(sigc::mem_fun(this, &PanelMenuView::OnMinimizeClicked));
  _window_buttons->restore_clicked.connect(sigc::mem_fun(this, &PanelMenuView::OnRestoreClicked));
  _window_buttons->mouse_enter.connect(sigc::mem_fun(this, &PanelMenuView::OnPanelViewMouseEnter));
  _window_buttons->mouse_leave.connect(sigc::mem_fun(this, &PanelMenuView::OnPanelViewMouseLeave));
  _window_buttons->mouse_move.connect(sigc::mem_fun(this, &PanelMenuView::OnPanelViewMouseMove));

  _panel_titlebar_grab_area = new PanelTitlebarGrabArea();
  _panel_titlebar_grab_area->SetParentObject(this);
  _panel_titlebar_grab_area->SinkReference();
  _panel_titlebar_grab_area->mouse_down.connect(sigc::mem_fun(this, &PanelMenuView::OnMaximizedGrabStart));
  _panel_titlebar_grab_area->mouse_drag.connect(sigc::mem_fun(this, &PanelMenuView::OnMaximizedGrabMove));
  _panel_titlebar_grab_area->mouse_up.connect(sigc::mem_fun(this, &PanelMenuView::OnMaximizedGrabEnd));
  _panel_titlebar_grab_area->mouse_doubleleftclick.connect(sigc::mem_fun(this, &PanelMenuView::OnMouseDoubleClicked));
  _panel_titlebar_grab_area->mouse_middleclick.connect(sigc::mem_fun(this, &PanelMenuView::OnMouseMiddleClicked));

  win_manager = WindowManager::Default();

  win_manager->window_minimized.connect(sigc::mem_fun(this, &PanelMenuView::OnWindowMinimized));
  win_manager->window_unminimized.connect(sigc::mem_fun(this, &PanelMenuView::OnWindowUnminimized));
  win_manager->initiate_spread.connect(sigc::mem_fun(this, &PanelMenuView::OnSpreadInitiate));
  win_manager->terminate_spread.connect(sigc::mem_fun(this, &PanelMenuView::OnSpreadTerminate));

  win_manager->window_maximized.connect(sigc::mem_fun(this, &PanelMenuView::OnWindowMaximized));
  win_manager->window_restored.connect(sigc::mem_fun(this, &PanelMenuView::OnWindowRestored));
  win_manager->window_unmapped.connect(sigc::mem_fun(this, &PanelMenuView::OnWindowUnmapped));
  win_manager->window_moved.connect(sigc::mem_fun(this, &PanelMenuView::OnWindowMoved));

  PanelStyle::GetDefault()->changed.connect(sigc::mem_fun(this, &PanelMenuView::Refresh));

  mouse_enter.connect(sigc::mem_fun(this, &PanelMenuView::OnPanelViewMouseEnter));
  mouse_leave.connect(sigc::mem_fun(this, &PanelMenuView::OnPanelViewMouseLeave));
  mouse_move.connect(sigc::mem_fun(this, &PanelMenuView::OnPanelViewMouseMove));

  _panel_titlebar_grab_area->mouse_enter.connect(sigc::mem_fun(this, &PanelMenuView::OnPanelViewMouseEnter));
  _panel_titlebar_grab_area->mouse_leave.connect(sigc::mem_fun(this, &PanelMenuView::OnPanelViewMouseLeave));

  // Register for all the interesting events
  UBusServer* ubus = ubus_server_get_default();
  _place_shown_interest = ubus_server_register_interest(ubus, UBUS_PLACE_VIEW_SHOWN,
                                                        (UBusCallback)PanelMenuView::OnPlaceViewShown,
                                                        this);
  _place_hidden_interest = ubus_server_register_interest(ubus, UBUS_PLACE_VIEW_HIDDEN,
                                                         (UBusCallback)PanelMenuView::OnPlaceViewHidden,
                                                         this);

  Refresh();
}

PanelMenuView::~PanelMenuView()
{
  if (_name_changed_callback_id)
    g_signal_handler_disconnect(_name_changed_callback_instance,
                                _name_changed_callback_id);
  if (_activate_window_changed_id)
    g_signal_handler_disconnect(_matcher,
                                _activate_window_changed_id);
  if (_active_moved_id)
    g_source_remove(_active_moved_id);

  if (_title_layer)
    delete _title_layer;
  if (_title_tex)
    _title_tex->UnReference();

  _menu_layout->UnReference();
  _window_buttons->UnReference();
  _panel_titlebar_grab_area->UnReference();

  UBusServer* ubus = ubus_server_get_default();
  if (_place_shown_interest != 0)
    ubus_server_unregister_interest(ubus, _place_shown_interest);

  if (_place_hidden_interest != 0)
    ubus_server_unregister_interest(ubus, _place_hidden_interest);
}

void
PanelMenuView::FullRedraw()
{
  _menu_layout->NeedRedraw();
  _window_buttons->NeedRedraw();
  NeedRedraw();
}

long
PanelMenuView::ProcessEvent(nux::IEvent& ievent, long TraverseInfo, long ProcessEventInfo)
{
  long ret = TraverseInfo;
  nux::Geometry geo = GetAbsoluteGeometry();
  nux::Geometry geo_buttons = _window_buttons->GetAbsoluteGeometry();

  if (!_we_control_active)
    return _panel_titlebar_grab_area->OnEvent(ievent, ret, ProcessEventInfo);

  if (geo.IsPointInside(ievent.e_x, ievent.e_y) && !(_is_maximized && geo_buttons.IsPointInside(ievent.e_x, ievent.e_y)))
  {
    if (_is_inside != true)
    {
      if (_is_grabbed)
        _is_grabbed = false;
      else
        _is_inside = true;
      FullRedraw();
    }
  }
  else
  {
    if (_is_inside != false)
    {
      _is_inside = false;
      FullRedraw();
    }
  }

  if (_is_maximized || _places_showing)
  {
    if (_window_buttons)
      ret = _window_buttons->ProcessEvent(ievent, ret, ProcessEventInfo);
    if (_panel_titlebar_grab_area)
      ret = _panel_titlebar_grab_area->OnEvent(ievent, ret, ProcessEventInfo);
  }
  ret = _panel_titlebar_grab_area->OnEvent(ievent, ret, ProcessEventInfo);

  if (!_is_own_window)
    ret = _menu_layout->ProcessEvent(ievent, ret, ProcessEventInfo);

  return ret;
}

nux::Area*
PanelMenuView::FindAreaUnderMouse(const nux::Point& mouse_position, nux::NuxEventType event_type)
{
  bool mouse_inside = TestMousePointerInclusionFilterMouseWheel(mouse_position, event_type);

  if (mouse_inside == false)
    return NULL;

  Area* found_area = NULL;
  if (!_we_control_active)
  {
    found_area = _panel_titlebar_grab_area->FindAreaUnderMouse(mouse_position, event_type);
    NUX_RETURN_VALUE_IF_NOTNULL(found_area, found_area);
  }

  if (_is_maximized || _places_showing)
  {
    if (_window_buttons)
    {
      found_area = _window_buttons->FindAreaUnderMouse(mouse_position, event_type);
      NUX_RETURN_VALUE_IF_NOTNULL(found_area, found_area);
    }

    if (_panel_titlebar_grab_area)
    {
      found_area = _panel_titlebar_grab_area->FindAreaUnderMouse(mouse_position, event_type);
      NUX_RETURN_VALUE_IF_NOTNULL(found_area, found_area);
    }
  }

  if (_panel_titlebar_grab_area)
  {
    found_area = _panel_titlebar_grab_area->FindAreaUnderMouse(mouse_position, event_type);
    NUX_RETURN_VALUE_IF_NOTNULL(found_area, found_area);
  }

  if (!_is_own_window)
  {
    found_area = _menu_layout->FindAreaUnderMouse(mouse_position, event_type);
    NUX_RETURN_VALUE_IF_NOTNULL(found_area, found_area);
  }

  return View::FindAreaUnderMouse(mouse_position, event_type);
}

long PanelMenuView::PostLayoutManagement(long LayoutResult)
{
  long res = View::PostLayoutManagement(LayoutResult);
  int old_window_buttons_w, new_window_buttons_w;
  int old_menu_area_w, new_menu_area_w;

  nux::Geometry geo = GetGeometry();

  old_window_buttons_w = _window_buttons->GetContentWidth();
  _window_buttons->SetGeometry(geo.x + _padding, geo.y, old_window_buttons_w, geo.height);
  _window_buttons->ComputeLayout2();
  new_window_buttons_w = _window_buttons->GetContentWidth();

  /* Explicitly set the size and position of the widgets */
  geo.x += _padding + new_window_buttons_w + _padding;
  geo.width -= _padding + new_window_buttons_w + _padding;

  old_menu_area_w = _menu_layout->GetContentWidth();
  _menu_layout->SetGeometry(geo.x, geo.y, old_menu_area_w, geo.height);
  _menu_layout->ComputeLayout2();
  new_menu_area_w = _menu_layout->GetContentWidth();

  geo.x += new_menu_area_w;
  geo.width -= new_menu_area_w;

  _panel_titlebar_grab_area->SetGeometry(geo.x, geo.y, geo.width, geo.height);

  if (_is_inside)
    NeedRedraw();

  return res;
}

void
PanelMenuView::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry geo = GetGeometry();
  int button_width = _padding + _window_buttons->GetContentWidth() + _padding;
  float factor = 4;
  button_width /= factor;

  if (geo.width != _last_width || geo.height != _last_height)
  {
    _last_width = geo.width;
    _last_height = geo.height;
    Refresh();
  }

  GfxContext.PushClippingRectangle(geo);

  /* "Clear" out the background */
  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

  nux::ColorLayer layer(nux::Color(0x00000000), true, rop);
  gPainter.PushDrawLayer(GfxContext, GetGeometry(), &layer);

  if (_is_own_window || !_we_control_active || (_is_maximized && _is_inside))
  {

  }
  else
  {
    bool have_valid_entries = false;
    Entries::iterator it, eit = entries_.end();

    for (it = entries_.begin(); it != eit; ++it)
    {
      if (it->second->IsEntryValid())
      {
        have_valid_entries = true;
        break;
      }
    }

    if ((_is_inside || _last_active_view || _show_now_activated) && have_valid_entries)
    {
      if (_gradient_texture.IsNull())
      {
        nux::NTextureData texture_data(nux::BITFMT_R8G8B8A8, geo.width, 1, 1);
        nux::ImageSurface surface = texture_data.GetSurface(0);
        nux::SURFACE_LOCKED_RECT lockrect;
        BYTE* dest;
        int num_row;

        _gradient_texture = nux::GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableDeviceTexture(texture_data.GetWidth(), texture_data.GetHeight(), 1, texture_data.GetFormat());

        _gradient_texture->LockRect(0, &lockrect, 0);

        dest = (BYTE*) lockrect.pBits;
        num_row = surface.GetBlockHeight();

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
              a = 255 - 255 * (((float)x - (button_width * (factor - 1))) / (float)(button_width));
            }
            else
            {
              a = 0x00;
            }

            *(dest + y * lockrect.Pitch + 4 * x + 0) = (223 * a) / 255; //red
            *(dest + y * lockrect.Pitch + 4 * x + 1) = (219 * a) / 255; //green
            *(dest + y * lockrect.Pitch + 4 * x + 2) = (210 * a) / 255; //blue
            *(dest + y * lockrect.Pitch + 4 * x + 3) = a;
          }
        }
        _gradient_texture->UnlockRect(0);
      }
      guint alpha = 0, src = 0, dest = 0;

      GfxContext.GetRenderStates().GetBlend(alpha, src, dest);
      GfxContext.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

      nux::TexCoordXForm texxform0;
      nux::TexCoordXForm texxform1;

      // Modulate the checkboard and the gradient texture
      if (_title_tex)
        GfxContext.QRP_2TexMod(geo.x, geo.y,
                               geo.width, geo.height,
                               _gradient_texture, texxform0,
                               nux::color::White,
                               _title_tex->GetDeviceTexture(),
                               texxform1,
                               nux::color::White);

      GfxContext.GetRenderStates().SetBlend(alpha, src, dest);
      // The previous blend is too aggressive on the texture and therefore there
      // is a slight loss of clarity. This fixes that
      geo.width = button_width * (factor - 1);
      if (_title_layer)
        gPainter.PushDrawLayer(GfxContext, geo, _title_layer);
      geo = GetGeometry();
    }
    else
    {
      if (_title_layer)
        gPainter.PushDrawLayer(GfxContext,
                               geo,
                               _title_layer);
    }
  }

  gPainter.PopBackground();

  GfxContext.PopClippingRectangle();
}

void
PanelMenuView::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry geo = GetGeometry();

  GfxContext.PushClippingRectangle(geo);

  if (!_is_own_window && !_places_showing && _we_control_active)
  {
    if (_is_inside || _last_active_view || _show_now_activated)
      layout_->ProcessDraw(GfxContext, force_draw);
  }

    if ((!_is_own_window && _we_control_active && _is_maximized && _is_inside) ||
        _places_showing)
    {
      _window_buttons->ProcessDraw(GfxContext, true);
    }

  GfxContext.PopClippingRectangle();
}

gchar*
PanelMenuView::GetActiveViewName()
{
  gchar*         label = NULL;
  BamfWindow*    window;

  _is_own_window = false;

  window = bamf_matcher_get_active_window(_matcher);
  if (BAMF_IS_WINDOW(window))
  {
    std::vector<Window> const& our_xids = nux::XInputWindow::NativeHandleList();

    if (std::find(our_xids.begin(), our_xids.end(), bamf_window_get_xid(BAMF_WINDOW(window))) != our_xids.end())
      _is_own_window = true;
  }

  if (_is_maximized)
  {
    BamfWindow* window = bamf_matcher_get_active_window(_matcher);

    if (BAMF_IS_WINDOW(window))
      label = bamf_view_get_name(BAMF_VIEW(window));
  }

  if (!label)
  {
    BamfApplication* app = bamf_matcher_get_active_application(_matcher);
    if (BAMF_IS_APPLICATION(app))
    {
      const gchar*     filename;

      filename = bamf_application_get_desktop_file(app);

      if (filename && g_strcmp0(filename, "") != 0)
      {
        GDesktopAppInfo* info;

        info = g_desktop_app_info_new_from_filename(bamf_application_get_desktop_file(app));

        if (info)
        {
          label = g_strdup(g_app_info_get_display_name(G_APP_INFO(info)));
          g_object_unref(info);
        }
        else
        {
          g_warning("Unable to get GDesktopAppInfo for %s",
                    bamf_application_get_desktop_file(app));
        }
      }

      if (label == NULL)
      {
        BamfView* active_view;

        active_view = (BamfView*)bamf_matcher_get_active_window(_matcher);
        if (BAMF_IS_VIEW(active_view))
          label = bamf_view_get_name(active_view);
        else
          label = g_strdup("");
      }
    }
    else
    {
      label = g_strdup(" ");
    }
  }

  char *escaped = g_markup_escape_text(label, -1);
  g_free(label);
  label = g_strdup_printf("<b>%s</b>", escaped);
  g_free(escaped);

  return label;
}

void
PanelMenuView::Refresh()
{
  nux::Geometry         geo = GetGeometry();

  // We can get into a race that causes the geometry to be wrong as there hasn't been a layout
  // cycle before the first callback. This is to protect from that.
  if (geo.width > _monitor_geo.width)
    return;

  char*                 label = GetActiveViewName();
  PangoLayout*          layout = NULL;
  PangoFontDescription* desc = NULL;
  GtkSettings*          settings = gtk_settings_get_default();
  cairo_t*              cr;
  cairo_pattern_t*      linpat;
  char*                 font_description = NULL;
  GdkScreen*            screen = gdk_screen_get_default();
  int                   dpi = 0;
  const int             fading_pixels = 35;

  int  x = 0;
  int  y = 0;
  int  width = geo.width;
  int  height = geo.height;
  int  text_width = 0;
  int  text_height = 0;
  int  text_space = 0;

  if (label)
  {
    GConfClient* client = gconf_client_get_default();
    PangoContext* cxt;
    PangoRectangle log_rect;

    cr = _util_cg.GetContext();

    g_object_get(settings,
                 "gtk-xft-dpi", &dpi,
                 NULL);

    font_description = gconf_client_get_string(client, WINDOW_TITLE_FONT_KEY, NULL);
    desc = pango_font_description_from_string(font_description);

    layout = pango_cairo_create_layout(cr);
    pango_layout_set_font_description(layout, desc);
    pango_layout_set_markup(layout, label, -1);

    cxt = pango_layout_get_context(layout);
    pango_cairo_context_set_font_options(cxt, gdk_screen_get_font_options(screen));
    pango_cairo_context_set_resolution(cxt, (float)dpi / (float)PANGO_SCALE);
    pango_layout_context_changed(layout);

    pango_layout_get_extents(layout, NULL, &log_rect);
    text_width = log_rect.width / PANGO_SCALE;
    text_height = log_rect.height / PANGO_SCALE;

    pango_font_description_free(desc);
    g_free(font_description);
    cairo_destroy(cr);
    g_object_unref(client);
  }

  nux::CairoGraphics cairo_graphics(CAIRO_FORMAT_ARGB32, width, height);
  cr = cairo_graphics.GetContext();
  cairo_set_line_width(cr, 1);

  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

  x = _padding;
  y = 0;

  if (label)
  {
    PanelStyle* style = PanelStyle::GetDefault();
    GtkStyleContext* style_context = style->GetStyleContext();
    text_space = width - x;

    gtk_style_context_save(style_context);

    GtkWidgetPath* widget_path = gtk_widget_path_new();
    gtk_widget_path_iter_set_name(widget_path, -1 , "UnityPanelWidget");
    gtk_widget_path_append_type(widget_path, GTK_TYPE_MENU_BAR);
    gtk_widget_path_append_type(widget_path, GTK_TYPE_MENU_ITEM);

    gtk_style_context_set_path(style_context, widget_path);
    gtk_style_context_add_class(style_context, GTK_STYLE_CLASS_MENUBAR);
    gtk_style_context_add_class(style_context, GTK_STYLE_CLASS_MENUITEM);

    y += (height - text_height) / 2;

    pango_cairo_update_layout(cr, layout);

    if (text_width > text_space)
    {
      int out_pixels = text_width - text_space;
      int fading_width = out_pixels < fading_pixels ? out_pixels : fading_pixels;

      cairo_push_group(cr);
      gtk_render_layout(style_context, cr, x, y, layout);
      cairo_pop_group_to_source(cr);

      linpat = cairo_pattern_create_linear(width - fading_width, y, width, y);
      cairo_pattern_add_color_stop_rgba(linpat, 0, 0, 0, 0, 1);
      cairo_pattern_add_color_stop_rgba(linpat, 1, 0, 0, 0, 0);
      cairo_mask(cr, linpat);

      cairo_pattern_destroy(linpat);
    }
    else
    {
      gtk_render_layout(style_context, cr, x, y, layout);
    }

    gtk_widget_path_free(widget_path);

    gtk_style_context_restore(style_context);
  }

  cairo_destroy(cr);
  if (layout)
    g_object_unref(layout);

  nux::BaseTexture* texture2D = texture_from_cairo_graphics(cairo_graphics);

  if (_title_layer)
    delete _title_layer;

  nux::TexCoordXForm texxform;
  texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
  texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);

  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
  _title_layer = new nux::TextureLayer(texture2D->GetDeviceTexture(),
                                       texxform,
                                       nux::color::White,
                                       true,
                                       rop);

  if (_title_tex)
    _title_tex->UnReference();

  _title_tex = texture2D;

  g_free(label);
}

void
PanelMenuView::OnActiveChanged(PanelIndicatorEntryView* view,
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

  Refresh();
  FullRedraw();
}

void PanelMenuView::OnEntryAdded(unity::indicator::Entry::Ptr const& proxy)
{
  PanelIndicatorEntryView* view = new PanelIndicatorEntryView(proxy, 6);
  view->active_changed.connect(sigc::mem_fun(this, &PanelMenuView::OnActiveChanged));
  view->refreshed.connect(sigc::mem_fun(this, &PanelIndicatorsView::OnEntryRefreshed));
  proxy->show_now_changed.connect(sigc::mem_fun(this, &PanelMenuView::UpdateShowNow));

  _menu_layout->AddView(view, 0, nux::eCenter, nux::eFull, 1.0, nux::NUX_LAYOUT_END);
  _menu_layout->SetContentDistribution(nux::eStackLeft);

  entries_[proxy->id()] = view;
  AddChild(view);

  view->mouse_enter.connect(sigc::mem_fun(this, &PanelMenuView::OnPanelViewMouseEnter));
  view->mouse_leave.connect(sigc::mem_fun(this, &PanelMenuView::OnPanelViewMouseLeave));

  QueueRelayout();
  QueueDraw();

  on_indicator_updated.emit(view);
}

void
PanelMenuView::AllMenusClosed()
{
  auto mouse = nux::GetGraphicsDisplay()->GetMouseScreenCoord();

  _is_inside = GetAbsoluteGeometry().IsPointInside(mouse.x, mouse.y);
  _last_active_view = NULL;

  FullRedraw();
}

void
PanelMenuView::OnNameChanged(gchar* new_name, gchar* old_name)
{
  Refresh();
  FullRedraw();
}

void
PanelMenuView::OnActiveWindowChanged(BamfView* old_view,
                                     BamfView* new_view)
{
  _show_now_activated = false;
  _is_maximized = false;
  _active_xid = 0;
  if (_active_moved_id)
    g_source_remove(_active_moved_id);
  _active_moved_id = 0;

  if (BAMF_IS_WINDOW(new_view))
  {
    BamfWindow* window = BAMF_WINDOW(new_view);
    guint32 xid = _active_xid = bamf_window_get_xid(window);
    _is_maximized = WindowManager::Default()->IsWindowMaximized(xid);
    nux::Geometry geo = WindowManager::Default()->GetWindowGeometry(xid);

    if (bamf_window_get_window_type(window) == BAMF_WINDOW_DESKTOP)
      _we_control_active = true;
    else
      _we_control_active = UScreen::GetDefault()->GetMonitorGeometry(_monitor).IsPointInside(geo.x + (geo.width / 2), geo.y);

    if (_decor_map.find(xid) == _decor_map.end())
    {
      _decor_map[xid] = true;

      // if we've just started tracking this window and it is maximized, let's
      // make sure it's undecorated just in case it slipped by us earlier
      // (I'm looking at you, Chromium!)
      if (_is_maximized)
      {
        WindowManager::Default()->Undecorate(xid);
        _maximized_set.insert(xid);
      }
    }

    // first see if we need to remove and old callback
    if (_name_changed_callback_id != 0)
      g_signal_handler_disconnect(_name_changed_callback_instance,
                                  _name_changed_callback_id);

    // register callback for new view and store handler-id
    _name_changed_callback_instance = G_OBJECT(new_view);
    _name_changed_callback_id = g_signal_connect(_name_changed_callback_instance,
                                                 "name-changed",
                                                 (GCallback) on_name_changed,
                                                 this);
  }

  Refresh();
  FullRedraw();
}

void
PanelMenuView::OnSpreadInitiate()
{
  /*foreach (guint32 &xid, windows)
  {
    if (WindowManager::Default ()->IsWindowMaximized (xid))
      WindowManager::Default ()->Decorate (xid);
  }*/
}

void
PanelMenuView::OnSpreadTerminate()
{
  /*foreach (guint32 &xid, windows)
  {
    if (WindowManager::Default ()->IsWindowMaximized (xid))
      WindowManager::Default ()->Undecorate (xid);
  }*/
}

void
PanelMenuView::OnWindowMinimized(guint32 xid)
{

  if (WindowManager::Default()->IsWindowMaximized(xid))
  {
    WindowManager::Default()->Decorate(xid);
    _maximized_set.erase(xid);
  }
}

void
PanelMenuView::OnWindowUnminimized(guint32 xid)
{
  if (WindowManager::Default()->IsWindowMaximized(xid))
  {
    WindowManager::Default()->Undecorate(xid);
    _maximized_set.insert(xid);
  }
}

void
PanelMenuView::OnWindowUnmapped(guint32 xid)
{
  _decor_map.erase(xid);
  _maximized_set.erase(xid);
}

void
PanelMenuView::OnWindowMaximized(guint xid)
{
  BamfWindow* window;

  window = bamf_matcher_get_active_window(_matcher);
  if (BAMF_IS_WINDOW(window) && bamf_window_get_xid(window) == xid)
  {
    _is_maximized = true;

    // We need to update the _is_inside state in the case of maximization by grab
    gint x, y;
    gdk_display_get_pointer(gdk_display_get_default(), NULL, &x, &y, NULL);
    _is_inside = GetAbsoluteGeometry().IsPointInside(x, y);

    updated = true;
  }

  // update the state of the window in the _decor_map
  _decor_map[xid] = WindowManager::Default()->IsWindowDecorated(xid);

  if (_decor_map[xid])
  {
    WindowManager::Default()->Undecorate(xid);
  }

  _maximized_set.insert(xid);

  if (updated)
  {
    Refresh();
    FullRedraw();
  }
}

void
PanelMenuView::OnWindowRestored(guint xid)
{
  BamfWindow* window;

  window = bamf_matcher_get_active_window(_matcher);
  if (BAMF_IS_WINDOW(window) && bamf_window_get_xid(window) == xid)
  {
    _is_maximized = false;
    _is_grabbed = false;
  }

  if (_decor_map[xid])
  {
    WindowManager::Default()->Decorate(xid);
  }

  _maximized_set.erase(xid);

  Refresh();
  FullRedraw();
}

gboolean
PanelMenuView::UpdateActiveWindowPosition(PanelMenuView* self)
{
  nux::Geometry geo = WindowManager::Default()->GetWindowGeometry(self->_active_xid);

  self->_we_control_active = UScreen::GetDefault()->GetMonitorGeometry(self->_monitor).IsPointInside(geo.x + (geo.width / 2), geo.y);

  self->_active_moved_id = 0;

  self->QueueDraw();

  return FALSE;
}

void
PanelMenuView::OnWindowMoved(guint xid)
{
  if (_active_xid == xid)
  {
    if (_active_moved_id)
      g_source_remove(_active_moved_id);

    _active_moved_id = g_timeout_add(250, (GSourceFunc)PanelMenuView::UpdateActiveWindowPosition, this);
  }
}

void
PanelMenuView::OnCloseClicked()
{
  if (_places_showing)
  {
    ubus_server_send_message(ubus_server_get_default(), UBUS_PLACE_VIEW_CLOSE_REQUEST, NULL);
  }
  else
  {
    BamfWindow* window;

    window = bamf_matcher_get_active_window(_matcher);
    if (BAMF_IS_WINDOW(window))
      WindowManager::Default()->Close(bamf_window_get_xid(window));
  }
}

void
PanelMenuView::OnMinimizeClicked()
{
  if (_places_showing)
  {
    // no action when dash is opened, LP bug #838875
    return;
  }
  else
  {
    BamfWindow* window;

    window = bamf_matcher_get_active_window(_matcher);
    if (BAMF_IS_WINDOW(window))
      WindowManager::Default()->Minimize(bamf_window_get_xid(window));
  }
}

void
PanelMenuView::OnRestoreClicked()
{
  if (_places_showing)
  {
    if (DashSettings::GetDefault()->GetFormFactor() == DashSettings::DESKTOP)
      DashSettings::GetDefault()->SetFormFactor(DashSettings::NETBOOK);
    else
      DashSettings::GetDefault()->SetFormFactor(DashSettings::DESKTOP);
  }
  else
  {
    BamfWindow* window;

    window = bamf_matcher_get_active_window(_matcher);
    if (BAMF_IS_WINDOW(window))
      WindowManager::Default()->Restore(bamf_window_get_xid(window));
  }
}

guint32
PanelMenuView::GetMaximizedWindow()
{
  guint32 window_xid = 0;
  nux::Geometry monitor =  UScreen::GetDefault()->GetMonitorGeometry(_monitor);

  // Find the front-most of the maximized windows we are controlling
  foreach(guint32 xid, _maximized_set)
  {
    // We can safely assume only the front-most is visible
    if (WindowManager::Default()->IsWindowOnCurrentDesktop(xid)
        && !WindowManager::Default()->IsWindowObscured(xid))
    {
      nux::Geometry geo = WindowManager::Default()->GetWindowGeometry(xid);
      if (monitor.IsPointInside(geo.x + (geo.width / 2), geo.y))
      {
        window_xid = xid;
        break;
      }
    }
  }
  return window_xid;
}

void
PanelMenuView::OnMaximizedGrabStart(int x, int y)
{
  // When Start dragging the panelmenu of a maximized window, change cursor
  // to simulate the dragging, waiting to go out of the panel area.
  //
  // This is a workaround to avoid that the grid plugin would be fired
  // showing the window shape preview effect. See bug #838923
  if (GetMaximizedWindow() != 0)
  {
    Display* d = nux::GetGraphicsDisplay()->GetX11Display();
    nux::BaseWindow *bw = static_cast<nux::BaseWindow*>(GetTopLevelViewWindow());
    Cursor c = XCreateFontCursor(d, XC_fleur);
    XDefineCursor(d, bw->GetInputWindowId(), c);
    XFreeCursor(d, c);
  }
}

void
PanelMenuView::OnMaximizedGrabMove(int x, int y, int, int, unsigned long, unsigned long)
{
  guint32 window_xid = GetMaximizedWindow();

  // When the drag goes out from the Panel, start the real movement.
  //
  // This is a workaround to avoid that the grid plugin would be fired
  // showing the window shape preview effect. See bug #838923
  if (window_xid != 0 && !GetAbsoluteGeometry().IsPointInside(x, y))
  {
    Display* d = nux::GetGraphicsDisplay()->GetX11Display();
    nux::BaseWindow *bw = static_cast<nux::BaseWindow*>(GetTopLevelViewWindow());
    XUndefineCursor(d, bw->GetInputWindowId());

    WindowManager::Default()->Activate(window_xid);
    _is_inside = true;
    _is_grabbed = true;
    Refresh();
    FullRedraw();
    WindowManager::Default()->StartMove(window_xid, x, y);
  }
}

void
PanelMenuView::OnMaximizedGrabEnd(int x, int y, unsigned long, unsigned long)
{
  // Restore the window cursor to default.
  Display* d = nux::GetGraphicsDisplay()->GetX11Display();
  nux::BaseWindow *bw = static_cast<nux::BaseWindow*>(GetTopLevelViewWindow());
  XUndefineCursor(d, bw->GetInputWindowId());
}

void
PanelMenuView::OnMouseDoubleClicked()
{
  guint32 window_xid = GetMaximizedWindow();

  if (window_xid != 0)
  {
    WindowManager::Default()->Restore(window_xid);
    _is_inside = true;
  }
}

void
PanelMenuView::OnMouseMiddleClicked()
{
  guint32 window_xid = GetMaximizedWindow();

  if (window_xid != 0)
  {
    WindowManager::Default()->Lower(window_xid);
  }
}

// Introspectable
const gchar*
PanelMenuView::GetName()
{
  return "MenuView";
}

const gchar*
PanelMenuView::GetChildsName()
{
  return "entries";
}

void PanelMenuView::AddProperties(GVariantBuilder* builder)
{
  variant::BuilderWrapper(builder).add(GetGeometry());
}

/*
 * C code for callbacks
 */
static void
on_active_window_changed(BamfMatcher*   matcher,
                         BamfView*      old_view,
                         BamfView*      new_view,
                         PanelMenuView* self)
{
  self->OnActiveWindowChanged(old_view, new_view);
}

static void
on_name_changed(BamfView*      bamf_view,
                gchar*         old_name,
                gchar*         new_name,
                PanelMenuView* self)
{
  self->OnNameChanged(new_name, old_name);
}

void
PanelMenuView::OnPlaceViewShown(GVariant* data, PanelMenuView* self)
{
  self->_places_showing = true;
  self->QueueDraw();
}

void
PanelMenuView::OnPlaceViewHidden(GVariant* data, PanelMenuView* self)
{
  self->_places_showing = false;
  self->QueueDraw();
}

void PanelMenuView::UpdateShowNow(bool ignore)
{
  // NOTE: This is sub-optimal.  We are getting a dbus event for every menu,
  // and every time that is setting the show now status of an indicator entry,
  // we are getting the event raised, and we are ignoring the status, and
  // looking through all the entries to see if any are shown.
  _show_now_activated = false;

  for (Entries::iterator it = entries_.begin(); it != entries_.end(); ++it)
  {
    PanelIndicatorEntryView* view = it->second;
    if (view->GetShowNow()) {
      _show_now_activated = true;
      break;
    }
  }
  QueueDraw();
}

void
PanelMenuView::SetMonitor(int monitor)
{
  _monitor = monitor;
  _monitor_geo = UScreen::GetDefault()->GetMonitorGeometry(_monitor);
}

bool
PanelMenuView::GetControlsActive()
{
  return _we_control_active;
}

bool
PanelMenuView::HasOurWindowFocused()
{
  return _is_own_window;
}

void
PanelMenuView::OnPanelViewMouseEnter(int x, int y, unsigned long mouse_button_state, unsigned long special_keys_state)
{
  if (_is_inside != true)
  {
    if (_is_grabbed)
      _is_grabbed = false;
    else
      _is_inside = true;
    FullRedraw();
  }
}

void
PanelMenuView::OnPanelViewMouseLeave(int x, int y, unsigned long mouse_button_state, unsigned long special_keys_state)
{
  if (_is_inside != false)
  {
    _is_inside = false;
    FullRedraw();
  }
}

void PanelMenuView::OnPanelViewMouseMove(int x, int y, int dx, int dy, unsigned long mouse_button_state, unsigned long special_keys_state)
{}


} // namespace unity
