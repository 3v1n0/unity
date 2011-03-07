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
 * Authored by: Jason Smith <jason.smith@canonical.com>
 * Authored by: Jay Taoko <jay.taoko@canonical.com>
 */

#include <math.h>

#include "Nux/Nux.h"
#include "Nux/VScrollBar.h"
#include "Nux/HLayout.h"
#include "Nux/VLayout.h"
#include "Nux/MenuPage.h"

#include "NuxGraphics/NuxGraphics.h"
#include "NuxGraphics/GpuDevice.h"
#include "NuxGraphics/GLTextureResourceManager.h"

#include "Nux/BaseWindow.h"
#include "Nux/WindowCompositor.h"

#include "Launcher.h"
#include "LauncherIcon.h"
#include "SpacerLauncherIcon.h"
#include "LauncherModel.h"
#include "QuicklistManager.h"
#include "QuicklistView.h"

#include "ubus-server.h"
#include "UBusMessages.h"

#define URGENT_BLINKS       3
#define WIGGLE_CYCLES       6

#define MAX_STARTING_BLINKS 5
#define STARTING_BLINK_LAMBDA 3

#define BACKLIGHT_STRENGTH  0.9f

NUX_IMPLEMENT_OBJECT_TYPE (Launcher);

int
TimeDelta (struct timespec const *x, struct timespec const *y)
{
  return ((x->tv_sec - y->tv_sec) * 1000) + ((x->tv_nsec - y->tv_nsec) / 1000000);
}

void SetTimeBack (struct timespec *timeref, int remove)
{
  timeref->tv_sec -= remove / 1000;
  remove = remove % 1000;

  if (remove > timeref->tv_nsec / 1000000)
  {
      timeref->tv_sec--;
      timeref->tv_nsec += 1000000000;
  }
  timeref->tv_nsec -= remove * 1000000;
}


/*
	        Use this shader to pass vertices in screen coordinates in the C++ code and compute use
	        the fragment shader to perform the texture perspective correct division.
	        This shader assume the following:
		        - the projection matrix is orthogonal: glOrtho(0, ScreenWidth, ScreenWidth, 0, Near, Far)
		        - vertices x and y are in screen coordinates: Vertex(x_screen, y_screen, 0, 1.0)
		        - the vertices w coordinates has been computed 'manually'
		        - vertices uv textture coordinates are passed to the shader as:  (u/w, v/w, 0, 1/w)

	        The texture coordinates s=u/w, t=v/w and q=1w are interpolated linearly in screen coordinates.
	        In the fragment shader we get the texture coordinates used for the sampling by dividing
	        s and t resulting from the interpolation by q.

        */

nux::NString gPerspectiveCorrectShader = TEXT (
"[Vertex Shader]                                                        \n\
#version 120                                                            \n\
uniform mat4 ViewProjectionMatrix;                                      \n\
                                                                        \n\
attribute vec4 iTexCoord0;                                              \n\
attribute vec4 iVertex;                                                 \n\
                                                                        \n\
varying vec4 varyTexCoord0;                                             \n\
                                                                        \n\
void main()                                                             \n\
{                                                                       \n\
    varyTexCoord0 = iTexCoord0;                                         \n\
    gl_Position =  ViewProjectionMatrix * iVertex;                      \n\
}                                                                       \n\
                                                                        \n\
[Fragment Shader]                                                       \n\
#version 110                                                            \n\
                                                                        \n\
varying vec4 varyTexCoord0;                                             \n\
                                                                        \n\
uniform sampler2D TextureObject0;                                       \n\
uniform vec4 color0;                                                    \n\
vec4 SampleTexture(sampler2D TexObject, vec4 TexCoord)                  \n\
{                                                                       \n\
  return texture2D(TexObject, TexCoord.st);                             \n\
}                                                                       \n\
                                                                        \n\
void main()                                                             \n\
{                                                                       \n\
  vec4 tex = varyTexCoord0;                                             \n\
  tex.s = tex.s/varyTexCoord0.w;                                        \n\
  tex.t = tex.t/varyTexCoord0.w;                                        \n\
	                                                                \n\
  vec4 texel = SampleTexture(TextureObject0, tex);                      \n\
  gl_FragColor = color0*texel;                                                 \n\
}                                                                       \n\
");

nux::NString PerspectiveCorrectVtx = TEXT (
                            "!!ARBvp1.0                                 \n\
                            ATTRIB iPos         = vertex.position;      \n\
                            ATTRIB iColor       = vertex.attrib[3];     \n\
                            PARAM  mvp[4]       = {state.matrix.mvp};   \n\
                            OUTPUT oPos         = result.position;      \n\
                            OUTPUT oColor       = result.color;         \n\
                            OUTPUT oTexCoord0   = result.texcoord[0];   \n\
                            # Transform the vertex to clip coordinates. \n\
                            DP4   oPos.x, mvp[0], iPos;                 \n\
                            DP4   oPos.y, mvp[1], iPos;                 \n\
                            DP4   oPos.z, mvp[2], iPos;                 \n\
                            DP4   oPos.w, mvp[3], iPos;                 \n\
                            MOV   oColor, iColor;                       \n\
                            MOV   oTexCoord0, vertex.attrib[8];         \n\
                            END");



nux::NString PerspectiveCorrectTexFrg = TEXT (
                            "!!ARBfp1.0                                 \n\
                            PARAM color0 = program.local[0];            \n\
                            TEMP temp;                                  \n\
                            TEMP pcoord;                                \n\
                            TEMP tex0;                                  \n\
                            TEMP temp1;                                 \n\
                            TEMP recip;                                 \n\
                            MOV pcoord, fragment.texcoord[0].w;         \n\
                            RCP temp, fragment.texcoord[0].w;           \n\
                            MUL pcoord.xy, fragment.texcoord[0], temp;  \n\
                            TEX tex0, pcoord, texture[0], 2D;           \n\
                            MUL result.color, color0, tex0;             \n\
                            END");

nux::NString PerspectiveCorrectTexRectFrg = TEXT (
                            "!!ARBfp1.0                                 \n\
                            PARAM color0 = program.local[0];            \n\
                            TEMP temp;                                  \n\
                            TEMP pcoord;                                \n\
                            TEMP tex0;                                  \n\
                            MOV pcoord, fragment.texcoord[0].w;         \n\
                            RCP temp, fragment.texcoord[0].w;           \n\
                            MUL pcoord.xy, fragment.texcoord[0], temp;  \n\
                            TEX tex0, pcoord, texture[0], RECT;         \n\
                            MUL result.color, color0, tex0;     \n\
                            END");

static void GetInverseScreenPerspectiveMatrix(nux::Matrix4& ViewMatrix, nux::Matrix4& PerspectiveMatrix,
                                       int ViewportWidth,
                                       int ViewportHeight,
                                       float NearClipPlane,
                                       float FarClipPlane,
                                       float Fovy);

Launcher::Launcher (nux::BaseWindow* parent,
                    CompScreen*      screen,
                    NUX_FILE_LINE_DECL)
:   View(NUX_FILE_LINE_PARAM)
,   m_ContentOffsetY(0)
,   m_BackgroundLayer(0)
,   _model (0)
{
    _parent = parent;
    _screen = screen;
    _active_quicklist = 0;

    m_Layout = new nux::HLayout(NUX_TRACKER_LOCATION);

    OnMouseDown.connect  (sigc::mem_fun (this, &Launcher::RecvMouseDown));
    OnMouseUp.connect    (sigc::mem_fun (this, &Launcher::RecvMouseUp));
    OnMouseDrag.connect  (sigc::mem_fun (this, &Launcher::RecvMouseDrag));
    OnMouseEnter.connect (sigc::mem_fun (this, &Launcher::RecvMouseEnter));
    OnMouseLeave.connect (sigc::mem_fun (this, &Launcher::RecvMouseLeave));
    OnMouseMove.connect  (sigc::mem_fun (this, &Launcher::RecvMouseMove));
    OnMouseWheel.connect (sigc::mem_fun (this, &Launcher::RecvMouseWheel));
    OnKeyPressed.connect (sigc::mem_fun (this, &Launcher::RecvKeyPressed));
    OnEndFocus.connect   (sigc::mem_fun (this, &Launcher::exitKeyNavMode));
    
    QuicklistManager::Default ()->quicklist_opened.connect (sigc::mem_fun(this, &Launcher::RecvQuicklistOpened));
    QuicklistManager::Default ()->quicklist_closed.connect (sigc::mem_fun(this, &Launcher::RecvQuicklistClosed));
    
    PluginAdapter::Default ()->window_maximized.connect   (sigc::mem_fun (this, &Launcher::OnWindowMaybeIntellihide));
    PluginAdapter::Default ()->window_restored.connect    (sigc::mem_fun (this, &Launcher::OnWindowMaybeIntellihide));
    PluginAdapter::Default ()->window_unminimized.connect (sigc::mem_fun (this, &Launcher::OnWindowMaybeIntellihide));
    PluginAdapter::Default ()->window_mapped.connect      (sigc::mem_fun (this, &Launcher::OnWindowMaybeIntellihide));
    PluginAdapter::Default ()->window_unmapped.connect    (sigc::mem_fun (this, &Launcher::OnWindowMaybeIntellihide));
    PluginAdapter::Default ()->window_shown.connect       (sigc::mem_fun (this, &Launcher::OnWindowMaybeIntellihide));
    PluginAdapter::Default ()->window_hidden.connect      (sigc::mem_fun (this, &Launcher::OnWindowMaybeIntellihide));
    PluginAdapter::Default ()->window_resized.connect     (sigc::mem_fun (this, &Launcher::OnWindowMaybeIntellihide));
    PluginAdapter::Default ()->window_moved.connect       (sigc::mem_fun (this, &Launcher::OnWindowMaybeIntellihide));
    
    PluginAdapter::Default ()->window_mapped.connect (sigc::mem_fun (this, &Launcher::OnWindowMapped));
    PluginAdapter::Default ()->window_unmapped.connect (sigc::mem_fun (this, &Launcher::OnWindowUnmapped));

    m_ActiveTooltipIcon = NULL;
    m_ActiveMenuIcon = NULL;
    m_LastSpreadIcon = NULL;

    _current_icon       = NULL;
    _last_selected_icon = NULL;
    _current_icon_index = -1;
    _last_icon_index    = -1;

    SetCompositionLayout(m_Layout);

    if(nux::GetGraphicsEngine ().UsingGLSLCodePath ())
    {
      _shader_program_uv_persp_correction = nux::GetThreadGLDeviceFactory()->CreateShaderProgram();
      _shader_program_uv_persp_correction->LoadIShader(gPerspectiveCorrectShader.GetTCharPtr());
      _shader_program_uv_persp_correction->Link();
    }
    else
    {
      _AsmShaderProg = nux::GetThreadGLDeviceFactory()->CreateAsmShaderProgram();
      _AsmShaderProg->LoadVertexShader (TCHAR_TO_ANSI (*PerspectiveCorrectVtx) );

      if ((nux::GetThreadGLDeviceFactory()->SUPPORT_GL_ARB_TEXTURE_NON_POWER_OF_TWO() == false) &&
        (nux::GetThreadGLDeviceFactory()->SUPPORT_GL_EXT_TEXTURE_RECTANGLE () || nux::GetThreadGLDeviceFactory()->SUPPORT_GL_ARB_TEXTURE_RECTANGLE ()))
      {
        // No support for non power of two textures but support for rectangle textures
        _AsmShaderProg->LoadPixelShader (TCHAR_TO_ANSI (*PerspectiveCorrectTexRectFrg) );
      }
      else
      {
        _AsmShaderProg->LoadPixelShader (TCHAR_TO_ANSI (*PerspectiveCorrectTexFrg) );
      }

      _AsmShaderProg->Link();
    }

    _folded_angle           = 1.0f;
    _neg_folded_angle       = -1.0f;
    _space_between_icons    = 5;
    _launcher_top_y         = 0;
    _launcher_bottom_y      = 0;
    _folded_z_distance      = 10.0f;
    _launcher_action_state  = ACTION_NONE;
    _launch_animation       = LAUNCH_ANIMATION_NONE;
    _urgent_animation       = URGENT_ANIMATION_NONE;
    _autohide_animation     = FADE_SLIDE;
    _hidemode               = LAUNCHER_HIDE_NEVER;
    _icon_under_mouse       = NULL;
    _icon_mouse_down        = NULL;
    _drag_icon              = NULL;
    _icon_image_size        = 48;
    _icon_glow_size         = 62;
    _icon_image_size_delta  = 6;
    _icon_size              = _icon_image_size + _icon_image_size_delta;

    _icon_bkg_texture       = nux::CreateTexture2DFromFile (PKGDATADIR"/round_corner_54x54.png", -1, true);
    _icon_outline_texture   = nux::CreateTexture2DFromFile (PKGDATADIR"/round_outline_54x54.png", -1, true);
    _icon_shine_texture     = nux::CreateTexture2DFromFile (PKGDATADIR"/round_shine_54x54.png", -1, true);
    _icon_glow_texture      = nux::CreateTexture2DFromFile (PKGDATADIR"/round_glow_62x62.png", -1, true);
    _icon_glow_hl_texture   = nux::CreateTexture2DFromFile (PKGDATADIR"/round_glow_hl_62x62.png", -1, true);
    _progress_bar_trough    = nux::CreateTexture2DFromFile (PKGDATADIR"/progress_bar_trough.png", -1, true);
    _progress_bar_fill      = nux::CreateTexture2DFromFile (PKGDATADIR"/progress_bar_fill.png", -1, true);
    
    _pip_ltr                = nux::CreateTexture2DFromFile (PKGDATADIR"/launcher_pip_ltr.png", -1, true);
    _arrow_ltr              = nux::CreateTexture2DFromFile (PKGDATADIR"/launcher_arrow_ltr.png", -1, true);
    _arrow_empty_ltr        = nux::CreateTexture2DFromFile (PKGDATADIR"/launcher_arrow_outline_ltr.png", -1, true);

    _pip_rtl                = nux::CreateTexture2DFromFile (PKGDATADIR"/launcher_pip_rtl.png", -1, true);
    _arrow_rtl              = nux::CreateTexture2DFromFile (PKGDATADIR"/launcher_arrow_rtl.png", -1, true);
    _arrow_empty_rtl        = nux::CreateTexture2DFromFile (PKGDATADIR"/launcher_arrow_outline_rtl.png", -1, true);

    for (int i = 0; i < MAX_SUPERKEY_LABELS; i++)
      _superkey_labels[i] = cairoToTexture2D ((char) ('0' + ((i  + 1) % 10)), _icon_size, _icon_size);

    _enter_y                = 0;
    _dnd_security           = 15;
    _launcher_drag_delta    = 0;
    _dnd_delta_y            = 0;
    _dnd_delta_x            = 0;
    _autohide_handle        = 0;
    _autoscroll_handle      = 0;
    _redraw_handle          = 0;
    _focus_keynav_handle    = 0;
    _floating               = false;
    _hovered                = false;
    _hidden                 = false;
    _mouse_inside_launcher  = false;
    _mouse_inside_trigger   = false;
    _mouseover_launcher_locked = false;
    _super_show_launcher    = false;
    _navmod_show_launcher   = false;
    _placeview_show_launcher = false;
    _window_over_launcher   = false;
    _hide_on_drag_hover     = false;
    _render_drag_window     = false;
    _dnd_window_is_mapped   = false;
    _drag_edge_touching     = false;
    _backlight_mode         = BACKLIGHT_NORMAL;
    _last_button_press      = 0;
    _selection_atom         = 0;
    
    // set them to 1 instead of 0 to avoid :0 in case something is racy
    _trigger_width = 1;
    _trigger_height = 1;

    // 0 out timers to avoid wonky startups
    int i;
    for (i = 0; i < TIME_LAST; i++)
    {
      _times[i].tv_sec = 0;
      _times[i].tv_nsec = 0;
    }
    
    _drag_window = NULL;
    _offscreen_drag_texture = nux::GetThreadGLDeviceFactory()->CreateSystemCapableDeviceTexture (2, 2, 1, nux::BITFMT_R8G8B8A8);
    _offscreen_progress_texture = nux::GetThreadGLDeviceFactory()->CreateSystemCapableDeviceTexture (2, 2, 1, nux::BITFMT_R8G8B8A8);
    
    UBusServer *ubus = ubus_server_get_default ();
    ubus_server_register_interest (ubus, UBUS_PLACE_VIEW_SHOWN,
                                   (UBusCallback)&Launcher::OnPlaceViewShown,
                                   this);
    ubus_server_register_interest (ubus, UBUS_PLACE_VIEW_HIDDEN,
                                   (UBusCallback)&Launcher::OnPlaceViewHidden,
                                   this);

    ubus_server_register_interest (ubus, UBUS_HOME_BUTTON_TRIGGER_UPDATE,
                                   (UBusCallback)&Launcher::OnTriggerUpdate,
                                   this);
                                   
    ubus_server_register_interest (ubus, UBUS_LAUNCHER_ACTION_DONE,
                                   (UBusCallback)&Launcher::OnActionDone,
                                   this);

    SetDndEnabled (false, true);
}

