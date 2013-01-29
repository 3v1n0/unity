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

#include "unity-mt-grab-handle-group.h"

unsigned int unity::MT::FADE_MSEC = 0;

void
unity::MT::GrabHandleGroup::show(unsigned int handles)
{
  for(const unity::MT::GrabHandle::Ptr & handle : mHandles)
    if (handles & handle->id ())
      handle->show();

  mState = State::FADE_IN;
}

void
unity::MT::GrabHandleGroup::hide()
{
  for(const unity::MT::GrabHandle::Ptr & handle : mHandles)
    handle->hide();

  mState = State::FADE_OUT;
}

void
unity::MT::GrabHandleGroup::raiseHandle(const std::shared_ptr <const unity::MT::GrabHandle> &h)
{
  mOwner->raiseGrabHandle (h);
}

bool
unity::MT::GrabHandleGroup::animate(unsigned int msec)
{
  mMoreAnimate = false;

  switch (mState)
  {
    case State::FADE_IN:

      mOpacity += ((float) msec / (float) unity::MT::FADE_MSEC) * std::numeric_limits <unsigned short>::max ();

      if (mOpacity >= std::numeric_limits <unsigned short>::max ())
      {
        mOpacity = std::numeric_limits <unsigned short>::max ();
        mState = State::NONE;
      }
      break;
    case State::FADE_OUT:
      mOpacity -= ((float) msec / (float) unity::MT::FADE_MSEC) * std::numeric_limits <unsigned short>::max ();

      if (mOpacity <= 0)
      {
        mOpacity = 0;
        mState = State::NONE;
      }
      break;
    default:
      break;
  }

  mMoreAnimate = mState != State::NONE;

  return mMoreAnimate;
}

int
unity::MT::GrabHandleGroup::opacity()
{
  return mOpacity;
}

bool
unity::MT::GrabHandleGroup::visible()
{
  return mOpacity > 0.0f;
}

bool
unity::MT::GrabHandleGroup::needsAnimate()
{
  return mMoreAnimate;
}

void
unity::MT::GrabHandleGroup::relayout(const nux::Geometry& rect, bool hard)
{
  /* Each grab handle at each vertex, eg:
   *
   * 1 - topleft
   * 2 - top
   * 3 - topright
   * 4 - right
   * 5 - bottom-right
   * 6 - bottom
   * 7 - bottom-left
   * 8 - left
   */

  const float pos[9][2] =
  {
    {0.0f, 0.0f}, {0.5f, 0.0f}, {1.0f, 0.0f},
    {1.0f, 0.5f}, {1.0f, 1.0f},
    {0.5f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.5f},
    {0.5f, 0.5f} /* middle */
  };

  for (unsigned int i = 0; i < NUM_HANDLES; i++)
  {
    unity::MT::GrabHandle::Ptr & handle = mHandles.at(i);

    handle->reposition (rect.x + rect.width * pos[i][0] -
                        handle->width () / 2,
                        rect.y + rect.height * pos[i][1] -
                        handle->height () / 2,
                        unity::MT::PositionSet | (hard ? unity::MT::PositionLock : 0));
  }
}

unity::MT::GrabHandleGroup::GrabHandleGroup(GrabHandleWindow *owner,
                                            std::vector <unity::MT::TextureSize>  &textures) :
  mState(State::NONE),
  mOpacity(0.0f),
  mMoreAnimate(false),
  mOwner(owner)
{
}

unity::MT::GrabHandleGroup::Ptr
unity::MT::GrabHandleGroup::create (GrabHandleWindow *owner,
                                    std::vector<unity::MT::TextureSize> &textures)
{
    unity::MT::GrabHandleGroup::Ptr p = unity::MT::GrabHandleGroup::Ptr (new unity::MT::GrabHandleGroup (owner, textures));
    for (unsigned int i = 0; i < NUM_HANDLES; i++)
      p->mHandles.push_back(unity::MT::GrabHandle::create (textures.at(i).first,
                                                           textures.at(i).second.width,
                                                           textures.at(i).second.height,
                                                           p,
                                                           handlesMask.find (i)->second));
    return p;
}

unity::MT::GrabHandleGroup::~GrabHandleGroup()
{
  for (unity::MT::GrabHandle::Ptr & handle : mHandles)
    handle->damage (nux::Geometry (handle->x (),
                                   handle->y (),
                                   handle->width (),
                                   handle->height ()));
}

void
unity::MT::GrabHandleGroup::requestMovement (int x,
                                             int y,
					     unsigned int direction,
					     unsigned int button)
{
  mOwner->requestMovement (x, y, direction, button);
}

std::vector <unity::MT::TextureLayout>
unity::MT::GrabHandleGroup::layout(unsigned int handles)
{
  std::vector <unity::MT::TextureLayout> layout;

  for(const unity::MT::GrabHandle::Ptr & handle : mHandles)
    if (handle->id () & handles)
      layout.push_back (handle->layout ());

  return layout;
}

void
unity::MT::GrabHandleGroup::forEachHandle (const std::function <void (const unity::MT::GrabHandle::Ptr &)> &f)
{
  for (unity::MT::GrabHandle::Ptr &h : mHandles)
    f (h);
}
