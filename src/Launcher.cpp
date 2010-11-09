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

#define TimeDelta(tv1, tv2)						   \
    (((tv1)->tv_sec == (tv2)->tv_sec || (tv1)->tv_usec >= (tv2)->tv_usec) ? \
    ((((tv1)->tv_sec - (tv2)->tv_sec) * 1000000) +			   \
     ((tv1)->tv_usec - (tv2)->tv_usec)) / 1000 :			   \
    ((((tv1)->tv_sec - 1 - (tv2)->tv_sec) * 1000000) +			   \
     (1000000 + (tv1)->tv_usec - (tv2)->tv_usec)) / 1000)

#define ANIM_DURATION_SHORT 125
#define ANIM_DURATION       200
#define ANIM_DURATION_LONG  350

#define BACKLIGHT_STRENGTH  0.9f

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
attribute vec4 iColor;                                                  \n\
attribute vec4 iTexCoord0;                                              \n\
attribute vec4 iVertex;                                                 \n\
                                                                        \n\
varying vec4 varyTexCoord0;                                             \n\
varying vec4 varyVertexColor;                                           \n\
                                                                        \n\
void main()                                                             \n\
{                                                                       \n\
    varyTexCoord0 = iTexCoord0;                                         \n\
    varyVertexColor = iColor;                                           \n\
    gl_Position =  ViewProjectionMatrix * iVertex;                      \n\
}                                                                       \n\
                                                                        \n\
[Fragment Shader]                                                       \n\
#version 120                                                            \n\
#extension GL_ARB_texture_rectangle : enable                            \n\
                                                                        \n\
varying vec4 varyTexCoord0;                                             \n\
varying vec4 varyVertexColor;                                           \n\
                                                                        \n\
uniform sampler2D TextureObject0;                                       \n\
uniform vec4 color0;                                                     \n\
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
  gl_FragColor = texel*varyVertexColor;                                 \n\
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
                            MOV pcoord, fragment.texcoord[0].w;         \n\
                            RCP temp, fragment.texcoord[0].w;           \n\
                            MUL pcoord.xy, fragment.texcoord[0], temp;  \n\
                            TEX tex0, pcoord, texture[0], 2D;           \n\
                            MUL result.color, color0, tex0;     \n\
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

Launcher::Launcher(nux::BaseWindow *parent, NUX_FILE_LINE_DECL)
:   View(NUX_FILE_LINE_PARAM)
,   m_ContentOffsetY(0)
,   m_RunningIndicator(0)
,   m_ActiveIndicator(0)
,   m_BackgroundLayer(0)
,   _model (0)
{
    _parent = parent;
    m_Layout = new nux::HLayout(NUX_TRACKER_LOCATION);

    OnMouseDown.connect(sigc::mem_fun(this, &Launcher::RecvMouseDown));
    OnMouseUp.connect(sigc::mem_fun(this, &Launcher::RecvMouseUp));
    OnMouseDrag.connect(sigc::mem_fun(this, &Launcher::RecvMouseDrag));
    OnMouseEnter.connect(sigc::mem_fun(this, &Launcher::RecvMouseEnter));
    OnMouseLeave.connect(sigc::mem_fun(this, &Launcher::RecvMouseLeave));
    OnMouseMove.connect(sigc::mem_fun(this, &Launcher::RecvMouseMove));
    OnMouseWheel.connect(sigc::mem_fun(this, &Launcher::RecvMouseWheel));

    m_ActiveTooltipIcon = NULL;
    m_ActiveMenuIcon = NULL;

    SetCompositionLayout(m_Layout);

    if(!USE_ARB_SHADERS)
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

    _folded_angle         = 1.0f;
    _neg_folded_angle     = -1.0f;
    _space_between_icons  = 5;
    _launcher_top_y       = 0;
    _launcher_bottom_y    = 0;
    _folded_z_distance    = 10.0f;
    _launcher_state         = LAUNCHER_FOLDED;
    _launcher_action_state  = ACTION_NONE;
    _icon_under_mouse       = NULL;
    _icon_mouse_down        = NULL;
    _icon_image_size        = 48;
    _icon_image_size_delta  = 6;
    _icon_size              = _icon_image_size + _icon_image_size_delta;
    _icon_bkg_texture       = nux::CreateTextureFromFile (PKGDATADIR"/round_corner_54x54.png");
    _icon_outline_texture   = nux::CreateTextureFromFile (PKGDATADIR"/round_outline_54x54.png");
    _icon_shine_texture     = nux::CreateTextureFromFile (PKGDATADIR"/round_shine_54x54.png");
    _dnd_security           = 15;
    _dnd_delta              = 0;
    _anim_handle            = 0;
    _autohide_handle        = 0;
    _floating               = false;
    _hovered                = false;
    _autohide               = false;
    _hidden                 = false;
    
    // 0 out timers to avoid wonky startups
    _enter_time.tv_sec = 0;
    _enter_time.tv_usec = 0;
    _exit_time.tv_sec = 0;
    _exit_time.tv_usec = 0;
    _drag_end_time.tv_sec = 0;
    _drag_end_time.tv_usec = 0;
    _autohide_time.tv_sec = 0;
    _autohide_time.tv_usec = 0;
}

Launcher::~Launcher()
{

}

float Launcher::GetHoverProgress ()
{
    struct timeval current;
    gettimeofday (&current, NULL);
    
    if (_hovered)
        return MIN (1.0f, (float) (TimeDelta (&current, &_enter_time)) / (float) ANIM_DURATION);
    else
        return 1.0f - MIN (1.0f, (float) (TimeDelta (&current, &_exit_time)) / (float) ANIM_DURATION);
}

float Launcher::DnDExitProgress ()
{
    struct timeval current;
    gettimeofday (&current, NULL);
    
    return 1.0f - MIN (1.0f, (float) (TimeDelta (&current, &_drag_end_time)) / (float) ANIM_DURATION_LONG);
}

