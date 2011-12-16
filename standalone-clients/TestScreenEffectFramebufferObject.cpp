// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/* Compiz unity plugin
 * unity.h
 *
 * Copyright (c) 2010-11 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Authored By: Sam Spilsbury <sam.spilsbury@canonical.com>
 */

#include "ScreenEffectFramebufferObject.h"
#include "BackgroundEffectHelper.h"
#include "GLFuncLoader.h"
#include <Nux/Nux.h>
#include <Nux/VLayout.h>
#include <Nux/View.h>
#include <Nux/BaseWindow.h>
#include <Nux/WindowCompositor.h>
#include <NuxCore/Logger.h>
#include <NuxCore/Object.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/glx.h>
#include <GL/gl.h>
#include <dlfcn.h>
#include <sys/poll.h>
#include <unistd.h>
#include <glib.h>
#include <gtk/gtk.h>

using namespace unity::GLLoader;

namespace
{
  nux::logging::Logger logger ("unity.test-screeneffectframebufferobject");
  static bool nuxReady;
  nux::VLayout *root_layout = NULL;
  nux::View    *root_view = NULL;
  nux::ColorLayer background (nux::color::Transparent);
  GLXGetProcAddressProc glXGetProcAddressP;
}

static int attributes[] = { GLX_RENDER_TYPE, GLX_RGBA_BIT,
                            GLX_X_RENDERABLE, True,
                            GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
                            GLX_DOUBLEBUFFER, True,
                            GLX_RED_SIZE, 8,
                            GLX_GREEN_SIZE, 8,
                            GLX_BLUE_SIZE, 8, 0};

class EffectView :
  public nux::View
{
public:
  EffectView (NUX_FILE_LINE_PROTO);
  virtual ~EffectView ();

  void Draw (nux::GraphicsEngine &context, bool force) { return; };
  void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);
private:
  BackgroundEffectHelper bg_effect_helper_;
};

EffectView::EffectView (NUX_FILE_LINE_DECL)
  : View (NUX_FILE_LINE_PARAM)
{
  bg_effect_helper_.owner = this;
}

EffectView::~EffectView ()
{
}

void EffectView::DrawContent (nux::GraphicsEngine &GfxContext, bool force_draw)
{
  nux::Geometry base = GetGeometry();
  GfxContext.PushClippingRectangle(base);
  nux::Geometry blur_geo (base.x, base.y, base.width, base.height);

  if (BackgroundEffectHelper::blur_type == unity::BLUR_ACTIVE)
  {
    bg_effect_helper_.enabled = true;

    auto blur_texture = bg_effect_helper_.GetBlurRegion(blur_geo);

    if (blur_texture.IsValid ())
    {
      nux::TexCoordXForm texxform_blur_bg;
      texxform_blur_bg.flip_v_coord = true;
      texxform_blur_bg.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
      texxform_blur_bg.uoffset = ((float) base.x) / (base.width);
      texxform_blur_bg.voffset = ((float) base.y) / (base.height);

      nux::ROPConfig rop;
      rop.Blend = false;
      rop.SrcBlend = GL_ONE;
      rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

      gPainter.PushDrawTextureLayer(GfxContext, base,
                                    blur_texture,
                                    texxform_blur_bg,
                                    nux::color::White,
                                    true,
                                    rop);
    }
  }
  else
    bg_effect_helper_.enabled = false;
  GfxContext.PopClippingRectangle();
}

struct paintLoopData
{
    Display *display;
    Window win;
    nux::WindowThread *wt;
    boost::shared_ptr <unity::ScreenEffectFramebufferObject> fbo;
    GLXWindow glxWindow;
    GLdouble rotTri;
    GLdouble rotQuad;
    unsigned int rotate;
    unsigned int side;
};

