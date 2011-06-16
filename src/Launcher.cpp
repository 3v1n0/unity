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

#define TRIGGER_SQR_RADIUS 25

#define MOUSE_DEADZONE 15

#define DRAG_OUT_PIXELS 300.0f

#define S_DBUS_NAME  "com.canonical.Unity.Launcher"
#define S_DBUS_PATH  "/com/canonical/Unity/Launcher"
#define S_DBUS_IFACE "com.canonical.Unity.Launcher"

// FIXME: key-code defines for Up/Down/Left/Right of numeric keypad - needs to
// be moved to the correct place in NuxGraphics-headers
#define NUX_KP_DOWN  0xFF99
#define NUX_KP_UP    0xFF97
#define NUX_KP_LEFT  0xFF96
#define NUX_KP_RIGHT 0xFF98

// halfed lumin values to provide darkening while desaturating in shader
#define LUMIN_R "0.15"
#define LUMIN_G "0.295"
#define LUMIN_B "0.055"

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

const gchar Launcher::introspection_xml[] =
  "<node>"
  "  <interface name='com.canonical.Unity.Launcher'>"
  ""
  "    <method name='AddLauncherItemFromPosition'>"
  "      <arg type='s' name='icon' direction='in'/>"
  "      <arg type='s' name='title' direction='in'/>"
  "      <arg type='i' name='icon_x' direction='in'/>"
  "      <arg type='i' name='icon_y' direction='in'/>"
  "      <arg type='i' name='icon_size' direction='in'/>"
  "      <arg type='s' name='desktop_file' direction='in'/>"
  "      <arg type='s' name='aptdaemon_task' direction='in'/>"
  "    </method>"
  ""
  "  </interface>"
  "</node>";
  
GDBusInterfaceVTable Launcher::interface_vtable =
{
  Launcher::handle_dbus_method_call,
  NULL,
  NULL
};


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
uniform vec4 desat_factor;                                              \n\
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
  vec4 texel = color0 * SampleTexture(TextureObject0, tex);             \n\
  vec4 desat = vec4 ("LUMIN_R"*texel.r + "LUMIN_G"*texel.g + "LUMIN_B"*texel.b);       \n\
  vec4 final_color = (vec4 (1.0, 1.0, 1.0, 1.0) - desat_factor) * desat + desat_factor * texel;   \n\
  final_color.a = texel.a;                                              \n\
  gl_FragColor = final_color;                                           \n\
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
                            PARAM factor = program.local[1];            \n\
                            PARAM luma = {"LUMIN_R", "LUMIN_G", "LUMIN_B", 0.0};       \n\
                            TEMP temp;                                  \n\
                            TEMP pcoord;                                \n\
                            TEMP tex0;                                  \n\
                            TEMP desat;                                 \n\
                            TEMP color;                                 \n\
                            MOV pcoord, fragment.texcoord[0].w;         \n\
                            RCP temp, fragment.texcoord[0].w;           \n\
                            MUL pcoord.xy, fragment.texcoord[0], temp;  \n\
                            TEX tex0, pcoord, texture[0], 2D;           \n\
                            MUL color, color0, tex0;                    \n\
                            DP4 desat, luma, color;                     \n\
                            LRP result.color.rgb, factor.x, color, desat;    \n\
                            MOV result.color.a, color;    \n\
                            END");

nux::NString PerspectiveCorrectTexRectFrg = TEXT (
                            "!!ARBfp1.0                                 \n\
                            PARAM color0 = program.local[0];            \n\
                            PARAM factor = program.local[1];            \n\
                            PARAM luma = {"LUMIN_R", "LUMIN_G", "LUMIN_B", 0.0};       \n\
                            TEMP temp;                                  \n\
                            TEMP pcoord;                                \n\
                            TEMP tex0;                                  \n\
                            MOV pcoord, fragment.texcoord[0].w;         \n\
                            RCP temp, fragment.texcoord[0].w;           \n\
                            MUL pcoord.xy, fragment.texcoord[0], temp;  \n\
                            TEX tex0, pcoord, texture[0], RECT;         \n\
                            MUL color, color0, tex0;                    \n\
                            DP4 desat, luma, color;                     \n\
                            LRP result.color.rgb, factor.x, color, desat;    \n\
                            MOV result.color.a, color;    \n\
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
    
    _hide_machine = new LauncherHideMachine ();
    _set_hidden_connection = (sigc::connection) _hide_machine->should_hide_changed.connect (sigc::mem_fun (this, &Launcher::SetHidden));    
    _hover_machine = new LauncherHoverMachine ();
    _set_hover_connection = (sigc::connection) _hover_machine->should_hover_changed.connect (sigc::mem_fun (this, &Launcher::SetHover));
    
    _launcher_animation_timeout = 0;

    m_Layout = new nux::HLayout(NUX_TRACKER_LOCATION);

    OnMouseDown.connect (sigc::mem_fun (this, &Launcher::RecvMouseDown));
    OnMouseUp.connect (sigc::mem_fun (this, &Launcher::RecvMouseUp));
    OnMouseDrag.connect (sigc::mem_fun (this, &Launcher::RecvMouseDrag));
    OnMouseEnter.connect (sigc::mem_fun (this, &Launcher::RecvMouseEnter));
    OnMouseLeave.connect (sigc::mem_fun (this, &Launcher::RecvMouseLeave));
    OnMouseMove.connect (sigc::mem_fun (this, &Launcher::RecvMouseMove));
    OnMouseWheel.connect (sigc::mem_fun (this, &Launcher::RecvMouseWheel));
    OnKeyPressed.connect (sigc::mem_fun (this, &Launcher::RecvKeyPressed));
    OnMouseDownOutsideArea.connect (sigc::mem_fun (this, &Launcher::RecvMouseDownOutsideArea));
    //OnEndFocus.connect   (sigc::mem_fun (this, &Launcher::exitKeyNavMode));
    
    CaptureMouseDownAnyWhereElse (true);
    
    _recv_quicklist_opened_connection = (sigc::connection) QuicklistManager::Default ()->quicklist_opened.connect (sigc::mem_fun(this, &Launcher::RecvQuicklistOpened));
    _recv_quicklist_closed_connection = (sigc::connection) QuicklistManager::Default ()->quicklist_closed.connect (sigc::mem_fun(this, &Launcher::RecvQuicklistClosed));

    _on_window_maximized_intellihide_connection = (sigc::connection) PluginAdapter::Default ()->window_maximized.connect (sigc::mem_fun (this, &Launcher::OnWindowMaybeIntellihide));
    _on_window_restored_intellihide_connection = (sigc::connection) PluginAdapter::Default ()->window_restored.connect (sigc::mem_fun (this, &Launcher::OnWindowMaybeIntellihide));
    _on_window_unminimized_intellihide_connection = (sigc::connection) PluginAdapter::Default ()->window_unminimized.connect (sigc::mem_fun (this, &Launcher::OnWindowMaybeIntellihide));
    _on_window_mapped_intellihide_connection = (sigc::connection) PluginAdapter::Default ()->window_mapped.connect (sigc::mem_fun (this, &Launcher::OnWindowMaybeIntellihide));
    _on_window_unmapped_intellihide_connection = (sigc::connection) PluginAdapter::Default ()->window_unmapped.connect (sigc::mem_fun (this, &Launcher::OnWindowMaybeIntellihide));
    _on_window_shown_intellihide_connection = (sigc::connection) PluginAdapter::Default ()->window_shown.connect (sigc::mem_fun (this, &Launcher::OnWindowMaybeIntellihide));
    _on_window_hidden_intellihide_connection = (sigc::connection) PluginAdapter::Default ()->window_hidden.connect (sigc::mem_fun (this, &Launcher::OnWindowMaybeIntellihide));
    _on_window_resized_intellihide_connection = (sigc::connection) PluginAdapter::Default ()->window_resized.connect (sigc::mem_fun (this, &Launcher::OnWindowMaybeIntellihide));
    _on_window_moved_intellihide_connection = (sigc::connection) PluginAdapter::Default ()->window_moved.connect (sigc::mem_fun (this, &Launcher::OnWindowMaybeIntellihide));
    _on_window_focuschanged_intellihide_connection = (sigc::connection) PluginAdapter::Default ()->window_focus_changed.connect (sigc::mem_fun (this, &Launcher::OnWindowMaybeIntellihideDelayed));
    
    _on_window_mapped_connection = (sigc::connection) PluginAdapter::Default ()->window_mapped.connect (sigc::mem_fun (this, &Launcher::OnWindowMapped));
    _on_window_unmapped_connection = (sigc::connection) PluginAdapter::Default ()->window_unmapped.connect (sigc::mem_fun (this, &Launcher::OnWindowUnmapped));
    
    _on_initiate_spread_connection = (sigc::connection) PluginAdapter::Default ()->initiate_spread.connect (sigc::mem_fun (this, &Launcher::OnPluginStateChanged));
    _on_initiate_expo_connection = (sigc::connection) PluginAdapter::Default ()->initiate_expo.connect (sigc::mem_fun (this, &Launcher::OnPluginStateChanged));
    _on_terminate_spread_connection = (sigc::connection) PluginAdapter::Default ()->terminate_spread.connect (sigc::mem_fun (this, &Launcher::OnPluginStateChanged));
    _on_terminate_expo_connection = (sigc::connection) PluginAdapter::Default ()->terminate_expo.connect (sigc::mem_fun (this, &Launcher::OnPluginStateChanged));
    
    GeisAdapter *adapter = GeisAdapter::Default (screen);
    _on_drag_start_connection = (sigc::connection) adapter->drag_start.connect (sigc::mem_fun (this, &Launcher::OnDragStart));
    _on_drag_update_connection = (sigc::connection) adapter->drag_update.connect (sigc::mem_fun (this, &Launcher::OnDragUpdate));
    _on_drag_finish_connection = (sigc::connection) adapter->drag_finish.connect (sigc::mem_fun (this, &Launcher::OnDragFinish));

    // FIXME: not used, remove (with Get function) in O
    m_ActiveMenuIcon = NULL;
    m_LastSpreadIcon = NULL;

    _current_icon       = NULL;
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
    _autohide_animation     = FADE_AND_SLIDE;
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
    _launcher_drag_delta    = 0;
    _dnd_delta_y            = 0;
    _dnd_delta_x            = 0;

    _autoscroll_handle             = 0;
    _super_show_launcher_handle    = 0;
    _super_hide_launcher_handle    = 0;
    _super_show_shortcuts_handle   = 0;
    _start_dragicon_handle         = 0;
    _focus_keynav_handle           = 0;
    _dnd_check_handle              = 0;
    _ignore_repeat_shortcut_handle = 0;

    _latest_shortcut        = 0;
    _super_pressed          = false;
    _shortcuts_shown        = false;
    _floating               = false;
    _hovered                = false;
    _hidden                 = false;
    _render_drag_window     = false;
    _drag_edge_touching     = false;
    _keynav_activated       = false;
    _backlight_mode         = BACKLIGHT_NORMAL;
    _last_button_press      = 0;
    _selection_atom         = 0;
    _drag_out_id            = 0;
    _drag_out_delta_x       = 0.0f;
    
    // FIXME: remove
    _initial_drag_animation = false;
    
    _check_window_over_launcher   = true;
    _postreveal_mousemove_delta_x = 0;
    _postreveal_mousemove_delta_y = 0;

    // set them to 1 instead of 0 to avoid :0 in case something is racy
    _bfb_width = 1;
    _bfb_height = 1;

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

    for (unsigned int i = 0; i < G_N_ELEMENTS (_ubus_handles); i++)
      _ubus_handles[i] = 0;

    UBusServer *ubus = ubus_server_get_default ();
    _ubus_handles[0] = ubus_server_register_interest (ubus,
                                                     UBUS_PLACE_VIEW_SHOWN,
                                                     (UBusCallback) &Launcher::OnPlaceViewShown,
                                                     this);

    _ubus_handles[1] = ubus_server_register_interest (ubus,
                                                     UBUS_PLACE_VIEW_HIDDEN,
                                                     (UBusCallback)&Launcher::OnPlaceViewHidden,
                                                     this);

    _ubus_handles[2] = ubus_server_register_interest (ubus,
                                                     UBUS_HOME_BUTTON_BFB_UPDATE,
                                                     (UBusCallback) &Launcher::OnBFBUpdate,
                                                     this);
                                   
    _ubus_handles[3] = ubus_server_register_interest (ubus,
                                                     UBUS_LAUNCHER_ACTION_DONE,
                                                     (UBusCallback) &Launcher::OnActionDone,
                                                     this);
                                   
    _ubus_handles[4] = ubus_server_register_interest (ubus,
                                                     UBUS_HOME_BUTTON_BFB_DND_ENTER,
                                                     (UBusCallback) &Launcher::OnBFBDndEnter,
                                                     this);
    
    _dbus_owner = g_bus_own_name (G_BUS_TYPE_SESSION,
                                  S_DBUS_NAME,
                                  (GBusNameOwnerFlags) (G_BUS_NAME_OWNER_FLAGS_REPLACE | G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT),
                                  OnBusAcquired,
                                  OnNameAcquired,
                                  OnNameLost,
                                  this,
                                  NULL);

    _settings = g_settings_new ("com.canonical.Unity.Launcher");
    _settings_changed_id = g_signal_connect (
        _settings, "changed", (GCallback)(Launcher::SettingsChanged), this);
    SettingsChanged (_settings, (gchar *)"shows-on-edge", this);

    SetDndEnabled (false, true);
}

