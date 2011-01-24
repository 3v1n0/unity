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

static bool USE_ARB_SHADERS = true;
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

Launcher::Launcher(nux::BaseWindow *parent, CompScreen *screen, NUX_FILE_LINE_DECL)
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

    m_ActiveTooltipIcon = NULL;
    m_ActiveMenuIcon = NULL;
    m_LastSpreadIcon = NULL;

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
    _icon_under_mouse       = NULL;
    _icon_mouse_down        = NULL;
    _drag_icon              = NULL;
    _icon_image_size        = 48;
    _icon_glow_size         = 62;
    _icon_image_size_delta  = 6;
    _icon_size              = _icon_image_size + _icon_image_size_delta;

    _icon_bkg_texture       = nux::CreateTextureFromFile (PKGDATADIR"/round_corner_54x54.png");
    _icon_outline_texture   = nux::CreateTextureFromFile (PKGDATADIR"/round_outline_54x54.png");
    _icon_shine_texture     = nux::CreateTextureFromFile (PKGDATADIR"/round_shine_54x54.png");
    _icon_glow_texture      = nux::CreateTextureFromFile (PKGDATADIR"/round_glow_62x62.png");
    _progress_bar_trough    = nux::CreateTextureFromFile (PKGDATADIR"/progress_bar_trough.png");
    _progress_bar_fill      = nux::CreateTextureFromFile (PKGDATADIR"/progress_bar_fill.png");
    
    _pip_ltr                = nux::CreateTextureFromFile (PKGDATADIR"/launcher_pip_ltr.png");
    _arrow_ltr              = nux::CreateTextureFromFile (PKGDATADIR"/launcher_arrow_ltr.png");
    _arrow_empty_ltr        = nux::CreateTextureFromFile (PKGDATADIR"/launcher_arrow_outline_ltr.png");

    _pip_rtl                = nux::CreateTextureFromFile (PKGDATADIR"/launcher_pip_rtl.png");
    _arrow_rtl              = nux::CreateTextureFromFile (PKGDATADIR"/launcher_arrow_rtl.png");
    _arrow_empty_rtl        = nux::CreateTextureFromFile (PKGDATADIR"/launcher_arrow_outline_rtl.png");

    _enter_y                = 0;
    _dnd_security           = 15;
    _launcher_drag_delta    = 0;
    _dnd_delta_y            = 0;
    _dnd_delta_x            = 0;
    _autohide_handle        = 0;
    _floating               = false;
    _hovered                = false;
    _autohide               = false;
    _hidden                 = false;
    _mouse_inside_launcher  = false;
    _mouse_inside_trigger   = false;
    _key_show_launcher      = false;
    _placeview_show_launcher = false;
    _window_over_launcher   = false;
    _render_drag_window     = false;
    _backlight_always_on    = false;
    

    // 0 out timers to avoid wonky startups
    _enter_time.tv_sec = 0;
    _enter_time.tv_nsec = 0;
    _exit_time.tv_sec = 0;
    _exit_time.tv_nsec = 0;
    _drag_end_time.tv_sec = 0;
    _drag_end_time.tv_nsec = 0;
    _drag_start_time.tv_sec = 0;
    _drag_start_time.tv_nsec = 0;
    _drag_threshold_time.tv_sec = 0;
    _drag_threshold_time.tv_nsec = 0;
    _autohide_time.tv_sec = 0;
    _autohide_time.tv_nsec = 0;
    
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
}

Launcher::~Launcher()
{

}

/* Introspection */
const gchar *
Launcher::GetName ()
{
  return "Launcher";
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
  g_variant_builder_add (builder, "{sv}", "autohide", g_variant_new_boolean (_autohide));
  g_variant_builder_add (builder, "{sv}", "hidden", g_variant_new_boolean (_hidden));
  g_variant_builder_add (builder, "{sv}", "autohide", g_variant_new_boolean (_autohide));
  g_variant_builder_add (builder, "{sv}", "mouse-inside-launcher", g_variant_new_boolean (_mouse_inside_launcher));
}

