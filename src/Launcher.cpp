#include <math.h>

#include "Nux/Nux.h"
#include "Nux/VScrollBar.h"
#include "Nux/HLayout.h"
#include "Nux/VLayout.h"
#include "Nux/MenuPage.h"

#include "NuxGraphics/NuxGraphics.h"
#include "NuxGraphics/GLDeviceFactory.h"
#include "NuxGraphics/GLTextureResourceManager.h"

#include "Nux/BaseWindow.h"
#include "Nux/WindowCompositor.h"

#include "Launcher.h"
#include "LauncherIcon.h"
#include "LauncherModel.h"

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

Launcher::Launcher(NUX_FILE_LINE_DECL)
:   View(NUX_FILE_LINE_PARAM)
,   m_ContentOffsetY(0)
,   m_RunningIndicator(0)
,   m_ActiveIndicator(0)
,   m_BackgroundLayer(0)
,   _model (0)
{
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

    _folding_functor = new nux::TimerFunctor();
    _folding_functor->OnTimerExpired.connect(sigc::mem_fun(this, &Launcher::FoldingCallback));

    _reveal_functor = new nux::TimerFunctor();
    _reveal_functor->OnTimerExpired.connect(sigc::mem_fun(this, &Launcher::RevealCallback));

    _folded_angle         = 1.0f;
    _neg_folded_angle     = -1.0f;
    _folding_angle        = _folded_angle;
    _timer_intervals      = 16;
    _angle_rate           = 0.1f;
    _space_between_icons  = 5;
    _anim_duration        = 200;
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
    _dnd_security           = 15;
}

Launcher::~Launcher()
{

}

std::list<Launcher::RenderArg> Launcher::RenderArgs (float hover_progress)
{
    std::list<Launcher::RenderArg> result;
    int icon_spacing = 10;
    int min_icon_size = 10;
    int animation_icon_size = (int) (10 + (_icon_size - min_icon_size) * hover_progress);
    float animation_neg_rads = _neg_folded_angle * (1.0f - hover_progress);

    nux::Geometry geo = GetGeometry ();
    LauncherModel::iterator it;
    nux::Point3 center;
    
    center.x = geo.width / 2;
    center.y = icon_spacing;
    center.z = 0;
    
    int max_flat_icons = (geo.height - icon_spacing) / (_icon_size + icon_spacing);
    int overflow = MAX (0, _model->Size () - max_flat_icons);
    int folding_threshold = geo.height;
        
    if (overflow > 0)
    {
        folding_threshold = geo.height - (overflow * min_icon_size);        
    }
    
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
        
        // animate this shit
        if (icon->Running ())
          arg.backlight_intensity = 1.0f;
        else
          arg.backlight_intensity = 0.0f;
        
        if (hover_progress >= 1.0f || overflow > 0)
        {
            //wewt no folding work to be done
            center.y += _icon_size / 2;         // move to center
            arg.center = nux::Point3 (center);       // copy center
            center.y += (_icon_size / 2) + 10;  // move to start of next icon
        }
        else
        {
            //foldy mathy time
            if (center.y >= folding_threshold)
            {
                // we are past the threshold, fully fold
                center.y += animation_icon_size / 2;
                arg.center = nux::Point3 (center);
                center.y += animation_icon_size / 2 + (10 * hover_progress);
                
                arg.folding_rads = animation_neg_rads;
            }
            else
            {
                //we can draw flat HUZZAH
                center.y += _icon_size / 2;         // move to center
                arg.center = nux::Point3 (center);       // copy center
                center.y += (_icon_size / 2) + 10;  // move to start of next icon
            }
        }
        
        result.push_back (arg);
    }
    
    return result;
}

void Launcher::SetIconSize(int tile_size, int icon_size, nux::BaseWindow *parent)
{
    nux::Geometry geo = parent->GetGeometry ();
    
    _icon_size = tile_size;
    _icon_image_size = icon_size;
    _icon_image_size_delta = tile_size - icon_size;
    
    // recreate tile textures
    
    parent->SetGeometry (nux::Geometry (geo.x, geo.y, tile_size + 12, geo.height));
}