Launcher::~Launcher()
{
  for (int i = 0; i < MAX_SUPERKEY_LABELS; i++)
  {
    if (_superkey_labels[i])
      _superkey_labels[i]->UnReference ();
  }
  g_bus_unown_name (_dbus_owner);
  
  if (_dnd_check_handle)
    g_source_remove (_dnd_check_handle);
  if (_autoscroll_handle)
    g_source_remove (_autoscroll_handle);
  if (_focus_keynav_handle)
    g_source_remove (_focus_keynav_handle);
  if (_super_show_launcher_handle)
    g_source_remove (_super_show_launcher_handle);
  if (_super_show_shortcuts_handle)
    g_source_remove (_super_show_shortcuts_handle);
  if (_start_dragicon_handle)
    g_source_remove (_start_dragicon_handle);
  if (_ignore_repeat_shortcut_handle)
    g_source_remove (_ignore_repeat_shortcut_handle);
  if (_super_hide_launcher_handle)
    g_source_remove (_super_hide_launcher_handle);

  if (_settings_changed_id != 0)
    g_signal_handler_disconnect ((gpointer) _settings, _settings_changed_id);
  g_object_unref (_settings);

  // disconnect the huge number of signal-slot callbacks
  if (_set_hidden_connection.connected ())
    _set_hidden_connection.disconnect ();

  if (_set_hover_connection.connected ())
    _set_hover_connection.disconnect ();
  
  if (_recv_quicklist_opened_connection.connected ())
    _recv_quicklist_opened_connection.disconnect ();
  
  if (_recv_quicklist_closed_connection.connected ())
    _recv_quicklist_closed_connection.disconnect ();

  if (_on_window_maximized_intellihide_connection.connected ())
    _on_window_maximized_intellihide_connection.disconnect ();
  
  if (_on_window_restored_intellihide_connection.connected ())
    _on_window_restored_intellihide_connection.disconnect ();
  
  if (_on_window_unminimized_intellihide_connection.connected ())
    _on_window_unminimized_intellihide_connection.disconnect ();
  
  if (_on_window_mapped_intellihide_connection.connected ())
    _on_window_mapped_intellihide_connection.disconnect ();
  
  if (_on_window_unmapped_intellihide_connection.connected ())
    _on_window_unmapped_intellihide_connection.disconnect ();
  
  if (_on_window_shown_intellihide_connection.connected ())
    _on_window_shown_intellihide_connection.disconnect ();
  
  if (_on_window_hidden_intellihide_connection.connected ())
    _on_window_hidden_intellihide_connection.disconnect ();
  
  if (_on_window_resized_intellihide_connection.connected ())
    _on_window_resized_intellihide_connection.disconnect ();
  
  if (_on_window_moved_intellihide_connection.connected ())
    _on_window_moved_intellihide_connection.disconnect ();
    
  if (_on_window_moved_intellihide_connection.connected ())
    _on_window_moved_intellihide_connection.disconnect ();

  if (_on_window_mapped_connection.connected ())
    _on_window_mapped_connection.disconnect ();
  
  if (_on_window_unmapped_connection.connected ())
    _on_window_unmapped_connection.disconnect ();
  
  if (_on_initiate_spread_connection.connected ())
    _on_initiate_spread_connection.disconnect ();
  
  if (_on_initiate_expo_connection.connected ())
    _on_initiate_expo_connection.disconnect ();
  
  if (_on_terminate_spread_connection.connected ())
    _on_terminate_spread_connection.disconnect ();
  
  if (_on_terminate_expo_connection.connected ())
    _on_terminate_expo_connection.disconnect ();
  
  if (_on_drag_start_connection.connected ())
    _on_drag_start_connection.disconnect ();
  
  if (_on_drag_update_connection.connected ())
    _on_drag_update_connection.disconnect ();
  
  if (_on_drag_finish_connection.connected ())
    _on_drag_finish_connection.disconnect ();
    
  if (_launcher_animation_timeout > 0)
    g_source_remove (_launcher_animation_timeout);

  UBusServer* ubus = ubus_server_get_default ();
  for (unsigned int i = 0; i < G_N_ELEMENTS (_ubus_handles); i++)
  {
    if (_ubus_handles[i] != 0)
      ubus_server_unregister_interest (ubus, _ubus_handles[i]);
  }
  
  g_idle_remove_by_data (this);

  delete _hover_machine;
  delete _hide_machine;
}

/* Introspection */
const gchar *
Launcher::GetName ()
{
  return "Launcher";
}

void
Launcher::SettingsChanged (GSettings *settings, char *key, Launcher *self)
{
  bool show_on_edge = g_settings_get_boolean (settings, "shows-on-edge") ? true : false;
  self->_hide_machine->SetShowOnEdge (show_on_edge);
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

  double label_pos = double(_icon_size / 3.0f);
  double text_size = double(_icon_size / 4.0f);
  double label_x = label_pos;
  double label_y = label_pos;
  double label_w = label_pos;
  double label_h = label_pos;
  double label_r = 3.0f;

  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);
  cairo_scale (cr, 1.0f, 1.0f);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cg->DrawRoundedRectangle (cr, 1.0f, label_x, label_y, label_r, label_w, label_h);
  cairo_set_source_rgba (cr, 0.0f, 0.0f, 0.0f, 0.65f);
  cairo_fill (cr);

  layout = pango_cairo_create_layout (cr);
  g_object_get (settings, "gtk-font-name", &fontName, NULL);
  desc = pango_font_description_from_string (fontName);
  pango_font_description_set_absolute_size (desc, text_size * PANGO_SCALE);
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
Launcher::OnDragStart (GeisAdapter::GeisDragData *data)
{
  if (_drag_out_id && _drag_out_id == data->id)
    return;
    
  if (data->touches == 4)
  {
    _drag_out_id = data->id;
    if (_hidden)
    {
      _drag_out_delta_x = 0.0f;
    }
    else
    {
      _drag_out_delta_x = DRAG_OUT_PIXELS;
      _hide_machine->SetQuirk (LauncherHideMachine::MT_DRAG_OUT, false);
    }
  }
}

void 
Launcher::OnDragUpdate (GeisAdapter::GeisDragData *data)
{
  if (data->id == _drag_out_id)
  {
    _drag_out_delta_x = CLAMP (_drag_out_delta_x + data->delta_x, 0.0f, DRAG_OUT_PIXELS);
    EnsureAnimation ();
  }
}

void 
Launcher::OnDragFinish (GeisAdapter::GeisDragData *data)
{
  if (data->id == _drag_out_id)
  { 
    if (_drag_out_delta_x >= DRAG_OUT_PIXELS - 90.0f)
      _hide_machine->SetQuirk (LauncherHideMachine::MT_DRAG_OUT, true);
    SetTimeStruct (&_times[TIME_DRAG_OUT], &_times[TIME_DRAG_OUT], ANIM_DURATION_SHORT);
    _drag_out_id = 0;
    EnsureAnimation ();
  }
}