float Launcher::AutohideProgress ()
{
    if (!_autohide)
        return 0.0f;
        
    struct timeval current;
    gettimeofday (&current, NULL);
    
    if (_hidden)
        return MIN (1.0f, (float) (TimeDelta (&current, &_autohide_time)) / (float) ANIM_DURATION_LONG);
    else
        return 1.0f - MIN (1.0f, (float) (TimeDelta (&current, &_autohide_time)) / (float) ANIM_DURATION_LONG);
}

gboolean Launcher::AnimationTimeout (gpointer data)
{
    Launcher *self = (Launcher*) data;
    
    self->NeedRedraw ();
    
    if (self->AnimationInProgress ())
      return true;
    
    // zero out handle so we know we are done
    self->_anim_handle = 0;
    return false;
}

void Launcher::EnsureAnimation ()
{
    if (_anim_handle)
      return;
    
    NeedRedraw ();
    
    if (AnimationInProgress ())
        _anim_handle = g_timeout_add (1000 / 60 - 1, &Launcher::AnimationTimeout, this);
}

bool Launcher::IconNeedsAnimation (LauncherIcon *icon, struct timeval current)
{
    struct timeval enter_time = icon->ShowTime ();
    if (TimeDelta (&current, &enter_time) < ANIM_DURATION_SHORT)
        return true;
    
    struct timeval hide_time = icon->HideTime ();
    if (TimeDelta (&current, &hide_time) < ANIM_DURATION_SHORT)
        return true;
    
    struct timeval running_time = icon->RunningTime ();
    if (TimeDelta (&current, &running_time) < ANIM_DURATION_SHORT)
        return true;
    
    if (icon->Urgent ())
    {
        struct timeval urgent_time = icon->UrgentTime ();
        if (TimeDelta (&current, &urgent_time) < (ANIM_DURATION_LONG * 4))
            return true;
    }
    
    return false;
}

bool Launcher::AnimationInProgress ()
{
    // performance here can be improved by caching the longer remaining animation found and short circuiting to that each time
    // this way extra checks may be avoided

    // short circuit to avoid unneeded calculations
    struct timeval current;
    gettimeofday (&current, NULL);
    
    // hover in animation
    if (TimeDelta (&current, &_enter_time) < ANIM_DURATION)
       return true;
    
    // hover out animation
    if (TimeDelta (&current, &_exit_time) < ANIM_DURATION)
        return true;
    
    // drag end animation
    if (TimeDelta (&current, &_drag_end_time) < ANIM_DURATION_LONG)
        return true;
    
    if (TimeDelta (&current, &_autohide_time) < ANIM_DURATION_LONG)
        return true;
    
    // animations happening on specific icons
    LauncherModel::iterator it;
    for (it = _model->begin  (); it != _model->end (); it++)
        if (IconNeedsAnimation (*it, current))
            return true;
    
    return false;
}

void Launcher::SetTimeStruct (struct timeval *timer, struct timeval *sister, int sister_relation)
{
    struct timeval current;
    gettimeofday (&current, NULL);
    
    if (sister)
    {
        int diff = TimeDelta (&current, sister);
        
        if (diff < sister_relation)
        {
            int remove = sister_relation - diff;
            current.tv_sec -= remove / 1000;
            remove = remove % 1000;
            
            if (remove > current.tv_usec / 1000)
            {
                current.tv_sec--;
                current.tv_usec += 1000000;
            }
            current.tv_usec -= remove * 1000;
        }
    }
    
    timer->tv_sec = current.tv_sec;
    timer->tv_usec = current.tv_usec;
}

float SizeModifierForIcon (LauncherIcon *icon, struct timeval current)
{
    if (icon->Visible ())
    {
        struct timeval icon_visible_time = icon->ShowTime ();
        int enter_ms = TimeDelta (&current, &icon_visible_time);
        return MIN (1.0f,  (float) enter_ms / (float) ANIM_DURATION_SHORT);
    }
    else
    {
        struct timeval icon_hide_time = icon->HideTime ();
        int hide_ms = TimeDelta (&current, &icon_hide_time);
        return 1.0f - MIN (1.0f,  (float) hide_ms / (float) ANIM_DURATION_SHORT);
    }
}