static Bool
paintLoop (void *data)
{
  paintLoopData *pld = static_cast <paintLoopData *> (data);

  bool done = false;

  XEvent ev;
  if (XCheckWindowEvent (pld->display, pld->win, KeyPressMask, &ev))
  {
    if (XLookupKeysym (&ev.xkey, 0) == XK_Escape)
      done = true;
    else if (XLookupKeysym (&ev.xkey, 0) == XK_F1)
    {
      if (!pld->fbo)
      {
        BackgroundEffectHelper::blur_type = unity::BLUR_ACTIVE;
        pld->fbo.reset (new unity::ScreenEffectFramebufferObject (glXGetProcAddressP, nux::Geometry (0, 0, 640, 480)));
      }
      else
      {
        BackgroundEffectHelper::blur_type = unity::BLUR_NONE;
        pld->fbo.reset ();
      }
    }
    else if (XLookupKeysym (&ev.xkey, 0) == XK_F2)
    {
      pld->rotate = (pld->rotate + 1) % 3;
    }
    else if (XLookupKeysym (&ev.xkey, 0) == XK_F3)
    {
      pld->side = (pld->side + 1) % 3;

      if (pld->side == 2)
        root_view->SetGeometry (nux::Geometry (0, 0, 640, 480));
      else if (pld->side == 1)
        root_view->SetGeometry (nux::Geometry (0, 0, 320, 480));
      else if (pld->side == 0)
        root_view->SetGeometry (nux::Geometry (320, 0, 320, 480));
    }
  }

  if (pld->fbo)
  {
    if (pld->rotate == 2)
      BackgroundEffectHelper::ProcessDamage (nux::Geometry (0, 0, 680, 480));
    else if (pld->rotate == 1)
      BackgroundEffectHelper::ProcessDamage (nux::Geometry (0, 0, 320, 480));
    else if (pld->rotate == 0)
      BackgroundEffectHelper::ProcessDamage (nux::Geometry (320, 0, 320, 480));
    pld->fbo->bind (nux::Geometry (0, 0, 640, 480));
  }

  if (!pld->fbo || pld->fbo->status ())
  {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPushMatrix ();
    glLoadIdentity();
    glTranslatef(-0.5f, -0.5f, -0.866025404f);
    glScalef (1.0f / 640, 1.0f / 480, 0.0f);
    glTranslatef(160, 0, 0);
    glRotatef(pld->rotTri, 0.0f, 1.0f, 0.0f);
    glTranslatef(-160, 0, 0);
    glBegin(GL_TRIANGLES);
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex3f(160.0f, 480.0f, 0.0f);
        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glColor3f(0.0f, 0.0f, 1.0f);
        glVertex3f(320.0f, 0.0f, 0.0f);
    glEnd();
    glLoadIdentity();
    glTranslatef(-0.5f, -0.5f, -0.866025404f);
    glScalef (1.0f / 640, 1.0f / 480, 0.0f);
    glTranslatef(480, 240, 0);
    glRotatef(pld->rotQuad, 1.0f, 0.0f, 0.0f);
    glTranslatef(-480, -240, 0);
    glColor3f(0.5f, 0.5f, 1.0f);
    glBegin(GL_QUADS);
        glVertex3f(320.f, 480.0f, 0.0f);
        glVertex3f(640.f, 480.0f, 0.0f);
        glVertex3f(640.f, 0.0f, 0.0f);
        glVertex3f(320.f, 0.0f, 0.0f);
    glEnd();
    glColor4f (1.0f, 1.0f, 1.0f, 5.0f);
    glPopMatrix ();
  }
  else
    LOG_INFO (logger) << "FBO not ok!";

  if (pld->fbo)
    pld->fbo->unbind ();

  if (pld->fbo && pld->fbo->status ())
  {
    glClearColor (1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPushMatrix ();
    glLoadIdentity();
    glTranslatef(-0.5f, 0.5f, -0.866025404f);
    glScalef (1.0f / 640, -(1.0f / 480), 0.0f);
    pld->fbo->paint (nux::Geometry (0, 0, 640, 480));
    glPopMatrix ();
  }

  if (pld->fbo && pld->fbo->status ())
  {
    nux::ObjectPtr<nux::IOpenGLTexture2D> device_texture =
        nux::GetGraphicsDisplay()->GetGpuDevice()->CreateTexture2DFromID(pld->fbo->texture(),
                                                                         640, 480, 1, nux::BITFMT_R8G8B8A8);

    nux::GetGraphicsDisplay()->GetGpuDevice()->backup_texture0_ = device_texture;

    nux::Geometry geo = nux::Geometry (0, 0, 640, 480);
    BackgroundEffectHelper::monitor_rect_ = geo;
  }
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glPushAttrib(GL_VIEWPORT_BIT | GL_ENABLE_BIT |
               GL_TEXTURE_BIT | GL_COLOR_BUFFER_BIT | GL_SCISSOR_BIT);

  root_view->ProcessDraw (pld->wt->GetGraphicsEngine(), true);
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  glDrawBuffer(GL_BACK);
  glReadBuffer(GL_BACK);

  glPopAttrib();

  glXSwapBuffers (pld->display, pld->glxWindow);

  if (pld->rotate == 2 || pld->rotate == 0)
    pld->rotTri += 5.0f;
  if (pld->rotate == 2 || pld->rotate == 1)
  pld->rotQuad -= 1.5f;

  return !done;
}

void initThread (nux::NThread *thread, void *d)
{
  static_cast<nux::WindowThread*> (thread)->SetWindowBackgroundPaintLayer(&background);
  static_cast<nux::WindowThread*> (thread)->SetLayout (root_layout);
  root_layout = new nux::VLayout ();
  root_view = new EffectView ();
  root_view->SetGeometry (nux::Geometry (0, 0, 640, 480));
  nuxReady = true;
  BackgroundEffectHelper::blur_type = unity::BLUR_ACTIVE;
}

int main (int argc, char **argv)
{
  XVisualInfo *visinfo;
  int numFBConfig = 0;  

  nuxReady = false;

  glXGetProcAddressP = (GLXGetProcAddressProc) getProcAddr ("glXGetProcAddress");
  PFNGLXCHOOSEFBCONFIGPROC glXChooseFBConfigP = (PFNGLXCHOOSEFBCONFIGPROC) (*glXGetProcAddressP) ((const GLubyte *) "glXChooseFBConfig");
  PFNGLXGETVISUALFROMFBCONFIGPROC glXGetVisualFromFBConfigP = (PFNGLXGETVISUALFROMFBCONFIGPROC) (*glXGetProcAddressP) ((const GLubyte *) "glXGetVisualFromFBConfig");
  PFNGLXCREATEWINDOWPROC glXCreateWindowP = (PFNGLXCREATEWINDOWPROC) (*glXGetProcAddressP) ((const GLubyte *) "glXCreateWindow");
  PFNGLXMAKECONTEXTCURRENTPROC glXMakeContextCurrentP = (PFNGLXMAKECONTEXTCURRENTPROC) (*glXGetProcAddressP) ((const GLubyte *) "glXMakeContextCurrent");
  //PFNGLXSWAPBUFFERSPROC glXSwapBuffersP = (PFNGLXSWAPBUFFERSPROC) (*glXGetProcAddressP) ((const GLubyte *) "glXSwapBuffers");
  Display *display = XOpenDisplay (NULL);
  nux::NuxInitialize (0);
  GLXFBConfig *fbConfigs = (*glXChooseFBConfigP) (display, DefaultScreen (display), attributes, &numFBConfig);
  visinfo = (*glXGetVisualFromFBConfigP) (display, fbConfigs[0]);

  GLXContext ctx = glXCreateContext (display, visinfo, 0, GL_TRUE);
  Colormap   cmap = XCreateColormap (display, DefaultRootWindow (display), visinfo->visual, AllocNone);

  XSetWindowAttributes wa;

  wa.colormap = cmap;
  wa.border_pixel = 0;
  wa.event_mask = StructureNotifyMask | KeyPressMask;
  Window win = XCreateWindow (display, DefaultRootWindow (display), 0, 0, 640, 480, 0, visinfo->depth, InputOutput, visinfo->visual,
                              CWColormap | CWEventMask | CWBorderPixel, &wa);

  GLXWindow glxWindow = (*glXCreateWindowP) (display, fbConfigs[0], win, NULL);

	g_type_init();
  g_thread_init(NULL);
  gtk_init(&argc, &argv);


  XStoreName (display, win, "F1: Toggle Effect, F2: Rotation, F3: Effect");
  XMapWindow (display, win);

  while (1)
  {
    XEvent ev;
    bool done = false;

    XNextEvent (display, &ev);
    switch (ev.type)
    {
      case MapNotify:
      case ConfigureNotify:
        done = true;
        break;
      default:
        break;
    }

    if (done)
      break;
  }

  (*glXMakeContextCurrentP) (display, glxWindow, glxWindow, ctx);

  glViewport(0, 0, 640, 480);
  glDrawBuffer (GL_BACK);
  glReadBuffer (GL_BACK);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60.0f, 1.0f, 0.1f, 100.0f);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity ();
  glClearColor (1, 1, 1, 1);
  glClear (GL_COLOR_BUFFER_BIT);
  glXSwapBuffers (display, glxWindow);

  GLenum error = glGetError ();

  nux::WindowThread *wt = nux::CreateFromForeignWindow (win,
			                                                  ctx,
			                                                  &initThread,
			                                                  NULL);

  wt->Run(NULL);

  bool done = false;
  while (!nuxReady);

  paintLoopData *pld = new paintLoopData;

  pld->wt = wt;
  pld->display = display;
  pld->win = win;
  pld->glxWindow = glxWindow;
  pld->rotQuad = 0;
  pld->rotTri = 0;
  pld->rotate = 2;
  pld->side = 2;

  g_timeout_add (16, &paintLoop, pld);
  gtk_main ();

  (*glXMakeContextCurrentP) (display, None, None, ctx);
  glXDestroyContext (display, ctx);

  return 0;
}