Launcher::~Launcher()
{
  for (int i = 0; i < MAX_SUPERKEY_LABELS; i++)
  {
    if (_superkey_labels[i])
      _superkey_labels[i]->UnReference ();
  }
}

/* Introspection */
const gchar *
Launcher::GetName ()
{
  return "Launcher";
}

void
Launcher::DrawRoundedRectangle (cairo_t* cr,
                                double   aspect,
                                double   x,
                                double   y,
                                double   cornerRadius,
                                double   width,
                                double   height)
{
  double radius = cornerRadius / aspect;
  
  // top-left, right of the corner
  cairo_move_to (cr, x + radius, y);
  
  // top-right, left of the corner
  cairo_line_to (cr, x + width - radius, y);
  
  // top-right, below the corner
  cairo_arc (cr,
             x + width - radius,
             y + radius,
             radius,
             -90.0f * G_PI / 180.0f,
             0.0f * G_PI / 180.0f);
  
  // bottom-right, above the corner
  cairo_line_to (cr, x + width, y + height - radius);

  // bottom-right, left of the corner
  cairo_arc (cr,
             x + width - radius,
             y + height - radius,
             radius,
             0.0f * G_PI / 180.0f,
             90.0f * G_PI / 180.0f);
  
  // bottom-left, right of the corner
  cairo_line_to (cr, x + radius, y + height);
  
  // bottom-left, above the corner
  cairo_arc (cr,
             x + radius,
             y + height - radius,
             radius,
             90.0f * G_PI / 180.0f,
             180.0f * G_PI / 180.0f);
  
  // top-left, right of the corner
  cairo_arc (cr,
             x + radius,
             y + radius,
             radius,
             180.0f * G_PI / 180.0f,
             270.0f * G_PI / 180.0f);
}

nux::BaseTexture*
Launcher::cairoToTexture2D (const char label, int width, int height)
{
  nux::BaseTexture*     texture  = NULL;
  nux::CairoGraphics*   cg       = new nux::CairoGraphics (CAIRO_FORMAT_ARGB32,
                                                           width,
                                                           height);
  cairo_t*              cr       = cg->GetContext ();
  PangoLayout*          layout   = NULL;
  PangoContext*         pangoCtx = NULL;
  PangoFontDescription* desc     = NULL;
  GtkSettings*          settings = gtk_settings_get_default (); // not ref'ed
  gchar*                fontName = NULL;
  double                label_x  = 18.0f;
  double                label_y  = 18.0f;
  double                label_w  = 18.0f;
  double                label_h  = 18.0f;
  double                label_r  = 3.0f;

  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);
  cairo_scale (cr, 1.0f, 1.0f);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  DrawRoundedRectangle (cr, 1.0f, label_x, label_y, label_r, label_w, label_h);
  cairo_set_source_rgba (cr, 0.0f, 0.0f, 0.0f, 0.65f);
  cairo_fill (cr);

  layout = pango_cairo_create_layout (cr);
  g_object_get (settings, "gtk-font-name", &fontName, NULL);
  desc = pango_font_description_from_string (fontName);
  pango_font_description_set_size (desc, 11 * PANGO_SCALE);
  pango_layout_set_font_description (layout, desc);
  pango_layout_set_text (layout, &label, 1);
  pangoCtx = pango_layout_get_context (layout); // is not ref'ed

  PangoRectangle logRect;
  PangoRectangle inkRect;
  pango_layout_get_extents (layout, &inkRect, &logRect);

  /* position and paint text */
  cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 1.0f);
  double x = label_x - ((logRect.width / PANGO_SCALE) - label_w) / 2.0f;
  double y = label_y - ((logRect.height / PANGO_SCALE) - label_h) / 2.0f - 1;
  cairo_move_to (cr, x, y);
  pango_cairo_show_layout (cr, layout);

  nux::NBitmapData* bitmap = cg->GetBitmap ();
  texture = nux::GetThreadGLDeviceFactory()->CreateSystemCapableTexture ();
  texture->Update (bitmap);
  delete bitmap;
  delete cg;
  g_object_unref (layout);
  pango_font_description_free (desc);
  g_free (fontName);

  return texture;
}

void
Launcher::startKeyNavMode ()
{
  _navmod_show_launcher = true;
  EnsureHiddenState ();
  
  // FIXME: long term solution is to rewrite the keynav handle
  if (_focus_keynav_handle > 0)
    g_source_remove (_focus_keynav_handle);
  _focus_keynav_handle = g_timeout_add (ANIM_DURATION_SHORT, &Launcher::MoveFocusToKeyNavModeTimeout, this);

}

gboolean
Launcher::MoveFocusToKeyNavModeTimeout (gpointer data)
{
  Launcher *self = (Launcher*) data;
      
  // move focus to key nav mode when activated
  if (!(self->_navmod_show_launcher))
    return false;
  
  if (self->_last_icon_index == -1)
     self->_current_icon_index = 0;
   else
     self->_current_icon_index = self->_last_icon_index;
   self->NeedRedraw ();

   ubus_server_send_message (ubus_server_get_default (),
                             UBUS_LAUNCHER_START_KEY_NAV,
                             NULL);

   self->selection_change.emit ();

   return false;
}

void
Launcher::leaveKeyNavMode (bool preserve_focus)
{
  _last_icon_index = _current_icon_index;
  _current_icon_index = -1;
  QueueDraw ();

  ubus_server_send_message (ubus_server_get_default (),
                            UBUS_LAUNCHER_END_KEY_NAV,
                            g_variant_new_boolean  (preserve_focus));

  selection_change.emit ();
}

void
Launcher::exitKeyNavMode ()
{
  if (!_navmod_show_launcher)
    return;
    
  _navmod_show_launcher = false;
  EnsureHiddenState ();

  _last_icon_index = _current_icon_index;
  _current_icon_index = -1;
  QueueDraw ();
  ubus_server_send_message (ubus_server_get_default (),
                            UBUS_LAUNCHER_END_KEY_NAV,
                            NULL);
  selection_change.emit ();
}

void
Launcher::AddProperties (GVariantBuilder *builder)
{
  struct timespec current;
  clock_gettime (CLOCK_MONOTONIC, &current);

  g_variant_builder_add (builder, "{sv}", "hover-progress", g_variant_new_double ((double) GetHoverProgress (current)));
  g_variant_builder_add (builder, "{sv}", "dnd-exit-progress", g_variant_new_double ((double) DnDExitProgress (current)));
  g_variant_builder_add (builder, "{sv}", "autohide-progress", g_variant_new_double ((double) AutohideProgress (current)));

  g_variant_builder_add (builder, "{sv}", "dnd-delta", g_variant_new_int32 (_dnd_delta_y));
  g_variant_builder_add (builder, "{sv}", "floating", g_variant_new_boolean (_floating));
  g_variant_builder_add (builder, "{sv}", "hovered", g_variant_new_boolean (_hovered));
  g_variant_builder_add (builder, "{sv}", "hidemode", g_variant_new_int32 (_hidemode));
  g_variant_builder_add (builder, "{sv}", "hidden", g_variant_new_boolean (_hidden));
  g_variant_builder_add (builder, "{sv}", "mouse-inside-launcher", g_variant_new_boolean (_mouse_inside_launcher));
}

void Launcher::SetMousePosition (int x, int y)
{
    bool beyond_drag_threshold = MouseBeyondDragThreshold ();
    _mouse_position = nux::Point2 (x, y);
    
    if (beyond_drag_threshold != MouseBeyondDragThreshold ())
      SetTimeStruct (&_times[TIME_DRAG_THRESHOLD], &_times[TIME_DRAG_THRESHOLD], ANIM_DURATION_SHORT);
    
    EnsureScrollTimer ();
}

bool Launcher::MouseBeyondDragThreshold ()
{
    if (GetActionState () == ACTION_DRAG_ICON)
      return _mouse_position.x > GetGeometry ().width + _icon_size / 2;
    return false;
}

/* Render Layout Logic */
float Launcher::GetHoverProgress (struct timespec const &current)
{
    if (_hovered)
        return CLAMP ((float) (TimeDelta (&current, &_times[TIME_ENTER])) / (float) ANIM_DURATION, 0.0f, 1.0f);
    else
        return 1.0f - CLAMP ((float) (TimeDelta (&current, &_times[TIME_LEAVE])) / (float) ANIM_DURATION, 0.0f, 1.0f);
}

float Launcher::DnDExitProgress (struct timespec const &current)
{
    return pow (1.0f - CLAMP ((float) (TimeDelta (&current, &_times[TIME_DRAG_END])) / (float) ANIM_DURATION_LONG, 0.0f, 1.0f), 2);
}

float Launcher::DnDStartProgress (struct timespec const &current)
{
  return CLAMP ((float) (TimeDelta (&current, &_times[TIME_DRAG_START])) / (float) ANIM_DURATION, 0.0f, 1.0f);
}

float Launcher::AutohideProgress (struct timespec const &current)
{
    
    // bfb position progress. Go from GetAutohidePositionMin() -> GetAutohidePositionMax() linearly
    if (_mouse_inside_trigger && !_mouseover_launcher_locked)
    {
        // all this code should only be triggered when _hidden is true
        if (!_hidden)
          return 0.0f;
          
        // "dead" zone
        if ((_trigger_mouse_position.x < 2) && (_trigger_mouse_position.y < 2))
            return GetAutohidePositionMin ();
        
       /* 
        * most of the mouse movement should be done by the inferior part
        * of the launcher, so prioritize this one
        */
                
        float _max_size_on_position;
        float position_on_border = _trigger_mouse_position.x * _trigger_height / _trigger_mouse_position.y;
        
        if (position_on_border < _trigger_width)
            _max_size_on_position = pow(pow(position_on_border, 2) + pow(_trigger_height, 2), 0.5);
        else
        {
            position_on_border = _trigger_mouse_position.y * _trigger_width / _trigger_mouse_position.x;
            _max_size_on_position = pow(pow(position_on_border, 2) + pow(_trigger_width, 2), 0.5);
        }
        
        float _position_min = GetAutohidePositionMin ();
        return pow(pow(_trigger_mouse_position.x, 2) + pow(_trigger_mouse_position.y, 2), 0.5) / _max_size_on_position * (GetAutohidePositionMax () - _position_min) + _position_min;
    }
    
    // time-based progress (full scale or finish the TRIGGER_AUTOHIDE_MIN -> 0.00f on bfb)
    else
    {
        float animation_progress;
        animation_progress = CLAMP ((float) (TimeDelta (&current, &_times[TIME_AUTOHIDE])) / (float) ANIM_DURATION_SHORT, 0.0f, 1.0f);
        if (_hidden)
            return animation_progress;
        else
            return 1.0f - animation_progress;  
    }
    
}

float Launcher::DragHideProgress (struct timespec const &current)
{
    if (_drag_edge_touching)
        return CLAMP ((float) (TimeDelta (&current, &_times[TIME_DRAG_EDGE_TOUCH])) / (float) (ANIM_DURATION * 3), 0.0f, 1.0f);
    else
        return 1.0f - CLAMP ((float) (TimeDelta (&current, &_times[TIME_DRAG_EDGE_TOUCH])) / (float) (ANIM_DURATION * 3), 0.0f, 1.0f);
}

float Launcher::DragThresholdProgress (struct timespec const &current)
{
  if (MouseBeyondDragThreshold ())
    return 1.0f - CLAMP ((float) (TimeDelta (&current, &_times[TIME_DRAG_THRESHOLD])) / (float) ANIM_DURATION_SHORT, 0.0f, 1.0f);
  else
    return CLAMP ((float) (TimeDelta (&current, &_times[TIME_DRAG_THRESHOLD])) / (float) ANIM_DURATION_SHORT, 0.0f, 1.0f);
}

gboolean Launcher::AnimationTimeout (gpointer data)
{
    Launcher *self = (Launcher*) data;
    self->NeedRedraw ();
    return false;
}

void Launcher::EnsureAnimation ()
{
    NeedRedraw ();
}

bool Launcher::IconNeedsAnimation (LauncherIcon *icon, struct timespec const &current)
{
    struct timespec time = icon->GetQuirkTime (LauncherIcon::QUIRK_VISIBLE);
    if (TimeDelta (&current, &time) < ANIM_DURATION_SHORT)
        return true;

    time = icon->GetQuirkTime (LauncherIcon::QUIRK_RUNNING);
    if (TimeDelta (&current, &time) < ANIM_DURATION_SHORT)
        return true;

    time = icon->GetQuirkTime (LauncherIcon::QUIRK_STARTING);
    if (TimeDelta (&current, &time) < (ANIM_DURATION_LONG * MAX_STARTING_BLINKS * STARTING_BLINK_LAMBDA * 2))
        return true;

    time = icon->GetQuirkTime (LauncherIcon::QUIRK_URGENT);
    if (TimeDelta (&current, &time) < (ANIM_DURATION_LONG * URGENT_BLINKS * 2))
        return true;

    time = icon->GetQuirkTime (LauncherIcon::QUIRK_PRESENTED);
    if (TimeDelta (&current, &time) < ANIM_DURATION)
        return true;

    time = icon->GetQuirkTime (LauncherIcon::QUIRK_SHIMMER);
    if (TimeDelta (&current, &time) < ANIM_DURATION_LONG)
        return true;

    time = icon->GetQuirkTime (LauncherIcon::QUIRK_CENTER_SAVED);
    if (TimeDelta (&current, &time) < ANIM_DURATION)
        return true;

    time = icon->GetQuirkTime (LauncherIcon::QUIRK_PROGRESS);
    if (TimeDelta (&current, &time) < ANIM_DURATION)
        return true;
    
    time = icon->GetQuirkTime (LauncherIcon::QUIRK_DROP_DIM);
    if (TimeDelta (&current, &time) < ANIM_DURATION)
        return true;
    
    time = icon->GetQuirkTime (LauncherIcon::QUIRK_DROP_PRELIGHT);
    if (TimeDelta (&current, &time) < ANIM_DURATION)
        return true;
    
    return false;
}

bool Launcher::AnimationInProgress ()
{
    // performance here can be improved by caching the longer remaining animation found and short circuiting to that each time
    // this way extra checks may be avoided

    // short circuit to avoid unneeded calculations
    struct timespec current;
    clock_gettime (CLOCK_MONOTONIC, &current);

    // hover in animation
    if (TimeDelta (&current, &_times[TIME_ENTER]) < ANIM_DURATION)
       return true;

    // hover out animation
    if (TimeDelta (&current, &_times[TIME_LEAVE]) < ANIM_DURATION)
        return true;
  
    // drag start animation
    if (TimeDelta (&current, &_times[TIME_DRAG_START]) < ANIM_DURATION)
        return true;
    
    // drag end animation
    if (TimeDelta (&current, &_times[TIME_DRAG_END]) < ANIM_DURATION_LONG)
        return true;

    // hide animation (time or position)
    if (TimeDelta (&current, &_times[TIME_AUTOHIDE]) < ANIM_DURATION_SHORT)
        return true;
    if (_mouse_inside_trigger && !_mouseover_launcher_locked)
        return true;
    
    // collapse animation on DND out of launcher space
    if (TimeDelta (&current, &_times[TIME_DRAG_THRESHOLD]) < ANIM_DURATION_SHORT)
        return true;
    
    // hide animation for dnd
    if (TimeDelta (&current, &_times[TIME_DRAG_EDGE_TOUCH]) < ANIM_DURATION * 6)
        return true;

    // animations happening on specific icons
    LauncherModel::iterator it;
    for (it = _model->begin  (); it != _model->end (); it++)
        if (IconNeedsAnimation (*it, current))
            return true;

    return false;
}

void Launcher::SetTimeStruct (struct timespec *timer, struct timespec *sister, int sister_relation)
{
    struct timespec current;
    clock_gettime (CLOCK_MONOTONIC, &current);

    if (sister)
    {
        int diff = TimeDelta (&current, sister);

        if (diff < sister_relation)
        {
            int remove = sister_relation - diff;
            SetTimeBack (&current, remove);
        }
    }

    timer->tv_sec = current.tv_sec;
    timer->tv_nsec = current.tv_nsec;
}
/* Min is when you lock the trigger */
float Launcher::GetAutohidePositionMin ()
{
    if (_autohide_animation == SLIDE_ONLY)
        return 0.55f;
    else
        return 0.25f;
}
/* Max is the initial state */
float Launcher::GetAutohidePositionMax ()
{
    if (_autohide_animation == SLIDE_ONLY)
        return 1.00f;
    else
        return 0.75f;
}