std::list<Launcher::RenderArg> Launcher::RenderArgs (nux::Geometry &box_geo)
{
    nux::Geometry geo = GetGeometry ();
    LauncherModel::iterator it;
    nux::Point3 center;
    std::list<Launcher::RenderArg> result;
    float hover_progress = GetHoverProgress ();
    float folded_z_distance = _folded_z_distance * (1.0f - hover_progress);
    float animation_neg_rads = _neg_folded_angle * (1.0f - hover_progress);
    struct timeval current;
    gettimeofday (&current, NULL);

    float folding_constant = 0.2f;
    float folding_not_constant = folding_constant + ((1.0f - folding_constant) * hover_progress);
    
    int folded_size = (int) (_icon_size * folding_not_constant);
    int folded_spacing = (int) (_space_between_icons * folding_not_constant);
    
    center.x = geo.width / 2;
    center.y = _space_between_icons;
    center.z = 0;
    
    float sum = 0.0f + center.y;
    for (it = _model->begin (); it != _model->end (); it++)
        sum += (_icon_size + _space_between_icons) * SizeModifierForIcon (*it, current);

    float overflow = sum - geo.height;
    int folding_threshold = geo.height - (overflow * folding_constant) - _icon_size / 3;

    if (hover_progress > 0.0f)
    {
        int delta_y = _dnd_delta;
        
        if (hover_progress > 0.0f)
            delta_y *= hover_progress;
        
        if (hover_progress >= 1.0f && _launcher_action_state != ACTION_DRAG_LAUNCHER)
        {
            float dnd_progress = DnDExitProgress ();
            
            float max = 0;
            float min = MIN (0.0f, geo.height - sum);
            if (_dnd_delta > max)
                delta_y = max + (delta_y - max) * dnd_progress * hover_progress;
            else if (_dnd_delta < min)
                delta_y = min + (delta_y - min) * dnd_progress * hover_progress;
            
            if (dnd_progress == 0.0f)
                _dnd_delta = (int) delta_y;
        }
        
        center.y += delta_y;
    } 
    else 
    {
        _dnd_delta = 0;
    }
    
    float autohide_progress = AutohideProgress ();
    if (_autohide && autohide_progress > 0.0f)
    {
        center.y -= geo.height * autohide_progress;
    }
    
    // Inform the painter where to paint the box
    box_geo = geo;

    if (_floating)
        box_geo.height = sum + _space_between_icons;
    
    if (_autohide)
        box_geo.height -= box_geo.height * autohide_progress;
    
    // The functional position we wish to represent for these icons is not smooth. Rather than introducing
    // special casing to represent this, we use MIN/MAX functions. This helps ensure that even though our
    // function is not smooth it is continuous, which is more important for our visual representation (icons
    // wont start jumping around).  As a general rule ANY if () statements that modify center.y should be seen
    // as bugs.
    for (it = _model->begin (); it != _model->end (); it++)
    {
        RenderArg arg;
        LauncherIcon *icon = *it;
        
        arg.icon           = icon;
        arg.alpha          = 1.0f;
        arg.glow_intensity = 0.0f;
        arg.running_arrow  = false;
        arg.active_arrow   = icon->Active ();
        arg.folding_rads   = 0.0f;
        arg.skip           = false;
        
        // animate this shit
        struct timeval running_time = icon->RunningTime ();
        int running_ms = TimeDelta (&current, &running_time);
        float running_progress = MIN (1.0f, (float) running_ms / (float) ANIM_DURATION_SHORT);

        if (icon->Running ())
        {
          arg.backlight_intensity = running_progress * BACKLIGHT_STRENGTH;
          
          if (icon->Urgent ())
          {
              struct timeval urgent_time = icon->UrgentTime ();
              int urgent_ms = TimeDelta (&current, &urgent_time);
              double urgent_progress = (double) MIN (1.0f, (float) urgent_ms / (float) (ANIM_DURATION_LONG * 4));
              
              arg.backlight_intensity *= 0.5f + (float) (std::cos (M_PI * 4.0f * urgent_progress)) * 0.5f;
          }
        }
        else
        {
          if (running_ms > ANIM_DURATION_SHORT)
            arg.backlight_intensity = 0.0f;
          else
            arg.backlight_intensity = BACKLIGHT_STRENGTH - running_progress * BACKLIGHT_STRENGTH;
        }
        
        // reset z
        center.z = 0;
        
        float size_modifier = SizeModifierForIcon (icon, current);
        if (size_modifier < 1.0f)
        {
            arg.alpha = size_modifier;
            center.z = 300.0f * (1.0f - size_modifier);
        }
        
        if (size_modifier <= 0.0f)
        {
            arg.skip = true;
            continue;
        }
        
        // goes for 0.0f when fully unfolded, to 1.0f folded
        float transition_progress = MIN (1.0f, MAX (0.0f, (center.y + _icon_size - folding_threshold)) / (float) _icon_size);
        float half_size = (folded_size / 2.0f) + (_icon_size / 2.0f - folded_size / 2.0f) * (1.0f - transition_progress);
      
        // icon is crossing threshold, start folding
        arg.center.z += folded_z_distance * transition_progress;
        arg.folding_rads = animation_neg_rads * transition_progress;
        
        center.y += half_size * size_modifier;   // move to center
        arg.center = nux::Point3 (center);       // copy center
        center.y += half_size * size_modifier;   // move to end
        
        float spacing_overlap = MAX (0.0f, 
                                 MIN (1.0f, 
                                  (float) (center.y + (_space_between_icons * size_modifier) - folding_threshold) / (float) _icon_size));
        //add spacing
        center.y += (_space_between_icons * (1.0f - spacing_overlap) + folded_spacing * spacing_overlap) * size_modifier;
        
        result.push_back (arg);
    }
    
    return result;
}

void Launcher::SetHidden (bool hidden)
{
    if (hidden == _hidden)
        return;
        
    _hidden = hidden;
    SetTimeStruct (&_autohide_time, &_autohide_time, ANIM_DURATION);
    
    _parent->EnableInputWindow(!hidden);
    
    EnsureAnimation ();
}

gboolean Launcher::OnAutohideTimeout (gpointer data)
{
    Launcher *self = (Launcher*) data;
 
    if (self->_hovered || self->_hidden)
        return false;
    
    self->SetHidden (true);

    self->_autohide_handle = 0;
    return false;
}

void Launcher::OnTriggerMouseEnter (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
    if (!_autohide || !_hidden)
        return;
    
    SetHidden (false);
}

void Launcher::SetupAutohideTimer ()
{
    if (_autohide)
    {
        if (_autohide_handle > 0)
            g_source_remove (_autohide_handle);
        _autohide_handle = g_timeout_add (2000, &Launcher::OnAutohideTimeout, this);
    }
}

void Launcher::OnTriggerMouseLeave (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
    SetupAutohideTimer ();
}