void
Launcher::startKeyNavMode ()
{
  SetStateKeyNav (true);
  _hide_machine->SetQuirk (LauncherHideMachine::LAST_ACTION_ACTIVATE, false);
  
  GrabKeyboard ();
  GrabPointer ();
  
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
  if (!(self->_keynav_activated))
    return false;

  if (self->_last_icon_index == -1) {
     self->_current_icon_index = 0;
   }
   else
     self->_current_icon_index = self->_last_icon_index;
   self->EnsureAnimation ();

   ubus_server_send_message (ubus_server_get_default (),
                             UBUS_LAUNCHER_START_KEY_NAV,
                             NULL);

   self->selection_change.emit ();
   self->_focus_keynav_handle = 0;

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
  if (!_keynav_activated)
    return;
  
  UnGrabKeyboard ();
  UnGrabPointer ();
  SetStateKeyNav (false);

  _current_icon_index = -1;
  _last_icon_index = _current_icon_index;
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
  char* hidequirks_mask = _hide_machine->DebugHideQuirks ();
  char* hoverquirks_mask = _hover_machine->DebugHoverQuirks ();
  
  g_variant_builder_add (builder, "{sv}", "hover-progress", g_variant_new_double ((double) GetHoverProgress (current)));
  g_variant_builder_add (builder, "{sv}", "dnd-exit-progress", g_variant_new_double ((double) DnDExitProgress (current)));
  g_variant_builder_add (builder, "{sv}", "autohide-progress", g_variant_new_double ((double) AutohideProgress (current)));

  g_variant_builder_add (builder, "{sv}", "dnd-delta", g_variant_new_int32 (_dnd_delta_y));
  g_variant_builder_add (builder, "{sv}", "floating", g_variant_new_boolean (_floating));
  g_variant_builder_add (builder, "{sv}", "hovered", g_variant_new_boolean (_hovered));
  g_variant_builder_add (builder, "{sv}", "hidemode", g_variant_new_int32 (_hidemode));
  g_variant_builder_add (builder, "{sv}", "hidden", g_variant_new_boolean (_hidden));
  g_variant_builder_add (builder, "{sv}", "hide-quirks", g_variant_new_string (hidequirks_mask));
  g_variant_builder_add (builder, "{sv}", "hover-quirks", g_variant_new_string (hoverquirks_mask));

  g_free (hidequirks_mask);
  g_free (hoverquirks_mask);
}

void Launcher::SetMousePosition (int x, int y)
{
    bool beyond_drag_threshold = MouseBeyondDragThreshold ();
    _mouse_position = nux::Point2 (x, y);
    
    if (beyond_drag_threshold != MouseBeyondDragThreshold ())
      SetTimeStruct (&_times[TIME_DRAG_THRESHOLD], &_times[TIME_DRAG_THRESHOLD], ANIM_DURATION_SHORT);
    
    EnsureScrollTimer ();
}

void Launcher::SetStateMouseOverLauncher (bool over_launcher)
{
    _hide_machine->SetQuirk (LauncherHideMachine::MOUSE_OVER_LAUNCHER, over_launcher);
    _hover_machine->SetQuirk (LauncherHoverMachine::MOUSE_OVER_LAUNCHER, over_launcher);
    
    if (over_launcher)
    {
      // avoid a race when the BFB doesn't see we are not over the trigger anymore
      _hide_machine->SetQuirk (LauncherHideMachine::MOUSE_OVER_TRIGGER, false);
    }
    else
    {
      // reset state for some corner case like x=0, show dash (leave event not received)
      _hide_machine->SetQuirk (LauncherHideMachine::MOUSE_OVER_ACTIVE_EDGE, false);
    }
}

void Launcher::SetStateMouseOverBFB (bool over_bfb)
{
    _hide_machine->SetQuirk (LauncherHideMachine::MOUSE_OVER_BFB, over_bfb);
    _hover_machine->SetQuirk (LauncherHoverMachine::MOUSE_OVER_BFB, over_bfb);
    
    // the case where it's x=0 isn't important here as OnBFBUpdate() isn't triggered
    if (over_bfb)
      _hide_machine->SetQuirk (LauncherHideMachine::MOUSE_OVER_ACTIVE_EDGE, false);
    // event not received like: mouse over trigger, press super -> dash here, put mouse away from trigger,
    // click to close
    else
      _hide_machine->SetQuirk (LauncherHideMachine::MOUSE_OVER_TRIGGER, false);
}