float IconVisibleProgress (LauncherIcon *icon, struct timespec const &current)
{
    if (icon->GetQuirk (LauncherIcon::QUIRK_VISIBLE))
    {
        struct timespec icon_visible_time = icon->GetQuirkTime (LauncherIcon::QUIRK_VISIBLE);
        int enter_ms = TimeDelta (&current, &icon_visible_time);
        return CLAMP ((float) enter_ms / (float) ANIM_DURATION_SHORT, 0.0f, 1.0f);
    }
    else
    {
        struct timespec icon_hide_time = icon->GetQuirkTime (LauncherIcon::QUIRK_VISIBLE);
        int hide_ms = TimeDelta (&current, &icon_hide_time);
        return 1.0f - CLAMP ((float) hide_ms / (float) ANIM_DURATION_SHORT, 0.0f, 1.0f);
    }
}

void Launcher::SetDndDelta (float x, float y, nux::Geometry geo, struct timespec const &current)
{
    LauncherIcon *anchor = 0;
    LauncherModel::iterator it;
    anchor = MouseIconIntersection (x, _enter_y);

    if (anchor)
    {
        float position = y;
        for (it = _model->begin (); it != _model->end (); it++)
        {
            if (*it == anchor)
            {
                position += _icon_size / 2;
                _launcher_drag_delta = _enter_y - position;

                if (position + _icon_size / 2 + _launcher_drag_delta > geo.height)
                    _launcher_drag_delta -= (position + _icon_size / 2 + _launcher_drag_delta) - geo.height;

                break;
            }
            position += (_icon_size + _space_between_icons) * IconVisibleProgress (*it, current);
        }
    }
}

float Launcher::IconPresentProgress (LauncherIcon *icon, struct timespec const &current)
{
    struct timespec icon_present_time = icon->GetQuirkTime (LauncherIcon::QUIRK_PRESENTED);
    int ms = TimeDelta (&current, &icon_present_time);
    float result = CLAMP ((float) ms / (float) ANIM_DURATION, 0.0f, 1.0f);

    if (icon->GetQuirk (LauncherIcon::QUIRK_PRESENTED))
        return result;
    else
        return 1.0f - result;
}

float Launcher::IconUrgentProgress (LauncherIcon *icon, struct timespec const &current)
{
    struct timespec urgent_time = icon->GetQuirkTime (LauncherIcon::QUIRK_URGENT);
    int urgent_ms = TimeDelta (&current, &urgent_time);
    float result;
    
    if (_urgent_animation == URGENT_ANIMATION_WIGGLE)
      result = CLAMP ((float) urgent_ms / (float) (ANIM_DURATION_SHORT * WIGGLE_CYCLES), 0.0f, 1.0f);
    else
      result = CLAMP ((float) urgent_ms / (float) (ANIM_DURATION_LONG * URGENT_BLINKS * 2), 0.0f, 1.0f);

    if (icon->GetQuirk (LauncherIcon::QUIRK_URGENT))
      return result;
    else
      return 1.0f - result;
}

float Launcher::IconDropDimValue (LauncherIcon *icon, struct timespec const &current)
{
    struct timespec dim_time = icon->GetQuirkTime (LauncherIcon::QUIRK_DROP_DIM);
    int dim_ms = TimeDelta (&current, &dim_time);
    float result = CLAMP ((float) dim_ms / (float) ANIM_DURATION, 0.0f, 1.0f);

    if (icon->GetQuirk (LauncherIcon::QUIRK_DROP_DIM))
      return 1.0f - result;
    else
      return result;
}

float Launcher::IconShimmerProgress (LauncherIcon *icon, struct timespec const &current)
{
    struct timespec shimmer_time = icon->GetQuirkTime (LauncherIcon::QUIRK_SHIMMER);
    int shimmer_ms = TimeDelta (&current, &shimmer_time);
    return CLAMP ((float) shimmer_ms / (float) ANIM_DURATION_LONG, 0.0f, 1.0f);
}

float Launcher::IconCenterTransitionProgress (LauncherIcon *icon, struct timespec const &current)
{
    struct timespec save_time = icon->GetQuirkTime (LauncherIcon::QUIRK_CENTER_SAVED);
    int save_ms = TimeDelta (&current, &save_time);
    return CLAMP ((float) save_ms / (float) ANIM_DURATION, 0.0f, 1.0f);
}

float Launcher::IconUrgentPulseValue (LauncherIcon *icon, struct timespec const &current)
{
    if (!icon->GetQuirk (LauncherIcon::QUIRK_URGENT))
        return 1.0f; // we are full on in a normal condition

    double urgent_progress = (double) IconUrgentProgress (icon, current);
    return 0.5f + (float) (std::cos (M_PI * (float) (URGENT_BLINKS * 2) * urgent_progress)) * 0.5f;
}

float Launcher::IconUrgentWiggleValue (LauncherIcon *icon, struct timespec const &current)
{
    if (!icon->GetQuirk (LauncherIcon::QUIRK_URGENT))
        return 0.0f; // we are full on in a normal condition

    double urgent_progress = (double) IconUrgentProgress (icon, current);
    return 0.3f * (float) (std::sin (M_PI * (float) (WIGGLE_CYCLES * 2) * urgent_progress)) * 0.5f;
}

float Launcher::IconStartingBlinkValue (LauncherIcon *icon, struct timespec const &current)
{
    struct timespec starting_time = icon->GetQuirkTime (LauncherIcon::QUIRK_STARTING);
    int starting_ms = TimeDelta (&current, &starting_time);
    double starting_progress = (double) CLAMP ((float) starting_ms / (float) (ANIM_DURATION_LONG * STARTING_BLINK_LAMBDA), 0.0f, 1.0f);
    return 0.5f + (float) (std::cos (M_PI * (_backlight_mode != BACKLIGHT_NORMAL ? 4.0f : 3.0f) * starting_progress)) * 0.5f;
}

float Launcher::IconStartingPulseValue (LauncherIcon *icon, struct timespec const &current)
{
    struct timespec starting_time = icon->GetQuirkTime (LauncherIcon::QUIRK_STARTING);
    int starting_ms = TimeDelta (&current, &starting_time);
    double starting_progress = (double) CLAMP ((float) starting_ms / (float) (ANIM_DURATION_LONG * MAX_STARTING_BLINKS * STARTING_BLINK_LAMBDA * 2), 0.0f, 1.0f);

    if (starting_progress == 1.0f && !icon->GetQuirk (LauncherIcon::QUIRK_RUNNING))
    {
        icon->SetQuirk (LauncherIcon::QUIRK_STARTING, false);
        icon->ResetQuirkTime (LauncherIcon::QUIRK_STARTING);
    }

    return 0.5f + (float) (std::cos (M_PI * (float) (MAX_STARTING_BLINKS * 2) * starting_progress)) * 0.5f;
}

float Launcher::IconBackgroundIntensity (LauncherIcon *icon, struct timespec const &current)
{
    float result = 0.0f;

    struct timespec running_time = icon->GetQuirkTime (LauncherIcon::QUIRK_RUNNING);
    int running_ms = TimeDelta (&current, &running_time);
    float running_progress = CLAMP ((float) running_ms / (float) ANIM_DURATION_SHORT, 0.0f, 1.0f);
    
    if (!icon->GetQuirk (LauncherIcon::QUIRK_RUNNING))
      running_progress = 1.0f - running_progress;

    // After we finish a fade in from running, we can reset the quirk
    if (running_progress == 1.0f && icon->GetQuirk (LauncherIcon::QUIRK_RUNNING))
       icon->SetQuirk (LauncherIcon::QUIRK_STARTING, false);
    
    float backlight_strength;
    if (_backlight_mode == BACKLIGHT_ALWAYS_ON)
      backlight_strength = BACKLIGHT_STRENGTH;
    else if (_backlight_mode == BACKLIGHT_NORMAL)
      backlight_strength = BACKLIGHT_STRENGTH * running_progress;
    else
      backlight_strength = 0.0f;
      
    switch (_launch_animation)
    {
      case LAUNCH_ANIMATION_NONE:
        result = backlight_strength;
        break;
      case LAUNCH_ANIMATION_BLINK:
        if (_backlight_mode == BACKLIGHT_ALWAYS_ON)
          result = IconStartingBlinkValue (icon, current);
        else if (_backlight_mode == BACKLIGHT_ALWAYS_OFF)
          result = 1.0f - IconStartingBlinkValue (icon, current);
        else
          result = backlight_strength; // The blink concept is a failure in this case (it just doesn't work right)
        break;
      case LAUNCH_ANIMATION_PULSE:
        if (running_progress == 1.0f && icon->GetQuirk (LauncherIcon::QUIRK_RUNNING))
          icon->ResetQuirkTime (LauncherIcon::QUIRK_STARTING);
        
        result = backlight_strength;
        if (_backlight_mode == BACKLIGHT_ALWAYS_ON)
          result *= CLAMP (running_progress + IconStartingPulseValue (icon, current), 0.0f, 1.0f);
        else if (_backlight_mode == BACKLIGHT_NORMAL)
          result += (BACKLIGHT_STRENGTH - result) * (1.0f - IconStartingPulseValue (icon, current));
        else
          result = 1.0f - CLAMP (running_progress + IconStartingPulseValue (icon, current), 0.0f, 1.0f);
        break;
    }
    
      // urgent serves to bring the total down only
    if (icon->GetQuirk (LauncherIcon::QUIRK_URGENT) && _urgent_animation == URGENT_ANIMATION_PULSE)
      result *= 0.2f + 0.8f * IconUrgentPulseValue (icon, current);
    
    return result;
}

float Launcher::IconProgressBias (LauncherIcon *icon, struct timespec const &current)
{
    struct timespec icon_progress_time = icon->GetQuirkTime (LauncherIcon::QUIRK_PROGRESS);
    int ms = TimeDelta (&current, &icon_progress_time);
    float result = CLAMP ((float) ms / (float) ANIM_DURATION, 0.0f, 1.0f);

    if (icon->GetQuirk (LauncherIcon::QUIRK_PROGRESS))
        return -1.0f + result;
    else
        return result;
}

void Launcher::SetupRenderArg (LauncherIcon *icon, struct timespec const &current, RenderArg &arg)
{
    arg.icon                = icon;
    arg.alpha               = 1.0f;
    arg.running_arrow       = icon->GetQuirk (LauncherIcon::QUIRK_RUNNING);
    arg.active_arrow        = icon->GetQuirk (LauncherIcon::QUIRK_ACTIVE);
    arg.running_colored     = icon->GetQuirk (LauncherIcon::QUIRK_URGENT);
    arg.running_on_viewport = icon->HasVisibleWindow ();
    arg.active_colored      = false;
    arg.x_rotation          = 0.0f;
    arg.y_rotation          = 0.0f;
    arg.z_rotation          = 0.0f;
    arg.skip                = false;
    arg.stick_thingy        = false;
    arg.keyboard_nav_hl     = false;
    arg.progress_bias       = IconProgressBias (icon, current);
    arg.progress            = CLAMP (icon->GetProgress (), 0.0f, 1.0f);

    // we dont need to show strays
    if (!icon->GetQuirk (LauncherIcon::QUIRK_RUNNING))
        arg.window_indicators = 0;
    else
        arg.window_indicators = icon->RelatedWindows ();

    arg.backlight_intensity = IconBackgroundIntensity (icon, current);
    arg.shimmer_progress = IconShimmerProgress (icon, current);

    float urgent_progress = IconUrgentProgress (icon, current);
    
    if (icon->GetQuirk (LauncherIcon::QUIRK_URGENT))
      urgent_progress = CLAMP (urgent_progress * 3.0f, 0.0f, 1.0f); // we want to go 3x faster than the urgent normal cycle
    else
      urgent_progress = CLAMP (urgent_progress * 3.0f - 2.0f, 0.0f, 1.0f); // we want to go 3x faster than the urgent normal cycle
    arg.glow_intensity = urgent_progress;
    
    if (icon->GetQuirk (LauncherIcon::QUIRK_URGENT) && _urgent_animation == URGENT_ANIMATION_WIGGLE)
    {
      arg.z_rotation = IconUrgentWiggleValue (icon, current);
    }

    // we've to walk the list since it is a STL-list and not a STL-vector, thus
    // we can't use the random-access operator [] :(
    LauncherModel::iterator it;
    int i;
    for (it = _model->begin (), i = 0; it != _model->end (); it++, i++)
      if (i == _current_icon_index && *it == icon)
        arg.keyboard_nav_hl = true;
}

void Launcher::FillRenderArg (LauncherIcon *icon,
                              RenderArg &arg,
                              nux::Point3 &center,
                              float folding_threshold,
                              float folded_size,
                              float folded_spacing,
                              float autohide_offset,
                              float folded_z_distance,
                              float animation_neg_rads,
                              struct timespec const &current)
{
    SetupRenderArg (icon, current, arg);

    // reset z
    center.z = 0;
    
    float size_modifier = IconVisibleProgress (icon, current);
    if (size_modifier < 1.0f)
    {
        arg.alpha *= size_modifier;
        center.z = 300.0f * (1.0f - size_modifier);
    }
    
    float drop_dim_value = 0.2f + 0.8f * IconDropDimValue (icon, current);
    
    if (drop_dim_value < 1.0f)
      arg.alpha *= drop_dim_value;

    if (icon == _drag_icon)
    {
        if (MouseBeyondDragThreshold ())
          arg.stick_thingy = true;
        
        if (GetActionState () == ACTION_DRAG_ICON || 
            (_drag_window && _drag_window->Animating ()) ||
            dynamic_cast<SpacerLauncherIcon *> (icon))
          arg.skip = true;
        size_modifier *= DragThresholdProgress (current);
    }
    
    if (size_modifier <= 0.0f)
        arg.skip = true;
    
    // goes for 0.0f when fully unfolded, to 1.0f folded
    float folding_progress = CLAMP ((center.y + _icon_size - folding_threshold) / (float) _icon_size, 0.0f, 1.0f);
    float present_progress = IconPresentProgress (icon, current);

    folding_progress *= 1.0f - present_progress;

    float half_size = (folded_size / 2.0f) + (_icon_size / 2.0f - folded_size / 2.0f) * (1.0f - folding_progress);
    float icon_hide_offset = autohide_offset;

    icon_hide_offset *= 1.0f - (present_progress * icon->PresentUrgency ());

    // icon is crossing threshold, start folding
    center.z += folded_z_distance * folding_progress;
    arg.x_rotation = animation_neg_rads * folding_progress;

    float spacing_overlap = CLAMP ((float) (center.y + (2.0f * half_size * size_modifier) + (_space_between_icons * size_modifier) - folding_threshold) / (float) _icon_size, 0.0f, 1.0f);
    float spacing = (_space_between_icons * (1.0f - spacing_overlap) + folded_spacing * spacing_overlap) * size_modifier;

    nux::Point3 centerOffset;
    float center_transit_progress = IconCenterTransitionProgress (icon, current);
    if (center_transit_progress <= 1.0f)
    {
      centerOffset.y = (icon->_saved_center.y - (center.y + (half_size * size_modifier))) * (1.0f - center_transit_progress);
    }
    
    center.y += half_size * size_modifier;   // move to center
    
    arg.render_center = nux::Point3 (roundf (center.x + icon_hide_offset), roundf (center.y + centerOffset.y), roundf (center.z));
    arg.logical_center = nux::Point3 (roundf (center.x + icon_hide_offset), roundf (center.y), roundf (center.z));
    
    icon->SetCenter (nux::Point3 (roundf (center.x), roundf (center.y), roundf (center.z)));
    
    if (icon == _drag_icon && _drag_window && _drag_window->Animating ())
      _drag_window->SetAnimationTarget ((int) center.x, (int) center.y + _parent->GetGeometry ().y);
    
    center.y += (half_size * size_modifier) + spacing;   // move to end
}

float Launcher::DragLimiter (float x)
{
  float result = (1 - std::pow (159.0 / 160,  std::abs (x))) * 160;

  if (x >= 0.0f)
    return result;
  return -result;
}