void Launcher::SetMousePosition (int x, int y)
{
    bool beyond_drag_threshold = MouseBeyondDragThreshold ();
    _mouse_position = nux::Point2 (x, y);
    
    if (beyond_drag_threshold != MouseBeyondDragThreshold ())
      SetTimeStruct (&_drag_threshold_time, &_drag_threshold_time, ANIM_DURATION_SHORT);
}

bool Launcher::MouseBeyondDragThreshold ()
{
    if (_launcher_action_state != ACTION_DRAG_ICON)
      return false;
    return _mouse_position.x > GetGeometry ().width + _icon_size / 2;
}

/* Render Layout Logic */
float Launcher::GetHoverProgress (struct timespec const &current)
{
    if (_hovered)
        return CLAMP ((float) (TimeDelta (&current, &_enter_time)) / (float) ANIM_DURATION, 0.0f, 1.0f);
    else
        return 1.0f - CLAMP ((float) (TimeDelta (&current, &_exit_time)) / (float) ANIM_DURATION, 0.0f, 1.0f);
}

float Launcher::DnDExitProgress (struct timespec const &current)
{
    return 1.0f - CLAMP ((float) (TimeDelta (&current, &_drag_end_time)) / (float) ANIM_DURATION_LONG, 0.0f, 1.0f);
}

float Launcher::DnDStartProgress (struct timespec const &current)
{
  return CLAMP ((float) (TimeDelta (&current, &_drag_start_time)) / (float) ANIM_DURATION, 0.0f, 1.0f);
}

float Launcher::AutohideProgress (struct timespec const &current)
{
    if (!_autohide)
        return 0.0f;

    if (_hidden)
        return CLAMP ((float) (TimeDelta (&current, &_autohide_time)) / (float) ANIM_DURATION_SHORT, 0.0f, 1.0f);
    else
        return 1.0f - CLAMP ((float) (TimeDelta (&current, &_autohide_time)) / (float) ANIM_DURATION_SHORT, 0.0f, 1.0f);
}