bool Launcher::AutohideEnabled ()
{
    return _autohide;
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

void Launcher::SetHover ()
{
    if (_hovered)
        return;
    
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

void Launcher::OnIconAdded (void *icon_pointer)
{
    LauncherIcon *icon = (LauncherIcon *) icon_pointer;
    icon->Reference ();
    EnsureAnimation();
    
    // needs to be disconnected
    icon->needs_redraw.connect (sigc::mem_fun(this, &Launcher::OnIconNeedsRedraw));
}

void Launcher::OnIconRemoved (void *icon_pointer)
{
    LauncherIcon *icon = (LauncherIcon *) icon_pointer;
    icon->UnReference ();
    EnsureAnimation();
}

void Launcher::OnOrderChanged ()
{

}

void Launcher::SetModel (LauncherModel *model)
{
    _model = model;
    _model->icon_added.connect (sigc::mem_fun (this, &Launcher::OnIconAdded));
    _model->icon_removed.connect (sigc::mem_fun (this, &Launcher::OnIconRemoved));
    _model->order_changed.connect (sigc::mem_fun (this, &Launcher::OnOrderChanged));
}

void Launcher::OnIconNeedsRedraw (void *icon)
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

void Launcher::RenderIcon(nux::GraphicsEngine& GfxContext, RenderArg arg, nux::BaseTexture *icon, nux::Color bkg_color, float alpha)
{
  nux::Geometry geo = GetGeometry();

  nux::Matrix4 ObjectMatrix;
  nux::Matrix4 ViewMatrix;
  nux::Matrix4 ProjectionMatrix;
  nux::Matrix4 ViewProjectionMatrix;
  
  if(nux::Abs (arg.folding_rads) < 0.15f)
    icon->GetDeviceTexture()->SetFiltering(GL_NEAREST, GL_NEAREST);
  else
    icon->GetDeviceTexture()->SetFiltering(GL_LINEAR, GL_LINEAR);

  nux::Vector4 v0;
  nux::Vector4 v1;
  nux::Vector4 v2;
  nux::Vector4 v3;

  v0.x = arg.icon->_xform_screen_coord[0].x ;
  v0.y = arg.icon->_xform_screen_coord[0].y ;
  v0.z = arg.icon->_xform_screen_coord[0].z ;
  v0.w = arg.icon->_xform_screen_coord[0].w ;
  v1.x = arg.icon->_xform_screen_coord[1].x ;
  v1.y = arg.icon->_xform_screen_coord[1].y ;
  v1.z = arg.icon->_xform_screen_coord[1].z ;
  v1.w = arg.icon->_xform_screen_coord[1].w ;
  v2.x = arg.icon->_xform_screen_coord[2].x ;
  v2.y = arg.icon->_xform_screen_coord[2].y ;
  v2.z = arg.icon->_xform_screen_coord[2].z ;
  v2.w = arg.icon->_xform_screen_coord[2].w ;
  v3.x = arg.icon->_xform_screen_coord[3].x ;
  v3.y = arg.icon->_xform_screen_coord[3].y ;
  v3.z = arg.icon->_xform_screen_coord[3].z ;
  v3.w = arg.icon->_xform_screen_coord[3].w ;

  nux::Color color;
  color = nux::Color::White;

  float s0, t0, s1, t1, s2, t2, s3, t3;
  
  if (icon->Type ().IsDerivedFromType(nux::TextureRectangle::StaticObjectType))
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
    v0.x, v0.y, 0.0f, 1.0f,     s0/v0.w, t0/v0.w, 0.0f, 1.0f/v0.w,     color.R(), color.G(), color.B(), color.A(),
    v1.x, v1.y, 0.0f, 1.0f,     s1/v1.w, t1/v1.w, 0.0f, 1.0f/v1.w,     color.R(), color.G(), color.B(), color.A(),
    v2.x, v2.y, 0.0f, 1.0f,     s2/v2.w, t2/v2.w, 0.0f, 1.0f/v2.w,     color.R(), color.G(), color.B(), color.A(),
    v3.x, v3.y, 0.0f, 1.0f,     s3/v3.w, t3/v3.w, 0.0f, 1.0f/v3.w,     color.R(), color.G(), color.B(), color.A(),
  };

  CHECKGL(glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0));
  CHECKGL(glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0));

  int TextureObjectLocation;
  int VertexLocation;
  int TextureCoord0Location;
  int VertexColorLocation;
  int FragmentColor;

  if(!USE_ARB_SHADERS)
  {
    _shader_program_uv_persp_correction->Begin();

    TextureObjectLocation   = _shader_program_uv_persp_correction->GetUniformLocationARB("TextureObject0");
    VertexLocation          = _shader_program_uv_persp_correction->GetAttributeLocation("iVertex");
    TextureCoord0Location   = _shader_program_uv_persp_correction->GetAttributeLocation("iTexCoord0");
    VertexColorLocation     = _shader_program_uv_persp_correction->GetAttributeLocation("iColor");
    FragmentColor           = _shader_program_uv_persp_correction->GetUniformLocationARB ("color");
    
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
  CHECKGL( glVertexAttribPointerARB((GLuint)VertexLocation, 4, GL_FLOAT, GL_FALSE, 48, VtxBuffer) );

  if(TextureCoord0Location != -1)
  {
    CHECKGL( glEnableVertexAttribArrayARB(TextureCoord0Location) );
    CHECKGL( glVertexAttribPointerARB((GLuint)TextureCoord0Location, 4, GL_FLOAT, GL_FALSE, 48, VtxBuffer + 4) );
  }

  if(VertexColorLocation != -1)
  {
    CHECKGL( glEnableVertexAttribArrayARB(VertexColorLocation) );
    CHECKGL( glVertexAttribPointerARB((GLuint)VertexColorLocation, 4, GL_FLOAT, GL_FALSE, 48, VtxBuffer + 8) );
  }
  
  bkg_color.SetAlpha (bkg_color.A () * alpha);
  
  if(!USE_ARB_SHADERS)
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
  if(VertexColorLocation != -1)
    CHECKGL( glDisableVertexAttribArrayARB(VertexColorLocation) );

  if(!USE_ARB_SHADERS)
  {
    _shader_program_uv_persp_correction->End();
  }
  else
  {
    _AsmShaderProg->End();
  }
}