void Launcher::RenderArgs (std::list<Launcher::RenderArg> &launcher_args,
                           nux::Geometry &box_geo, float *launcher_alpha)
{
    nux::Geometry geo = GetGeometry ();
    LauncherModel::iterator it;
    nux::Point3 center;
    struct timespec current;
    clock_gettime (CLOCK_MONOTONIC, &current);

    float hover_progress = GetHoverProgress (current);
    float folded_z_distance = _folded_z_distance * (1.0f - hover_progress);
    float animation_neg_rads = _neg_folded_angle * (1.0f - hover_progress);

    float folding_constant = 0.25f;
    float folding_not_constant = folding_constant + ((1.0f - folding_constant) * hover_progress);

    float folded_size = _icon_size * folding_not_constant;
    float folded_spacing = _space_between_icons * folding_not_constant;

    center.x = geo.width / 2;
    center.y = _space_between_icons;
    center.z = 0;

    int launcher_height = geo.height;

    // compute required height of launcher AND folding threshold
    float sum = 0.0f + center.y;
    float folding_threshold = launcher_height - _icon_size / 2.5f;
    for (it = _model->begin (); it != _model->end (); it++)
    {
        float height = (_icon_size + _space_between_icons) * IconVisibleProgress (*it, current);
        sum += height;

        // magic constant must some day be explained, for now suffice to say this constant prevents the bottom from "marching";
        float magic_constant = 1.2f;

        float present_progress = IconPresentProgress (*it, current);
        folding_threshold -= CLAMP (sum - launcher_height, 0.0f, height * magic_constant) * (folding_constant + (1.0f - folding_constant) * present_progress);
    }

    // this happens on hover, basically its a flag and a value in one, we translate this into a dnd offset
    if (_enter_y != 0 && _enter_y + _icon_size / 2 > folding_threshold)
        SetDndDelta (center.x, center.y, nux::Geometry (geo.x, geo.y, geo.width, geo.height), current);

    _enter_y = 0;

    if (hover_progress > 0.0f && _launcher_drag_delta != 0)
    {
        float delta_y = _launcher_drag_delta;

        // logically dnd exit only restores to the clamped ranges
        // hover_progress restores to 0
        float max = 0.0f;
        float min = MIN (0.0f, launcher_height - sum);

        if (_launcher_drag_delta > max)
            delta_y = max + DragLimiter (delta_y - max);
        else if (_launcher_drag_delta < min)
            delta_y = min + DragLimiter (delta_y - min);

        if (GetActionState () != ACTION_DRAG_LAUNCHER)
        {
            float dnd_progress = DnDExitProgress (current);

            if (_launcher_drag_delta > max)
                delta_y = max + (delta_y - max) * dnd_progress;
            else if (_launcher_drag_delta < min)
                delta_y = min + (delta_y - min) * dnd_progress;

            if (dnd_progress == 0.0f)
                _launcher_drag_delta = (int) delta_y;
        }

        delta_y *= hover_progress;
        center.y += delta_y;
        folding_threshold += delta_y;
    }
    else
    {
        _launcher_drag_delta = 0;
    }

    float autohide_offset = 0.0f;
    *launcher_alpha = 1.0f; 
    if (_hidemode != LAUNCHER_HIDE_NEVER)
    {
        float autohide_progress = AutohideProgress (current);

        if (_autohide_animation == FADE_ONLY
            || (_autohide_animation == FADE_SLIDE && _mouse_inside_trigger))
            *launcher_alpha = 1.0f - autohide_progress;
        else
        {
            if (autohide_progress > 0.0f)
                autohide_offset -= geo.width * autohide_progress;
        }
        /* _mouse_inside_trigger is particular because we don't change _hidden state
        * as we can go back and for. We have to lock the launcher manually changing
        * the _hidden state then
        */
        float _position_min = GetAutohidePositionMin ();
        if (autohide_progress == _position_min && _mouse_inside_trigger && !_mouseover_launcher_locked)
        {
            ForceHiddenState (false); // lock the launcher
            _times[TIME_AUTOHIDE] = current;
            SetTimeBack (&_times[TIME_AUTOHIDE], ANIM_DURATION_SHORT * _position_min);
            SetTimeStruct (&_times[TIME_AUTOHIDE], &_times[TIME_AUTOHIDE], ANIM_DURATION_SHORT); // finish the animation 
        }
    }
    
    float drag_hide_progress = DragHideProgress (current);
    if (_hidemode != LAUNCHER_HIDE_NEVER && drag_hide_progress > 0.0f)
    {
        autohide_offset -= geo.width * 0.25f * drag_hide_progress;
        
        if (drag_hide_progress >= 1.0f)
        {
          _hide_on_drag_hover = true;
          EnsureHiddenState ();
        }
    }

    // Inform the painter where to paint the box
    box_geo = geo;

    if (_hidemode != LAUNCHER_HIDE_NEVER)
        box_geo.x += autohide_offset;

    // The functional position we wish to represent for these icons is not smooth. Rather than introducing
    // special casing to represent this, we use MIN/MAX functions. This helps ensure that even though our
    // function is not smooth it is continuous, which is more important for our visual representation (icons
    // wont start jumping around).  As a general rule ANY if () statements that modify center.y should be seen
    // as bugs.
    for (it = _model->main_begin (); it != _model->main_end (); it++)
    {
        RenderArg arg;
        LauncherIcon *icon = *it;

        FillRenderArg (icon, arg, center, folding_threshold, folded_size, folded_spacing, 
                       autohide_offset, folded_z_distance, animation_neg_rads, current);

        launcher_args.push_back (arg);
    }

    // compute maximum height of shelf
    float shelf_sum = 0.0f;
    for (it = _model->shelf_begin (); it != _model->shelf_end (); it++)
    {
        float height = (_icon_size + _space_between_icons) * IconVisibleProgress (*it, current);
        shelf_sum += height;
    }

    // add bottom padding
    if (shelf_sum > 0.0f)
      shelf_sum += _space_between_icons;

    float shelf_delta = MAX (((launcher_height - shelf_sum) + _space_between_icons) - center.y, 0.0f);
    folding_threshold += shelf_delta;
    center.y += shelf_delta;

    for (it = _model->shelf_begin (); it != _model->shelf_end (); it++)
    {
        RenderArg arg;
        LauncherIcon *icon = *it;
        
        FillRenderArg (icon, arg, center, folding_threshold, folded_size, folded_spacing, 
                       autohide_offset, folded_z_distance, animation_neg_rads, current);
        
        launcher_args.push_back (arg);
    }
}

/* End Render Layout Logic */

gboolean Launcher::TapOnSuper ()
{
    struct timespec current;
    clock_gettime (CLOCK_MONOTONIC, &current);  
        
    return (TimeDelta (&current, &_times[TIME_TAP_SUPER]) < SUPER_TAP_DURATION);
}

/* Launcher Show/Hide logic */

void Launcher::StartKeyShowLauncher ()
{
    _super_show_launcher = true;
    QueueDraw ();
    SetTimeStruct (&_times[TIME_TAP_SUPER], NULL, SUPER_TAP_DURATION);
    if (_redraw_handle > 0)
      g_source_remove (_redraw_handle);
    _redraw_handle = g_timeout_add (SUPER_TAP_DURATION, &Launcher::DrawLauncherTimeout, this);
    EnsureHiddenState ();
}

void Launcher::EndKeyShowLauncher ()
{

    _super_show_launcher = false;
    QueueDraw ();

    // it's a tap on super
    if (TapOnSuper ())
      ubus_server_send_message (ubus_server_get_default (), UBUS_DASH_EXTERNAL_ACTIVATION, NULL);      
      
    SetupAutohideTimer ();
}

void Launcher::OnPlaceViewShown (GVariant *data, void *val)
{
    Launcher *self = (Launcher*)val;
    self->_placeview_show_launcher = true;
    self->EnsureHiddenState ();
}

void Launcher::OnPlaceViewHidden (GVariant *data, void *val)
{
    Launcher *self = (Launcher*)val;
    self->_placeview_show_launcher = false;
    self->EnsureHiddenState ();
}

void Launcher::OnTriggerUpdate (GVariant *data, gpointer user_data)
{
  gchar        *prop_key;
  GVariant     *prop_value;
  GVariantIter *prop_iter;
  int x, y, trigger_width, trigger_height;

  Launcher *self = (Launcher*)user_data;
  
  g_variant_get (data, "(iiiia{sv})", &x, &y, &trigger_width, &trigger_height, &prop_iter);
  self->_trigger_mouse_position = nux::Point2 (x, y);
  
  self->_trigger_width = trigger_width;
  self->_trigger_height = trigger_height;
    
  g_return_if_fail (prop_iter != NULL);

  while (g_variant_iter_loop (prop_iter, "{sv}", &prop_key, &prop_value))
  {
    if (g_str_equal ("hovered", prop_key))
    {
      self->_mouse_inside_trigger = g_variant_get_boolean (prop_value);
      if (self->_mouse_inside_trigger)
        self->_hide_on_drag_hover = false;
      self->EnsureHiddenState ();
      self->EnsureHoverState ();
      self->EnsureScrollTimer ();    
    }
  }
}

void Launcher::OnActionDone (GVariant *data, void *val)
{
    Launcher *self = (Launcher*)val;
    self->_mouseover_launcher_locked = false;
    self->SetupAutohideTimer ();
}

void Launcher::ForceHiddenState (bool hidden)
{
    // force hidden state skipping the animation
    SetHidden (hidden);
    _times[TIME_AUTOHIDE].tv_sec = 0;
    _times[TIME_AUTOHIDE].tv_nsec = 0;
}

void Launcher::SetHidden (bool hidden)
{
    if (hidden == _hidden)
        return;
    
    printf ("Hidden state changed to %i\n********************************************\n", hidden);

    // auto lock/unlock the launcher depending on the state switch
    if (hidden)
    {
        _mouseover_launcher_locked = false;
        // RecvMouseLeave isn't receive if the launcher is hiding while we have the mouse on it
        // (like, click on a icon, wait the timeout time without moving the mouse, then the launcher hide)
        if (_mouse_inside_launcher)
          RecvMouseLeave(-1, -1, 0, 0);
    }
    else
        _mouseover_launcher_locked = true;

    _hidden = hidden;
    SetTimeStruct (&_times[TIME_AUTOHIDE], &_times[TIME_AUTOHIDE], ANIM_DURATION_SHORT);

    _parent->EnableInputWindow(!hidden, "launcher", false, false);

    if (!hidden && GetActionState () == ACTION_DRAG_EXTERNAL)
      ProcessDndLeave ();

    EnsureAnimation ();
}

gboolean Launcher::OnAutohideTimeout (gpointer data)
{
    Launcher *self = (Launcher*) data;

    self->_autohide_handle = 0;
    self->EnsureHiddenState ();
    return false;
}

gboolean Launcher::DrawLauncherTimeout (gpointer data)
{
    Launcher *self = (Launcher*) data;
    
    self->QueueDraw ();
    return false;    
}

void
Launcher::EnsureHiddenState ()
{
  // compiler should optimize this, we do this for readability
  bool mouse_over_launcher = _mouseover_launcher_locked && (_mouse_inside_trigger || _mouse_inside_launcher);
  
  bool required_for_external_purpose = _super_show_launcher || _placeview_show_launcher || _navmod_show_launcher ||
                                       QuicklistManager::Default ()->Current() || PluginAdapter::Default ()->IsScaleActive ();
                                       
  bool in_must_be_open_mode = GetActionState () != ACTION_NONE || _dnd_window_is_mapped;
  
  bool must_be_hidden = _hide_on_drag_hover && _hidemode != LAUNCHER_HIDE_NEVER;
  
  bool autohide_handle_hold = _autohide_handle && !_hidden;
  
  printf ("_hidden: %i\n must_be_hidden: %i (%i && %i), mouse_over_launcher: %i (%i && (%i || %i))\n required_for_external_purpose: %i (%i || %i || %i || %i || %i)\n in_must_be_open_mode: %i (%i || %i)\n _window_over_launcher: %i, autohide_handle_hold: %i (%i && %i)\n",
    _hidden, must_be_hidden, _hide_on_drag_hover, _hidemode != LAUNCHER_HIDE_NEVER, mouse_over_launcher, _mouseover_launcher_locked, _mouse_inside_trigger, _mouse_inside_launcher, required_for_external_purpose,
    _super_show_launcher, _placeview_show_launcher, _navmod_show_launcher, QuicklistManager::Default ()->Current() != NULL, PluginAdapter::Default ()->IsScaleActive (), in_must_be_open_mode, _launcher_action_state != ACTION_NONE,
    _dnd_window_is_mapped, _window_over_launcher, autohide_handle_hold, _autohide_handle, !_hidden
  );
  
  if (must_be_hidden || (!mouse_over_launcher && !required_for_external_purpose && 
                         !in_must_be_open_mode && _window_over_launcher && !autohide_handle_hold))
    SetHidden (true);
  else
    SetHidden (false);
}

bool
Launcher::CheckIntersectWindow (CompWindow *window)
{
  nux::Geometry geo = GetGeometry ();
  int intersect_types = CompWindowTypeNormalMask | CompWindowTypeDialogMask |
                        CompWindowTypeModalDialogMask | CompWindowTypeUtilMask;

  if (!window || !(window->type () & intersect_types) || !window->isMapped () || !window->isViewable ())
    return false;

  if (CompRegion (window->inputRect ()).intersects (CompRect (geo.x, geo.y, geo.width, geo.height)))
    return true;

  return false;
}

void
Launcher::CheckWindowOverLauncher ()
{
  CompWindowList window_list = _screen->windows ();
  CompWindowList::iterator it;
  CompWindow *window = NULL;

  if (_hidemode == LAUNCHER_HIDE_DODGE_ACTIVE_WINDOW)
  {
    window = _screen->findWindow (_screen->activeWindow ());
    if (!CheckIntersectWindow (window))
      window = NULL;
  }
  else
  {
    for (it = window_list.begin (); it != window_list.end (); it++)
    {
      if (CheckIntersectWindow (*it)) {
        window = *it;
        break;
      }
    }
  }

  _window_over_launcher = (window != NULL);
  EnsureHiddenState ();
}

gboolean
Launcher::OnUpdateDragManagerTimeout (gpointer data)
{
  Launcher *self = (Launcher *) data;
 
  if (!self->_selection_atom)
    self->_selection_atom = XInternAtom (self->_screen->dpy (), "XdndSelection", false);
  
  Window drag_owner = XGetSelectionOwner (self->_screen->dpy (), self->_selection_atom);
  
  if (drag_owner)
  {
    self->_dnd_window_is_mapped = true;
  }
  else
  {
    self->_hide_on_drag_hover = false;
    self->_dnd_window_is_mapped = false;
  }
  self->EnsureHiddenState ();

  return false;
}

void
Launcher::OnWindowMapped (guint32 xid)
{
  CompWindow *window = _screen->findWindow (xid);
  if (window && window->type () | CompWindowTypeDndMask)
  {
    g_timeout_add (200, &Launcher::OnUpdateDragManagerTimeout, this);
  }
}

void
Launcher::OnWindowUnmapped (guint32 xid)
{
  CompWindow *window = _screen->findWindow (xid);
  if (window && window->type () | CompWindowTypeDndMask)
  {
    g_timeout_add (200, &Launcher::OnUpdateDragManagerTimeout, this);
  }
}

void
Launcher::OnWindowMaybeIntellihide (guint32 xid)
{
  if (_hidemode != LAUNCHER_HIDE_NEVER)
  {
    if (_hidemode == LAUNCHER_HIDE_AUTOHIDE)
    {
      _window_over_launcher = true;
      EnsureHiddenState ();
    }
    else
    {
      CheckWindowOverLauncher ();
    }
  }
}

void Launcher::SetupAutohideTimer ()
{
  if (_hidemode != LAUNCHER_HIDE_NEVER)
  {
    if (_autohide_handle > 0)
      g_source_remove (_autohide_handle);
    _autohide_handle = g_timeout_add (1000, &Launcher::OnAutohideTimeout, this);
  }
}

Launcher::LauncherHideMode Launcher::GetHideMode ()
{
  return _hidemode;
}

/* End Launcher Show/Hide logic */

// Hacks around compiz failing to see the struts because the window was just mapped.
gboolean Launcher::StrutHack (gpointer data)
{
  Launcher *self = (Launcher *) data;
  self->_parent->InputWindowEnableStruts(false);
  self->_parent->InputWindowEnableStruts(true);

  return false;
}

void Launcher::SetHideMode (LauncherHideMode hidemode)
{
  if (_hidemode == hidemode)
    return;

  if (hidemode != LAUNCHER_HIDE_NEVER)
  {
    _parent->InputWindowEnableStruts(false);
  }
  else
  {
    _parent->EnableInputWindow(true, "launcher", false, false);
    g_timeout_add (1000, &Launcher::StrutHack, this);
    _parent->InputWindowEnableStruts(true);
  }

  _hidemode = hidemode;
  EnsureAnimation ();
}

Launcher::AutoHideAnimation Launcher::GetAutoHideAnimation ()
{
  return _autohide_animation;
}