float Launcher::DragThresholdProgress (struct timespec const &current)
{
  if (MouseBeyondDragThreshold ())
    return 1.0f - CLAMP ((float) (TimeDelta (&current, &_drag_threshold_time)) / (float) ANIM_DURATION_SHORT, 0.0f, 1.0f);
  else
    return CLAMP ((float) (TimeDelta (&current, &_drag_threshold_time)) / (float) ANIM_DURATION_SHORT, 0.0f, 1.0f);
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
    if (TimeDelta (&current, &_enter_time) < ANIM_DURATION)
       return true;

    // hover out animation
    if (TimeDelta (&current, &_exit_time) < ANIM_DURATION)
        return true;
    
    // drag start animation
    if (TimeDelta (&current, &_drag_start_time) < ANIM_DURATION)
        return true;
    
    // drag end animation
    if (TimeDelta (&current, &_drag_end_time) < ANIM_DURATION_LONG)
        return true;

    if (TimeDelta (&current, &_autohide_time) < ANIM_DURATION_SHORT)
        return true;
        
    if (TimeDelta (&current, &_drag_threshold_time) < ANIM_DURATION_SHORT)
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
            current.tv_sec -= remove / 1000;
            remove = remove % 1000;

            if (remove > current.tv_nsec / 1000000)
            {
                current.tv_sec--;
                current.tv_nsec += 1000000000;
            }
            current.tv_nsec -= remove * 1000000;
        }
    }

    timer->tv_sec = current.tv_sec;
    timer->tv_nsec = current.tv_nsec;
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
    return 0.5f + (float) (std::cos (M_PI * (_backlight_always_on ? 4.0f : 3.0f) * starting_progress)) * 0.5f;
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
    if (_backlight_always_on)
      backlight_strength = BACKLIGHT_STRENGTH;
    else
      backlight_strength = BACKLIGHT_STRENGTH * running_progress;
      
    switch (_launch_animation)
    {
      case LAUNCH_ANIMATION_NONE:
        result = backlight_strength;
        break;
      case LAUNCH_ANIMATION_BLINK:
        if (_backlight_always_on)
          result = IconStartingBlinkValue (icon, current);
        else
          result = backlight_strength; // The blink concept is a failure in this case (it just doesn't work right)
        break;
      case LAUNCH_ANIMATION_PULSE:
        if (running_progress == 1.0f && icon->GetQuirk (LauncherIcon::QUIRK_RUNNING))
          icon->ResetQuirkTime (LauncherIcon::QUIRK_STARTING);
        
        result = backlight_strength;
        if (_backlight_always_on)
          result *= CLAMP (running_progress + IconStartingPulseValue (icon, current), 0.0f, 1.0f);
        else
          result += (BACKLIGHT_STRENGTH - result) * (1.0f - IconStartingPulseValue (icon, current));
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

    if (icon == _drag_icon)
    {
        if (MouseBeyondDragThreshold ())
          arg.stick_thingy = true;
        
        if (_launcher_action_state == ACTION_DRAG_ICON || (_drag_window && _drag_window->Animating ()))
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
                           nux::Geometry &box_geo)
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

        if (_launcher_action_state != ACTION_DRAG_LAUNCHER)
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

    float autohide_progress = AutohideProgress (current);
    float autohide_offset = 0.0f;
    if (_autohide && autohide_progress > 0.0f)
    {
        autohide_offset -= geo.width * autohide_progress;
    }

    // Inform the painter where to paint the box
    box_geo = geo;

    if (_autohide)
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

/* Launcher Show/Hide logic */

void Launcher::StartKeyShowLauncher ()
{
    _key_show_launcher = true;
    EnsureHiddenState ();
    // trigger the timer now as we want to hide the launcher immediately
    // if we release the key after 1s happened.
    SetupAutohideTimer ();
}

void Launcher::EndKeyShowLauncher ()
{
    _key_show_launcher = false;
    EnsureHiddenState ();
}

void Launcher::OnPlaceViewShown (GVariant *data, void *val)
{
    Launcher *self = (Launcher*)val;
    self->_placeview_show_launcher = true;
    // trigger the timer now as we want to hide the launcher immediately
    // if we close the dash after 1s happened.
    self->EnsureHiddenState ();
    self->SetupAutohideTimer ();
}

void Launcher::OnPlaceViewHidden (GVariant *data, void *val)
{
    Launcher *self = (Launcher*)val;
    self->_placeview_show_launcher = false;
    self->EnsureHiddenState ();
}

void Launcher::SetHidden (bool hidden)
{
    if (hidden == _hidden)
        return;

    _hidden = hidden;
    SetTimeStruct (&_autohide_time, &_autohide_time, ANIM_DURATION_SHORT);

    _parent->EnableInputWindow(!hidden);

    EnsureAnimation ();
}

gboolean Launcher::OnAutohideTimeout (gpointer data)
{
    Launcher *self = (Launcher*) data;

    self->_autohide_handle = 0;
    self->EnsureHiddenState ();
    return false;
}

void
Launcher::EnsureHiddenState ()
{
  if (!_mouse_inside_trigger && 
      !_mouse_inside_launcher && 
      !_key_show_launcher &&
      !_placeview_show_launcher &&
       _launcher_action_state == ACTION_NONE &&
      !QuicklistManager::Default ()->Current() &&
      !_autohide_handle &&
      _window_over_launcher) 
    SetHidden (true);
  else
    SetHidden (false);
}

void
Launcher::CheckWindowOverLauncher ()
{
  CompWindowList window_list = _screen->windows ();
  CompWindowList::iterator it;
  nux::Geometry geo = GetGeometry ();

  for (it = window_list.begin (); it != window_list.end (); it++)
  {
    CompWindow *window = *it;
    int intersect_types = CompWindowTypeNormalMask | CompWindowTypeDialogMask |
                          CompWindowTypeModalDialogMask;

    if (!(window->type () & intersect_types) || !window->isMapped () || !window->isViewable ())
      continue;

    if (CompRegion (window->inputRect ()).intersects (CompRect (geo.x, geo.y, geo.width, geo.height)))
    {
      _window_over_launcher = true;
      EnsureHiddenState ();
      return;
    }
  }

  _window_over_launcher = false;
  EnsureHiddenState ();
}

void
Launcher::OnWindowMaybeIntellihide (guint32 xid)
{
  if (_autohide)
    CheckWindowOverLauncher ();
}

void Launcher::OnTriggerMouseEnter (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  _mouse_inside_trigger = true;
  EnsureHiddenState ();
}

void Launcher::SetupAutohideTimer ()
{
  if (_autohide)
  {
    if (_autohide_handle > 0)
      g_source_remove (_autohide_handle);
    _autohide_handle = g_timeout_add (1000, &Launcher::OnAutohideTimeout, this);
  }
}

void Launcher::OnTriggerMouseLeave (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  _mouse_inside_trigger = false;
  SetupAutohideTimer ();
}

bool Launcher::AutohideEnabled ()
{
    return _autohide;
}

/* End Launcher Show/Hide logic */

gboolean Launcher::StrutHack (gpointer data)
{
    Launcher *self = (Launcher *) data;
    self->_parent->InputWindowEnableStruts(false);
    self->_parent->InputWindowEnableStruts(true);

    return false;
}

void Launcher::SetAutohide (bool autohide, nux::View *trigger)
{
    if (_autohide == autohide)
        return;

    if (autohide)
    {
        _parent->InputWindowEnableStruts(false);
        _autohide_trigger = trigger;
        _autohide_trigger->OnMouseEnter.connect (sigc::mem_fun(this, &Launcher::OnTriggerMouseEnter));
        _autohide_trigger->OnMouseLeave.connect (sigc::mem_fun(this, &Launcher::OnTriggerMouseLeave));
    }
    else
    {
        _parent->EnableInputWindow(true);
        g_timeout_add (1000, &Launcher::StrutHack, this);
        _parent->InputWindowEnableStruts(true);
    }

    _autohide = autohide;
    EnsureAnimation ();
}

void Launcher::SetFloating (bool floating)
{
    if (_floating == floating)
        return;

    _floating = floating;
    EnsureAnimation ();
}

void Launcher::SetBacklightAlwaysOn (bool always_on)
{
  if (_backlight_always_on == always_on)
    return;
  
  _backlight_always_on = always_on;
  EnsureAnimation ();
}

bool Launcher::GetBacklightAlwaysOn ()
{
  return _backlight_always_on;
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
Launcher::EnsureHoverState ()
{
  if (_mouse_inside_launcher || QuicklistManager::Default ()->Current() || _launcher_action_state != ACTION_NONE)
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
    SetTimeStruct (&_enter_time, &_exit_time, ANIM_DURATION);
}

void Launcher::UnsetHover ()
{
    if (!_hovered)
        return;

    _hovered = false;
    SetTimeStruct (&_exit_time, &_enter_time, ANIM_DURATION);
    SetupAutohideTimer ();
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

    // How to free these properly?
    icon->_xform_coords["HitArea"]      = new nux::Vector4[4];
    icon->_xform_coords["Image"]        = new nux::Vector4[4];
    icon->_xform_coords["Tile"]         = new nux::Vector4[4];
    icon->_xform_coords["Glow"]         = new nux::Vector4[4];

    // needs to be disconnected
    icon->needs_redraw.connect (sigc::mem_fun(this, &Launcher::OnIconNeedsRedraw));

    AddChild (icon);
}

void Launcher::OnIconRemoved (LauncherIcon *icon)
{
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
      nux::Matrix4 mat = nux::GetGraphicsEngine ().GetModelViewProjectionMatrix ();
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

  bkg_color.SetAlpha (bkg_color.A () * alpha);

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

  GfxContext.GetRenderStates ().SetSeparateBlend (true,
                                                GL_SRC_ALPHA,
                                                GL_ONE_MINUS_SRC_ALPHA,
                                                GL_ONE_MINUS_DST_ALPHA,
                                                GL_ONE);

  GfxContext.GetRenderStates ().SetColorMask (true, true, true, true);

  /* draw tile */
  if (arg.backlight_intensity < 1.0f)
  {
    RenderIcon(GfxContext,
               arg,
               _icon_outline_texture->GetDeviceTexture (),
               nux::Color(0xAAFFFFFF),
               1.0f - arg.backlight_intensity,
               arg.icon->_xform_coords["Tile"]);
  }

  if (arg.backlight_intensity > 0.0f)
  {
    RenderIcon(GfxContext,
               arg,
               _icon_bkg_texture->GetDeviceTexture (),
               arg.icon->BackgroundColor (),
               arg.backlight_intensity,
               arg.icon->_xform_coords["Tile"]);
  }
  /* end tile draw */

  GfxContext.GetRenderStates ().SetSeparateBlend (true,
                                                GL_SRC_ALPHA,
                                                GL_ONE_MINUS_SRC_ALPHA,
                                                GL_ONE_MINUS_DST_ALPHA,
                                                GL_ONE);
  GfxContext.GetRenderStates ().SetColorMask (true, true, true, true);

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
               arg.backlight_intensity,
               arg.icon->_xform_coords["Tile"]);
  }

  /* draw glow */
  if (arg.glow_intensity > 0.0f)
  {
    RenderIcon(GfxContext,
               arg,
               _icon_glow_texture->GetDeviceTexture (),
               arg.icon->GlowColor (),
               arg.glow_intensity,
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
               fade_out,
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
               1.0f,
               arg.icon->_xform_coords["Tile"]);
  }

  /* draw indicators */
  RenderIndicators (GfxContext,
                    arg,
                    arg.running_arrow ? arg.window_indicators : 0,
                    arg.active_arrow ? 1 : 0,
                    geo);
}