void Launcher::RenderIconImage(nux::GraphicsEngine& GfxContext, RenderArg arg)
{
  nux::Geometry geo = GetGeometry();

  nux::Matrix4 ObjectMatrix;
  nux::Matrix4 ViewMatrix;
  nux::Matrix4 ProjectionMatrix;
  nux::Matrix4 ViewProjectionMatrix;
  nux::BaseTexture *icon;
 
  icon = arg.icon->TextureForSize (_icon_image_size);
  
  if(nux::Abs (arg.folding_rads) < 0.15f)
  {
    icon->GetDeviceTexture()->SetFiltering(GL_NEAREST, GL_NEAREST);
  }
  else
  {
    icon->GetDeviceTexture()->SetFiltering(GL_LINEAR, GL_LINEAR);
  }

  nux::Vector4 v0;
  nux::Vector4 v1;
  nux::Vector4 v2;
  nux::Vector4 v3;

  v0.x = arg.icon->_xform_icon_screen_coord[0].x;
  v0.y = arg.icon->_xform_icon_screen_coord[0].y;
  v0.z = arg.icon->_xform_icon_screen_coord[0].z;
  v0.w = arg.icon->_xform_icon_screen_coord[0].w;
  v1.x = arg.icon->_xform_icon_screen_coord[1].x;
  v1.y = arg.icon->_xform_icon_screen_coord[1].y;
  v1.z = arg.icon->_xform_icon_screen_coord[1].z;
  v1.w = arg.icon->_xform_icon_screen_coord[1].w;
  v2.x = arg.icon->_xform_icon_screen_coord[2].x;
  v2.y = arg.icon->_xform_icon_screen_coord[2].y;
  v2.z = arg.icon->_xform_icon_screen_coord[2].z;
  v2.w = arg.icon->_xform_icon_screen_coord[2].w;
  v3.x = arg.icon->_xform_icon_screen_coord[3].x;
  v3.y = arg.icon->_xform_icon_screen_coord[3].y;
  v3.z = arg.icon->_xform_icon_screen_coord[3].z;
  v3.w = arg.icon->_xform_icon_screen_coord[3].w;
  int inside = arg.icon->_mouse_inside; //PointInside2DPolygon(arg.icon->_xform_screen_coord, 4, _mouse_position, 1);

  nux::Color color;
  if(inside)
    color = nux::Color::Red;
  else
    color = nux::Color::White;

  float s0, t0, s1, t1, s2, t2, s3, t3;
  
  if (icon->Type ().IsDerivedFromType(nux::TextureRectangle::StaticObjectType))
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
    v0.x, v0.y, 0.0f, 1.0f,     s0/v0.w, t0/v0.w, 0.0f, 1.0f/v0.w,     color.R(), color.G(), color.B(), color.A(),
    v1.x, v1.y, 0.0f, 1.0f,     s1/v1.w, t1/v1.w, 0.0f, 1.0f/v1.w,     color.R(), color.G(), color.B(), color.A(),
    v2.x, v2.y, 0.0f, 1.0f,     s2/v2.w, t2/v2.w, 0.0f, 1.0f/v2.w,     color.R(), color.G(), color.B(), color.A(),
    v3.x, v3.y, 0.0f, 1.0f,     s3/v3.w, t3/v3.w, 0.0f, 1.0f/v3.w,     color.R(), color.G(), color.B(), color.A(),
  };

  CHECKGL(glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0));
  CHECKGL(glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0));

  int TextureObjectLocation;
  int VertexLocation;
  int TextureCoord0Location;
  int VertexColorLocation;
  int FragmentColor;

  if(!USE_ARB_SHADERS)
  {
    _shader_program_uv_persp_correction->Begin();

    TextureObjectLocation   = _shader_program_uv_persp_correction->GetUniformLocationARB("TextureObject0");
    VertexLocation          = _shader_program_uv_persp_correction->GetAttributeLocation("iVertex");
    TextureCoord0Location   = _shader_program_uv_persp_correction->GetAttributeLocation("iTexCoord0");
    VertexColorLocation     = _shader_program_uv_persp_correction->GetAttributeLocation("iColor");
    FragmentColor           = _shader_program_uv_persp_correction->GetUniformLocationARB ("color");
    
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

    nux::GetGraphicsEngine ().SetTexture(GL_TEXTURE0, icon);
  }

  CHECKGL( glEnableVertexAttribArrayARB(VertexLocation) );
  CHECKGL( glVertexAttribPointerARB((GLuint)VertexLocation, 4, GL_FLOAT, GL_FALSE, 48, VtxBuffer) );

  if(TextureCoord0Location != -1)
  {
    CHECKGL( glEnableVertexAttribArrayARB(TextureCoord0Location) );
    CHECKGL( glVertexAttribPointerARB((GLuint)TextureCoord0Location, 4, GL_FLOAT, GL_FALSE, 48, VtxBuffer + 4) );
  }

  if(VertexColorLocation != -1)
  {
    CHECKGL( glEnableVertexAttribArrayARB(VertexColorLocation) );
    CHECKGL( glVertexAttribPointerARB((GLuint)VertexColorLocation, 4, GL_FLOAT, GL_FALSE, 48, VtxBuffer + 8) );
  }

  nux::Color bkg_color = nux::Color::White;
  nux::Color white = nux::Color::White;
  
  bkg_color.SetAlpha (bkg_color.A () * arg.alpha);
  
  if(!USE_ARB_SHADERS)
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
  if(VertexColorLocation != -1)
    CHECKGL( glDisableVertexAttribArrayARB(VertexColorLocation) );

  if(!USE_ARB_SHADERS)
  {
    _shader_program_uv_persp_correction->End();
  }
  else
  {
    _AsmShaderProg->End();
  }
  
  int markerCenter = (v1.y + v0.y) / 2;
  
  if (arg.running_arrow)
  {
    if (!m_RunningIndicator)
    {
      GdkPixbuf *pbuf = gdk_pixbuf_new_from_file (PKGDATADIR"/running_indicator.png", NULL);
      m_RunningIndicator = nux::CreateTextureFromPixbuf (pbuf);
      g_object_unref (pbuf);
    }
    gPainter.Draw2DTexture (GfxContext, m_RunningIndicator, 0, markerCenter - (m_ActiveIndicator->GetHeight () / 2));
  }
  
  if (arg.active_arrow)
  {
    if (!m_ActiveIndicator)
    {
      GdkPixbuf *pbuf = gdk_pixbuf_new_from_file (PKGDATADIR"/focused_indicator.png", NULL);
      m_ActiveIndicator = nux::CreateTextureFromPixbuf (pbuf);
      g_object_unref (pbuf);
    }
    gPainter.Draw2DTexture (GfxContext, m_ActiveIndicator, geo.width - m_ActiveIndicator->GetWidth (), markerCenter - (m_ActiveIndicator->GetHeight () / 2));
  }
}