void Launcher::OnIconAdded (void *icon_pointer)
{
    LauncherIcon *icon = (LauncherIcon *) icon_pointer;

    if (_launcher_state == LAUNCHER_UNFOLDED)
      ScheduleRevealAnimation ();
    else
      ScheduleFoldAnimation ();

    NeedRedraw();
    
    // needs to be disconnected
    icon->needs_redraw.connect (sigc::mem_fun(this, &Launcher::OnIconNeedsRedraw));
}

void Launcher::OnIconRemoved (void *icon_pointer)
{
    if (_launcher_state == LAUNCHER_UNFOLDED)
      ScheduleRevealAnimation ();
    else
      ScheduleFoldAnimation ();
    
    NeedRedraw();
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
    NeedRedraw();
}

long Launcher::ProcessEvent(nux::IEvent &ievent, long TraverseInfo, long ProcessEventInfo)
{
    long ret = TraverseInfo;
    ret = PostProcessEvent2(ievent, ret, ProcessEventInfo);
    return ret;
}

void Launcher::Draw(nux::GraphicsContext& GfxContext, bool force_draw)
{

}

void Launcher::RenderIcon(nux::GraphicsContext& GfxContext, LauncherIcon* launcher_view)
{
  nux::Geometry geo = GetGeometry();

  nux::Matrix4 ObjectMatrix;
  nux::Matrix4 ViewMatrix;
  nux::Matrix4 ProjectionMatrix;
  nux::Matrix4 ViewProjectionMatrix;
  nux::BaseTexture *icon;
  
  if (launcher_view->Running ())
    icon = _icon_bkg_texture; 
  else
    icon = _icon_outline_texture;
  
  icon->GetDeviceTexture()->SetFiltering(GL_NEAREST, GL_NEAREST);

  if(launcher_view->_folding_angle == 0.0f)
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

  v0.x = launcher_view->_xform_screen_coord[0].x ;
  v0.y = launcher_view->_xform_screen_coord[0].y ;
  v0.z = launcher_view->_xform_screen_coord[0].z ;
  v0.w = launcher_view->_xform_screen_coord[0].w ;
  v1.x = launcher_view->_xform_screen_coord[1].x ;
  v1.y = launcher_view->_xform_screen_coord[1].y ;
  v1.z = launcher_view->_xform_screen_coord[1].z ;
  v1.w = launcher_view->_xform_screen_coord[1].w ;
  v2.x = launcher_view->_xform_screen_coord[2].x ;
  v2.y = launcher_view->_xform_screen_coord[2].y ;
  v2.z = launcher_view->_xform_screen_coord[2].z ;
  v2.w = launcher_view->_xform_screen_coord[2].w ;
  v3.x = launcher_view->_xform_screen_coord[3].x ;
  v3.y = launcher_view->_xform_screen_coord[3].y ;
  v3.z = launcher_view->_xform_screen_coord[3].z ;
  v3.w = launcher_view->_xform_screen_coord[3].w ;

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
    
    nux::GetThreadGraphicsContext ()->SetTexture(GL_TEXTURE0, icon);

    if(TextureObjectLocation != -1)
      CHECKGL( glUniform1iARB (TextureObjectLocation, 0) );

    int VPMatrixLocation = _shader_program_uv_persp_correction->GetUniformLocationARB("ViewProjectionMatrix");
    if(VPMatrixLocation != -1)
    {
      nux::Matrix4 mat = nux::GetThreadGraphicsContext ()->GetModelViewProjectionMatrix ();
      _shader_program_uv_persp_correction->SetUniformLocMatrix4fv ((GLint)VPMatrixLocation, 1, false, (GLfloat*)&(mat.m));
    }
  }
  else
  {
    _AsmShaderProg->Begin();

    VertexLocation        = nux::VTXATTRIB_POSITION;
    TextureCoord0Location = nux::VTXATTRIB_TEXCOORD0;
    VertexColorLocation   = nux::VTXATTRIB_COLOR;

    nux::GetThreadGraphicsContext()->SetTexture(GL_TEXTURE0, icon);
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
  
  nux::Color bkg_color;
  if (launcher_view->Running ())
    bkg_color = launcher_view->BackgroundColor ();
  else    
    bkg_color = nux::Color(0xFF6D6D6D);
    
  nux::Color white = nux::Color::White;
  
  if(!USE_ARB_SHADERS)
  {
    CHECKGL ( glUniform4fARB (FragmentColor, bkg_color.R(), bkg_color.G(), bkg_color.B(), bkg_color.A() ) );
    nux::GetThreadGraphicsContext()->SetTexture(GL_TEXTURE0, icon);
    CHECKGL( glDrawArrays(GL_QUADS, 0, 4) );
  }
  else
  {
    CHECKGL ( glProgramLocalParameter4fARB (GL_FRAGMENT_PROGRAM_ARB, 0, bkg_color.R(), bkg_color.G(), bkg_color.B(), bkg_color.A() ) );
    nux::GetThreadGraphicsContext()->SetTexture(GL_TEXTURE0, icon);
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

void Launcher::RenderIconImage(nux::GraphicsContext& GfxContext, LauncherIcon* launcher_view)
{
  nux::Geometry geo = GetGeometry();

  nux::Matrix4 ObjectMatrix;
  nux::Matrix4 ViewMatrix;
  nux::Matrix4 ProjectionMatrix;
  nux::Matrix4 ViewProjectionMatrix;
  nux::BaseTexture *icon;
 
  icon = launcher_view->TextureForSize (_icon_image_size);
  
  if(launcher_view->_folding_angle == 0.0f)
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

  v0.x = launcher_view->_xform_icon_screen_coord[0].x;
  v0.y = launcher_view->_xform_icon_screen_coord[0].y;
  v0.z = launcher_view->_xform_icon_screen_coord[0].z;
  v0.w = launcher_view->_xform_icon_screen_coord[0].w;
  v1.x = launcher_view->_xform_icon_screen_coord[1].x;
  v1.y = launcher_view->_xform_icon_screen_coord[1].y;
  v1.z = launcher_view->_xform_icon_screen_coord[1].z;
  v1.w = launcher_view->_xform_icon_screen_coord[1].w;
  v2.x = launcher_view->_xform_icon_screen_coord[2].x;
  v2.y = launcher_view->_xform_icon_screen_coord[2].y;
  v2.z = launcher_view->_xform_icon_screen_coord[2].z;
  v2.w = launcher_view->_xform_icon_screen_coord[2].w;
  v3.x = launcher_view->_xform_icon_screen_coord[3].x;
  v3.y = launcher_view->_xform_icon_screen_coord[3].y;
  v3.z = launcher_view->_xform_icon_screen_coord[3].z;
  v3.w = launcher_view->_xform_icon_screen_coord[3].w;
  int inside = launcher_view->_mouse_inside; //PointInside2DPolygon(launcher_view->_xform_screen_coord, 4, _mouse_position, 1);

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

  //         float VtxBuffer[] =
  //         {// Not perspective correct
  //             v0.x, v0.y, 0.0f, 1.0f,      0.0f, 0.0f, 0.0f, 1.0f,      1.0f, 1.0f, 1.0f, 1.0f,
  //             v1.x, v1.y, 0.0f, 1.0f,      0.0f, 1.0f, 0.0f, 1.0f,      1.0f, 1.0f, 1.0f, 1.0f,
  //             v2.x, v2.y, 0.0f, 1.0f,      1.0f, 1.0f, 0.0f, 1.0f,      1.0f, 1.0f, 1.0f, 1.0f,
  //             v3.x, v3.y, 0.0f, 1.0f,      1.0f, 0.0f, 0.0f, 1.0f,      1.0f, 1.0f, 1.0f, 1.0f,
  //         };

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
    
    nux::GetThreadGraphicsContext ()->SetTexture(GL_TEXTURE0, icon);

    if(TextureObjectLocation != -1)
      CHECKGL( glUniform1iARB (TextureObjectLocation, 0) );

    int VPMatrixLocation = _shader_program_uv_persp_correction->GetUniformLocationARB("ViewProjectionMatrix");
    if(VPMatrixLocation != -1)
    {
      nux::Matrix4 mat = nux::GetThreadGraphicsContext ()->GetModelViewProjectionMatrix ();
      _shader_program_uv_persp_correction->SetUniformLocMatrix4fv ((GLint)VPMatrixLocation, 1, false, (GLfloat*)&(mat.m));
    }
  }
  else
  {
    _AsmShaderProg->Begin();

    VertexLocation        = nux::VTXATTRIB_POSITION;
    TextureCoord0Location = nux::VTXATTRIB_TEXCOORD0;
    VertexColorLocation   = nux::VTXATTRIB_COLOR;

    nux::GetThreadGraphicsContext()->SetTexture(GL_TEXTURE0, icon);
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
  
  if(!USE_ARB_SHADERS)
  {
    CHECKGL ( glUniform4fARB (FragmentColor, white.R(), white.G(), white.B(), white.A() ) );
    nux::GetThreadGraphicsContext()->SetTexture(GL_TEXTURE0, icon);
    CHECKGL( glDrawArrays(GL_QUADS, 0, 4) );
  }
  else
  {
    CHECKGL ( glProgramLocalParameter4fARB (GL_FRAGMENT_PROGRAM_ARB, 0, white.R(), white.G(), white.B(), white.A() ) );
    nux::GetThreadGraphicsContext()->SetTexture(GL_TEXTURE0, icon);
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
  
  if (launcher_view->Active ())
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

void Launcher::DrawContent(nux::GraphicsContext& GfxContext, bool force_draw)
{
    nux::Geometry base = GetGeometry();
    GfxContext.PushClippingRectangle(base);

    nux::ROPConfig ROP;
    ROP.Blend = true;
    ROP.SrcBlend = GL_SRC_ALPHA;
    ROP.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

    gPainter.PushDrawColorLayer(GfxContext, base, nux::Color(0x99000000), true, ROP);
    gPainter.PushDrawColorLayer(GfxContext, nux::Geometry (base.x + base.width - 1, base.y, 1, base.height), nux::Color(0x60FFFFFF), true, ROP);
    
    std::list<Launcher::RenderArg> args = RenderArgs (0.0f);
    
    std::list<Launcher::RenderArg>::iterator arg_it;
    
    for (arg_it = args.begin (); arg_it != args.end (); arg_it++)
    {
        RenderArg arg = *arg_it;
        
        printf ("Center: %f, %f, %f\n", arg.center.x, arg.center.y, arg.center.z);
    }
    
    printf ("\n\n");
    
    LauncherModel::reverse_iterator rev_it;
    for (rev_it = _model->rbegin (); rev_it != _model->rend (); rev_it++)
    {
      if ((*rev_it)->_folding_angle >= 0.0f)
        continue;
    
      GfxContext.GetRenderStates ().SetSeparateBlend (true,
                                                    GL_SRC_ALPHA,
                                                    GL_ONE_MINUS_SRC_ALPHA,
                                                    GL_ONE_MINUS_DST_ALPHA,
                                                    GL_ONE);

      GfxContext.GetRenderStates ().SetColorMask (true, true, true, true);
      RenderIcon(GfxContext, *rev_it);

      GfxContext.GetRenderStates ().SetSeparateBlend (true,
                                                    GL_SRC_ALPHA,
                                                    GL_ONE_MINUS_SRC_ALPHA,
                                                    GL_ONE_MINUS_DST_ALPHA,
                                                    GL_ONE);
      GfxContext.GetRenderStates ().SetColorMask (true, true, true, true);
      RenderIconImage (GfxContext, *rev_it);
    }
    
    LauncherModel::iterator it;
    for (it = _model->begin(); it != _model->end(); it++)
    {
      if ((*it)->_folding_angle < 0.0f)
        continue;
      GfxContext.GetRenderStates ().SetSeparateBlend (true,
                                                    GL_SRC_ALPHA,
                                                    GL_ONE_MINUS_SRC_ALPHA,
                                                    GL_ONE_MINUS_DST_ALPHA,
                                                    GL_ONE);

      GfxContext.GetRenderStates ().SetColorMask (true, true, true, true);
      RenderIcon(GfxContext, *it);

      GfxContext.GetRenderStates ().SetSeparateBlend (true,
                                                    GL_SRC_ALPHA,
                                                    GL_ONE_MINUS_SRC_ALPHA,
                                                    GL_ONE_MINUS_DST_ALPHA,
                                                    GL_ONE);
      GfxContext.GetRenderStates ().SetColorMask (true, true, true, true);
      RenderIconImage (GfxContext, *it);
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

void Launcher::PostDraw(nux::GraphicsContext& GfxContext, bool force_draw)
{

}

void Launcher::OrderRevealedIcons()
{
  nux::Geometry geo = GetGeometry();
  int x = geo.x + (geo.width - _icon_size) / 2;
  int y = geo.y + _launcher_top_y;

  LauncherModel::iterator it;
  for(it = _model->begin(); it != _model->end(); it++)
  {
    (*it)->_folding_angle_delta = 0.0f - (*it)->_folding_angle;
    (*it)->_position_delta.x = x - (*it)->_position.x;
    (*it)->_position_delta.y = y - (*it)->_position.y;
    (*it)->_position_delta.z = 0 - (*it)->_position.z;

    y += _icon_size + _space_between_icons;
  }

  _launcher_bottom_y = y - _space_between_icons;
}

void Launcher::OrderFoldedIcons(int focus_icon_index)
{
  nux::Geometry geo = GetGeometry();
  int total_height = geo.GetHeight();

  int x = geo.x + (geo.width - _icon_size) / 2;
  int y = geo.y;

  int num_icons = _model->Size();

  if (num_icons == 0)
    return;

  focus_icon_index = 0;

  if (focus_icon_index > num_icons - 1)
    focus_icon_index = num_icons - 1;
  
  if (focus_icon_index < 0)
    focus_icon_index = 0;

  int icons_revealed;      // a block of consecutive, totally expanded icons. Should be odd number.

  if (total_height > num_icons * (_icon_size + _space_between_icons))
  {
    // all icons will be expanded
    icons_revealed = num_icons;
  }
  else
  {
    icons_revealed = total_height / (_icon_size + _space_between_icons);
    if (icons_revealed > num_icons)
      icons_revealed = num_icons;
    if (icons_revealed <= 0)
      icons_revealed = 1;
  }

  int half = (icons_revealed)/2;
  int icons_top_center = half;       // number of icons around the middle icon in the block of expanded icons.
  int icons_bottom_center = half;    // number of icons around the middle icon in the block of expanded icons.

  if ((icons_revealed % 2) == 0)
  {
    icons_top_center = half - 1;
    icons_bottom_center = half;
  }
  else
  {
    icons_top_center = half;
    icons_bottom_center = half;
  }

  if (focus_icon_index - icons_top_center < 0)
  {
    icons_bottom_center += icons_top_center - (focus_icon_index);
    icons_top_center = focus_icon_index;
  }

  if (focus_icon_index + icons_bottom_center > num_icons - 1)
  {
    icons_bottom_center = (num_icons - 1) - focus_icon_index;
    icons_top_center += focus_icon_index + half - (num_icons - 1);
  }

  int icons_above = nux::Max<int> (0, focus_icon_index - icons_top_center);
  int icons_below = nux::Max<int> (0, (num_icons - 1) - (focus_icon_index + icons_bottom_center));

  int box_height = icons_revealed * (_icon_size + _space_between_icons);
  int space_above = icons_above * float (total_height - box_height) / float (icons_above + icons_below);
  int space_below = icons_below * float (total_height - box_height) / float (icons_above + icons_below);

  int index = 0;

  LauncherModel::iterator it;
  for(it = _model->begin(), index = 0; it != _model->end(); it++, index++)
  {
    if (index < icons_above)
    {
      (*it)->_folding_angle_delta = _folded_angle - (*it)->_folding_angle;
      (*it)->_position_delta.x = x - (*it)->_position.x;
      (*it)->_position_delta.y = y - (*it)->_position.y;
      (*it)->_position_delta.z = _folded_z_distance - (*it)->_position.z;
      y += space_above / icons_above;
    }

    if ((index >= focus_icon_index - icons_top_center) && (index <= focus_icon_index + icons_bottom_center))
    {
      if(index == focus_icon_index)
        _launcher_top_y = y - (icons_top_center + icons_above) * (_icon_size + _space_between_icons);

      (*it)->_folding_angle_delta = 0.0f - (*it)->_folding_angle;
      (*it)->_position_delta.x = x - (*it)->_position.x;
      (*it)->_position_delta.y = y - (*it)->_position.y;
      (*it)->_position_delta.z = 0.0f - (*it)->_position.z;
      y += _icon_size + _space_between_icons;
    }


    if ((index > focus_icon_index + icons_bottom_center))
    {
      if(index == focus_icon_index + icons_bottom_center + 1)
        y -= 16;

      (*it)->_folding_angle_delta = _neg_folded_angle - (*it)->_folding_angle;
      (*it)->_position_delta.x = x - (*it)->_position.x;
      (*it)->_position_delta.y = y - (*it)->_position.y;
      (*it)->_position_delta.z = _folded_z_distance - (*it)->_position.z;
      y += space_below / icons_below;
    }
  }

  _launcher_bottom_y = y;
}

void Launcher::ScheduleRevealAnimation ()
{
  _launcher_state = LAUNCHER_UNFOLDED;
  OrderRevealedIcons();

  if(nux::GetThreadTimer().FindTimerHandle(_folding_timer_handle))
  {
    nux::GetThreadTimer().RemoveTimerHandler(_folding_timer_handle);
  }
  _folding_timer_handle = 0;

  if(!nux::GetThreadTimer().FindTimerHandle(_reveal_timer_handle))
  {
    _reveal_timer_handle = nux::GetThreadTimer().AddPeriodicTimerHandler(_timer_intervals, _anim_duration, _reveal_functor, this);
  }
}

void Launcher::ScheduleFoldAnimation ()
{
  _launcher_state = LAUNCHER_FOLDED;
  OrderFoldedIcons(0);

  if(nux::GetThreadTimer().FindTimerHandle(_reveal_timer_handle))
  {
    nux::GetThreadTimer().RemoveTimerHandler(_reveal_timer_handle);
  }
  _reveal_timer_handle = 0;

  if(!nux::GetThreadTimer().FindTimerHandle(_folding_timer_handle))
  {
    _folding_timer_handle = nux::GetThreadTimer().AddPeriodicTimerHandler(_timer_intervals, _anim_duration, _folding_functor, this);
  }
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
  ScheduleFoldAnimation ();
  
  return nux::eCompliantHeight | nux::eCompliantWidth;
}

void Launcher::PositionChildLayout(float offsetX, float offsetY)
{
}

void Launcher::SlideDown(float stepy, int mousedy)
{
    NeedRedraw();
}

void Launcher::SlideUp(float stepy, int mousedy)
{
    NeedRedraw();
}

bool Launcher::TooltipNotify(LauncherIcon* Icon)
{
    if(GetActiveMenuIcon())
        return false;
    return true;
}

bool Launcher::MenuNotify(LauncherIcon* Icon)
{
//     if(GetActiveMenuIcon())
//         return false;
// 
//     m_ActiveMenuIcon = Icon;
    return true;
}

void Launcher::NotifyMenuTermination(LauncherIcon* Icon)
{
//     if(Icon == GetActiveMenuIcon())
//         m_ActiveMenuIcon = NULL;
}

void Launcher::RecvMouseDown(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  _mouse_position = nux::Point2 (x, y);
  _dnd_delta = 0;
  
  UpdateIconXForm ();
  MouseDownLogic ();
  NeedRedraw ();
}

void Launcher::RecvMouseUp(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  _mouse_position = nux::Point2 (x, y);
  nux::Geometry geo = GetGeometry ();

  if (!geo.IsInside(nux::Point(x, y)))
    ScheduleFoldAnimation ();
  else if (_launcher_action_state == ACTION_DRAG_LAUNCHER)
  {
    if (_launcher_top_y > 0)
    {
      _launcher_top_y = 0;
    }
    else if ((_launcher_bottom_y < geo.height))
    {
      _launcher_top_y += geo.height - _launcher_bottom_y;
      if (_launcher_top_y > 0)
        _launcher_top_y = 0;
    }
    ScheduleRevealAnimation ();
  }

  UpdateIconXForm ();
  MouseUpLogic ();
  _launcher_action_state = ACTION_NONE;
  NeedRedraw ();
}

void Launcher::RecvMouseDrag(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags)
{
  _mouse_position = nux::Point2 (x, y);
  nux::Geometry geo = GetGeometry ();
  float dampening_factor;
  
  if (nux::Abs (_dnd_delta) < 15)
  {
    _dnd_delta += dy;
    if (nux::Abs (_dnd_delta) < 15)
      return;
  }

  int DY = 0;
  if ((nux::Abs (_dnd_delta) >= _dnd_security) && (_launcher_action_state != ACTION_DRAG_LAUNCHER))
  {
    _launcher_action_state = ACTION_DRAG_LAUNCHER;
    DY = _dnd_delta;
    LauncherModel::iterator it;
    for(it = _model->begin(); it != _model->end(); it++)
    {
      (*it)->HideTooltip ();
    }
  }
  else
  {
    DY = dy;
  }

  {
    if (DY > 0)
    {
      dampening_factor = 1.0f;

      _launcher_top_y += dampening_factor * DY;
      _launcher_bottom_y += dampening_factor * DY;

      LauncherModel::iterator it;
      for(it = _model->begin(); it != _model->end(); it++)
      {
        (*it)->_position.y += DY * dampening_factor;
      }
    }
    else
    {
      dampening_factor = 1.0f;

      _launcher_top_y += dampening_factor * DY;
      _launcher_bottom_y += dampening_factor * DY;


      LauncherModel::iterator it;
      for(it = _model->begin(); it != _model->end(); it++)
      {
        (*it)->_position.y += DY * dampening_factor;
      }
    }
  }
  
  UpdateIconXForm ();
  EventLogic ();
  NeedRedraw ();
}

void Launcher::RecvMouseEnter(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  _mouse_position = nux::Point2 (x, y);
  if  (_launcher_action_state == ACTION_NONE)
    ScheduleRevealAnimation ();

  UpdateIconXForm ();
  EventLogic ();
  NeedRedraw ();
}

void Launcher::RecvMouseLeave(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  _mouse_position = nux::Point2 (x, y);
  if  (_launcher_action_state == ACTION_NONE)
    ScheduleFoldAnimation ();

  UpdateIconXForm ();
  EventLogic ();
  NeedRedraw ();
}

void Launcher::RecvMouseMove(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags)
{
  _mouse_position = nux::Point2 (x, y);

  // Every time the mouse moves, we check if it is inside an icon...

  UpdateIconXForm ();
  EventLogic ();
  NeedRedraw ();
}

void Launcher::RecvMouseWheel(int x, int y, int wheel_delta, unsigned long button_flags, unsigned long key_flags)
{
  nux::Geometry geo = GetGeometry ();
  float dampening_factor;
  if (!geo.IsInside(nux::Point(x, y)))
    return;

  if (wheel_delta > 0)
  {
    if(_launcher_top_y >= 200.0f)
      return;

    dampening_factor = nux::Min<float> (1.0f, float (200.0f - _launcher_top_y) / 200.0f);

    _launcher_top_y += dampening_factor * wheel_delta;
    _launcher_bottom_y += dampening_factor * wheel_delta;

    if(_launcher_top_y >= 200.0f)
      _launcher_top_y = 200;

    LauncherModel::iterator it;
    for(it = _model->begin(); it != _model->end(); it++)
    {
      (*it)->_position.y += wheel_delta * dampening_factor;
    }
  }
  else
  {
    if(_launcher_bottom_y <= geo.height - 200.0f)
      return;

    dampening_factor = nux::Min<float> (1.0f, float (_launcher_bottom_y - (geo.height - 200.0f)) / 200.0f);

    _launcher_top_y += dampening_factor * wheel_delta;
    _launcher_bottom_y += dampening_factor * wheel_delta;

    if(_launcher_bottom_y <= geo.height - 200.0f)
      _launcher_bottom_y = geo.height - 200.0f;

    LauncherModel::iterator it;
    for(it = _model->begin(); it != _model->end(); it++)
    {
      (*it)->_position.y += wheel_delta * dampening_factor;
    }
  }

  UpdateIconXForm ();
  EventLogic ();
  NeedRedraw ();
}

void Launcher::FoldingCallback(void* v)
{
  LauncherModel::iterator it;
  for(it = _model->begin(); it != _model->end(); it++)
  {
    (*it)->_folding_angle += _folding_timer_handle.GetProgressDelta() * (*it)->_folding_angle_delta;

    (*it)->_position.x += _folding_timer_handle.GetProgressDelta() * (*it)->_position_delta.x;
    (*it)->_position.y += _folding_timer_handle.GetProgressDelta() * (*it)->_position_delta.y;
    (*it)->_position.z += _folding_timer_handle.GetProgressDelta() * (*it)->_position_delta.z;
  }

  UpdateIconXForm ();
  EventLogic ();
  NeedRedraw ();
}

void Launcher::RevealCallback(void* v)
{
  LauncherModel::iterator it;
  for(it = _model->begin(); it != _model->end(); it++)
  {
    (*it)->_folding_angle += _reveal_timer_handle.GetProgressDelta() * (*it)->_folding_angle_delta;

    (*it)->_position.x += _reveal_timer_handle.GetProgressDelta() * (*it)->_position_delta.x;
    (*it)->_position.y += _reveal_timer_handle.GetProgressDelta() * (*it)->_position_delta.y;
    (*it)->_position.z += _reveal_timer_handle.GetProgressDelta() * (*it)->_position_delta.z;
  }

  UpdateIconXForm ();
  EventLogic ();
  NeedRedraw ();
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
    if ((*rev_it)->_folding_angle < 0.0f)
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
    if ((*it)->_folding_angle >= 0.0f)
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

void Launcher::UpdateIconXForm ()
{

  nux::Geometry geo = GetGeometry ();
  nux::Matrix4 ObjectMatrix;
  nux::Matrix4 ViewMatrix;
  nux::Matrix4 ProjectionMatrix;
  nux::Matrix4 ViewProjectionMatrix;

  GetInverseScreenPerspectiveMatrix(ViewMatrix, ProjectionMatrix, geo.width, geo.height, 0.1f, 1000.0f, DEGTORAD(90));

  LauncherModel::iterator it;
  for(it = _model->begin(); it != _model->end(); it++)
  {
    LauncherIcon* launcher_icon = *it;

    float w = _icon_size;
    float h = _icon_size;
    float x = launcher_icon->_position.x;
    float y = launcher_icon->_position.y;
    float z = launcher_icon->_position.z;

    ObjectMatrix = nux::Matrix4::TRANSLATE(geo.width/2.0f, geo.height/2.0f, z) * // Translate the icon to the center of the viewport
      nux::Matrix4::ROTATEX(launcher_icon->_folding_angle) *      // rotate the icon
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
    x = launcher_icon->_position.x + _icon_image_size_delta/2;
    y = launcher_icon->_position.y + _icon_image_size_delta/2;
    z = launcher_icon->_position.z;
    
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