void Launcher::SetAutoHideAnimation (AutoHideAnimation animation)
{
  if (_autohide_animation == animation)
    return;

  _autohide_animation = animation;
}

void Launcher::SetFloating (bool floating)
{
  if (_floating == floating)
    return;

  _floating = floating;
  EnsureAnimation ();
}

void Launcher::SetBacklightMode (BacklightMode mode)
{
  if (_backlight_mode == mode)
    return;
  
  _backlight_mode = mode;
  EnsureAnimation ();
}

Launcher::BacklightMode Launcher::GetBacklightMode ()
{
  return _backlight_mode;
}

void 
Launcher::SetLaunchAnimation (LaunchAnimation animation)
{
  if (_launch_animation == animation)
    return;
    
  _launch_animation = animation;  
}

Launcher::LaunchAnimation 
Launcher::GetLaunchAnimation ()
{
  return _launch_animation;
}

void 
Launcher::SetUrgentAnimation (UrgentAnimation animation)
{
  if (_urgent_animation == animation)
    return;
    
  _urgent_animation = animation;  
}

Launcher::UrgentAnimation 
Launcher::GetUrgentAnimation ()
{
  return _urgent_animation;
}

void
Launcher::SetActionState (LauncherActionState actionstate)
{
  _launcher_action_state = actionstate;

  if (_navmod_show_launcher)
    exitKeyNavMode ();
}

Launcher::LauncherActionState
Launcher::GetActionState ()
{
  return _launcher_action_state;
}

void
Launcher::EnsureHoverState ()
{
  if (_mouse_inside_launcher || _mouse_inside_trigger || 
      QuicklistManager::Default ()->Current() || GetActionState () != ACTION_NONE)
  {
    SetHover ();
  }
  else
  {
    UnsetHover ();
  }
}

void Launcher::SetHover ()
{
  if (_hovered)
    return;

  _enter_y = (int) _mouse_position.y;

  _hovered = true;
  SetTimeStruct (&_times[TIME_ENTER], &_times[TIME_LEAVE], ANIM_DURATION);
  EnsureAnimation ();
}

void Launcher::UnsetHover ()
{
  if (!_hovered)
    return;

  _hovered = false;
  SetTimeStruct (&_times[TIME_LEAVE], &_times[TIME_ENTER], ANIM_DURATION);
  SetupAutohideTimer ();
  EnsureAnimation ();
}

bool Launcher::MouseOverTopScrollArea ()
{
  if (GetActionState () == ACTION_NONE)
    return _mouse_inside_trigger;
  
  return _mouse_position.y < 0;
}

bool Launcher::MouseOverTopScrollExtrema ()
{
  // since we are not dragging the trigger will pick up events
  if (GetActionState () == ACTION_NONE)
    return _trigger_mouse_position.y == 0;
    
  return _mouse_position.y == 0 - _parent->GetGeometry ().y;
}

bool Launcher::MouseOverBottomScrollArea ()
{
  return _mouse_position.y > GetGeometry ().height - 24;
}

bool Launcher::MouseOverBottomScrollExtrema ()
{
  return _mouse_position.y == GetGeometry ().height - 1;
}

gboolean Launcher::OnScrollTimeout (gpointer data)
{
  Launcher *self = (Launcher*) data;
  nux::Geometry geo = self->GetGeometry ();

  if (!self->_hovered || (self->GetActionState () != ACTION_DRAG_ICON && self->GetActionState () != ACTION_DRAG_EXTERNAL))
    return TRUE;
  
  if (self->MouseOverTopScrollArea ())
  {
    if (self->MouseOverTopScrollExtrema ())
      self->_launcher_drag_delta += 6;
    else
      self->_launcher_drag_delta += 3;
  }
  else if (self->MouseOverBottomScrollArea ())
  {
    if (self->MouseOverBottomScrollExtrema ())
      self->_launcher_drag_delta -= 6;
    else
      self->_launcher_drag_delta -= 3;
  }
  
  self->EnsureAnimation ();
  
  return TRUE;
}

void Launcher::EnsureScrollTimer ()
{
  bool needed = MouseOverTopScrollArea () || MouseOverBottomScrollArea ();
  
  if (needed && !_autoscroll_handle)
  {
    _autoscroll_handle = g_timeout_add (15, &Launcher::OnScrollTimeout, this);
  }
  else if (!needed && _autoscroll_handle)
  {
    g_source_remove (_autoscroll_handle);
    _autoscroll_handle = 0;
  }
}

void Launcher::SetIconSize(int tile_size, int icon_size)
{
    nux::Geometry geo = _parent->GetGeometry ();

    _icon_size = tile_size;
    _icon_image_size = icon_size;
    _icon_image_size_delta = tile_size - icon_size;

    // recreate tile textures

    _parent->SetGeometry (nux::Geometry (geo.x, geo.y, tile_size + 12, geo.height));
}

void Launcher::OnIconAdded (LauncherIcon *icon)
{
    icon->Reference ();
    EnsureAnimation();

    icon->_xform_coords["HitArea"]      = new nux::Vector4[4];
    icon->_xform_coords["Image"]        = new nux::Vector4[4];
    icon->_xform_coords["Tile"]         = new nux::Vector4[4];
    icon->_xform_coords["Glow"]         = new nux::Vector4[4];
    icon->_xform_coords["Emblem"]       = new nux::Vector4[4];

    // needs to be disconnected
    icon->needs_redraw.connect (sigc::mem_fun(this, &Launcher::OnIconNeedsRedraw));

   guint64 shortcut = icon->GetShortcut ();
    if (shortcut != 0 && !g_ascii_isdigit ((gchar) shortcut))
      icon->SetSuperkeyLabel (cairoToTexture2D ((gchar) shortcut, _icon_size, _icon_size));

    AddChild (icon);
}

void Launcher::OnIconRemoved (LauncherIcon *icon)
{
    delete [] icon->_xform_coords["HitArea"];
    delete [] icon->_xform_coords["Image"];
    delete [] icon->_xform_coords["Tile"];
    delete [] icon->_xform_coords["Glow"];
    delete [] icon->_xform_coords["Emblem"];

    icon->UnReference ();
    EnsureAnimation();
    RemoveChild (icon);
}

void Launcher::OnOrderChanged ()
{
    EnsureAnimation ();
}

void Launcher::SetModel (LauncherModel *model)
{
    _model = model;
    _model->icon_added.connect (sigc::mem_fun (this, &Launcher::OnIconAdded));
    _model->icon_removed.connect (sigc::mem_fun (this, &Launcher::OnIconRemoved));
    _model->order_changed.connect (sigc::mem_fun (this, &Launcher::OnOrderChanged));
}

LauncherModel* Launcher::GetModel ()
{
  return _model;
}

void Launcher::OnIconNeedsRedraw (LauncherIcon *icon)
{
    EnsureAnimation();
}

long Launcher::ProcessEvent(nux::IEvent &ievent, long TraverseInfo, long ProcessEventInfo)
{
    long ret = TraverseInfo;
    ret = PostProcessEvent2(ievent, ret, ProcessEventInfo);
    return ret;
}

void Launcher::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{

}

void Launcher::RenderIndicators (nux::GraphicsEngine& GfxContext,
                                 RenderArg const &arg,
                                 int running,
                                 int active,
                                 nux::Geometry geo)
{
  int markerCenter = (int) arg.render_center.y;

  if (running > 0)
  {
    nux::TexCoordXForm texxform;

    nux::Color color = nux::Color::LightGrey;

    if (arg.running_colored)
      color = nux::Color::SkyBlue;

    nux::BaseTexture *texture;

    std::vector<int> markers;
    
    /*if (!arg.running_on_viewport)
    {
      markers.push_back (markerCenter);
      texture = _arrow_empty_ltr;
    }
    else*/ if (running == 1)
    {
      markers.push_back (markerCenter);
      texture = _arrow_ltr;
    }
    else if (running == 2)
    {
      markers.push_back (markerCenter - 2);
      markers.push_back (markerCenter + 2);
      texture = _pip_ltr;
    }
    else
    {
      markers.push_back (markerCenter - 4);
      markers.push_back (markerCenter);
      markers.push_back (markerCenter + 4);
      texture = _pip_ltr;
    }

    std::vector<int>::iterator it;
    for (it = markers.begin (); it != markers.end (); it++)
    {
      int center = *it;
      GfxContext.QRP_1Tex (geo.x,
                           center - (texture->GetHeight () / 2),
                           (float) texture->GetWidth(),
                           (float) texture->GetHeight(),
                           texture->GetDeviceTexture(),
                           texxform,
                           color);
    }
  }

  if (active > 0)
  {
    nux::TexCoordXForm texxform;

    nux::Color color = nux::Color::LightGrey;
    GfxContext.QRP_1Tex ((geo.x + geo.width) - _arrow_rtl->GetWidth (),
                              markerCenter - (_arrow_rtl->GetHeight () / 2),
                              (float) _arrow_rtl->GetWidth(),
                              (float) _arrow_rtl->GetHeight(),
                              _arrow_rtl->GetDeviceTexture(),
                              texxform,
                              color);
  }
}

void Launcher::RenderIcon(nux::GraphicsEngine& GfxContext,
                          RenderArg const &arg,
                          nux::IntrusiveSP<nux::IOpenGLBaseTexture> icon,
                          nux::Color bkg_color,
                          float alpha,
                          nux::Vector4 xform_coords[])
{
  if (icon == NULL)
    return;

  nux::Matrix4 ObjectMatrix;
  nux::Matrix4 ViewMatrix;
  nux::Matrix4 ProjectionMatrix;
  nux::Matrix4 ViewProjectionMatrix;

  if(nux::Abs (arg.x_rotation) < 0.01f && nux::Abs (arg.y_rotation) < 0.01f && nux::Abs (arg.z_rotation) < 0.01f)
    icon->SetFiltering(GL_NEAREST, GL_NEAREST);
  else
    icon->SetFiltering(GL_LINEAR, GL_LINEAR);

  nux::Vector4 v0;
  nux::Vector4 v1;
  nux::Vector4 v2;
  nux::Vector4 v3;

  v0.x = xform_coords[0].x ;
  v0.y = xform_coords[0].y ;
  v0.z = xform_coords[0].z ;
  v0.w = xform_coords[0].w ;
  v1.x = xform_coords[1].x ;
  v1.y = xform_coords[1].y ;
  v1.z = xform_coords[1].z ;
  v1.w = xform_coords[1].w ;
  v2.x = xform_coords[2].x ;
  v2.y = xform_coords[2].y ;
  v2.z = xform_coords[2].z ;
  v2.w = xform_coords[2].w ;
  v3.x = xform_coords[3].x ;
  v3.y = xform_coords[3].y ;
  v3.z = xform_coords[3].z ;
  v3.w = xform_coords[3].w ;

  float s0, t0, s1, t1, s2, t2, s3, t3;
  nux::Color color = bkg_color;

  if (icon->GetResourceType () == nux::RTTEXTURERECTANGLE)
  {
    s0 = 0.0f;                                  t0 = 0.0f;
    s1 = 0.0f;                                  t1 = icon->GetHeight();
    s2 = icon->GetWidth();     t2 = icon->GetHeight();
    s3 = icon->GetWidth();     t3 = 0.0f;
  }
  else
  {
    s0 = 0.0f;    t0 = 0.0f;
    s1 = 0.0f;    t1 = 1.0f;
    s2 = 1.0f;    t2 = 1.0f;
    s3 = 1.0f;    t3 = 0.0f;
  }

  float VtxBuffer[] =
  {// Perspective correct
    v0.x, v0.y, 0.0f, 1.0f,     s0/v0.w, t0/v0.w, 0.0f, 1.0f/v0.w,
    v1.x, v1.y, 0.0f, 1.0f,     s1/v1.w, t1/v1.w, 0.0f, 1.0f/v1.w,
    v2.x, v2.y, 0.0f, 1.0f,     s2/v2.w, t2/v2.w, 0.0f, 1.0f/v2.w,
    v3.x, v3.y, 0.0f, 1.0f,     s3/v3.w, t3/v3.w, 0.0f, 1.0f/v3.w,
  };

  CHECKGL(glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0));
  CHECKGL(glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0));

  int TextureObjectLocation;
  int VertexLocation;
  int TextureCoord0Location;
  int VertexColorLocation;
  int FragmentColor;

  if(nux::GetGraphicsEngine ().UsingGLSLCodePath ())
  {
    _shader_program_uv_persp_correction->Begin();

    TextureObjectLocation   = _shader_program_uv_persp_correction->GetUniformLocationARB("TextureObject0");
    VertexLocation          = _shader_program_uv_persp_correction->GetAttributeLocation("iVertex");
    TextureCoord0Location   = _shader_program_uv_persp_correction->GetAttributeLocation("iTexCoord0");
    VertexColorLocation     = _shader_program_uv_persp_correction->GetAttributeLocation("iColor");
    FragmentColor           = _shader_program_uv_persp_correction->GetUniformLocationARB ("color0");

    nux::GetGraphicsEngine ().SetTexture(GL_TEXTURE0, icon);

    if(TextureObjectLocation != -1)
      CHECKGL( glUniform1iARB (TextureObjectLocation, 0) );

    int VPMatrixLocation = _shader_program_uv_persp_correction->GetUniformLocationARB("ViewProjectionMatrix");
    if(VPMatrixLocation != -1)
    {
      nux::Matrix4 mat = nux::GetGraphicsEngine ().GetOpenGLModelViewProjectionMatrix ();
      _shader_program_uv_persp_correction->SetUniformLocMatrix4fv ((GLint)VPMatrixLocation, 1, false, (GLfloat*)&(mat.m));
    }
  }
  else
  {
    _AsmShaderProg->Begin();

    VertexLocation        = nux::VTXATTRIB_POSITION;
    TextureCoord0Location = nux::VTXATTRIB_TEXCOORD0;
    VertexColorLocation   = nux::VTXATTRIB_COLOR;

    nux::GetGraphicsEngine().SetTexture(GL_TEXTURE0, icon);
  }

  CHECKGL( glEnableVertexAttribArrayARB(VertexLocation) );
  CHECKGL( glVertexAttribPointerARB((GLuint)VertexLocation, 4, GL_FLOAT, GL_FALSE, 32, VtxBuffer) );

  if(TextureCoord0Location != -1)
  {
    CHECKGL( glEnableVertexAttribArrayARB(TextureCoord0Location) );
    CHECKGL( glVertexAttribPointerARB((GLuint)TextureCoord0Location, 4, GL_FLOAT, GL_FALSE, 32, VtxBuffer + 4) );
  }

//   if(VertexColorLocation != -1)
//   {
//     CHECKGL( glEnableVertexAttribArrayARB(VertexColorLocation) );
//     CHECKGL( glVertexAttribPointerARB((GLuint)VertexColorLocation, 4, GL_FLOAT, GL_FALSE, 32, VtxBuffer + 8) );
//   }

  bkg_color.SetRGBA (bkg_color.R () * alpha, bkg_color.G () * alpha,
                     bkg_color.B () * alpha, alpha);

  if(nux::GetGraphicsEngine ().UsingGLSLCodePath ())
  {
    CHECKGL ( glUniform4fARB (FragmentColor, bkg_color.R(), bkg_color.G(), bkg_color.B(), bkg_color.A() ) );
    nux::GetGraphicsEngine ().SetTexture(GL_TEXTURE0, icon);
    CHECKGL( glDrawArrays(GL_QUADS, 0, 4) );
  }
  else
  {
    CHECKGL ( glProgramLocalParameter4fARB (GL_FRAGMENT_PROGRAM_ARB, 0, bkg_color.R(), bkg_color.G(), bkg_color.B(), bkg_color.A() ) );
    nux::GetGraphicsEngine ().SetTexture(GL_TEXTURE0, icon);
    CHECKGL( glDrawArrays(GL_QUADS, 0, 4) );
  }

  if(VertexLocation != -1)
    CHECKGL( glDisableVertexAttribArrayARB(VertexLocation) );
  if(TextureCoord0Location != -1)
    CHECKGL( glDisableVertexAttribArrayARB(TextureCoord0Location) );
//   if(VertexColorLocation != -1)
//     CHECKGL( glDisableVertexAttribArrayARB(VertexColorLocation) );

  if(nux::GetGraphicsEngine ().UsingGLSLCodePath ())
  {
    _shader_program_uv_persp_correction->End();
  }
  else
  {
    _AsmShaderProg->End();
  }
}