void Launcher::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw)
{
    nux::Geometry base = GetGeometry();
    GfxContext.PushClippingRectangle(base);
    nux::Geometry bkg_box;

    nux::ROPConfig ROP;
    ROP.Blend = false;
    ROP.SrcBlend = GL_SRC_ALPHA;
    ROP.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

    std::list<Launcher::RenderArg> args = RenderArgs (bkg_box);

    gPainter.PushDrawColorLayer(GfxContext, base, nux::Color(0x00000000), true, ROP);
    gPainter.PushDrawColorLayer(GfxContext, bkg_box, nux::Color(0x99000000), true, ROP);
    gPainter.PushDrawColorLayer(GfxContext, nux::Geometry (bkg_box.x + bkg_box.width - 1, bkg_box.y, 1, bkg_box.height), nux::Color(0x60FFFFFF), true, ROP);
    
    UpdateIconXForm (args);
    EventLogic ();
    
    std::list<Launcher::RenderArg>::reverse_iterator rev_it;
    for (rev_it = args.rbegin (); rev_it != args.rend (); rev_it++)
    {
      if ((*rev_it).folding_rads >= 0.0f || (*rev_it).skip)
        continue;
        
      RenderArg arg = *rev_it;

      GfxContext.GetRenderStates ().SetSeparateBlend (true,
                                                    GL_SRC_ALPHA,
                                                    GL_ONE_MINUS_SRC_ALPHA,
                                                    GL_ONE_MINUS_DST_ALPHA,
                                                    GL_ONE);

      GfxContext.GetRenderStates ().SetColorMask (true, true, true, true);
      
      if (arg.backlight_intensity < 1.0f)
        RenderIcon(GfxContext, arg, _icon_outline_texture, nux::Color(0xFF6D6D6D), 1.0f - arg.backlight_intensity);
      if (arg.backlight_intensity > 0.0f)
        RenderIcon(GfxContext, arg, _icon_bkg_texture, arg.icon->BackgroundColor (), arg.backlight_intensity);

      GfxContext.GetRenderStates ().SetSeparateBlend (true,
                                                    GL_SRC_ALPHA,
                                                    GL_ONE_MINUS_SRC_ALPHA,
                                                    GL_ONE_MINUS_DST_ALPHA,
                                                    GL_ONE);
      GfxContext.GetRenderStates ().SetColorMask (true, true, true, true);
      RenderIconImage (GfxContext, arg);
      
      if (arg.backlight_intensity > 0.0f)
        RenderIcon(GfxContext, arg, _icon_shine_texture, nux::Color::White, arg.backlight_intensity);
    }

    std::list<Launcher::RenderArg>::iterator it;
    for (it = args.begin(); it != args.end(); it++)
    {
      if ((*it).folding_rads < 0.0f || (*it).skip)
        continue;
      
      RenderArg arg = *it;  
      
      GfxContext.GetRenderStates ().SetSeparateBlend (true,
                                                    GL_SRC_ALPHA,
                                                    GL_ONE_MINUS_SRC_ALPHA,
                                                    GL_ONE_MINUS_DST_ALPHA,
                                                    GL_ONE);

      GfxContext.GetRenderStates ().SetColorMask (true, true, true, true);
      if (arg.backlight_intensity < 1.0f)
        RenderIcon(GfxContext, arg, _icon_outline_texture, nux::Color(0xFF6D6D6D), 1.0f - arg.backlight_intensity);
      if (arg.backlight_intensity > 0.0f)
        RenderIcon(GfxContext, arg, _icon_bkg_texture, arg.icon->BackgroundColor (), arg.backlight_intensity);

      GfxContext.GetRenderStates ().SetSeparateBlend (true,
                                                    GL_SRC_ALPHA,
                                                    GL_ONE_MINUS_SRC_ALPHA,
                                                    GL_ONE_MINUS_DST_ALPHA,
                                                    GL_ONE);
      GfxContext.GetRenderStates ().SetColorMask (true, true, true, true);
      RenderIconImage (GfxContext, arg);
      
      if (arg.backlight_intensity > 0.0f)
        RenderIcon(GfxContext, arg, _icon_shine_texture, nux::Color::White, arg.backlight_intensity);
    }
    
    GfxContext.GetRenderStates().SetColorMask (true, true, true, true);
    GfxContext.GetRenderStates ().SetSeparateBlend (false,
                                                    GL_SRC_ALPHA,
                                                    GL_ONE_MINUS_SRC_ALPHA,
                                                    GL_SRC_ALPHA,
                                                    GL_ONE_MINUS_SRC_ALPHA);
          
    gPainter.PopBackground();
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

  _mouse_position = nux::Point2 (0, 0);
  
  return nux::eCompliantHeight | nux::eCompliantWidth;
}

