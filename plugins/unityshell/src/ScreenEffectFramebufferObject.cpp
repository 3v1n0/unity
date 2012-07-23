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

#ifndef USE_MODERN_COMPIZ_GL
#include "ScreenEffectFramebufferObject.h"
#include "BackgroundEffectHelper.h"
#include <NuxCore/Logger.h>
#include <dlfcn.h>

namespace
{
  nux::logging::Logger logger ("unity.screeneffectframebufferobject");
}

void unity::ScreenEffectFramebufferObject::paint (const nux::Geometry &output)
{
  /* Draw the bit of the relevant framebuffer for each output */

  glPushAttrib (GL_VIEWPORT_BIT);
  glViewport (0, 0, mScreenSize.width, mScreenSize.height);

  if (mFBTexture)
  {
    glEnable (GL_TEXTURE_2D);
    activeTexture (GL_TEXTURE0_ARB);
    glBindTexture (GL_TEXTURE_2D, mFBTexture);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glPushAttrib (GL_SCISSOR_BIT);
    glEnable (GL_SCISSOR_TEST);

    glScissor (output.x, mScreenSize.height - (output.y + output.height),
	       output.width, output.height);

    /* FIXME: This needs to be GL_TRIANGLE_STRIP */
    glBegin (GL_QUADS);
    glTexCoord2f (0, 1);
    glVertex2i   (mGeometry.x, mGeometry.y);
    glTexCoord2f (0, 0);
    glVertex2i   (mGeometry.x, mGeometry.y + mGeometry.height);
    glTexCoord2f (1, 0);
    glVertex2i   (mGeometry.x + mGeometry.width, mGeometry.y + mGeometry.height);
    glTexCoord2f (1, 1);
    glVertex2i   (mGeometry.x + mGeometry.width, mGeometry.y);
    glEnd ();

    activeTexture (GL_TEXTURE0_ARB);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture (GL_TEXTURE_2D, 0);
    glDisable (GL_TEXTURE_2D);
    glPopAttrib ();
  }
  glPopAttrib ();
}

void unity::ScreenEffectFramebufferObject::onScreenSizeChanged(const nux::Geometry& screenSize)
{
  mScreenSize = screenSize;
}


void unity::ScreenEffectFramebufferObject::unbind ()
{
  if (!mBoundCnt)
    return;

  mBoundCnt--;

  (*bindFramebuffer) (GL_FRAMEBUFFER_EXT, 0);

  glDrawBuffer (GL_BACK);
  glReadBuffer (GL_BACK);

  /* Matches the viewport set we did in ::bind () */
  glPopAttrib ();

}

bool unity::ScreenEffectFramebufferObject::status ()
{
  return mFboStatus;
}

void unity::ScreenEffectFramebufferObject::bind (const nux::Geometry &output)
{
  /* Very important!
   * Don't bind unless BackgroundEffectHelper says it's necessary.
   * Because binding has a severe impact on graphics performance and we
   * can't afford to do it every frame. (LP: #861061) (LP: #987304)
   */
  if (!BackgroundEffectHelper::HasDirtyHelpers())
    return;

  /* Clear the error bit */
  glGetError ();

  if (!mFBTexture)
  {
    glGenTextures (1, &mFBTexture);

    glBindTexture (GL_TEXTURE_2D, mFBTexture);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, mGeometry.width, mGeometry.height, 0, GL_BGRA,
#if IMAGE_BYTE_ORDER == MSBFirst
                 GL_UNSIGNED_INT_8_8_8_8_REV,
#else
                 GL_UNSIGNED_BYTE,
#endif
                 NULL);

    glBindTexture (GL_TEXTURE_2D, 0);

    if (glGetError () != GL_NO_ERROR)
    {
      mFboHandle = 0;
      mFboStatus = false;
      return;
    }
  }

  (*bindFramebuffer) (GL_FRAMEBUFFER_EXT, mFboHandle);

  (*framebufferTexture2D) (GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                               GL_TEXTURE_2D, mFBTexture, 0);

  (*framebufferTexture2D) (GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
                               GL_TEXTURE_2D, 0, 0);

  /* Ensure that a framebuffer is actually available */
  if (!mFboStatus)
  {
    GLint status = (*checkFramebufferStatus) (GL_DRAW_FRAMEBUFFER);

    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
      switch (status)
      {
        case GL_FRAMEBUFFER_UNDEFINED:
          LOG_WARN (logger) <<  "no window";
          break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
          LOG_WARN (logger) <<  "attachment incomplete";
          break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
          LOG_WARN (logger) <<  "no buffers attached to fbo";
          break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
          LOG_WARN (logger) <<  "some attachment in glDrawBuffers doesn't exist in FBO";
          break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
          LOG_WARN (logger) <<  "some attachment in glReadBuffers doesn't exist in FBO";
          break;
        case GL_FRAMEBUFFER_UNSUPPORTED:
          LOG_WARN (logger) <<  "unsupported internal format";
          break;
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
          LOG_WARN (logger) <<  "different levels of sampling for each attachment";
          break;
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
          LOG_WARN (logger) <<  "number of layers is different";
          break;
        default:
          LOG_WARN (logger) <<  "unable to bind the framebuffer for an unknown reason";
          break;
      }

      bindFramebuffer (GL_FRAMEBUFFER_EXT, 0);
      deleteFramebuffers (1, &mFboHandle);

      glDrawBuffer (GL_BACK);
      glReadBuffer (GL_BACK);

      mFboHandle = 0;

      mFboStatus = false;
    }
    else
      mFboStatus = true;
  }

  if (mFboStatus)
  {
    glPushAttrib (GL_VIEWPORT_BIT);

    glViewport (output.x,
	       mScreenSize.height - (output.y + output.height),
	       output.width,
	       output.height);
  }

  mBoundCnt++;
}


unity::ScreenEffectFramebufferObject::ScreenEffectFramebufferObject (GLXGetProcAddressProc p, const nux::Geometry &geom)
 : getProcAddressGLX (p)
 , mFboStatus (false)
 , mFBTexture (0)
 , mGeometry (geom)
 , mBoundCnt (0)
 , mScreenSize (geom)
{
  activeTexture = (GLActiveTextureProc) (*getProcAddressGLX) ((GLubyte *) "glActiveTexture");
  genFramebuffers = (GLGenFramebuffersProc) (*getProcAddressGLX) ((GLubyte *)"glGenFramebuffersEXT");
  deleteFramebuffers = (GLDeleteFramebuffersProc) (*getProcAddressGLX) ((GLubyte *)"glDeleteFramebuffersEXT");
  bindFramebuffer = (GLBindFramebufferProc) (*getProcAddressGLX) ((GLubyte *)"glBindFramebufferEXT");
  checkFramebufferStatus = (GLCheckFramebufferStatusProc) (*getProcAddressGLX) ((GLubyte *) "glCheckFramebufferStatusEXT");
  framebufferTexture2D = (GLFramebufferTexture2DProc) (*getProcAddressGLX) ((GLubyte *) "glFramebufferTexture2DEXT");
  
  (*genFramebuffers) (1, &mFboHandle);
}

unity::ScreenEffectFramebufferObject::~ScreenEffectFramebufferObject ()
{
  (*deleteFramebuffers) (1, &mFboHandle);

  if (mFBTexture)
    glDeleteTextures (1, &mFBTexture);
}

#endif // USE_MODERN_COMPIZ_GL

