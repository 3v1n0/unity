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
#include <cmath>

using namespace unity::GLLoader;

namespace
{
  nux::logging::Logger logger ("unity.test-screeneffectframebufferobject");

  const static int attributes[] = { GLX_RENDER_TYPE, GLX_RGBA_BIT,
                                    GLX_X_RENDERABLE, True,
                                    GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
                                    GLX_DOUBLEBUFFER, True,
                                    GLX_RED_SIZE, 8,
                                    GLX_GREEN_SIZE, 8,
                                    GLX_BLUE_SIZE, 8, 0};
}

namespace GLFuncs
{
  GLXGetProcAddressProc           glXGetProcAddressP;
  PFNGLXCHOOSEFBCONFIGPROC        glXChooseFBConfigP;
  PFNGLXGETVISUALFROMFBCONFIGPROC glXGetVisualFromFBConfigP;
  PFNGLXCREATEWINDOWPROC          glXCreateWindowP;
  PFNGLXDESTROYWINDOWPROC         glXDestroyWindowP;
  PFNGLXMAKECONTEXTCURRENTPROC    glXMakeContextCurrentP;

  void init ()
  {
    glXGetProcAddressP = (GLXGetProcAddressProc) getProcAddr ("glXGetProcAddress");
    glXChooseFBConfigP = (PFNGLXCHOOSEFBCONFIGPROC) (*glXGetProcAddressP) ((const GLubyte *) "glXChooseFBConfig");
    glXGetVisualFromFBConfigP = (PFNGLXGETVISUALFROMFBCONFIGPROC) (*glXGetProcAddressP) ((const GLubyte *) "glXGetVisualFromFBConfig");
    glXCreateWindowP = (PFNGLXCREATEWINDOWPROC) (*glXGetProcAddressP) ((const GLubyte *) "glXCreateWindow");
    glXMakeContextCurrentP = (PFNGLXMAKECONTEXTCURRENTPROC) (*glXGetProcAddressP) ((const GLubyte *) "glXMakeContextCurrent");
    glXDestroyWindowP      = (PFNGLXDESTROYWINDOWPROC) (*glXGetProcAddressP) ((const GLubyte *) "glXDestroyWindow");
  }
}

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

class Shape
{
  public:

    typedef boost::shared_ptr <Shape> Ptr;

    Shape ();
    virtual ~Shape ();

    float rotation () { return mRotation; }

    void draw (unsigned int width, unsigned int height) { glDraw (width, height); }
    void rotate () { applyRotation (); }

  protected:

    float mRotation;

    virtual void glDraw (unsigned int width, unsigned int height) = 0;
    virtual void applyRotation () = 0;
    virtual void getRotationAxes (float &x, float &y, float &z) = 0;
};

Shape::Shape () :
 mRotation (0.0f)
{
}

Shape::~Shape ()
{
}

class Triangle :
  public Shape
{
  public:

    typedef boost::shared_ptr <Triangle> Ptr;

    Triangle ();
    virtual ~Triangle ();

  protected:

    void glDraw (unsigned int width, unsigned int height);
    void applyRotation () { mRotation += 5.0f; }
    void getRotationAxes (float &x, float &y, float &z) { x = 0.0f; y = 1.0f; z = 0.0f; }
};

Triangle::Triangle () :
  Shape ()
{
}

Triangle::~Triangle ()
{
}

void
Triangle::glDraw (unsigned int width, unsigned int height)
{
  glBegin(GL_TRIANGLES);
      glColor3f(1.0f, 0.0f, 0.0f);
      glVertex3f(width / 4, height, 0.0f);
      glColor3f(0.0f, 1.0f, 0.0f);
      glVertex3f(0.0f, 0.0f, 0.0f);
      glColor3f(0.0f, 0.0f, 1.0f);
      glVertex3f(width / 2, 0.0f, 0.0f);
  glEnd();
}

class Square :
  public Shape
{
  public:

    typedef boost::shared_ptr <Square> Ptr;

    Square ();
    virtual ~Square ();

  protected:

    void glDraw (unsigned int width, unsigned int height);
    void applyRotation () { mRotation -= 2.5f; }
    void getRotationAxes (float &x, float &y, float &z) { x = 1.0f; y = 0.0f; z = 0.0f; }
    
};

Square::Square () :
  Shape ()
{
}

Square::~Square ()
{
}