void Launcher::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw)
{
    nux::Geometry base = GetGeometry();
    nux::Geometry bkg_box;
    std::list<Launcher::RenderArg> args;
    std::list<Launcher::RenderArg>::reverse_iterator rev_it;
    std::list<Launcher::RenderArg>::iterator it;

    // rely on the compiz event loop to come back to us in a nice throttling
    if (AnimationInProgress ())   
      g_timeout_add (0, &Launcher::AnimationTimeout, this);

    nux::ROPConfig ROP;
    ROP.Blend = false;
    ROP.SrcBlend = GL_SRC_ALPHA;
    ROP.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

    RenderArgs (args, bkg_box);

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
    GfxContext.GetRenderStates ().SetSeparateBlend (true,
                                                    GL_SRC_ALPHA,
                                                    GL_ONE_MINUS_SRC_ALPHA,
                                                    GL_ONE_MINUS_DST_ALPHA,
                                                    GL_ONE);

    gPainter.Paint2DQuadColor (GfxContext, bkg_box, nux::Color(0xAA000000));

    UpdateIconXForm (args);
    EventLogic ();

    /* draw launcher */
    for (rev_it = args.rbegin (); rev_it != args.rend (); rev_it++)
    {
      if ((*rev_it).stick_thingy)
        gPainter.Paint2DQuadColor (GfxContext, 
                                   nux::Geometry (bkg_box.x, (*rev_it).render_center.y - 3, bkg_box.width, 2), 
                                   nux::Color(0xAAFFFFFF));
      
      if ((*rev_it).x_rotation >= 0.0f || (*rev_it).skip)
        continue;

      DrawRenderArg (GfxContext, *rev_it, bkg_box);
    }

    for (it = args.begin(); it != args.end(); it++)
    {
      if ((*it).stick_thingy)
        gPainter.Paint2DQuadColor (GfxContext, 
                                   nux::Geometry (bkg_box.x, (*it).render_center.y - 3, bkg_box.width, 2), 
                                   nux::Color(0xAAFFFFFF));
                                   
      if ((*it).x_rotation < 0.0f || (*it).skip)
        continue;

      DrawRenderArg (GfxContext, *it, bkg_box);
    }


    gPainter.Paint2DQuadColor (GfxContext, nux::Geometry (bkg_box.x + bkg_box.width - 1, bkg_box.y, 1, bkg_box.height), nux::Color(0x60FFFFFF));

    GfxContext.GetRenderStates().SetColorMask (true, true, true, true);
    GfxContext.GetRenderStates ().SetSeparateBlend (false,
                                                    GL_SRC_ALPHA,
                                                    GL_ONE_MINUS_SRC_ALPHA,
                                                    GL_SRC_ALPHA,
                                                    GL_ONE_MINUS_SRC_ALPHA);

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
    SetTimeStruct (&_drag_threshold_time, &_drag_threshold_time, ANIM_DURATION_SHORT);
  
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
        request_reorder_smart.emit (_drag_icon, hovered_icon, true);
      else if (progress == 0.0f)
        request_reorder_before.emit (_drag_icon, hovered_icon, false);
    }
  }
}