void Launcher::PositionChildLayout(float offsetX, float offsetY)
{
}

bool Launcher::TooltipNotify(LauncherIcon* Icon)
{
    if(GetActiveMenuIcon())
        return false;
    return true;
}

bool Launcher::MenuNotify(LauncherIcon* Icon)
{
    return true;
}

void Launcher::NotifyMenuTermination(LauncherIcon* Icon)
{
}

void Launcher::RecvMouseDown(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  _mouse_position = nux::Point2 (x, y);
  
  MouseDownLogic ();
  EnsureAnimation ();
}

void Launcher::RecvMouseUp(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  _mouse_position = nux::Point2 (x, y);
  nux::Geometry geo = GetGeometry ();

  if (_launcher_action_state == ACTION_DRAG_LAUNCHER && !geo.IsInside(nux::Point(x, y)))
  {
    // we are no longer hovered
    UnsetHover ();
  }

  MouseUpLogic ();
  _launcher_action_state = ACTION_NONE;
  EnsureAnimation ();
}

void Launcher::RecvMouseDrag(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags)
{
  _mouse_position = nux::Point2 (x, y);
  
  _dnd_delta += dy;

  if (nux::Abs (_dnd_delta) < 15 && _launcher_action_state != ACTION_DRAG_LAUNCHER)
      return;
  
  if (_icon_under_mouse)
  {
    _icon_under_mouse->MouseLeave.emit ();
    _icon_under_mouse->_mouse_inside = false;
    _icon_under_mouse = 0;
  }
  
  _launcher_action_state = ACTION_DRAG_LAUNCHER;
  EnsureAnimation ();
}

void Launcher::RecvMouseEnter(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  _mouse_position = nux::Point2 (x, y);
  SetHover ();

  EventLogic ();
  EnsureAnimation ();
}

void Launcher::RecvMouseLeave(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  _mouse_position = nux::Point2 (x, y);
  
  if (_launcher_action_state != ACTION_DRAG_LAUNCHER)
      UnsetHover ();
  
  EventLogic ();
  EnsureAnimation ();
}

void Launcher::RecvMouseMove(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags)
{
  _mouse_position = nux::Point2 (x, y);

  // Every time the mouse moves, we check if it is inside an icon...

  EventLogic ();
}

void Launcher::RecvMouseWheel(int x, int y, int wheel_delta, unsigned long button_flags, unsigned long key_flags)
{
}