void
Square::glDraw (unsigned int width, unsigned int height)
{
  glBegin(GL_QUADS);
    glColor3f(sin (rotation () / 100.0f), -sin (rotation () / 100.0f), cos (rotation () / 100.0f));
    glVertex3f(width / 2, height, 0.0f);
    glColor3f(-sin (rotation () / 100.0f), sin (rotation () / 100.0f), cos (rotation () / 100.0f));
    glVertex3f(width, height, 0.0f);
    glColor3f(sin (rotation () / 100.0f), sin (rotation () / 100.0f), sin (rotation () / 100.0f));
    glVertex3f(width, 0.0f, 0.0f);
    glColor3f(-sin (rotation () / 100.0f), cos (rotation () / 100.0f), cos (rotation () / 100.0f));
    glVertex3f(width / 2, 0.0f, 0.0f);
  glEnd();
}

class BaseContext
{
  public:

    BaseContext (Display *);
    ~BaseContext ();

    void run ();

  protected:

    bool eventHandler ();
    bool paintDispatch ();

    enum class ModifierApplication
    {
      Square,
      Triangle,
      Both
    };

    void nextWindowPosition ();
    void nextShapeRotation ();
    void setupContextForSize (unsigned int width,
                              unsigned int height);
    void drawShape (const Shape::Ptr &) {};

  private:

    static gboolean onNewEvents (GIOChannel   *channel,
                                 GIOCondition condition,
                                 gpointer     data);

    static gboolean onPaintTimeout (gpointer data);

    static void onWindowThreadCreation (nux::NThread *thread, void *d);

    Display                                   *mDisplay;
    Window                                    mWindow;
    Colormap                                  mColormap;
    nux::WindowThread                         *mWindowThread;
    nux::View                                 *mRootView;
    unity::ScreenEffectFramebufferObject::Ptr mFbo;
    GLXWindow                                 mGlxWindow;
    GLXContext                                mContext;
    ModifierApplication                       mRotating;
    ModifierApplication                       mBlur;
    unsigned int                              mWidth;
    unsigned int                              mHeight;
    bool                                      mNuxReady;
    Shape::Ptr                                mTriangle;
    Shape::Ptr                                mSquare;
};

BaseContext::BaseContext (Display *display) :
  mDisplay (display),
  mWindowThread (NULL),
  mRotating (ModifierApplication::Both),
  mBlur (ModifierApplication::Both),
  mWidth (640),
  mHeight (480),
  mNuxReady (false),
  mTriangle  (new Triangle ()),
  mSquare (new Square ())
{
  int            numFBConfig = 0;  
  GLXFBConfig    *fbConfigs = (*GLFuncs::glXChooseFBConfigP) (mDisplay,
                                                              DefaultScreen (mDisplay),
                                                              attributes,
                                                              &numFBConfig);
  XVisualInfo    *visinfo = (*GLFuncs::glXGetVisualFromFBConfigP) (mDisplay,
                                                                   fbConfigs[0]);

  mContext = glXCreateContext (mDisplay, visinfo, 0, GL_TRUE);
  mColormap = XCreateColormap (mDisplay,
                               DefaultRootWindow (mDisplay),
                               visinfo->visual,
                               AllocNone);

  XSetWindowAttributes wa;

  wa.colormap = mColormap;
  wa.border_pixel = 0;
  wa.event_mask = StructureNotifyMask | KeyPressMask | ExposureMask;

  mWindow = XCreateWindow (mDisplay, DefaultRootWindow (mDisplay),
                           0, 0, mWidth, mHeight, 0, visinfo->depth, InputOutput,
                           visinfo->visual, CWColormap | CWEventMask | CWBorderPixel,
                           &wa);

  mGlxWindow = (*GLFuncs::glXCreateWindowP) (mDisplay, fbConfigs[0], mWindow, NULL);

  XStoreName (mDisplay, mWindow, "F1: Toggle Effect, F2: Rotation, F3: Effect");
  XMapWindow (mDisplay, mWindow);

  bool ready = false;

  do
  {
    XEvent ev;
    XNextEvent (mDisplay, &ev);
    switch (ev.type)
    {
      case MapNotify:
      case ConfigureNotify:
      case Expose:
        ready = true;
        break;
      default:
        break;
    }

  } while (!ready);

  (*GLFuncs::glXMakeContextCurrentP) (mDisplay, mGlxWindow, mGlxWindow, mContext);

  setupContextForSize (mWidth, mHeight);
}

void
BaseContext::run ()
{
  GIOChannel        *channel;
  mWindowThread = nux::CreateFromForeignWindow (mWindow,
                                                mContext,
                                                &BaseContext::onWindowThreadCreation,
                                                (void *) this);

  mWindowThread->Run(NULL);

  while (!mNuxReady);
  g_timeout_add (128, &BaseContext::onPaintTimeout, (gpointer) this);

  channel = g_io_channel_unix_new (ConnectionNumber (mDisplay));

  g_io_add_watch (channel, (GIOCondition) (G_IO_IN | G_IO_HUP | G_IO_ERR),
                  &BaseContext::onNewEvents, (gpointer) this);
  gtk_main ();
}

