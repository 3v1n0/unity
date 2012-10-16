/*
 * Copyright (C) 2011 Canonical Ltd
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
 * Authored by: Sam Spilsbury <sam.spilsbury@canonical.com>
 */

#ifndef _UNITY_MT_GRAB_HANDLE_H
#define _UNITY_MT_GRAB_HANDLE_H

#include <Nux/Nux.h>
#include <glib.h>
#include <memory>
#include <boost/noncopyable.hpp>

#include "unity-mt-texture.h"
#include "unity-mt-grab-handle-window.h"

namespace unity
{
namespace MT
{

static const unsigned int NUM_HANDLES = 9;

/* Update the server side position */
static const unsigned int PositionLock = (1 << 0);

/* Update the client side position */
static const unsigned int PositionSet = (1 << 2);

static const unsigned int TopLeftHandle = (1 << 0);
static const unsigned int TopHandle = (1 << 1);
static const unsigned int TopRightHandle = (1 << 2);
static const unsigned int RightHandle = (1 << 3);
static const unsigned int BottomRightHandle = (1 << 4);
static const unsigned int BottomHandle = (1 << 5);
static const unsigned int BottomLeftHandle = (1 << 6);
static const unsigned int LeftHandle = (1 << 7);
static const unsigned int MiddleHandle = (1 << 8);

static const std::map <unsigned int, int> maskHandles = {
 { TopLeftHandle, 0 },
 { TopHandle, 1 },
 { TopRightHandle, 2 },
 { RightHandle, 3 },
 { BottomRightHandle, 4},
 { BottomHandle, 5 },
 { BottomLeftHandle, 6 },
 { LeftHandle, 7 },
 { MiddleHandle, 8 }
};

static const std::map <int, unsigned int> handlesMask = {
 { 0, TopLeftHandle },
 { 1, TopHandle },
 { 2, TopRightHandle },
 { 3, RightHandle },
 { 4, BottomRightHandle},
 { 5, BottomHandle },
 { 6, BottomLeftHandle },
 { 7, LeftHandle },
 { 8, MiddleHandle }
};

class GrabHandleGroup;

class GrabHandle :
  public std::enable_shared_from_this <GrabHandle>,
  boost::noncopyable
{
public:

  typedef std::shared_ptr <GrabHandle> Ptr;

  static GrabHandle::Ptr create (Texture::Ptr    texture,
                                 unsigned int    width,
                                 unsigned int    height,
                                 const std::shared_ptr <GrabHandleGroup> &owner,
                                 unsigned int id);
  ~GrabHandle();

  bool operator== (const GrabHandle &other) const
  {
    return mId == other.mId;
  }

  bool operator!= (const GrabHandle &other) const
  {
    return !(*this == other);
  }

  void buttonPress (int x,
                    int y,
                    unsigned int button) const;

  void requestMovement (int x,
                        int y,
                        unsigned int button) const;

  void reposition(int x,
		  int y,
                  unsigned int flags);
  void reposition(int x, int y, unsigned int flags) const;

  void show();
  void hide();
  void raise() const;

  TextureLayout layout();

  unsigned int id () const { return mId; }
  unsigned int width () const { return mRect.width; }
  unsigned int height () const { return mRect.height; }
  int          x () const { return mRect.x; }
  int          y () const { return mRect.y; }

  void damage (const nux::Geometry &g) const { mImpl->damage (g); }

public:

  class Impl :
    boost::noncopyable
  {
    public:

      virtual ~Impl () {};

      virtual void show () = 0;
      virtual void hide () = 0;

      virtual void buttonPress (int x,
                                int y,
                                unsigned int button) const = 0;

      virtual void lockPosition (int x,
                                 int y,
                                 unsigned int flags) = 0;

      virtual void damage (const nux::Geometry &g) = 0;
  };

  class ImplFactory;

private:

  GrabHandle(Texture::Ptr    texture,
             unsigned int    width,
             unsigned int    height,
             const std::shared_ptr <GrabHandleGroup> &owner,
             unsigned int id);

  std::weak_ptr <GrabHandleGroup>      mOwner;
  Texture::Ptr                           mTexture;
  unsigned int                           mId;
  nux::Geometry                          mRect;
  Impl                                   *mImpl;
};
};
};

#endif