void Launcher::DrawRenderArg (nux::GraphicsEngine& GfxContext, RenderArg const &arg, nux::Geometry geo)
{
  // This check avoids a crash when the icon is not available on the system.
  if (arg.icon->TextureForSize (_icon_image_size) == 0)
    return;

  GfxContext.GetRenderStates ().SetBlend (true);
  GfxContext.GetRenderStates ().SetPremultipliedBlend (nux::SRC_OVER);
  GfxContext.GetRenderStates ().SetColorMask (true, true, true, true);

  /* draw tile */
  if (arg.backlight_intensity < 1.0f)
  {
    RenderIcon(GfxContext,
               arg,
               _icon_outline_texture->GetDeviceTexture (),
               nux::Color(0xAAAAAAAA),
               (1.0f - arg.backlight_intensity) * arg.alpha,
               arg.icon->_xform_coords["Tile"]);
  }

  if (arg.backlight_intensity > 0.0f)
  {
    RenderIcon(GfxContext,
               arg,
               _icon_bkg_texture->GetDeviceTexture (),
               arg.icon->BackgroundColor (),
               arg.backlight_intensity * arg.alpha,
               arg.icon->_xform_coords["Tile"]);
  }
  /* end tile draw */

  /* draw icon */
  RenderIcon (GfxContext,
              arg,
              arg.icon->TextureForSize (_icon_image_size)->GetDeviceTexture (),
              nux::Color::White,
              arg.alpha,
              arg.icon->_xform_coords["Image"]);

  /* draw overlay shine */
  if (arg.backlight_intensity > 0.0f)
  {
    RenderIcon(GfxContext,
               arg,
               _icon_shine_texture->GetDeviceTexture (),
               nux::Color::White,
               arg.backlight_intensity * arg.alpha,
               arg.icon->_xform_coords["Tile"]);
  }

  /* draw glow */
  if (arg.glow_intensity > 0.0f)
  {
    RenderIcon(GfxContext,
               arg,
               _icon_glow_texture->GetDeviceTexture (),
               arg.icon->GlowColor (),
               arg.glow_intensity * arg.alpha,
               arg.icon->_xform_coords["Glow"]);
  }
  
  /* draw shimmer */
  if (arg.shimmer_progress > 0.0f && arg.shimmer_progress < 1.0f)
  {
    nux::Geometry base = GetGeometry ();
    int x1 = base.x + base.width;
    int x2 = base.x + base.width;
    float shimmer_constant = 1.9f;

    x1 -= geo.width * arg.shimmer_progress * shimmer_constant;
    GfxContext.PushClippingRectangle(nux::Geometry (x1, geo.y, x2 - x1, geo.height));

    float fade_out = 1.0f - CLAMP (((x2 - x1) - geo.width) / (geo.width * (shimmer_constant - 1.0f)), 0.0f, 1.0f);

    RenderIcon(GfxContext,
               arg,
               _icon_glow_texture->GetDeviceTexture (),
               arg.icon->GlowColor (),
               fade_out * arg.alpha,
               arg.icon->_xform_coords["Glow"]);

    GfxContext.PopClippingRectangle();
  }
  
  /* draw progress bar */
  if (arg.progress_bias > -1.0f && arg.progress_bias < 1.0f)
  {
    if (_offscreen_progress_texture->GetWidth () != _icon_size || _offscreen_progress_texture->GetHeight () != _icon_size)
      _offscreen_progress_texture = nux::GetThreadGLDeviceFactory()->CreateSystemCapableDeviceTexture (_icon_size, _icon_size, 1, nux::BITFMT_R8G8B8A8);
    RenderProgressToTexture (GfxContext, _offscreen_progress_texture, arg.progress, arg.progress_bias);
    
    RenderIcon(GfxContext,
               arg,
               _offscreen_progress_texture,
               nux::Color::White,
               arg.alpha,
               arg.icon->_xform_coords["Tile"]);
  }
  
  if (arg.icon->Emblem ())
  {
    RenderIcon(GfxContext,
               arg,
               arg.icon->Emblem ()->GetDeviceTexture (),
               nux::Color::White,
               arg.alpha,
               arg.icon->_xform_coords["Emblem"]);
  }

  /* draw indicators */
  RenderIndicators (GfxContext,
                    arg,
                    arg.running_arrow ? arg.window_indicators : 0,
                    arg.active_arrow ? 1 : 0,
                    geo);

  /* draw keyboard-navigation "highlight" if any */
  if (arg.keyboard_nav_hl)
    RenderIcon (GfxContext,
                arg,
                _icon_glow_hl_texture->GetDeviceTexture (),
                nux::Color (0xFFFFFFFF),
                arg.alpha,
                arg.icon->_xform_coords["Glow"]);

  /* draw superkey-shortcut label */ 
  if (_super_show_launcher && !TapOnSuper ())
  {
    guint64 shortcut = arg.icon->GetShortcut ();

    /* deal with dynamic labels for places, which can be set via the locale */
    if (shortcut != 0 && !g_ascii_isdigit ((gchar) shortcut))
    {
      RenderIcon (GfxContext,
                  arg,
                  arg.icon->GetSuperkeyLabel ()->GetDeviceTexture (),
                  nux::Color (0xFFFFFFFF),
                  arg.alpha,
                  arg.icon->_xform_coords["Tile"]);
    }
    else
    {
      /* deal with the hardcoded labels used for the first 10 icons on the launcher */
      gchar key   = (gchar) shortcut;
      int   index = -1;

      switch (key)
      {
        case '1': index = 0; break;
        case '2': index = 1; break;
        case '3': index = 2; break;
        case '4': index = 3; break;
        case '5': index = 4; break;
        case '6': index = 5; break;
        case '7': index = 6; break;
        case '8': index = 7; break;
        case '9': index = 8; break;
        case '0': index = 9; break;
      }

      if (index != -1)
        RenderIcon (GfxContext,
                    arg,
                    _superkey_labels[index]->GetDeviceTexture (),
                    nux::Color (0xFFFFFFFF),
                    arg.alpha,
                    arg.icon->_xform_coords["Tile"]);
    }
  }
}

void Launcher::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw)
{
    nux::Geometry base = GetGeometry();
    nux::Geometry bkg_box;
    std::list<Launcher::RenderArg> args;
    std::list<Launcher::RenderArg>::reverse_iterator rev_it;
    std::list<Launcher::RenderArg>::iterator it;
    float launcher_alpha = 1.0f;

    // rely on the compiz event loop to come back to us in a nice throttling
    if (AnimationInProgress ())   
      g_timeout_add (0, &Launcher::AnimationTimeout, this);

    nux::ROPConfig ROP;
    ROP.Blend = false;
    ROP.SrcBlend = GL_ONE;
    ROP.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

    RenderArgs (args, bkg_box, &launcher_alpha);

    if (_drag_icon && _render_drag_window)
    {
      RenderIconToTexture (GfxContext, _drag_icon, _offscreen_drag_texture);
      _drag_window->ShowWindow (true);
      nux::GetWindowCompositor ().SetAlwaysOnFrontWindow (_drag_window);
      
      _render_drag_window = false;
    }


    // clear region
    GfxContext.PushClippingRectangle(base);
    gPainter.PushDrawColorLayer(GfxContext, base, nux::Color(0x00000000), true, ROP);

    // clip vertically but not horizontally
    GfxContext.PushClippingRectangle(nux::Geometry (base.x, bkg_box.y, base.width, bkg_box.height));
    GfxContext.GetRenderStates ().SetPremultipliedBlend (nux::SRC_OVER);

    gPainter.Paint2DQuadColor (GfxContext, bkg_box, nux::Color(0xAA000000));

    UpdateIconXForm (args);
    EventLogic ();

    /* draw launcher */
    for (rev_it = args.rbegin (); rev_it != args.rend (); rev_it++)
    {
      if ((*rev_it).stick_thingy)
        gPainter.Paint2DQuadColor (GfxContext, 
                                   nux::Geometry (bkg_box.x, (*rev_it).render_center.y - 3, bkg_box.width, 2), 
                                   nux::Color(0xAAAAAAAA));
      
      if ((*rev_it).x_rotation >= 0.0f || (*rev_it).skip)
        continue;

      DrawRenderArg (GfxContext, *rev_it, bkg_box);
    }

    for (it = args.begin(); it != args.end(); it++)
    {
      if ((*it).stick_thingy)
        gPainter.Paint2DQuadColor (GfxContext, 
                                   nux::Geometry (bkg_box.x, (*it).render_center.y - 3, bkg_box.width, 2), 
                                   nux::Color(0xAAAAAAAA));
                                   
      if ((*it).x_rotation < 0.0f || (*it).skip)
        continue;

      DrawRenderArg (GfxContext, *it, bkg_box);
    }


    gPainter.Paint2DQuadColor (GfxContext, nux::Geometry (bkg_box.x + bkg_box.width - 1, bkg_box.y, 1, bkg_box.height), nux::Color(0x60606060));

    // FIXME: can be removed for a bgk_box->SetAlpha once implemented    
    GfxContext.GetRenderStates ().SetPremultipliedBlend (nux::DST_IN);
    nux::Color alpha_mask = nux::Color(0xAAAAAAAA);
    alpha_mask.SetRGBA (alpha_mask.R () * launcher_alpha, alpha_mask.G () * launcher_alpha,
                        alpha_mask.B () * launcher_alpha, launcher_alpha);
    gPainter.Paint2DQuadColor (GfxContext, bkg_box, alpha_mask);
    
    GfxContext.GetRenderStates().SetColorMask (true, true, true, true);
    GfxContext.GetRenderStates ().SetBlend (false);

    gPainter.PopBackground();
    GfxContext.PopClippingRectangle();
    GfxContext.PopClippingRectangle();
}

void Launcher::PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
}

void Launcher::PreLayoutManagement()
{
    View::PreLayoutManagement();
    if(m_CompositionLayout)
    {
        m_CompositionLayout->SetGeometry(GetGeometry());
    }
}

long Launcher::PostLayoutManagement(long LayoutResult)
{
  View::PostLayoutManagement(LayoutResult);

  SetMousePosition (0, 0);

  return nux::eCompliantHeight | nux::eCompliantWidth;
}

void Launcher::PositionChildLayout(float offsetX, float offsetY)
{
}

void Launcher::OnDragWindowAnimCompleted ()
{
  if (_drag_window)
    _drag_window->ShowWindow (false);
  
  EnsureAnimation ();
}

void Launcher::StartIconDrag (LauncherIcon *icon)
{
  if (!icon)
    return;
    
  _drag_icon = icon;

  if (_drag_window)
  {
    _drag_window->ShowWindow (false);
    _drag_window->UnReference ();
    _drag_window = NULL;
  }
  
  _offscreen_drag_texture = nux::GetThreadGLDeviceFactory()->CreateSystemCapableDeviceTexture (_icon_size, _icon_size, 1, nux::BITFMT_R8G8B8A8);
  _drag_window = new LauncherDragWindow (_offscreen_drag_texture);
  _drag_window->SinkReference ();
  
  _render_drag_window = true;
}

void Launcher::EndIconDrag ()
{
  if (_drag_window)
  {
    _drag_window->SetAnimationTarget ((int) (_drag_icon->GetCenter ().x), (int) (_drag_icon->GetCenter ().y));
    _drag_window->StartAnimation ();
    _drag_window->anim_completed.connect (sigc::mem_fun (this, &Launcher::OnDragWindowAnimCompleted));
  }
  
  if (MouseBeyondDragThreshold ())
    SetTimeStruct (&_times[TIME_DRAG_THRESHOLD], &_times[TIME_DRAG_THRESHOLD], ANIM_DURATION_SHORT);
  
  _render_drag_window = false;
}

void Launcher::UpdateDragWindowPosition (int x, int y)
{
  if (_drag_window)
  {
    nux::Geometry geo = _drag_window->GetGeometry ();
    _drag_window->SetBaseXY (x - geo.width / 2 + _parent->GetGeometry ().x, y - geo.height / 2 + _parent->GetGeometry ().y);
    
    LauncherIcon *hovered_icon = MouseIconIntersection ((int) (GetGeometry ().x / 2.0f), y);
    
    struct timespec current;
    clock_gettime (CLOCK_MONOTONIC, &current);
    if (_drag_icon && hovered_icon && _drag_icon != hovered_icon)
    {
      float progress = DragThresholdProgress (current);
      
      if (progress >= 1.0f)
        _model->ReorderSmart (_drag_icon, hovered_icon, true);
      else if (progress == 0.0f)
        _model->ReorderBefore (_drag_icon, hovered_icon, false);
    }
  }
}

void Launcher::RecvMouseDown(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  _last_button_press = nux::GetEventButton (button_flags);
  SetMousePosition (x, y);

  MouseDownLogic (x, y, button_flags, key_flags);
  EnsureAnimation ();
}

void Launcher::RecvMouseUp(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  SetMousePosition (x, y);
  nux::Geometry geo = GetGeometry ();

  if (GetActionState () != ACTION_NONE && !geo.IsInside(nux::Point(x, y)))
  {
    // we are no longer hovered
    EnsureHoverState ();
  }
  
  MouseUpLogic (x, y, button_flags, key_flags);

  if (GetActionState () == ACTION_DRAG_ICON)
    EndIconDrag ();

  SetActionState (ACTION_NONE);
  _dnd_delta_x = 0;
  _dnd_delta_y = 0;
  _last_button_press = 0;
  EnsureHoverState ();
  EnsureAnimation ();
}

void Launcher::RecvMouseDrag(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags)
{
  SetMousePosition (x, y);

  _dnd_delta_y += dy;
  _dnd_delta_x += dx;
  
  if (nux::Abs (_dnd_delta_y) < 15 &&
      nux::Abs (_dnd_delta_x) < 15 && 
      GetActionState () == ACTION_NONE)
      return;

  if (_icon_under_mouse)
  {
    _icon_under_mouse->MouseLeave.emit ();
    _icon_under_mouse->_mouse_inside = false;
    _icon_under_mouse = 0;
  }

  if (GetActionState () == ACTION_NONE)
  {
    SetTimeStruct (&_times[TIME_DRAG_START]);
    
    if (nux::Abs (_dnd_delta_y) >= nux::Abs (_dnd_delta_x))
    {
      _launcher_drag_delta += _dnd_delta_y;
      SetActionState (ACTION_DRAG_LAUNCHER);
    }
    else
    {
      LauncherIcon *drag_icon = MouseIconIntersection ((int) (GetGeometry ().x / 2.0f), y);
      
      // FIXME: nux doesn't give nux::GetEventButton (button_flags) there, relying
      // on an internal Launcher property then
      if (drag_icon && (_last_button_press == 1) && _model->IconHasSister (drag_icon))
      {
        StartIconDrag (drag_icon);
        SetActionState (ACTION_DRAG_ICON);
        UpdateDragWindowPosition (x, y);
      }

    }
  }
  else if (GetActionState () == ACTION_DRAG_LAUNCHER)
  {
    _launcher_drag_delta += dy;
  }
  else if (GetActionState () == ACTION_DRAG_ICON)
  {
    UpdateDragWindowPosition (x, y);
  }
  
  EnsureAnimation ();
}

void Launcher::RecvMouseEnter(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  SetMousePosition (x, y);
  _mouse_inside_launcher = true;

  EnsureHoverState ();

  EventLogic ();
  EnsureAnimation ();
}

void Launcher::RecvMouseLeave(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  SetMousePosition (x, y);
  _mouse_inside_launcher = false;
      
  if (GetActionState () == ACTION_NONE)
      EnsureHoverState ();

  // exit immediatly on action and mouse leaving the launcher (avoid loop when called manually)
  if (!_mouseover_launcher_locked && (x != -1) && (y != -1))
  {
    if (_autohide_handle > 0)
      g_source_remove (_autohide_handle);
    _autohide_handle = 0;
    EnsureHiddenState ();
  }

  EventLogic ();
  EnsureAnimation ();
}

void Launcher::RecvMouseMove(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags)
{
  SetMousePosition (x, y);

  // Every time the mouse moves, we check if it is inside an icon...
  EventLogic ();
}

void Launcher::RecvMouseWheel(int x, int y, int wheel_delta, unsigned long button_flags, unsigned long key_flags)
{
  if (!_hovered)
    return;
  
  if (wheel_delta < 0)
  {
    // scroll up
    _launcher_drag_delta += 10;
  }
  else
  {
    // scroll down
    _launcher_drag_delta -= 10;
  }
  
  EnsureAnimation ();
}

void
Launcher::CheckSuperShortcutPressed (unsigned int key_sym,
                                     unsigned long key_code,
                                     unsigned long key_state)
{
  if (_super_show_launcher)
  {
    RecvKeyPressed (key_sym, key_code, key_state);
    QueueDraw ();
  }
}