void Launcher::EventLogic ()
{ 
  if (_launcher_action_state == ACTION_DRAG_LAUNCHER)
    return;
  
  LauncherIcon* launcher_icon = 0;
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

void Launcher::MouseDownLogic ()
{
  LauncherIcon* launcher_icon = 0;
  launcher_icon = MouseIconIntersection (_mouse_position.x, _mouse_position.y);

  if (launcher_icon)
  {
    _icon_mouse_down = launcher_icon;
    launcher_icon->MouseDown.emit ();
  }
}

void Launcher::MouseUpLogic ()
{
  LauncherIcon* launcher_icon = 0;
  launcher_icon = MouseIconIntersection (_mouse_position.x, _mouse_position.y);

  if (_icon_mouse_down && (_icon_mouse_down == launcher_icon))
  {
    _icon_mouse_down->MouseUp.emit ();
    
    if (_launcher_action_state != ACTION_DRAG_LAUNCHER)
      _icon_mouse_down->MouseClick.emit ();
  }

  if (launcher_icon && (_icon_mouse_down != launcher_icon))
  {
    launcher_icon->MouseUp.emit ();
  }
  
  if (_launcher_action_state == ACTION_DRAG_LAUNCHER)
  {
    SetTimeStruct (&_drag_end_time);
  }
  
  _icon_mouse_down = 0;
}

LauncherIcon* Launcher::MouseIconIntersection (int x, int y)
{
  // We are looking for the icon at screen coordinates x, y;
  nux::Point2 mouse_position(x, y);
  int inside = 0;

  // Because of the way icons fold and stack on one another, we must proceed in 2 steps.
  LauncherModel::reverse_iterator rev_it;
  for (rev_it = _model->rbegin (); rev_it != _model->rend (); rev_it++)
  {
    if ((*rev_it)->_folding_angle < 0.0f || !(*rev_it)->Visible ())
      continue;

    nux::Point2 screen_coord [4];
    for (int i = 0; i < 4; i++)
    {
      screen_coord [i].x = (*rev_it)->_xform_screen_coord [i].x;
      screen_coord [i].y = (*rev_it)->_xform_screen_coord [i].y;
    }
    inside = PointInside2DPolygon (screen_coord, 4, mouse_position, 1);
    if (inside)
      return (*rev_it);
  }

  LauncherModel::iterator it;
  for (it = _model->begin(); it != _model->end (); it++)
  {
    if ((*it)->_folding_angle >= 0.0f || !(*it)->Visible ())
      continue;

    nux::Point2 screen_coord [4];
    for (int i = 0; i < 4; i++)
    {
      screen_coord [i].x = (*it)->_xform_screen_coord [i].x;
      screen_coord [i].y = (*it)->_xform_screen_coord [i].y;
    }
    inside = PointInside2DPolygon (screen_coord, 4, mouse_position, 1);
    if (inside)
      return (*it);
  }

  return 0;
}

void Launcher::UpdateIconXForm (std::list<Launcher::RenderArg> args)
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
    if ((*it).skip)
      continue;
    
    LauncherIcon* launcher_icon = (*it).icon;
    
    // We to store the icon angle in the icons itself. Makes one thing easier afterward.
    launcher_icon->_folding_angle = (*it).folding_rads;
    
    float w = _icon_size;
    float h = _icon_size;
    float x = (*it).center.x - w/2.0f; // x: top left corner
    float y = (*it).center.y - h/2.0f; // y: top left corner
    float z = (*it).center.z;

    ObjectMatrix = nux::Matrix4::TRANSLATE(geo.width/2.0f, geo.height/2.0f, z) * // Translate the icon to the center of the viewport
      nux::Matrix4::ROTATEX((*it).folding_rads) *              // rotate the icon
      nux::Matrix4::TRANSLATE(-x - w/2.0f, -y - h/2.0f, -z);    // Put the center the icon to (0, 0)

    ViewProjectionMatrix = ProjectionMatrix*ViewMatrix*ObjectMatrix;

    // Icon 
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


    launcher_icon->_xform_screen_coord[0].x = v0.x;
    launcher_icon->_xform_screen_coord[0].y = v0.y;
    launcher_icon->_xform_screen_coord[0].z = v0.z;
    launcher_icon->_xform_screen_coord[0].w = v0.w;
    launcher_icon->_xform_screen_coord[1].x = v1.x;
    launcher_icon->_xform_screen_coord[1].y = v1.y;
    launcher_icon->_xform_screen_coord[1].z = v1.z;
    launcher_icon->_xform_screen_coord[1].w = v1.w;
    launcher_icon->_xform_screen_coord[2].x = v2.x;
    launcher_icon->_xform_screen_coord[2].y = v2.y;
    launcher_icon->_xform_screen_coord[2].z = v2.z;
    launcher_icon->_xform_screen_coord[2].w = v2.w;
    launcher_icon->_xform_screen_coord[3].x = v3.x;
    launcher_icon->_xform_screen_coord[3].y = v3.y;
    launcher_icon->_xform_screen_coord[3].z = v3.z;
    launcher_icon->_xform_screen_coord[3].w = v3.w;
    
    
    //// icon image
    w = _icon_image_size;
    h = _icon_image_size;
    x = (*it).center.x - _icon_size/2.0f + _icon_image_size_delta/2.0f;
    y = (*it).center.y - _icon_size/2.0f + _icon_image_size_delta/2.0f;
    z = (*it).center.z;
    
    v0 = nux::Vector4(x,   y,    z, 1.0f);
    v1 = nux::Vector4(x,   y+h,  z, 1.0f);
    v2 = nux::Vector4(x+w, y+h,  z, 1.0f);
    v3 = nux::Vector4(x+w, y,    z, 1.0f);
    
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


    launcher_icon->_xform_icon_screen_coord[0].x = v0.x;
    launcher_icon->_xform_icon_screen_coord[0].y = v0.y;
    launcher_icon->_xform_icon_screen_coord[0].z = v0.z;
    launcher_icon->_xform_icon_screen_coord[0].w = v0.w;

    launcher_icon->_xform_icon_screen_coord[1].x = v1.x;
    launcher_icon->_xform_icon_screen_coord[1].y = v1.y;
    launcher_icon->_xform_icon_screen_coord[1].z = v1.z;
    launcher_icon->_xform_icon_screen_coord[1].w = v1.w;
    launcher_icon->_xform_icon_screen_coord[2].x = v2.x;
    launcher_icon->_xform_icon_screen_coord[2].y = v2.y;
    launcher_icon->_xform_icon_screen_coord[2].z = v2.z;
    launcher_icon->_xform_icon_screen_coord[2].w = v2.w;
    launcher_icon->_xform_icon_screen_coord[3].x = v3.x;
    launcher_icon->_xform_icon_screen_coord[3].y = v3.y;
    launcher_icon->_xform_icon_screen_coord[3].z = v3.z;
    launcher_icon->_xform_icon_screen_coord[3].w = v3.w;    
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
  float y_cs = -CameraToScreenDistance*tanf(0.5f*Fovy/* *3.1415926/180.0f*/);
  float x_cs = y_cs*AspectRatio;
  //float CameraToScreenDistance = -y_cs*1.0f/(tanf(0.5f*Fovy/* *3.1415926/180.0f*/));

  ViewMatrix = nux::Matrix4::TRANSLATE(-x_cs, y_cs, CameraToScreenDistance) * 
    nux::Matrix4::SCALE(2.0f*x_cs/ViewportWidth, -2.0f*y_cs/ViewportHeight, -2.0f * 3 * y_cs/ViewportHeight /* or -2.0f * x_cs/ViewportWidth*/ );

  PerspectiveMatrix.Perspective(Fovy, AspectRatio, NearClipPlane, FarClipPlane);

//   // Example usage with the matrices above:
//   float W = 300;
//   float H = 300;
//   // centered quad
//   float X = (ViewportWidth - W)/2.0;
//   float Y = (ViewportHeight - H)/2.0;
//   float Z = 0.0f;
// 
//   {
//     glPushMatrix();
//     // Local Transformation of the object
//     glTranslatef(0.0f, 0.0f, ObjectDistanceToCamera);
//     glTranslatef(X  + W/2.0f, Y + H/2.0f, 0.0f);
//     glRotatef(cameraAngleY, 1, 0, 0);
//     glRotatef(cameraAngleX, 0, 1, 0);
//     glTranslatef(-X - W/2.0f, -Y - H/2.0f, 0.0f);
// 
//     glBegin(GL_QUADS);
//     {
//       glNormal3f(0.0f, 0.0f, 1.0f);
// 
//       glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
//       glVertex4f(X, Y, Z, 1.0f);
// 
//       glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
//       glVertex4f(X, Y+H, Z, 1.0f);
// 
//       glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
//       glVertex4f(X+W, Y+H, Z, 1.0f);
// 
//       glColor4f(0.0f, 1.0f, 1.0f, 1.0f);
//       glVertex4f(X+W, Y, Z, 1.0f);
//     }
//     glEnd();
}
