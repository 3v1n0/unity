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

#ifndef UNITY_SCREENEFFECT_FRAMEBUFFER_H
#define UNITY_SCREENEFFECT_FRAMEBUFFER_H

#ifndef USE_MODERN_COMPIZ_GL
#include <Nux/Nux.h>

namespace unity
{
class ScreenEffectFramebufferObject
{
public:

  typedef boost::shared_ptr <ScreenEffectFramebufferObject> Ptr;
  typedef void (*FuncPtr) (void);
  typedef FuncPtr (*GLXGetProcAddressProc) (const GLubyte *procName);

  ScreenEffectFramebufferObject (GLXGetProcAddressProc, const nux::Geometry &geom);
  ~ScreenEffectFramebufferObject ();

public:

  void bind (const nux::Geometry &geom);
  void unbind ();

  bool status ();
  void paint (const nux::Geometry &geom);
  bool bound () { return mBoundCnt > 0; }

  GLuint texture () { return mFBTexture; }
  
  void onScreenSizeChanged (const nux::Geometry &screenSize);

private:

  FuncPtr getProcAddr (const std::string &);

  typedef void (*GLActiveTextureProc) (GLenum texture);
  typedef void (*GLGenFramebuffersProc) (GLsizei n,
                                         GLuint  *framebuffers);
  typedef void (*GLDeleteFramebuffersProc) (GLsizei n,
                                            GLuint  *framebuffers);
  typedef void (*GLBindFramebufferProc) (GLenum target,
                                         GLuint framebuffer);
  typedef GLenum (*GLCheckFramebufferStatusProc) (GLenum target);
  typedef void (*GLFramebufferTexture2DProc) (GLenum target,
                                              GLenum attachment,
                                              GLenum textarget,
                                              GLuint texture,
                                              GLint  level);
  
  GLXGetProcAddressProc getProcAddressGLX;
  GLActiveTextureProc activeTexture;
  GLGenFramebuffersProc genFramebuffers;
  GLDeleteFramebuffersProc deleteFramebuffers;
  GLBindFramebufferProc bindFramebuffer;
  GLCheckFramebufferStatusProc checkFramebufferStatus;
  GLFramebufferTexture2DProc framebufferTexture2D;
  /* compiz fbo handle that goes through to nux */
  GLuint   mFboHandle; // actual handle to the framebuffer_ext
  bool    mFboStatus; // did the framebuffer texture bind succeed
  GLuint   mFBTexture;
  nux::Geometry mGeometry;
  unsigned int mBoundCnt;
  
  nux::Geometry mScreenSize;
};
} // namespace unity

#endif // USE_MODERN_COMPIZ_GL
#endif