void Launcher::RecvMouseDown(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  SetMousePosition (x, y);

  MouseDownLogic (x, y, button_flags, key_flags);
  EnsureAnimation ();
}

void Launcher::RecvMouseUp(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  SetMousePosition (x, y);
  nux::Geometry geo = GetGeometry ();

  if (_launcher_action_state != ACTION_NONE && !geo.IsInside(nux::Point(x, y)))
  {
    // we are no longer hovered
    EnsureHoverState ();
  }
  
  MouseUpLogic (x, y, button_flags, key_flags);

  if (_launcher_action_state == ACTION_DRAG_ICON)
    EndIconDrag ();

  _launcher_action_state = ACTION_NONE;
  _dnd_delta_x = 0;
  _dnd_delta_y = 0;
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
      _launcher_action_state == ACTION_NONE)
      return;

  if (_icon_under_mouse)
  {
    _icon_under_mouse->MouseLeave.emit ();
    _icon_under_mouse->_mouse_inside = false;
    _icon_under_mouse = 0;
  }

  if (_launcher_action_state == ACTION_NONE)
  {
    SetTimeStruct (&_drag_start_time);
    
    if (nux::Abs (_dnd_delta_y) >= nux::Abs (_dnd_delta_x))
    {
      _launcher_drag_delta += _dnd_delta_y;
      _launcher_action_state = ACTION_DRAG_LAUNCHER;
    }
    else
    {
      LauncherIcon *drag_icon = MouseIconIntersection ((int) (GetGeometry ().x / 2.0f), y);
      
      if (drag_icon)
      {
        StartIconDrag (drag_icon);
        _launcher_action_state = ACTION_DRAG_ICON;
        UpdateDragWindowPosition (x, y);
      }

    }
  }
  else if (_launcher_action_state == ACTION_DRAG_LAUNCHER)
  {
    _launcher_drag_delta += dy;
  }
  else if (_launcher_action_state == ACTION_DRAG_ICON)
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

  if (_launcher_action_state == ACTION_NONE)
      EnsureHoverState ();

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
  if (_launcher_action_state != ACTION_NONE)
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

    if (_launcher_action_state == ACTION_NONE)
      _icon_mouse_down->MouseClick.emit (nux::GetEventButton (button_flags));
  }

  if (launcher_icon && (_icon_mouse_down != launcher_icon))
  {
    launcher_icon->MouseUp.emit (nux::GetEventButton (button_flags));
  }

  if (_launcher_action_state == ACTION_DRAG_LAUNCHER)
  {
    SetTimeStruct (&_drag_end_time);
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