void
Launcher::RecvKeyPressed (unsigned int  key_sym,
                          unsigned long key_code,
                          unsigned long key_state)
{

  LauncherModel::iterator it;
  
  switch (key_sym)
  {
    // up (move selection up or go to global-menu if at top-most icon)
    case NUX_VK_UP:
      if (_current_icon_index > 0)
        _current_icon_index--;
      else
      {
        _current_icon_index = -1;
        // FIXME: switch to global-menu here still needs to be implemented 
      }
      NeedRedraw ();
      selection_change.emit ();
    break;

    // down (move selection down and unfold launcher if needed)
    case NUX_VK_DOWN:
      if (_current_icon_index < _model->Size ())
      {
        _current_icon_index++;
        NeedRedraw ();
        selection_change.emit ();
      }
    break;

    // esc/left (close quicklist or exit laucher key-focus)
    case NUX_VK_LEFT:
    case NUX_VK_ESCAPE:
      // hide again
      exitKeyNavMode ();
    break;

    // right/shift-f10 (open quicklist of currently selected icon)      
    case XK_F10:
      if (key_state & NUX_STATE_SHIFT)
      {
        {
          // open quicklist of currently selected icon
          it = _model->at (_current_icon_index);
          if (it != (LauncherModel::iterator)NULL)
            (*it)->OpenQuicklist (true);
        }
        leaveKeyNavMode ();
      }
    break;

    case NUX_VK_RIGHT:
      {
        // open quicklist of currently selected icon
        it = _model->at (_current_icon_index);
        if (it != (LauncherModel::iterator)NULL)
          (*it)->OpenQuicklist (true);
      }
      leaveKeyNavMode ();
    break;

    // <SPACE> (open a new instance)
    case NUX_VK_SPACE:
      {
        // start currently selected icon
        it = _model->at (_current_icon_index);
        if (it != (LauncherModel::iterator)NULL)
          (*it)->OpenInstance ();
      }
      leaveKeyNavMode ();
      break;

    // <RETURN> (start/activate currently selected icon)
    case NUX_VK_ENTER:
      {
        // start currently selected icon
        it = _model->at (_current_icon_index);
        if (it != (LauncherModel::iterator)NULL)
          (*it)->Activate ();
      }
      leaveKeyNavMode (false);
    break;
      
    // Shortcut to start launcher icons. Only relies on Keycode, ignore modifier
    default:
    {
        if (_super_show_launcher && !TapOnSuper ())
        {
            int i;
            for (it = _model->begin (), i = 0; it != _model->end (); it++, i++)
            {
              if (XKeysymToKeycode (screen->dpy (), (*it)->GetShortcut ()) == key_code)
              {
                if (g_ascii_isdigit ((gchar) (*it)->GetShortcut ()) && (key_state & ShiftMask))
                  (*it)->OpenInstance ();
                else
                  (*it)->Activate ();
              }
            }
        }
      
      
    }
    break;
  }
}

void Launcher::RecvQuicklistOpened (QuicklistView *quicklist)
{
  EventLogic ();
  EnsureHoverState ();
  EnsureAnimation ();
}

void Launcher::RecvQuicklistClosed (QuicklistView *quicklist)
{
  SetupAutohideTimer ();

  EventLogic ();
  EnsureHoverState ();
  EnsureAnimation ();
}

void Launcher::EventLogic ()
{
  if (GetActionState () == ACTION_DRAG_ICON ||
      GetActionState () == ACTION_DRAG_LAUNCHER)
    return;

  LauncherIcon* launcher_icon = 0;

  if (_mouse_inside_launcher)
    launcher_icon = MouseIconIntersection (_mouse_position.x, _mouse_position.y);

  if (_icon_under_mouse && (_icon_under_mouse != launcher_icon))
  {
    _icon_under_mouse->MouseLeave.emit ();
    _icon_under_mouse->_mouse_inside = false;
    _icon_under_mouse = 0;
  }

  if (launcher_icon && (_icon_under_mouse != launcher_icon))
  {
    launcher_icon->MouseEnter.emit ();
    launcher_icon->_mouse_inside = true;
    _icon_under_mouse = launcher_icon;
    // reset trigger has the mouse moved to another item (only if the launcher is supposed to be seen)
    if (!_hidden)
      _mouseover_launcher_locked = true;
  }
}

void Launcher::MouseDownLogic (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  LauncherIcon* launcher_icon = 0;
  launcher_icon = MouseIconIntersection (_mouse_position.x, _mouse_position.y);

  if (launcher_icon)
  {
    _icon_mouse_down = launcher_icon;
    launcher_icon->MouseDown.emit (nux::GetEventButton (button_flags));
  }
}

void Launcher::MouseUpLogic (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  LauncherIcon* launcher_icon = 0;
  launcher_icon = MouseIconIntersection (_mouse_position.x, _mouse_position.y);

  if (_icon_mouse_down && (_icon_mouse_down == launcher_icon))
  {
    _icon_mouse_down->MouseUp.emit (nux::GetEventButton (button_flags));

    if (GetActionState () == ACTION_NONE)
      _icon_mouse_down->MouseClick.emit (nux::GetEventButton (button_flags));
  }

  if (launcher_icon && (_icon_mouse_down != launcher_icon))
  {
    launcher_icon->MouseUp.emit (nux::GetEventButton (button_flags));
  }

  if (GetActionState () == ACTION_DRAG_LAUNCHER)
  {
    SetTimeStruct (&_times[TIME_DRAG_END]);
  }

  _icon_mouse_down = 0;
}

LauncherIcon* Launcher::MouseIconIntersection (int x, int y)
{
  LauncherModel::iterator it;
  LauncherModel::reverse_iterator rev_it;
  // We are looking for the icon at screen coordinates x, y;
  nux::Point2 mouse_position(x, y);
  int inside = 0;

  // Because of the way icons fold and stack on one another, we must proceed in 2 steps.
  for (rev_it = _model->rbegin (); rev_it != _model->rend (); rev_it++)
  {
    if ((*rev_it)->_folding_angle < 0.0f || !(*rev_it)->GetQuirk (LauncherIcon::QUIRK_VISIBLE))
      continue;

    nux::Point2 screen_coord [4];
    for (int i = 0; i < 4; i++)
    {
      screen_coord [i].x = (*rev_it)->_xform_coords["HitArea"] [i].x;
      screen_coord [i].y = (*rev_it)->_xform_coords["HitArea"] [i].y;
    }
    inside = PointInside2DPolygon (screen_coord, 4, mouse_position, 1);
    if (inside)
      return (*rev_it);
  }

  for (it = _model->begin(); it != _model->end (); it++)
  {
    if ((*it)->_folding_angle >= 0.0f || !(*it)->GetQuirk (LauncherIcon::QUIRK_VISIBLE))
      continue;

    nux::Point2 screen_coord [4];
    for (int i = 0; i < 4; i++)
    {
      screen_coord [i].x = (*it)->_xform_coords["HitArea"] [i].x;
      screen_coord [i].y = (*it)->_xform_coords["HitArea"] [i].y;
    }
    inside = PointInside2DPolygon (screen_coord, 4, mouse_position, 1);
    if (inside)
      return (*it);
  }

  return 0;
}

void Launcher::SetIconXForm (LauncherIcon *icon, nux::Matrix4 ViewProjectionMatrix, nux::Geometry geo,
                             float x, float y, float w, float h, float z, std::string name)
{
  nux::Vector4 v0 = nux::Vector4(x,   y,    z, 1.0f);
  nux::Vector4 v1 = nux::Vector4(x,   y+h,  z, 1.0f);
  nux::Vector4 v2 = nux::Vector4(x+w, y+h,  z, 1.0f);
  nux::Vector4 v3 = nux::Vector4(x+w, y,    z, 1.0f);

  v0 = ViewProjectionMatrix * v0;
  v1 = ViewProjectionMatrix * v1;
  v2 = ViewProjectionMatrix * v2;
  v3 = ViewProjectionMatrix * v3;

  v0.divide_xyz_by_w();
  v1.divide_xyz_by_w();
  v2.divide_xyz_by_w();
  v3.divide_xyz_by_w();

  // normalize to the viewport coordinates and translate to the correct location
  v0.x =  geo.width *(v0.x + 1.0f)/2.0f - geo.width/2.0f + x + w/2.0f;
  v0.y = -geo.height*(v0.y - 1.0f)/2.0f - geo.height/2.0f + y + h/2.0f;
  v1.x =  geo.width *(v1.x + 1.0f)/2.0f - geo.width/2.0f + x + w/2.0f;;
  v1.y = -geo.height*(v1.y - 1.0f)/2.0f - geo.height/2.0f + y + h/2.0f;
  v2.x =  geo.width *(v2.x + 1.0f)/2.0f - geo.width/2.0f + x + w/2.0f;
  v2.y = -geo.height*(v2.y - 1.0f)/2.0f - geo.height/2.0f + y + h/2.0f;
  v3.x =  geo.width *(v3.x + 1.0f)/2.0f - geo.width/2.0f + x + w/2.0f;
  v3.y = -geo.height*(v3.y - 1.0f)/2.0f - geo.height/2.0f + y + h/2.0f;


  nux::Vector4* vectors = icon->_xform_coords[name];

  vectors[0].x = v0.x;
  vectors[0].y = v0.y;
  vectors[0].z = v0.z;
  vectors[0].w = v0.w;
  vectors[1].x = v1.x;
  vectors[1].y = v1.y;
  vectors[1].z = v1.z;
  vectors[1].w = v1.w;
  vectors[2].x = v2.x;
  vectors[2].y = v2.y;
  vectors[2].z = v2.z;
  vectors[2].w = v2.w;
  vectors[3].x = v3.x;
  vectors[3].y = v3.y;
  vectors[3].z = v3.z;
  vectors[3].w = v3.w;
}

void Launcher::SetIconSectionXForm (LauncherIcon *icon, nux::Matrix4 ViewProjectionMatrix, nux::Geometry geo,
                             float x, float y, float w, float h, float z, float xx, float yy, float ww, float hh, std::string name)
{
  nux::Vector4 v0 = nux::Vector4(x,   y,    z, 1.0f);
  nux::Vector4 v1 = nux::Vector4(x,   y+h,  z, 1.0f);
  nux::Vector4 v2 = nux::Vector4(x+w, y+h,  z, 1.0f);
  nux::Vector4 v3 = nux::Vector4(x+w, y,    z, 1.0f);

  v0 = ViewProjectionMatrix * v0;
  v1 = ViewProjectionMatrix * v1;
  v2 = ViewProjectionMatrix * v2;
  v3 = ViewProjectionMatrix * v3;

  v0.divide_xyz_by_w();
  v1.divide_xyz_by_w();
  v2.divide_xyz_by_w();
  v3.divide_xyz_by_w();

  // normalize to the viewport coordinates and translate to the correct location
  v0.x =  geo.width *(v0.x + 1.0f)/2.0f - geo.width /2.0f + xx + ww/2.0f;
  v0.y = -geo.height*(v0.y - 1.0f)/2.0f - geo.height/2.0f + yy + hh/2.0f;
  v1.x =  geo.width *(v1.x + 1.0f)/2.0f - geo.width /2.0f + xx + ww/2.0f;;
  v1.y = -geo.height*(v1.y - 1.0f)/2.0f - geo.height/2.0f + yy + hh/2.0f;
  v2.x =  geo.width *(v2.x + 1.0f)/2.0f - geo.width /2.0f + xx + ww/2.0f;
  v2.y = -geo.height*(v2.y - 1.0f)/2.0f - geo.height/2.0f + yy + hh/2.0f;
  v3.x =  geo.width *(v3.x + 1.0f)/2.0f - geo.width /2.0f + xx + ww/2.0f;
  v3.y = -geo.height*(v3.y - 1.0f)/2.0f - geo.height/2.0f + yy + hh/2.0f;


  nux::Vector4* vectors = icon->_xform_coords[name];

  vectors[0].x = v0.x;
  vectors[0].y = v0.y;
  vectors[0].z = v0.z;
  vectors[0].w = v0.w;
  vectors[1].x = v1.x;
  vectors[1].y = v1.y;
  vectors[1].z = v1.z;
  vectors[1].w = v1.w;
  vectors[2].x = v2.x;
  vectors[2].y = v2.y;
  vectors[2].z = v2.z;
  vectors[2].w = v2.w;
  vectors[3].x = v3.x;
  vectors[3].y = v3.y;
  vectors[3].z = v3.z;
  vectors[3].w = v3.w;
}


void Launcher::UpdateIconXForm (std::list<Launcher::RenderArg> &args)
{
  nux::Geometry geo = GetGeometry ();
  nux::Matrix4 ObjectMatrix;
  nux::Matrix4 ViewMatrix;
  nux::Matrix4 ProjectionMatrix;
  nux::Matrix4 ViewProjectionMatrix;

  GetInverseScreenPerspectiveMatrix(ViewMatrix, ProjectionMatrix, geo.width, geo.height, 0.1f, 1000.0f, DEGTORAD(90));

  //LauncherModel::iterator it;
  std::list<Launcher::RenderArg>::iterator it;
  for(it = args.begin(); it != args.end(); it++)
  {

    LauncherIcon* launcher_icon = (*it).icon;

    // We to store the icon angle in the icons itself. Makes one thing easier afterward.
    launcher_icon->_folding_angle = (*it).x_rotation;

    float w = _icon_size;
    float h = _icon_size;
    float x = (*it).render_center.x - w/2.0f; // x: top left corner
    float y = (*it).render_center.y - h/2.0f; // y: top left corner
    float z = (*it).render_center.z;

    if ((*it).skip)
    {
      w = 1;
      h = 1;
      x = -100;
      y = -100;
    }
    
    ObjectMatrix = nux::Matrix4::TRANSLATE(geo.width/2.0f, geo.height/2.0f, z) * // Translate the icon to the center of the viewport
      nux::Matrix4::ROTATEX((*it).x_rotation) *              // rotate the icon
      nux::Matrix4::ROTATEY((*it).y_rotation) *
      nux::Matrix4::ROTATEZ((*it).z_rotation) *
      nux::Matrix4::TRANSLATE(-x - w/2.0f, -y - h/2.0f, -z);    // Put the center the icon to (0, 0)

    ViewProjectionMatrix = ProjectionMatrix*ViewMatrix*ObjectMatrix;

    SetIconXForm (launcher_icon, ViewProjectionMatrix, geo, x, y, w, h, z, "Tile");

    w = _icon_image_size;
    h = _icon_image_size;
    x = (*it).render_center.x - _icon_size/2.0f + _icon_image_size_delta/2.0f;
    y = (*it).render_center.y - _icon_size/2.0f + _icon_image_size_delta/2.0f;
    z = (*it).render_center.z;

    SetIconXForm (launcher_icon, ViewProjectionMatrix, geo, x, y, w, h, z, "Image");

    w = _icon_glow_size;
    h = _icon_glow_size;
    x = (*it).render_center.x - _icon_glow_size/2.0f;
    y = (*it).render_center.y - _icon_glow_size/2.0f;
    z = (*it).render_center.z;

    SetIconXForm (launcher_icon, ViewProjectionMatrix, geo, x, y, w, h, z, "Glow");

    w = geo.width + 2;
    h = _icon_size + _space_between_icons;
    x = (*it).logical_center.x - w/2.0f;
    y = (*it).logical_center.y - h/2.0f;
    z = (*it).logical_center.z;

    SetIconXForm (launcher_icon, ViewProjectionMatrix, geo, x, y, w, h, z, "HitArea");
    
    if (launcher_icon->Emblem ())
    {
      nux::BaseTexture *emblem = launcher_icon->Emblem ();
      
      float w = _icon_size;
      float h = _icon_size;
    
      float emb_w = emblem->GetWidth ();
      float emb_h = emblem->GetHeight ();
      x = (*it).render_center.x - _icon_size/2.0f; // x = top left corner position of emblem
      y = (*it).render_center.y - _icon_size/2.0f;     // y = top left corner position of emblem
      z = (*it).render_center.z;
      
      ObjectMatrix = nux::Matrix4::TRANSLATE(geo.width/2.0f, geo.height/2.0f, z) * // Translate the icon to the center of the viewport
      nux::Matrix4::ROTATEX((*it).x_rotation) *              // rotate the icon
      nux::Matrix4::ROTATEY((*it).y_rotation) *
      nux::Matrix4::ROTATEZ((*it).z_rotation) *
      nux::Matrix4::TRANSLATE(-((*it).render_center.x - w/2.0f) - w/2.0f, -((*it).render_center.y - h/2.0f) - h/2.0f, -z);    // Put the center the icon to (0, 0)

      ViewProjectionMatrix = ProjectionMatrix*ViewMatrix*ObjectMatrix;

      SetIconSectionXForm (launcher_icon, ViewProjectionMatrix, geo, x, y, emb_w, emb_h, z,
                           (*it).render_center.x - w/2.0f, (*it).render_center.y - h/2.0f, w, h, "Emblem");
    }
  }
}