BaseContext::~BaseContext ()
{
  delete mWindowThread;

  (*GLFuncs::glXMakeContextCurrentP) (mDisplay, None, None, mContext);
  glXDestroyContext (mDisplay, mContext);
  (*GLFuncs::glXDestroyWindowP) (mDisplay, mGlxWindow);

  XFreeColormap (mDisplay, mColormap);
  XDestroyWindow (mDisplay, mWindow);
}

void
BaseContext::setupContextForSize (unsigned int width,
                                  unsigned int height)
{
  mWidth = width;
  mHeight = height;

  glViewport(0, 0, width, height);
  glDrawBuffer (GL_BACK);
  glReadBuffer (GL_BACK);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60.0f, 1.0f, 0.1f, 100.0f);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity ();
  glClearColor (1, 1, 1, 1);
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glXSwapBuffers (mDisplay, mGlxWindow);

  if (mFbo)
    mFbo.reset (new unity::ScreenEffectFramebufferObject (GLFuncs::glXGetProcAddressP, nux::Geometry (0, 0, mWidth, mHeight)));

  if (mRootView && mNuxReady)
  {
    switch (mBlur)
    {
      case ModifierApplication::Both:
        mRootView->SetGeometry (nux::Geometry (0, 0, mWidth / 2, mHeight));
        break;
      case ModifierApplication::Triangle:
        mRootView->SetGeometry (nux::Geometry (mWidth / 2, 0, mWidth / 2, mHeight));
        break;
      case ModifierApplication::Square:
        mRootView->SetGeometry (nux::Geometry (0, 0, mWidth, mHeight));
        break;
      default:
        break;
    }
  }
}

bool
BaseContext::eventHandler ()
{
  XEvent event;
  XEvent *ev = &event;

  XNextEvent (mDisplay, &event);

  switch (ev->type)
  {
    case KeyPress:
      if (XLookupKeysym (&ev->xkey, 0) == XK_Escape)
        return false;
      else if (XLookupKeysym (&ev->xkey, 0) == XK_F1)
      {
        if (!mFbo)
        {
          BackgroundEffectHelper::blur_type = unity::BLUR_ACTIVE;
          mFbo.reset (new unity::ScreenEffectFramebufferObject (GLFuncs::glXGetProcAddressP, nux::Geometry (0, 0, mWidth, mHeight)));
        }
        else
        {
          BackgroundEffectHelper::blur_type = unity::BLUR_NONE;
          mFbo.reset ();
        }
      }
      else if (XLookupKeysym (&ev->xkey, 0) == XK_F2)
        nextShapeRotation ();
      else if (XLookupKeysym (&ev->xkey, 0) == XK_F3)
        nextWindowPosition ();
      break;
    case ConfigureNotify:
      setupContextForSize (ev->xconfigure.width, ev->xconfigure.height);
    default:
      break;
  }

  return true;
}

gboolean
BaseContext::onNewEvents (GIOChannel   *channel,
                          GIOCondition condition,
                          gpointer     data)
{
  BaseContext *self = static_cast <BaseContext *> (data);
  gboolean    keep_going = TRUE;

  if (condition & G_IO_IN)
  {
    if (self->eventHandler ())
      return TRUE;
    else
      return FALSE;
  }

  return TRUE;
}

gboolean
BaseContext::onPaintTimeout (gpointer data)
{
  BaseContext *self = static_cast <BaseContext *> (data);

  if (self->paintDispatch ())
    return TRUE;

  return FALSE;
}

void
BaseContext::nextShapeRotation ()
{
  switch (mRotating)
  {
    case ModifierApplication::Both:
      mRotating = ModifierApplication::Triangle;
      break;
    case ModifierApplication::Triangle:
      mRotating = ModifierApplication::Square;
      break;
    case ModifierApplication::Square:
      mRotating = ModifierApplication::Both;
      break;
    default:
      break;
  }
}

void
BaseContext::nextWindowPosition ()
{
  switch (mBlur)
  {
    case ModifierApplication::Both:
      mBlur = ModifierApplication::Triangle;
      mRootView->SetGeometry (nux::Geometry (0, 0, mWidth / 2, mHeight));
      break;
    case ModifierApplication::Triangle:
      mBlur = ModifierApplication::Square;
      mRootView->SetGeometry (nux::Geometry (mWidth / 2, 0, mWidth / 2, mHeight));
      break;
    case ModifierApplication::Square:
      mBlur = ModifierApplication::Both;
      mRootView->SetGeometry (nux::Geometry (0, 0, mWidth, mHeight));
      break;
    default:
      break;
  }
}

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

