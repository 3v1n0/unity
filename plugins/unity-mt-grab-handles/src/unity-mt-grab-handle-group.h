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

#ifndef _UNITY_MT_GRAB_HANDLE_GROUP_H
#define _UNITY_MT_GRAB_HANDLE_GROUP_H

#include <Nux/Nux.h>
#include <glib.h>
#include <boost/noncopyable.hpp>
#include <memory>

#include "unity-mt-grab-handle.h"
#include "unity-mt-texture.h"
#include "unity-mt-grab-handle-window.h"

namespace unity
{
namespace MT
{
extern unsigned int FADE_MSEC;

class GrabHandleGroup :
  public std::enable_shared_from_this <GrabHandleGroup>,
  boost::noncopyable
{
public:

  typedef std::shared_ptr <GrabHandleGroup> Ptr;

  static GrabHandleGroup::Ptr create (GrabHandleWindow *owner,
                                      std::vector<TextureSize> &textures);
  ~GrabHandleGroup();

  void relayout(const nux::Geometry&, bool);
  void restack();

  bool visible();
  bool animate(unsigned int);
  bool needsAnimate();

  int opacity();

  void hide();
  void show(unsigned int handles = ~0);
  void raiseHandle (const std::shared_ptr <const unity::MT::GrabHandle> &);

  std::vector <TextureLayout> layout(unsigned int handles);

  void forEachHandle (const std::function<void (const unity::MT::GrabHandle::Ptr &)> &);

  void requestMovement (int x,
                        int y,
                        unsigned int direction,
                        unsigned int button);
private:

  GrabHandleGroup(GrabHandleWindow *owner,
                  std::vector<TextureSize> &textures);

  enum class State
  {
    FADE_IN = 1,
    FADE_OUT,
    NONE
  };

  State  			      mState;
  int   			      mOpacity;

  bool 				           mMoreAnimate;
  std::vector <unity::MT::GrabHandle::Ptr> mHandles;
  GrabHandleWindow 		           *mOwner;
};
};
};

#endif