void GetInverseScreenPerspectiveMatrix(nux::Matrix4& ViewMatrix, nux::Matrix4& PerspectiveMatrix,
                                       int ViewportWidth,
                                       int ViewportHeight,
                                       float NearClipPlane,
                                       float FarClipPlane,
                                       float Fovy)
{
/*
  Objective:
  Given a perspective matrix defined by (Fovy, AspectRatio, NearPlane, FarPlane), we want to compute
  the ModelView matrix that transform a quad defined by its top-left coord (0, 0) and its
  bottom-right coord (WindowWidth, WindowHeight) into a full screen quad that covers the entire viewport (one to one).
  Any quad that is facing the camera and whose 4 points are on the 4 guiding line of the view frustum (pyramid)
  will always cover the entire viewport one to one (when projected on the screen).
  So all we have to do is to define a quad with x:[-x_cs, x_cs] and y:[-y_cs, y_cs] and find the z distance in eye space (z_cs) so that
  the quad touches the 4 guiding lines of the view frustum.
  We consider a well centered projection view (no skewing, no oblique clipping plane, ...) and derive the following equations:
     x_cs = AspectRatio*y_cs
     y_cs/z_cs = tanf(Fovy/2) ==> z_cs = y_cs*1/tanf(Fovy/2) (this is the absolute value the quad depth value will be -z_cs since we are using OpenGL right hand coord system).

  The quad (in camera space) facing the camera and centered around the camera view axis is defined by the points (-x_cs, y_cs) (top-left)
  and the point (x_cs, -y_cs) (bottom-right). If we move that quad along the camera view axis and place it at a distance z_cs of the camera,
  then its 4 corners are each on the 4 lines of the view frustum.

    (-x_cs, y_cs)          Camera Space
                 ^
       __________|__________
      |          |          |
      |          |          |
      |          |(0, 0)    |
      |----------|----------|->
      |          |          |
      |          |          |
      |__________|__________|
                         (x_cs, -y_cs)

  The full-screen quad (in screen space) is defined by the point (0, 0) (top-left) and (WindowWidth, WindowHeight) (bottom-right).
  We can choose and arbitrary value y_cs and compute the z_cs position in camera space that will produce a quad in camera space that projects into
  the full-screen space.

    (0, 0)            Screen Space
       _____________________->
      |          |          |
      |          |          |
      |          |          |
      |----------|----------|
      |          |          |
      |          |          |
      |__________|__________|
      v                   (WindowWidth, WindowHeight)

  The model view matrix is the succession of transformation that convert the quad (0, 0, WindowWidth, WindowHeight) into
  the quad (-x_cs, y_cs, x_cs, -y_cs)

  Screen Space           Camera Space
       x        ---->    x_ = x*2*x_cs/WindowWidth - x_cs
       y        ---->    y_ = -y*2*y_cs/WindowHeight + y_cs
       z        ---->    z_ = A*z -y_cs*1/tanf(Fovy/2)
     where A is a coefficient that can attenuate the rate of change in depth when the quad moves along the camera axis

  If the following is the projection matrix:

  (a, 0,  0, 0)     a = 1/(AspectRatio*tan(Fovy/2))
  (0, b,  0, 0)     b = 1/tan(Fovy/2)
  (0, 0,  c, d)
  (0, 0, -1, 0)

  and the camera space vertex is (x_cs, y_cs, z_cs, w_cs) projects to the top left corner of the view port on the screen ((-1, 1) in clip space), then
    x_cs*a/(-z_cs) = -1; |        z_cs = x_cs*a           x_cs*a = -y_cs*b  ==> x_cs = y_cs*AspectRatio
                         |  ==>                     ==>
    y_cs*b/(-z_cs) = +1; |        z_cs = -y_cs*b          z_cs = -y_cs*1/tanf(Fovy/2)
*/


  float AspectRatio = (float)ViewportWidth/(float)ViewportHeight;
  float CameraToScreenDistance = -1.0f;
  float y_cs = -CameraToScreenDistance*tanf(0.5f*Fovy/* *M_PI/180.0f*/);
  float x_cs = y_cs*AspectRatio;

  ViewMatrix = nux::Matrix4::TRANSLATE(-x_cs, y_cs, CameraToScreenDistance) *
    nux::Matrix4::SCALE(2.0f*x_cs/ViewportWidth, -2.0f*y_cs/ViewportHeight, -2.0f * 3 * y_cs/ViewportHeight /* or -2.0f * x_cs/ViewportWidth*/ );

  PerspectiveMatrix.Perspective(Fovy, AspectRatio, NearClipPlane, FarClipPlane);
}

void
Launcher::RenderIconToTexture (nux::GraphicsEngine& GfxContext, LauncherIcon *icon, nux::IntrusiveSP<nux::IOpenGLBaseTexture> texture)
{
  RenderArg arg;
  struct timespec current;
  clock_gettime (CLOCK_MONOTONIC, &current);
  
  SetupRenderArg (icon, current, arg);
  arg.render_center = nux::Point3 (_icon_size / 2.0f, _icon_size / 2.0f, 0.0f);
  arg.logical_center = arg.render_center;
  arg.x_rotation = 0.0f;
  arg.running_arrow = false;
  arg.active_arrow = false;
  arg.skip = false;
  arg.window_indicators = 0;
  arg.alpha = 1.0f;

  std::list<Launcher::RenderArg> drag_args;
  drag_args.push_front (arg);
  UpdateIconXForm (drag_args);
  
  SetOffscreenRenderTarget (texture);
  DrawRenderArg (nux::GetGraphicsEngine (), arg, nux::Geometry (0, 0, _icon_size, _icon_size));
  RestoreSystemRenderTarget ();
}

void
Launcher::RenderProgressToTexture (nux::GraphicsEngine& GfxContext, nux::IntrusiveSP<nux::IOpenGLBaseTexture> texture, float progress_fill, float bias)
{
  int width = texture->GetWidth ();
  int height = texture->GetHeight ();
  
  int progress_width = _progress_bar_trough->GetWidth ();
  int progress_height = _progress_bar_trough->GetHeight ();

  int fill_width = _progress_bar_fill->GetWidth ();
  int fill_height = _progress_bar_fill->GetHeight ();
  
  int fill_offset = (progress_width - fill_width) / 2;

  /* We need to perform a barn doors effect to acheive the slide in and out */

  int left_edge = width / 2 - progress_width / 2;
  int right_edge = width / 2 + progress_width / 2;
  
  if (bias < 0.0f)
  {
    // pulls the right edge in
    right_edge -= (int) (-bias * (float) progress_width);
  }
  else if (bias > 0.0f)
  {
    // pulls the left edge in
    left_edge += (int) (bias * progress_width);
  }
  
  int fill_y = (height - fill_height) / 2;
  int progress_y = (height - progress_height) / 2;
  int half_size = (right_edge - left_edge) / 2;
  
  SetOffscreenRenderTarget (texture);
  
  // FIXME
  glClear (GL_COLOR_BUFFER_BIT);
  nux::TexCoordXForm texxform;
  
  fill_width *= progress_fill;

  // left door
  GfxContext.PushClippingRectangle(nux::Geometry (left_edge, 0, half_size, height));
  
  GfxContext.QRP_1Tex (left_edge, progress_y, progress_width, progress_height, 
                            _progress_bar_trough->GetDeviceTexture (), texxform, nux::Color::White);
                            
  GfxContext.QRP_1Tex (left_edge + fill_offset, fill_y, fill_width, fill_height, 
                            _progress_bar_fill->GetDeviceTexture (), texxform, nux::Color::White);  

  GfxContext.PopClippingRectangle (); 


  // right door
  GfxContext.PushClippingRectangle(nux::Geometry (left_edge + half_size, 0, half_size, height));
  
  GfxContext.QRP_1Tex (right_edge - progress_width, progress_y, progress_width, progress_height, 
                            _progress_bar_trough->GetDeviceTexture (), texxform, nux::Color::White);
  
  GfxContext.QRP_1Tex (right_edge - progress_width + fill_offset, fill_y, fill_width, fill_height, 
                            _progress_bar_fill->GetDeviceTexture (), texxform, nux::Color::White);
  
  GfxContext.PopClippingRectangle (); 

  
  RestoreSystemRenderTarget ();
}

void 
Launcher::SetOffscreenRenderTarget (nux::IntrusiveSP<nux::IOpenGLBaseTexture> texture)
{
  int width = texture->GetWidth ();
  int height = texture->GetHeight ();
  
  nux::GetThreadGLDeviceFactory ()->FormatFrameBufferObject (width, height, nux::BITFMT_R8G8B8A8);
  nux::GetThreadGLDeviceFactory ()->SetColorRenderTargetSurface (0, texture->GetSurfaceLevel (0));
  nux::GetThreadGLDeviceFactory ()->ActivateFrameBuffer ();

  nux::GetThreadGraphicsContext ()->SetContext   (0, 0, width, height);
  nux::GetThreadGraphicsContext ()->SetViewport  (0, 0, width, height);
  nux::GetThreadGraphicsContext ()->Push2DWindow (width, height);
  nux::GetThreadGraphicsContext ()->EmptyClippingRegion();
}

void 
Launcher::RestoreSystemRenderTarget ()
{
  nux::GetWindowCompositor ().RestoreRenderingSurface ();
}

void 
Launcher::ProcessDndEnter ()
{
  _drag_data.clear ();
  _drag_action = nux::DNDACTION_NONE;
  _steal_drag = false;
  _data_checked = false;
  _drag_edge_touching = false;
  _dnd_hovered_icon = 0;
}

void 
Launcher::ProcessDndLeave ()
{
  _mouse_inside_launcher = false;
  _drag_edge_touching = false;

  SetActionState (ACTION_NONE);

  if (!_drag_data.empty ())
  {
    std::list<char *>::iterator it;
    for (it = _drag_data.begin (); it != _drag_data.end (); it++)
    {
      g_free (*it);
    }
  }
  _drag_data.clear ();
  
  LauncherModel::iterator it;
  for (it = _model->begin (); it != _model->end (); it++)
  {
    (*it)->SetQuirk (LauncherIcon::QUIRK_DROP_PRELIGHT, false);
    (*it)->SetQuirk (LauncherIcon::QUIRK_DROP_DIM, false);
  }
  
  if (_steal_drag && _dnd_hovered_icon)
  {
    _dnd_hovered_icon->SetQuirk (LauncherIcon::QUIRK_VISIBLE, false);
    _dnd_hovered_icon->remove.emit (_dnd_hovered_icon);
    
  }
  
  if (!_steal_drag && _dnd_hovered_icon)
  {
    _dnd_hovered_icon->SendDndLeave ();
    _dnd_hovered_icon = 0;
  }
  
  _steal_drag = false;
  _dnd_hovered_icon = 0;
  
  EnsureHoverState ();
}

std::list<char *>
Launcher::StringToUriList (char * input)
{
  std::list<char *> result;
  
  if (!input)
    return result;
  
  char ** imtrappedinastringfactory = g_strsplit (input, "\r\n", -1);
  int i = 0;
  while (imtrappedinastringfactory[i]) // get kinda bored
  {
    // empty string check
    if (imtrappedinastringfactory[i][0])
      result.push_back (g_strdup (imtrappedinastringfactory[i]));
    i++;
  }
  
  g_strfreev (imtrappedinastringfactory);
  
  return result;
}

void 
Launcher::ProcessDndMove (int x, int y, std::list<char *> mimes)
{
  std::list<char *>::iterator it;
  nux::Area *parent = GetToplevel ();
  char *remote_desktop_path = NULL;
  char *uri_list_const = g_strdup ("text/uri-list");
  
  if (!_data_checked)
  {
    _data_checked = true;
    _drag_data.clear ();
    
    // get the data
    for (it = mimes.begin (); it != mimes.end (); it++)
    {
      if (!g_str_equal (*it, uri_list_const))
        continue;
      
      _drag_data = StringToUriList (nux::GetWindow ().GetDndData (uri_list_const));
      break;
    }
    
    // see if the launcher wants this one
    for (it = _drag_data.begin (); it != _drag_data.end (); it++)
    {
      if (g_str_has_suffix (*it, ".desktop"))
      {
        remote_desktop_path = *it;
        _steal_drag = true;
        break;
      }
    }

    // only set hover once we know our first x/y
    SetActionState (ACTION_DRAG_EXTERNAL);
    _mouse_inside_launcher = true;
    
    LauncherModel::iterator it;
    for (it = _model->begin (); it != _model->end () && !_steal_drag; it++)
    {
      if ((*it)->QueryAcceptDrop (_drag_data) != nux::DNDACTION_NONE && !_steal_drag)
        (*it)->SetQuirk (LauncherIcon::QUIRK_DROP_PRELIGHT, true);
      else
        (*it)->SetQuirk (LauncherIcon::QUIRK_DROP_DIM, true);
    }
  
    EnsureHoverState ();
  }
  
  g_free (uri_list_const);
  
  SetMousePosition (x - parent->GetGeometry ().x, y - parent->GetGeometry ().y);
  
  if (_mouse_position.x == 0 && !_drag_edge_touching)
  {
    _drag_edge_touching = true;
    SetTimeStruct (&_times[TIME_DRAG_EDGE_TOUCH], &_times[TIME_DRAG_EDGE_TOUCH], ANIM_DURATION * 3);
    EnsureAnimation ();
  } 
  else if (_mouse_position.x != 0 && _drag_edge_touching)
  {
    _drag_edge_touching = false;
    SetTimeStruct (&_times[TIME_DRAG_EDGE_TOUCH], &_times[TIME_DRAG_EDGE_TOUCH], ANIM_DURATION * 3);
    EnsureAnimation ();
  }

  EventLogic ();
  LauncherIcon* hovered_icon = MouseIconIntersection (_mouse_position.x, _mouse_position.y);

  if (_steal_drag)
  {
    _drag_action = nux::DNDACTION_COPY;
    if (!_dnd_hovered_icon)
    {
      _dnd_hovered_icon = new SpacerLauncherIcon (this);
      _dnd_hovered_icon->SetSortPriority (G_MAXINT);
      _model->AddIcon (_dnd_hovered_icon);
      
      if (hovered_icon)
        _model->ReorderBefore (_dnd_hovered_icon, hovered_icon, true);
    }
    else if (hovered_icon)
    {
      _model->ReorderSmart (_dnd_hovered_icon, hovered_icon, true);
    }
  }
  else
  {
    if (hovered_icon != _dnd_hovered_icon)
    {
      if (hovered_icon)
      {
        hovered_icon->SendDndEnter ();
        _drag_action = hovered_icon->QueryAcceptDrop (_drag_data);
      }
      else
      {
        _drag_action = nux::DNDACTION_NONE;
      }
      
      if (_dnd_hovered_icon)
        _dnd_hovered_icon->SendDndLeave ();
        
      _dnd_hovered_icon = hovered_icon;
    }
  }
  
  bool accept;
  if (_drag_action != nux::DNDACTION_NONE)
    accept = true;
  else
    accept = false;

  SendDndStatus (accept, _drag_action, nux::Geometry (x, y, 1, 1));
}

void 
Launcher::ProcessDndDrop (int x, int y)
{
  if (_steal_drag)
  {
    char *path = 0;
    std::list<char *>::iterator it;
    
    for (it = _drag_data.begin (); it != _drag_data.end (); it++)
    {
      if (g_str_has_suffix (*it, ".desktop"))
      {
        if (g_str_has_prefix (*it, "application://"))
        {
          const char *tmp = *it + strlen ("application://");
          char *tmp2 = g_strdup_printf ("file:///usr/share/applications/%s", tmp);
          path = g_filename_from_uri (tmp2, NULL, NULL);
          g_free (tmp2);
          break;
        }
        else if (g_str_has_prefix (*it, "file://"))
        {
          path = g_filename_from_uri (*it, NULL, NULL);
          break;
        }
      }
    }
    
    if (path)
    {
      launcher_dropped.emit (path, _dnd_hovered_icon);
      g_free (path);
    }
  }
  else if (_dnd_hovered_icon && _drag_action != nux::DNDACTION_NONE)
  {
    _dnd_hovered_icon->AcceptDrop (_drag_data);
  }

  if (_drag_action != nux::DNDACTION_NONE)
    SendDndFinished (true, _drag_action);
  else
    SendDndFinished (false, _drag_action);
  
  // reset our shiz
  ProcessDndLeave ();
}


/*
 * Returns the current selected icon if it is in keynavmode
 * It will return NULL if it is not on keynavmode
 */
LauncherIcon*
Launcher::GetSelectedMenuIcon ()
{
  LauncherModel::iterator it;

  if (_current_icon_index == -1)
    return NULL;

  it = _model->at (_current_icon_index);

  return *it;
}