bool
BaseContext::paintDispatch ()
{
  if (mFbo)
  {
    switch (mRotating)
    {
      case ModifierApplication::Both:
        BackgroundEffectHelper::ProcessDamage (nux::Geometry (0, 0, mWidth, mHeight));
        break;
      case ModifierApplication::Triangle:
        BackgroundEffectHelper::ProcessDamage (nux::Geometry (0, 0, mWidth / 2, mHeight));
        break;
      case ModifierApplication::Square:
        BackgroundEffectHelper::ProcessDamage (nux::Geometry (mWidth / 2, 0, mWidth / 2, mHeight));
        break;
    }

    mFbo->bind (nux::Geometry (0, 0, mWidth, mHeight));

    if (!mFbo->status ())
      LOG_INFO (logger) << "FBO not ok!";
  }

  glClearColor (1, 1, 1, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glPushMatrix ();
  glLoadIdentity();
  glTranslatef(-0.5f, -0.5f, -0.866025404f);
  glScalef (1.0f / mWidth, 1.0f / mHeight, 0.0f);
  glTranslatef(mWidth * 0.25, 0, 0);
  glRotatef(mTriangle->rotation (), 0.0f, 1.0f, 0.0f);
  glTranslatef(-(mWidth * 0.25), 0, 0);

  mTriangle->draw (mWidth, mHeight);

  glLoadIdentity();
  glTranslatef(-0.5f, -0.5f, -0.866025404f);
  glScalef (1.0f / mWidth, 1.0f / mHeight, 0.0f);
  glTranslatef(mWidth * 0.75, 0, 0);
  glRotatef(mSquare->rotation (), 0.0f, 1.0f, 0.0f);
  glTranslatef(-(mWidth * 0.75), 0, 0);

  mSquare->draw (mWidth, mHeight);

  glColor4f (1.0f, 1.0f, 1.0f, 5.0f);
  glPopMatrix ();

  if (mFbo)
    mFbo->unbind ();

  if (mFbo && mFbo->status ())
  {
    glClearColor (1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPushMatrix ();
    glLoadIdentity();
    glTranslatef(-0.5f, 0.5f, -0.866025404f);
    glScalef (1.0f / mWidth, -(1.0f / mHeight), 0.0f);
    mFbo->paint (nux::Geometry (0, 0, mWidth, mHeight));
    glPopMatrix ();

    nux::ObjectPtr<nux::IOpenGLTexture2D> device_texture =
        nux::GetGraphicsDisplay()->GetGpuDevice()->CreateTexture2DFromID (mFbo->texture(),
                                                                          mWidth, mHeight, 1, nux::BITFMT_R8G8B8A8);

    nux::GetGraphicsDisplay()->GetGpuDevice()->backup_texture0_ = device_texture;

    nux::Geometry geo = nux::Geometry (0, 0, mWidth, mHeight);
    BackgroundEffectHelper::monitor_rect_ = geo;
  }

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glPushAttrib(GL_VIEWPORT_BIT | GL_ENABLE_BIT |
               GL_TEXTURE_BIT | GL_COLOR_BUFFER_BIT | GL_SCISSOR_BIT);
  mRootView->ProcessDraw (mWindowThread->GetGraphicsEngine (), true);
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  glDrawBuffer(GL_BACK);
  glReadBuffer(GL_BACK);

  glPopAttrib();

  glXSwapBuffers (mDisplay, mGlxWindow);

  switch (mRotating)
  {
    case ModifierApplication::Both:
      mTriangle->rotate ();
      mSquare->rotate ();
      break;
    case ModifierApplication::Triangle:
      mTriangle->rotate ();
      break;
    case ModifierApplication::Square:
      mSquare->rotate ();
      break;
  }

  return true;
}

void
BaseContext::onWindowThreadCreation (nux::NThread *thread, void *data)
{
  BaseContext *bc = static_cast <BaseContext *> (data);

  bc->mRootView = new EffectView ();
  bc->mRootView->SetGeometry (nux::Geometry (0, 0, 640, 480));
  bc->mNuxReady = true;
  BackgroundEffectHelper::blur_type = unity::BLUR_ACTIVE;
}

int main (int argc, char **argv)
{
  Display *display = XOpenDisplay (NULL);
  nux::NuxInitialize (0);
  GLFuncs::init ();
	g_type_init ();
  g_thread_init (NULL);
  gtk_init(&argc, &argv);

  BaseContext *bc = new BaseContext (display);

  bc->run ();

  delete bc;

  XCloseDisplay (display);

  return 0;
}