void Launcher::SetStateKeyNav (bool keynav_activated)
{
    _hide_machine->SetQuirk (LauncherHideMachine::KEY_NAV_ACTIVE, keynav_activated);
    _hover_machine->SetQuirk (LauncherHoverMachine::KEY_NAV_ACTIVE, keynav_activated);
  
    _keynav_activated = keynav_activated;
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

float Launcher::DragOutProgress (struct timespec const &current)
{
    float timeout = CLAMP ((float) (TimeDelta (&current, &_times[TIME_DRAG_OUT])) / (float) ANIM_DURATION_SHORT, 0.0f, 1.0f);
    float progress = CLAMP (_drag_out_delta_x / DRAG_OUT_PIXELS, 0.0f, 1.0f);
    
    if (_drag_out_id || _hide_machine->GetQuirk (LauncherHideMachine::MT_DRAG_OUT))
      return progress;
    return progress * (1.0f - timeout);
}

float Launcher::AutohideProgress (struct timespec const &current)
{
    
    // bfb position progress. Go from GetAutohidePositionMin() -> GetAutohidePositionMax() linearly
    if (_hide_machine->GetQuirk (LauncherHideMachine::MOUSE_OVER_BFB) && _hidden)
    {
      
       // Be evil, but safe: position based == removing all existing time-based autohide
       _times[TIME_AUTOHIDE].tv_sec = 0;
       _times[TIME_AUTOHIDE].tv_nsec = 0;
        
       /* 
        * most of the mouse movement should be done by the inferior part
        * of the launcher, so prioritize this one
        */
                
        float _max_size_on_position;
        float position_on_border = _bfb_mouse_position.x * _bfb_height / _bfb_mouse_position.y;
        
        if (position_on_border < _bfb_width)
            _max_size_on_position = pow(pow(position_on_border, 2) + pow(_bfb_height, 2), 0.5);
        else
        {
            position_on_border = _bfb_mouse_position.y * _bfb_width / _bfb_mouse_position.x;
            _max_size_on_position = pow(pow(position_on_border, 2) + pow(_bfb_width, 2), 0.5);
        }
        
        float _position_min = GetAutohidePositionMin ();
        return pow(pow(_bfb_mouse_position.x, 2) + pow(_bfb_mouse_position.y, 2), 0.5) / _max_size_on_position * (GetAutohidePositionMax () - _position_min) + _position_min;
    }
    else
    {
        // time-based progress (full scale or finish the TRIGGER_AUTOHIDE_MIN -> 0.00f on bfb)
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
    self->_launcher_animation_timeout = 0;
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
    
    time = icon->GetQuirkTime (LauncherIcon::QUIRK_DESAT);
    if (TimeDelta (&current, &time) < ANIM_DURATION_LONG)
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
    
    // drag end animation
    if (TimeDelta (&current, &_times[TIME_DRAG_END]) < ANIM_DURATION_LONG)
        return true;

    // hide animation (time only), position is trigger manually on the bfb
    if (TimeDelta (&current, &_times[TIME_AUTOHIDE]) < ANIM_DURATION_SHORT)
        return true;
    
    // collapse animation on DND out of launcher space
    if (TimeDelta (&current, &_times[TIME_DRAG_THRESHOLD]) < ANIM_DURATION_SHORT)
        return true;
    
    // hide animation for dnd
    if (TimeDelta (&current, &_times[TIME_DRAG_EDGE_TOUCH]) < ANIM_DURATION * 6)
        return true;
        
    // restore from drag_out animation
    if (TimeDelta (&current, &_times[TIME_DRAG_OUT]) < ANIM_DURATION_SHORT)
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
/* Min is when you are on the trigger */
float Launcher::GetAutohidePositionMin ()
{
    if (_autohide_animation == SLIDE_ONLY || _autohide_animation == FADE_AND_SLIDE)
        return 0.35f;
    else
        return 0.25f;
}
/* Max is the initial state over the bfb */
float Launcher::GetAutohidePositionMax ()
{
    if (_autohide_animation == SLIDE_ONLY || _autohide_animation == FADE_AND_SLIDE)
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

float Launcher::IconDesatValue (LauncherIcon *icon, struct timespec const &current)
{
    struct timespec dim_time = icon->GetQuirkTime (LauncherIcon::QUIRK_DESAT);
    int ms = TimeDelta (&current, &dim_time);
    float result = CLAMP ((float) ms / (float) ANIM_DURATION_LONG, 0.0f, 1.0f);

    if (icon->GetQuirk (LauncherIcon::QUIRK_DESAT))
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
    arg.saturation          = IconDesatValue (icon, current);
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
    {
      if (icon->GetQuirk (LauncherIcon::QUIRK_URGENT))
      {
        arg.running_arrow = true;
        arg.window_indicators = 1;
      }
      else
        arg.window_indicators = 0;
    }
    else
    {
      arg.window_indicators = icon->RelatedWindows ();
    }
    
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
      if (i == _current_icon_index && *it == icon) {
        arg.keyboard_nav_hl = true;
      }
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
            icon->IsSpacer ())
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

    icon_hide_offset *= 1.0f - (present_progress * (_hide_machine->GetShowOnEdge () ? icon->PresentUrgency () : 0.0f));

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
    
    // FIXME: this is a hack, we should have a look why SetAnimationTarget is necessary in SetAnimationTarget
    // we should ideally just need it at start to set the target
    if (!_initial_drag_animation && icon == _drag_icon && _drag_window && _drag_window->Animating ())
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
        float magic_constant = 1.3f;

        float present_progress = IconPresentProgress (*it, current);
        folding_threshold -= CLAMP (sum - launcher_height, 0.0f, height * magic_constant) * (folding_constant + (1.0f - folding_constant) * present_progress);
    }
    
    if (sum - _space_between_icons <= launcher_height)
      folding_threshold = launcher_height;

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
        
        float autohide_progress = AutohideProgress (current) * (1.0f - DragOutProgress (current));
        if (_autohide_animation == FADE_ONLY
            || (_autohide_animation == FADE_OR_SLIDE && _hide_machine->GetQuirk (LauncherHideMachine::MOUSE_OVER_BFB)))
            *launcher_alpha = 1.0f - autohide_progress;
        else
        {
            if (autohide_progress > 0.0f)
            {
                autohide_offset -= geo.width * autohide_progress;
                if (_autohide_animation == FADE_AND_SLIDE)
                    *launcher_alpha = 1.0f - 0.5f * autohide_progress;
            }
        }
    }
    
    float drag_hide_progress = DragHideProgress (current);
    if (_hidemode != LAUNCHER_HIDE_NEVER && drag_hide_progress > 0.0f)
    {
        autohide_offset -= geo.width * 0.25f * drag_hide_progress;
        
        if (drag_hide_progress >= 1.0f)
          _hide_machine->SetQuirk (LauncherHideMachine::DND_PUSHED_OFF, true);
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
    _super_pressed = true;
    _hide_machine->SetQuirk (LauncherHideMachine::LAST_ACTION_ACTIVATE, false);
    
    SetTimeStruct (&_times[TIME_TAP_SUPER]);
    SetTimeStruct (&_times[TIME_SUPER_PRESSED]);
    
    if (_super_show_launcher_handle > 0)
      g_source_remove (_super_show_launcher_handle);
    _super_show_launcher_handle = g_timeout_add (SUPER_TAP_DURATION, &Launcher::SuperShowLauncherTimeout, this);
    
    if (_super_show_shortcuts_handle > 0)
      g_source_remove (_super_show_shortcuts_handle);
    _super_show_shortcuts_handle = g_timeout_add (SHORTCUTS_SHOWN_DELAY, &Launcher::SuperShowShortcutsTimeout, this);
}

void Launcher::EndKeyShowLauncher ()
{
    int remaining_time_before_hide;
    struct timespec current;
    clock_gettime (CLOCK_MONOTONIC, &current);
 
    _hover_machine->SetQuirk (LauncherHoverMachine::SHORTCUT_KEYS_VISIBLE, false);
    _super_pressed = false;
    _shortcuts_shown = false;
    QueueDraw ();
    
    // remove further show launcher (which can happen when we close the dash with super)
    if (_super_show_launcher_handle > 0)
      g_source_remove (_super_show_launcher_handle);
    if (_super_show_shortcuts_handle > 0)
      g_source_remove (_super_show_shortcuts_handle);
    _super_show_launcher_handle = 0;
    _super_show_shortcuts_handle = 0;

    // it's a tap on super and we didn't use any shortcuts
    if (TapOnSuper () && !_latest_shortcut)
      ubus_server_send_message (ubus_server_get_default (), UBUS_DASH_EXTERNAL_ACTIVATION, NULL);
      
    remaining_time_before_hide = BEFORE_HIDE_LAUNCHER_ON_SUPER_DURATION - CLAMP ((int) (TimeDelta (&current, &_times[TIME_SUPER_PRESSED])), 0, BEFORE_HIDE_LAUNCHER_ON_SUPER_DURATION);
    
    if (_super_hide_launcher_handle > 0)
      g_source_remove (_super_hide_launcher_handle);
    _super_hide_launcher_handle = g_timeout_add (remaining_time_before_hide, &Launcher::SuperHideLauncherTimeout, this);
}

gboolean Launcher::SuperHideLauncherTimeout (gpointer data)
{
    Launcher *self = (Launcher*) data;
    
    self->_hide_machine->SetQuirk (LauncherHideMachine::TRIGGER_BUTTON_SHOW, false);
    
    self->_super_hide_launcher_handle = 0;
    return false;    
}

gboolean Launcher::SuperShowLauncherTimeout (gpointer data)
{
    Launcher *self = (Launcher*) data;
    
    self->_hide_machine->SetQuirk (LauncherHideMachine::TRIGGER_BUTTON_SHOW, true);
    
    self->_super_show_launcher_handle = 0;
    return false;    
}

gboolean Launcher::SuperShowShortcutsTimeout (gpointer data)
{
    Launcher *self = (Launcher*) data;
    
    self->_shortcuts_shown = true;
    self->_hover_machine->SetQuirk (LauncherHoverMachine::SHORTCUT_KEYS_VISIBLE, true);

    self->QueueDraw ();
    
    self->_super_show_shortcuts_handle = 0;
    return false;    
}

void Launcher::OnPlaceViewShown (GVariant *data, void *val)
{
    Launcher *self = (Launcher*)val;
    LauncherModel::iterator it;
    
    self->_hide_machine->SetQuirk (LauncherHideMachine::PLACES_VISIBLE, true);
    self->_hover_machine->SetQuirk (LauncherHoverMachine::PLACES_VISIBLE, true);
    
    // TODO: add in a timeout for seeing the animation (and make it smoother)
    for (it = self->_model->begin (); it != self->_model->end (); it++)
    {
      (*it)->SetQuirk (LauncherIcon::QUIRK_DESAT, true);
      (*it)->HideTooltip ();
    }
    
    // hack around issue in nux where leave events dont always come after a grab
    self->SetStateMouseOverBFB (false);
}

void Launcher::OnPlaceViewHidden (GVariant *data, void *val)
{
    Launcher *self = (Launcher*)val;
    LauncherModel::iterator it;
    
    self->_hide_machine->SetQuirk (LauncherHideMachine::PLACES_VISIBLE, false);
    self->_hover_machine->SetQuirk (LauncherHoverMachine::PLACES_VISIBLE, false);
    
    // as the leave event is no more received when the place is opened
    // FIXME: remove when we change the mouse grab strategy in nux
    self->SetStateMouseOverLauncher (pointerX < self->GetAbsoluteGeometry ().x + self->GetGeometry ().width &&
                                     pointerY >= self->GetAbsoluteGeometry ().y);
    self->SetStateMouseOverBFB (false);
    
    // TODO: add in a timeout for seeing the animation (and make it smoother)
    for (it = self->_model->begin (); it != self->_model->end (); it++)
    {
      (*it)->SetQuirk (LauncherIcon::QUIRK_DESAT, false);
    }
}

void Launcher::OnBFBDndEnter (GVariant *data, gpointer user_data)
{
  Launcher *self = static_cast<Launcher *> (user_data);
  self->_hide_machine->SetQuirk (LauncherHideMachine::DND_PUSHED_OFF, false);
}

void Launcher::OnBFBUpdate (GVariant *data, gpointer user_data)
{
  gchar        *prop_key;
  GVariant     *prop_value;
  GVariantIter *prop_iter;
  int x, y, bfb_width, bfb_height;

  Launcher *self = (Launcher*)user_data;
  
  g_variant_get (data, "(iiiia{sv})", &x, &y, &bfb_width, &bfb_height, &prop_iter);
  self->_bfb_mouse_position = nux::Point2 (x, y);
  
  bool inside_trigger_area = (pow (x, 2) + pow (y, 2) < TRIGGER_SQR_RADIUS) && x >= 0 && y >= 0;
  /*
   * if we are currently hidden and we are over the trigger, prepare the change
   * from a position-based move to a time-based one
   * Fake that we were currently hiding with a corresponding position of GetAutohidePositionMin ()
   * translated time-based so that we pick from the current position
   */
  if (inside_trigger_area && self->_hidden) {
      self->_hide_machine->SetQuirk (LauncherHideMachine::LAST_ACTION_ACTIVATE, false);

      struct timespec current;
      clock_gettime (CLOCK_MONOTONIC, &current);
      self->_times[TIME_AUTOHIDE] = current;
      SetTimeBack (&(self->_times[TIME_AUTOHIDE]), ANIM_DURATION_SHORT * (1.0f - self->GetAutohidePositionMin ()));
      SetTimeStruct (&(self->_times[TIME_AUTOHIDE]), &(self->_times[TIME_AUTOHIDE]), ANIM_DURATION_SHORT);
      self->_hide_machine->SetQuirk (LauncherHideMachine::MOUSE_MOVE_POST_REVEAL, true);
  }
  self->_hide_machine->SetQuirk (LauncherHideMachine::MOUSE_OVER_TRIGGER, inside_trigger_area);
  
  self->_bfb_width = bfb_width;
  self->_bfb_height = bfb_height;
    
  g_return_if_fail (prop_iter != NULL);

  while (g_variant_iter_loop (prop_iter, "{sv}", &prop_key, &prop_value))
  {
    if (g_str_equal ("hovered", prop_key))
    {
      self->SetStateMouseOverBFB (g_variant_get_boolean (prop_value));
      self->EnsureScrollTimer ();    
    }
  }
  
  self->EnsureAnimation ();
  
  g_variant_iter_free (prop_iter);
}

void Launcher::OnActionDone (GVariant *data, void *val)
{
  Launcher *self = (Launcher*)val;
  self->_hide_machine->SetQuirk (LauncherHideMachine::LAST_ACTION_ACTIVATE, true);
}

void Launcher::SetHidden (bool hidden)
{
    if (hidden == _hidden)
        return;
    
    _hidden = hidden;
    _hide_machine->SetQuirk (LauncherHideMachine::LAUNCHER_HIDDEN, hidden);
    _hover_machine->SetQuirk (LauncherHoverMachine::LAUNCHER_HIDDEN, hidden);

    _hide_machine->SetQuirk (LauncherHideMachine::LAST_ACTION_ACTIVATE, false);
    if (_hide_machine->GetQuirk (LauncherHideMachine::MOUSE_OVER_ACTIVE_EDGE))
      _hide_machine->SetQuirk (LauncherHideMachine::MOUSE_MOVE_POST_REVEAL, true);
    else
      _hide_machine->SetQuirk (LauncherHideMachine::MOUSE_MOVE_POST_REVEAL, false);
    
    if (hidden)
    {
      _hide_machine->SetQuirk (LauncherHideMachine::MT_DRAG_OUT, false);
      SetStateMouseOverLauncher  (false);
    }
    
    _postreveal_mousemove_delta_x = 0;
    _postreveal_mousemove_delta_y = 0;

    SetTimeStruct (&_times[TIME_AUTOHIDE], &_times[TIME_AUTOHIDE], ANIM_DURATION_SHORT);

    _parent->EnableInputWindow(!hidden, "launcher", false, false);

    if (!hidden && GetActionState () == ACTION_DRAG_EXTERNAL)
      ProcessDndLeave ();

    EnsureAnimation ();
    
    hidden_changed.emit ();
}

int
Launcher::GetMouseX ()
{
  return _mouse_position.x;
}

int
Launcher::GetMouseY ()
{
  return _mouse_position.y;
}

bool
Launcher::CheckIntersectWindow (CompWindow *window)
{
  nux::Geometry geo = GetAbsoluteGeometry ();
  int intersect_types = CompWindowTypeNormalMask | CompWindowTypeDialogMask |
                        CompWindowTypeModalDialogMask | CompWindowTypeUtilMask;

  if (!window || !(window->type () & intersect_types) || !window->isMapped () || !window->isViewable ())
    return false;

  if (CompRegion (window->borderRect ()).intersects (CompRect (geo.x, geo.y, geo.width, geo.height)))
    return true;

  return false;
}

void
Launcher::EnableCheckWindowOverLauncher (gboolean enabled)
{
    _check_window_over_launcher = enabled;
}

void
Launcher::CheckWindowOverLauncher ()
{
  CompWindowList window_list = _screen->windows ();
  CompWindowList::iterator it;
  CompWindow *window = NULL;
  CompWindow *parent = NULL;
  int type_dialogs = CompWindowTypeDialogMask | CompWindowTypeModalDialogMask 
                     | CompWindowTypeUtilMask;

  bool any = false;
  bool active = false;
  
  // state has no mean right now, the check will be done again later
  if (!_check_window_over_launcher)
    return;

  window = _screen->findWindow (_screen->activeWindow ());

  if (window && (window->type () & type_dialogs))
    parent = _screen->findWindow (window->transientFor ());

  if (CheckIntersectWindow (window) || CheckIntersectWindow (parent))
  {
    any = true;
    active = true;
  }
  else
  {
    for (it = window_list.begin (); it != window_list.end (); it++)
    {
      if (CheckIntersectWindow (*it)) 
      {
        any = true;
        break;
      }
    }
  }
  _hide_machine->SetQuirk (LauncherHideMachine::ANY_WINDOW_UNDER, any);
  _hide_machine->SetQuirk (LauncherHideMachine::ACTIVE_WINDOW_UNDER, active);
}

gboolean
Launcher::OnUpdateDragManagerTimeout (gpointer data)
{
  Launcher *self = (Launcher *) data;
 
  if (!self->_selection_atom)
    self->_selection_atom = XInternAtom (self->_screen->dpy (), "XdndSelection", false);
  
  Window drag_owner = XGetSelectionOwner (self->_screen->dpy (), self->_selection_atom);
  
  // evil hack because Qt does not release the seelction owner on drag finished
  Window root_r, child_r;
  int root_x_r, root_y_r, win_x_r, win_y_r;
  unsigned int mask;
  XQueryPointer (self->_screen->dpy (), self->_screen->root (), &root_r, &child_r, &root_x_r, &root_y_r, &win_x_r, &win_y_r, &mask);
  
  if (drag_owner && (mask | (Button1Mask & Button2Mask & Button3Mask)))
  {
    self->_hide_machine->SetQuirk (LauncherHideMachine::EXTERNAL_DND_ACTIVE, true);
    return true;
  }

  self->_hide_machine->SetQuirk (LauncherHideMachine::EXTERNAL_DND_ACTIVE, false);
  self->_hide_machine->SetQuirk (LauncherHideMachine::DND_PUSHED_OFF, false);
  
  self->_dnd_check_handle = 0;
  return false;
}

void
Launcher::OnWindowMapped (guint32 xid)
{
  CompWindow *window = _screen->findWindow (xid);
  if (window && window->type () | CompWindowTypeDndMask)
  {
    if (!_dnd_check_handle)
      _dnd_check_handle = g_timeout_add (200, &Launcher::OnUpdateDragManagerTimeout, this);
  }
}

void
Launcher::OnWindowUnmapped (guint32 xid)
{
  CompWindow *window = _screen->findWindow (xid);
  if (window && window->type () | CompWindowTypeDndMask)
  {
    if (!_dnd_check_handle)
      _dnd_check_handle = g_timeout_add (200, &Launcher::OnUpdateDragManagerTimeout, this);
  }
}

// FIXME: remove those 2 for Oneiric
void
Launcher::OnWindowMaybeIntellihide (guint32 xid)
{
  if (_hidemode != LAUNCHER_HIDE_NEVER)
    CheckWindowOverLauncher ();
}

void
Launcher::OnWindowMaybeIntellihideDelayed (guint32 xid)
{
  /*
   * Delay to let the other window taking the focus first (otherwise focuschanged
   * is emmited with the root window focus
   */
  if (_hidemode != LAUNCHER_HIDE_NEVER)
    g_idle_add ((GSourceFunc)CheckWindowOverLauncherSync, this);
}

gboolean
Launcher::CheckWindowOverLauncherSync (Launcher *self)
{
  self->CheckWindowOverLauncher ();
  return FALSE;
}

void
Launcher::OnPluginStateChanged ()
{
  _hide_machine->SetQuirk (LauncherHideMachine::EXPO_ACTIVE, PluginAdapter::Default ()->IsExpoActive ());
  _hide_machine->SetQuirk (LauncherHideMachine::SCALE_ACTIVE, PluginAdapter::Default ()->IsScaleActive ());
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
  
  if (self->_hidemode == LAUNCHER_HIDE_NEVER)
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
  _hide_machine->SetMode ((LauncherHideMachine::HideMode) hidemode);
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
  if (_launcher_action_state == actionstate)
    return;
    
  _launcher_action_state = actionstate;
  
  _hover_machine->SetQuirk (LauncherHoverMachine::LAUNCHER_IN_ACTION, (actionstate != ACTION_NONE));
  
  if (_keynav_activated)
    exitKeyNavMode ();
}

Launcher::LauncherActionState
Launcher::GetActionState ()
{
  return _launcher_action_state;
}

void Launcher::SetHover (bool hovered)
{
  
  if (hovered == _hovered)
    return;
    
  _hovered = hovered;

  if (_hovered)
  {
    _enter_y = (int) _mouse_position.y;
    SetTimeStruct (&_times[TIME_ENTER], &_times[TIME_LEAVE], ANIM_DURATION);
  }
  else
  {
    SetTimeStruct (&_times[TIME_LEAVE], &_times[TIME_ENTER], ANIM_DURATION);
  }
  
  EnsureAnimation ();
}

bool Launcher::MouseOverTopScrollArea ()
{
  if (GetActionState () == ACTION_NONE)
    return _hide_machine->GetQuirk (LauncherHideMachine::MOUSE_OVER_BFB);
  
  return _mouse_position.y < 0;
}

bool Launcher::MouseOverTopScrollExtrema ()
{
  // since we are not dragging the bfb will pick up events
  if (GetActionState () == ACTION_NONE)
    return _bfb_mouse_position.y == 0;
    
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

  if (!self->_hovered || self->GetActionState () == ACTION_DRAG_LAUNCHER)
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
    _autoscroll_handle = g_timeout_add (20, &Launcher::OnScrollTimeout, this);
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
    _icon_glow_size = icon_size + 14;

    for (int i = 0; i < MAX_SUPERKEY_LABELS; i++)
    {
      if (_superkey_labels[i])
        _superkey_labels[i]->UnReference();
      _superkey_labels[i] = cairoToTexture2D ((char) ('0' + ((i  + 1) % 10)), _icon_size, _icon_size);
    }

    LauncherModel::iterator it;
    for (it = _model->main_begin(); it != _model->main_end(); it++)
    {
        LauncherIcon *icon = *it;
        guint64 shortcut = icon->GetShortcut();
        if (shortcut > 32 && !g_ascii_isdigit ((gchar)shortcut))
                icon->SetSuperkeyLabel (cairoToTexture2D ((gchar)shortcut, _icon_size, _icon_size));
    }

    // recreate tile textures

    _parent->SetGeometry (nux::Geometry (geo.x, geo.y, tile_size + 12, geo.height));
}

void Launcher::OnIconAdded (LauncherIcon *icon)
{
    EnsureAnimation();

    icon->_xform_coords["HitArea"]      = new nux::Vector4[4];
    icon->_xform_coords["Image"]        = new nux::Vector4[4];
    icon->_xform_coords["Tile"]         = new nux::Vector4[4];
    icon->_xform_coords["Glow"]         = new nux::Vector4[4];
    icon->_xform_coords["Emblem"]       = new nux::Vector4[4];

    // needs to be disconnected
    icon->needs_redraw_connection = (sigc::connection) icon->needs_redraw.connect (sigc::mem_fun(this, &Launcher::OnIconNeedsRedraw));

    guint64 shortcut = icon->GetShortcut ();
    if (shortcut > 32 && !g_ascii_isdigit ((gchar) shortcut))
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

    if (icon->needs_redraw_connection.connected ())
      icon->needs_redraw_connection.disconnect ();

    if (icon == _current_icon)
      _current_icon = 0;
    if (icon == m_ActiveMenuIcon)
      m_ActiveMenuIcon = 0;
    if (icon == m_LastSpreadIcon)
      m_LastSpreadIcon = 0;
    if (icon == _icon_under_mouse)
      _icon_under_mouse = 0;
    if (icon == _icon_mouse_down)
      _icon_mouse_down = 0;
    if (icon == _drag_icon)
      _drag_icon = 0;

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

    if (_model->on_icon_added_connection.connected ())
      _model->on_icon_added_connection.disconnect ();
    _model->on_icon_added_connection = (sigc::connection) _model->icon_added.connect (sigc::mem_fun (this, &Launcher::OnIconAdded));

    if (_model->on_icon_removed_connection.connected ())
      _model->on_icon_removed_connection.disconnect ();
    _model->on_icon_removed_connection = (sigc::connection) _model->icon_removed.connect (sigc::mem_fun (this, &Launcher::OnIconRemoved));

    if (_model->on_order_changed_connection.connected ())
      _model->on_order_changed_connection.disconnect ();
    _model->on_order_changed_connection = (sigc::connection) _model->order_changed.connect (sigc::mem_fun (this, &Launcher::OnOrderChanged));
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
                                 float alpha,
                                 nux::Geometry& geo)
{
  int markerCenter = (int) arg.render_center.y;
  markerCenter -= (int) (arg.x_rotation / (2 * M_PI) * _icon_size);
  
  if (running > 0)
  {
    nux::TexCoordXForm texxform;

    nux::Color color = nux::Colors::LightGrey;

    if (arg.running_colored)
      color = nux::Colors::SkyBlue;
      
    color.SetRGBA (color.R () * alpha, color.G () * alpha,
                   color.B () * alpha, alpha);

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

    nux::Color color = nux::Colors::LightGrey;
    color.SetRGBA (color.R () * alpha, color.G () * alpha,
                   color.B () * alpha, alpha);
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
    s0 = 0.0f;                 t0 = 0.0f;
    s1 = 0.0f;                 t1 = icon->GetHeight();
    s2 = icon->GetWidth();     t2 = t1;
    s3 = s2;                   t3 = 0.0f;
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
  int DesatFactor;

  if(nux::GetGraphicsEngine ().UsingGLSLCodePath ())
  {
    _shader_program_uv_persp_correction->Begin();

    TextureObjectLocation   = _shader_program_uv_persp_correction->GetUniformLocationARB ("TextureObject0");
    VertexLocation          = _shader_program_uv_persp_correction->GetAttributeLocation  ("iVertex");
    TextureCoord0Location   = _shader_program_uv_persp_correction->GetAttributeLocation  ("iTexCoord0");
    VertexColorLocation     = _shader_program_uv_persp_correction->GetAttributeLocation  ("iColor");
    FragmentColor           = _shader_program_uv_persp_correction->GetUniformLocationARB ("color0");
    DesatFactor             = _shader_program_uv_persp_correction->GetUniformLocationARB ("desat_factor");

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

    // Set the model-view matrix
    CHECKGL (glMatrixMode (GL_MODELVIEW));
    CHECKGL (glLoadMatrixf ((float*) GfxContext.GetOpenGLModelViewMatrix ().m));
    
    // Set the projection matrix
    CHECKGL (glMatrixMode (GL_PROJECTION));
    CHECKGL (glLoadMatrixf ((float*) GfxContext.GetOpenGLProjectionMatrix ().m));
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
    CHECKGL ( glUniform4fARB (DesatFactor, arg.saturation, arg.saturation, arg.saturation, arg.saturation));

    nux::GetGraphicsEngine ().SetTexture(GL_TEXTURE0, icon);
    CHECKGL( glDrawArrays(GL_QUADS, 0, 4) );
  }
  else
  {
    CHECKGL ( glProgramLocalParameter4fARB (GL_FRAGMENT_PROGRAM_ARB, 0, bkg_color.R(), bkg_color.G(), bkg_color.B(), bkg_color.A() ) );
    CHECKGL ( glProgramLocalParameter4fARB (GL_FRAGMENT_PROGRAM_ARB, 1, arg.saturation, arg.saturation, arg.saturation, arg.saturation));

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

    /* draw keyboard-navigation "highlight" if any */
  if (arg.keyboard_nav_hl)
  {
    RenderIcon (GfxContext,
                arg,
                _icon_glow_hl_texture->GetDeviceTexture (),
                nux::Color (0xFFFFFFFF),
                arg.alpha,
                arg.icon->_xform_coords["Glow"]);

    RenderIcon (GfxContext,
                arg,
                _icon_outline_texture->GetDeviceTexture (),
                nux::Color(0xFFFFFFFF),
                0.95f * arg.alpha,
                arg.icon->_xform_coords["Tile"]);

    RenderIcon (GfxContext,
                arg,
                _icon_bkg_texture->GetDeviceTexture (),
                nux::Color(0xFFFFFFFF),
                0.9f * arg.alpha,
                arg.icon->_xform_coords["Tile"]);
  }
  else
  {
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
  }

  /* draw icon */
  RenderIcon (GfxContext,
              arg,
              arg.icon->TextureForSize (_icon_image_size)->GetDeviceTexture (),
              nux::Colors::White,
              arg.alpha,
              arg.icon->_xform_coords["Image"]);

  /* draw overlay shine */
  if (arg.backlight_intensity > 0.0f)
  {
    RenderIcon(GfxContext,
               arg,
               _icon_shine_texture->GetDeviceTexture (),
               nux::Colors::White,
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
               nux::Colors::White,
               arg.alpha,
               arg.icon->_xform_coords["Tile"]);
  }
  
  if (arg.icon->Emblem ())
  {
    RenderIcon(GfxContext,
               arg,
               arg.icon->Emblem ()->GetDeviceTexture (),
               nux::Colors::White,
               arg.alpha,
               arg.icon->_xform_coords["Emblem"]);
  }

  /* draw indicators */
  RenderIndicators (GfxContext,
                    arg,
                    arg.running_arrow ? arg.window_indicators : 0,
                    arg.active_arrow ? 1 : 0,
                    arg.alpha,
                    geo);

  /* draw superkey-shortcut label */ 
  if (_shortcuts_shown && !_hide_machine->GetQuirk (LauncherHideMachine::PLACES_VISIBLE))
  {
    guint64 shortcut = arg.icon->GetShortcut ();

    if (shortcut > 32)
    {
      if (!g_ascii_isdigit ((gchar) shortcut))
      {
        /* deal with dynamic labels for places, which can be set via the locale */
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
        gchar *shortcut_str = g_strdup_printf ("%c", (gchar)shortcut);
        int index = (atoi (shortcut_str) + 9) % 10; // Not -1 as -1 % 10 = -1…
        g_free (shortcut_str);
        
        RenderIcon (GfxContext,
                    arg,
                    _superkey_labels[index]->GetDeviceTexture (),
                    nux::Color (0xFFFFFFFF),
                    arg.alpha,
                    arg.icon->_xform_coords["Tile"]);
      }
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
      _launcher_animation_timeout = g_timeout_add (0, &Launcher::AnimationTimeout, this);

    nux::ROPConfig ROP;
    ROP.Blend = false;
    ROP.SrcBlend = GL_ONE;
    ROP.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

    RenderArgs (args, bkg_box, &launcher_alpha);

    if (_drag_icon && _render_drag_window)
    {
      RenderIconToTexture (GfxContext, _drag_icon, _offscreen_drag_texture);
      _drag_window->ShowWindow (true);
      
      _render_drag_window = false;
    }
    
    // clear region
    GfxContext.PushClippingRectangle(base);
    gPainter.PushDrawColorLayer(GfxContext, base, nux::Color(0x00000000), true, ROP);

    // clip vertically but not horizontally
    GfxContext.PushClippingRectangle(nux::Geometry (base.x, bkg_box.y, base.width, bkg_box.height));
    GfxContext.GetRenderStates ().SetPremultipliedBlend (nux::SRC_OVER);

    gPainter.Paint2DQuadColor (GfxContext, bkg_box, nux::Color(0xAA000000));

    UpdateIconXForm (args, GetGeometry ());
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
    gPainter.Paint2DQuadColor (GfxContext, nux::Geometry (bkg_box.x, bkg_box.y, bkg_box.width, 20), nux::Color(0x60000000), 
                                                                                                    nux::Color(0x00000000), 
                                                                                                    nux::Color(0x00000000), 
                                                                                                    nux::Color(0x60000000));

    // FIXME: can be removed for a bgk_box->SetAlpha once implemented
    GfxContext.GetRenderStates ().SetPremultipliedBlend (nux::DST_IN);
    nux::Color alpha_mask = nux::Color(0xAAAAAAAA);
    alpha_mask.SetRGBA (alpha_mask.R () * launcher_alpha, alpha_mask.G () * launcher_alpha,
                        alpha_mask.B () * launcher_alpha, launcher_alpha);
    gPainter.Paint2DQuadColor (GfxContext, bkg_box, alpha_mask);
    
    GfxContext.GetRenderStates ().SetColorMask (true, true, true, true);
    GfxContext.GetRenderStates ().SetPremultipliedBlend (nux::SRC_OVER);

    gPainter.PopBackground ();
    GfxContext.PopClippingRectangle ();
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


gboolean Launcher::StartIconDragTimeout (gpointer data)
{
    Launcher *self = (Launcher*) data;
    
    // if we are still waiting…
    if (self->GetActionState () == ACTION_NONE) {
      if (self->_icon_under_mouse)
      {
      self->_icon_under_mouse->MouseLeave.emit ();
      self->_icon_under_mouse->_mouse_inside = false;
      self->_icon_under_mouse = 0;
      }
      self->_initial_drag_animation = true;
      self->StartIconDragRequest (self->GetMouseX (), self->GetMouseY ());
    }
    self->_start_dragicon_handle = 0;
    return false;    
}

void Launcher::StartIconDragRequest (int x, int y)
{
  LauncherIcon *drag_icon = MouseIconIntersection ((int) (GetGeometry ().x / 2.0f), y);
  SetActionState (ACTION_DRAG_ICON);
  
  // FIXME: nux doesn't give nux::GetEventButton (button_flags) there, relying
  // on an internal Launcher property then
  if (drag_icon && (_last_button_press == 1) && _model->IconHasSister (drag_icon))
  {
    StartIconDrag (drag_icon);
    UpdateDragWindowPosition (drag_icon->GetCenter ().x, drag_icon->GetCenter ().y);
    if(_initial_drag_animation) {
      _drag_window->SetAnimationTarget (x, y + _drag_window->GetGeometry ().height/2);
      _drag_window->StartAnimation ();
    }
    EnsureAnimation ();
  }
  else
  {
    _drag_icon = NULL;
    if (_drag_window)
    {
      _drag_window->ShowWindow (false);
      _drag_window->UnReference ();
      _drag_window = NULL;
    }
  
  } 
}

void Launcher::StartIconDrag (LauncherIcon *icon)
{
  if (!icon)
    return;
  
  _hide_machine->SetQuirk (LauncherHideMachine::INTERNAL_DND_ACTIVE, true);
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
    LauncherIcon* hovered_icon = MouseIconIntersection (_mouse_position.x, _mouse_position.y);
  
    if(hovered_icon && hovered_icon->Type () == LauncherIcon::TYPE_TRASH)
    {
      launcher_removerequest.emit (_drag_icon);
      _drag_window->ShowWindow (false);
      EnsureAnimation ();
    }
    else
    {
      _drag_window->SetAnimationTarget ((int) (_drag_icon->GetCenter ().x), (int) (_drag_icon->GetCenter ().y));
      _drag_window->StartAnimation ();

      if (_drag_window->on_anim_completed.connected ())
        _drag_window->on_anim_completed.disconnect ();
      _drag_window->on_anim_completed = (sigc::connection) _drag_window->anim_completed.connect (sigc::mem_fun (this, &Launcher::OnDragWindowAnimCompleted));
    }
  }
  
  if (MouseBeyondDragThreshold ())
    SetTimeStruct (&_times[TIME_DRAG_THRESHOLD], &_times[TIME_DRAG_THRESHOLD], ANIM_DURATION_SHORT);
  
  _render_drag_window = false;
  _hide_machine->SetQuirk (LauncherHideMachine::INTERNAL_DND_ACTIVE, false);
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

void Launcher::RecvMouseDownOutsideArea (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  if (_keynav_activated)
    exitKeyNavMode ();
}

void Launcher::RecvMouseUp(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  SetMousePosition (x, y);
  nux::Geometry geo = GetGeometry ();
  
  MouseUpLogic (x, y, button_flags, key_flags);
  
  if (GetActionState () == ACTION_DRAG_ICON)
    EndIconDrag ();

  if (GetActionState () == ACTION_DRAG_LAUNCHER)   
    _hide_machine->SetQuirk (LauncherHideMachine::VERTICAL_SLIDE_ACTIVE, false);
  SetActionState (ACTION_NONE);
  _dnd_delta_x = 0;
  _dnd_delta_y = 0;
  _last_button_press = 0;
  EnsureAnimation ();
}

void Launcher::RecvMouseDrag(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags)
{
  /* FIXME: nux doesn't give nux::GetEventButton (button_flags) there, relying
   * on an internal Launcher property then
   */
  if (_last_button_press != 1)
    return;

  SetMousePosition (x, y);
  
  // FIXME: hack (see SetupRenderArg)
  _initial_drag_animation = false;

  _dnd_delta_y += dy;
  _dnd_delta_x += dx;
  
  if (nux::Abs (_dnd_delta_y) < MOUSE_DEADZONE &&
      nux::Abs (_dnd_delta_x) < MOUSE_DEADZONE && 
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
    if (nux::Abs (_dnd_delta_y) >= nux::Abs (_dnd_delta_x))
    {
      _launcher_drag_delta += _dnd_delta_y;
      SetActionState (ACTION_DRAG_LAUNCHER);
      _hide_machine->SetQuirk (LauncherHideMachine::VERTICAL_SLIDE_ACTIVE, true);
    }
    else
    {
      StartIconDragRequest (x, y);
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
  SetStateMouseOverLauncher (true);
  
  // make sure we actually get a chance to get events before turning this off
  if (x > 0)
    _hide_machine->SetQuirk (LauncherHideMachine::MOUSE_OVER_ACTIVE_EDGE, false);

  EventLogic ();
  EnsureAnimation ();
}

void Launcher::RecvMouseLeave(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  SetMousePosition (x, y);
  SetStateMouseOverLauncher (false);

  EventLogic ();
  EnsureAnimation ();
}

void Launcher::RecvMouseMove(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags)
{
  SetMousePosition (x, y);
  
  // make sure we actually get a chance to get events before turning this off
  if (x > 0)
    _hide_machine->SetQuirk (LauncherHideMachine::MOUSE_OVER_ACTIVE_EDGE, false);
    
  _postreveal_mousemove_delta_x += dx;
  _postreveal_mousemove_delta_y += dy;
  
  // check the state before changing it to avoid uneeded hide calls
  if (!_hide_machine->GetQuirk (LauncherHideMachine::MOUSE_MOVE_POST_REVEAL) &&
      (nux::Abs (_postreveal_mousemove_delta_x) > MOUSE_DEADZONE ||
      nux::Abs (_postreveal_mousemove_delta_y) > MOUSE_DEADZONE))
    _hide_machine->SetQuirk (LauncherHideMachine::MOUSE_MOVE_POST_REVEAL, true);
  

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


gboolean
Launcher::ResetRepeatShorcutTimeout (gpointer data)
{
  Launcher *self = (Launcher*) data;
  
  self->_latest_shortcut = 0;
  
  self->_ignore_repeat_shortcut_handle = 0;
  return false;
}

gboolean
Launcher::CheckSuperShortcutPressed (unsigned int  key_sym,
                                     unsigned long key_code,
                                     unsigned long key_state,
                                     char*         key_string)
{
  if (!_super_pressed)
    return false;

  LauncherModel::iterator it;
  
  // Shortcut to start launcher icons. Only relies on Keycode, ignore modifier
  for (it = _model->begin (); it != _model->end (); it++)
  {
    if ((XKeysymToKeycode (screen->dpy (), (*it)->GetShortcut ()) == key_code) ||
        ((gchar)((*it)->GetShortcut ()) == key_string[0]))
    {      
      /*
       * start a timeout while repressing the same shortcut will be ignored.
       * This is because the keypress repeat is handled by Xorg and we have no
       * way to know if a press is an actual press or just an automated repetition
       * because the button is hold down. (key release events are sent in both cases)
      */
      if (_ignore_repeat_shortcut_handle > 0)
        g_source_remove (_ignore_repeat_shortcut_handle);
      _ignore_repeat_shortcut_handle = g_timeout_add (IGNORE_REPEAT_SHORTCUT_DURATION, &Launcher::ResetRepeatShorcutTimeout, this);
      
      if (_latest_shortcut == (*it)->GetShortcut ())
        return true;
      
      if (g_ascii_isdigit ((gchar) (*it)->GetShortcut ()) && (key_state & ShiftMask))
        (*it)->OpenInstance ();
      else
        (*it)->Activate ();

      _latest_shortcut = (*it)->GetShortcut ();
      
      // disable the "tap on super" check
      _times[TIME_TAP_SUPER].tv_sec = 0;
      _times[TIME_TAP_SUPER].tv_nsec = 0;
      return true;
    }
  }
  
  return false;
}

void 
Launcher::EdgeRevealTriggered ()
{
  _hide_machine->SetQuirk (LauncherHideMachine::MOUSE_OVER_ACTIVE_EDGE, true);
  _hide_machine->SetQuirk (LauncherHideMachine::MOUSE_MOVE_POST_REVEAL, true);
}

void
Launcher::RecvKeyPressed (unsigned int  key_sym,
                          unsigned long key_code,
                          unsigned long key_state)
{

  LauncherModel::iterator it;

  /*
   * all key events below are related to keynavigation. Make an additional
   * check that we are in a keynav mode when we inadvertadly receive the focus
   */
  if (!_keynav_activated)
    return;
   
  switch (key_sym)
  {
    // up (move selection up or go to global-menu if at top-most icon)
    case NUX_VK_UP:
    case NUX_KP_UP:
      if (_current_icon_index > 0)
      {
        int temp_current_icon_index = _current_icon_index;
        do
        {
          temp_current_icon_index --;
          it = _model->at (temp_current_icon_index );
        }while (it != (LauncherModel::iterator)NULL && !(*it)->GetQuirk (LauncherIcon::QUIRK_VISIBLE));
      
        if (it != (LauncherModel::iterator)NULL) {
          _current_icon_index = temp_current_icon_index;
          _launcher_drag_delta += (_icon_size + _space_between_icons);
        }
        EnsureAnimation ();
        selection_change.emit ();
      }
    break;

    // down (move selection down and unfold launcher if needed)
    case NUX_VK_DOWN:
    case NUX_KP_DOWN:
      if (_current_icon_index < _model->Size () - 1)
      {
        int temp_current_icon_index = _current_icon_index;

        do
        {
          temp_current_icon_index ++;
          it = _model->at (temp_current_icon_index );
        }while (it != (LauncherModel::iterator)NULL && !(*it)->GetQuirk (LauncherIcon::QUIRK_VISIBLE));
      
        if (it != (LauncherModel::iterator)NULL) {
          _current_icon_index = temp_current_icon_index;
          _launcher_drag_delta -= (_icon_size + _space_between_icons);
        }
      
        EnsureAnimation ();
        selection_change.emit ();
      }
    break;

    // esc/left (close quicklist or exit laucher key-focus)
    case NUX_VK_LEFT:
    case NUX_KP_LEFT:
    case NUX_VK_ESCAPE:
      // hide again
      exitKeyNavMode ();
    break;

    // right/shift-f10 (open quicklist of currently selected icon)      
    case XK_F10:
      if (!(key_state & NUX_STATE_SHIFT))
        break;
    case NUX_VK_RIGHT:
    case NUX_KP_RIGHT:
    case XK_Menu:
      // open quicklist of currently selected icon
      it = _model->at (_current_icon_index);
      if (it != (LauncherModel::iterator)NULL)
      {
        if ((*it)->OpenQuicklist (true))
          leaveKeyNavMode (true);
      }
    break;

    // <SPACE> (open a new instance)
    case NUX_VK_SPACE:
      // start currently selected icon
      it = _model->at (_current_icon_index);
      if (it != (LauncherModel::iterator)NULL)
      {
        (*it)->OpenInstance ();
      }
      exitKeyNavMode ();
      break;

    // <RETURN> (start/activate currently selected icon)
    case NUX_VK_ENTER:
    case NUX_KP_ENTER:
      {
        // start currently selected icon
        it = _model->at (_current_icon_index);
        if (it != (LauncherModel::iterator)NULL)
          (*it)->Activate ();
      }
      exitKeyNavMode ();
    break;
      
    default:
    break;
  }
}

void Launcher::RecvQuicklistOpened (QuicklistView *quicklist)
{
  _hide_machine->SetQuirk (LauncherHideMachine::QUICKLIST_OPEN, true);
  _hover_machine->SetQuirk (LauncherHoverMachine::QUICKLIST_OPEN, true);
  EventLogic ();
  EnsureAnimation ();
}

void Launcher::RecvQuicklistClosed (QuicklistView *quicklist)
{
  _hide_machine->SetQuirk (LauncherHideMachine::QUICKLIST_OPEN, false);
  _hover_machine->SetQuirk (LauncherHoverMachine::QUICKLIST_OPEN, false);

  EventLogic ();
  EnsureAnimation ();
}

void Launcher::EventLogic ()
{
  if (GetActionState () == ACTION_DRAG_ICON ||
      GetActionState () == ACTION_DRAG_LAUNCHER)
    return;

  LauncherIcon* launcher_icon = 0;

  if (_hide_machine->GetQuirk (LauncherHideMachine::MOUSE_OVER_LAUNCHER)
      && _hide_machine->GetQuirk (LauncherHideMachine::MOUSE_MOVE_POST_REVEAL)) {
    launcher_icon = MouseIconIntersection (_mouse_position.x, _mouse_position.y);
  }  
  

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
    
    _hide_machine->SetQuirk (LauncherHideMachine::LAST_ACTION_ACTIVATE, false);
  }
}

void Launcher::MouseDownLogic (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  LauncherIcon* launcher_icon = 0;
  launcher_icon = MouseIconIntersection (_mouse_position.x, _mouse_position.y);
  
  _hide_machine->SetQuirk (LauncherHideMachine::LAST_ACTION_ACTIVATE, false);

  if (launcher_icon)
  {
    _icon_mouse_down = launcher_icon;
    // if MouseUp after the time ended -> it's an icon drag, otherwise, it's starting an app
    if (_start_dragicon_handle > 0)
      g_source_remove (_start_dragicon_handle);
    _start_dragicon_handle = g_timeout_add (START_DRAGICON_DURATION, &Launcher::StartIconDragTimeout, this);
    
    launcher_icon->MouseDown.emit (nux::GetEventButton (button_flags));
  }
}

void Launcher::MouseUpLogic (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  LauncherIcon* launcher_icon = 0;
  
  launcher_icon = MouseIconIntersection (_mouse_position.x, _mouse_position.y);
  
  if (_start_dragicon_handle > 0)
      g_source_remove (_start_dragicon_handle);
  _start_dragicon_handle = 0;

  if (_icon_mouse_down && (_icon_mouse_down == launcher_icon))
  {
    _icon_mouse_down->MouseUp.emit (nux::GetEventButton (button_flags));

    if (GetActionState () == ACTION_NONE) {
      _icon_mouse_down->MouseClick.emit (nux::GetEventButton (button_flags));
    }
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


/*
    geo: The viewport geometry.
*/
void Launcher::UpdateIconXForm (std::list<Launcher::RenderArg> &args, nux::Geometry geo)
{
  nux::Matrix4 ObjectMatrix;
  nux::Matrix4 ViewMatrix;
  nux::Matrix4 ProjectionMatrix;
  nux::Matrix4 ViewProjectionMatrix;

  GetInverseScreenPerspectiveMatrix(ViewMatrix, ProjectionMatrix, geo.width, geo.height, 0.1f, 1000.0f, DEGTORAD(90));

  //LauncherModel::iterator it;
  std::list<Launcher::RenderArg>::iterator it;
  int i;
  for(it = args.begin(), i = 0; it != args.end(); it++, i++)
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
    if (i == _model->Size () - 1)
      h += 4;
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
  UpdateIconXForm (drag_args, nux::Geometry (0, 0, _icon_size, _icon_size));
  
  SetOffscreenRenderTarget (texture);
  DrawRenderArg (nux::GetGraphicsEngine (), arg, nux::Geometry (0, 0, _icon_size, _icon_size));
  RestoreSystemRenderTarget ();
}

void
Launcher::RenderProgressToTexture (nux::GraphicsEngine& GfxContext, nux::IntrusiveSP<nux::IOpenGLBaseTexture> texture, float progress_fill, float bias)
{
  int width = texture->GetWidth ();
  int height = texture->GetHeight ();
  
  int progress_width =  _icon_size;
  int progress_height = _progress_bar_trough->GetHeight ();

  int fill_width = _icon_image_size - _icon_image_size_delta;
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
                            _progress_bar_trough->GetDeviceTexture (), texxform, nux::Colors::White);
                            
  GfxContext.QRP_1Tex (left_edge + fill_offset, fill_y, fill_width, fill_height, 
                            _progress_bar_fill->GetDeviceTexture (), texxform, nux::Colors::White);  

  GfxContext.PopClippingRectangle (); 


  // right door
  GfxContext.PushClippingRectangle(nux::Geometry (left_edge + half_size, 0, half_size, height));
  
  GfxContext.QRP_1Tex (right_edge - progress_width, progress_y, progress_width, progress_height, 
                            _progress_bar_trough->GetDeviceTexture (), texxform, nux::Colors::White);
  
  GfxContext.QRP_1Tex (right_edge - progress_width + fill_offset, fill_y, fill_width, fill_height, 
                            _progress_bar_fill->GetDeviceTexture (), texxform, nux::Colors::White);
  
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
  SetStateMouseOverLauncher (false);
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
    SetStateMouseOverLauncher (true);
    
    LauncherModel::iterator it;
    for (it = _model->begin (); it != _model->end () && !_steal_drag; it++)
    {
      if ((*it)->QueryAcceptDrop (_drag_data) != nux::DNDACTION_NONE && !_steal_drag)
        (*it)->SetQuirk (LauncherIcon::QUIRK_DROP_PRELIGHT, true);
      else
        (*it)->SetQuirk (LauncherIcon::QUIRK_DROP_DIM, true);
    }
  
  }
  
  g_free (uri_list_const);
  
  SetMousePosition (x - parent->GetGeometry ().x, y - parent->GetGeometry ().y);
  
  if (_mouse_position.x == 0 && _mouse_position.y <= (_parent->GetGeometry ().height - _icon_size - 2*_space_between_icons) && !_drag_edge_touching)
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
  
  bool hovered_icon_is_appropriate = false;
  if(hovered_icon)
    {
      if(hovered_icon->Type () == LauncherIcon::TYPE_TRASH)
        _steal_drag = false;
      
      if(hovered_icon->Type () == LauncherIcon::TYPE_APPLICATION || hovered_icon->Type () == LauncherIcon::TYPE_EXPO)
        hovered_icon_is_appropriate = true;
    }

  if (_steal_drag)
  {
    _drag_action = nux::DNDACTION_COPY;
    if (!_dnd_hovered_icon && hovered_icon_is_appropriate)
    {
      _dnd_hovered_icon = new SpacerLauncherIcon (this);
      _dnd_hovered_icon->SetSortPriority (G_MAXINT);
      _model->AddIcon (_dnd_hovered_icon);
      _model->ReorderBefore (_dnd_hovered_icon, hovered_icon, true);
    }
    else if(_dnd_hovered_icon)
    {
      if(hovered_icon)
      {  
        if(hovered_icon_is_appropriate)
        {
         _model->ReorderSmart (_dnd_hovered_icon, hovered_icon, true);
        }
        else
        {
          _dnd_hovered_icon->SetQuirk (LauncherIcon::QUIRK_VISIBLE, false);
          _dnd_hovered_icon->remove.emit (_dnd_hovered_icon);
          _dnd_hovered_icon = 0;
        }
      }
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
      launcher_addrequest.emit (path, _dnd_hovered_icon);
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

  if (it != (LauncherModel::iterator)NULL)
    return *it;
  else
    return NULL;
}

/* dbus handlers */

void
Launcher::handle_dbus_method_call (GDBusConnection       *connection,
                                   const gchar           *sender,
                                   const gchar           *object_path,
                                   const gchar           *interface_name,
                                   const gchar           *method_name,
                                   GVariant              *parameters,
                                   GDBusMethodInvocation *invocation,
                                   gpointer               user_data)
{

  if (g_strcmp0 (method_name, "AddLauncherItemFromPosition") == 0)
  {
    gchar  *icon;
    gchar  *title;
    gint32  icon_x;
    gint32  icon_y;
    gint32  icon_size;
    gchar  *desktop_file;   
    gchar  *aptdaemon_task;
    
    g_variant_get (parameters, "(ssiiiss)", &icon, &title, &icon_x, &icon_y, &icon_size, &desktop_file, &aptdaemon_task, NULL);
    
    Launcher *self = (Launcher*)user_data;
    self->launcher_addrequest.emit (desktop_file, NULL);
    
    g_dbus_method_invocation_return_value (invocation, NULL);
    g_free (icon);
    g_free (title);
    g_free (desktop_file);
    g_free (aptdaemon_task);
  }
  
}

void
Launcher::OnBusAcquired (GDBusConnection *connection,
                         const gchar     *name,
                         gpointer         user_data)
{
  GDBusNodeInfo *introspection_data = g_dbus_node_info_new_for_xml (introspection_xml, NULL);
  guint registration_id;
  
  if (!introspection_data) {
    g_warning ("No introspection data loaded. Won't get dynamic launcher addition.");
    return;
  }
  
  
  
  registration_id = g_dbus_connection_register_object (connection,
                                                       S_DBUS_PATH,
                                                       introspection_data->interfaces[0],
                                                       &interface_vtable,
                                                       user_data,
                                                       NULL,
                                                       NULL);
  if (!registration_id)
    g_warning ("Object registration failed. Won't get dynamic launcher addition.");
  
}

void
Launcher::OnNameAcquired (GDBusConnection *connection,
                          const gchar     *name,
                          gpointer         user_data)
{
  g_debug ("Acquired the name %s on the session bus\n", name);
}

void
Launcher::OnNameLost (GDBusConnection *connection,
                       const gchar     *name,
                       gpointer         user_data)
{
  g_debug ("Lost the name %s on the session bus\n", name);
}
