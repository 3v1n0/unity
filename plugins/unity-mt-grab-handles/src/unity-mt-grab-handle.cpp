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

#include "unity-mt-grab-handle.h"
#include "unity-mt-grab-handle-group.h"
#include "unity-mt-grab-handle-window.h"
#include "unity-mt-texture.h"
#include "unity-mt-grab-handle-impl-factory.h"

void
unity::MT::GrabHandle::buttonPress (int x,
                                    int y,
                                    unsigned int button) const
{
  mImpl->buttonPress (x, y, button);
}

void
unity::MT::GrabHandle::requestMovement (int x,
                                        int y,
                                        unsigned int button) const
{
  unity::MT::GrabHandleGroup::Ptr ghg = mOwner.lock ();
  ghg->requestMovement (x, y, (maskHandles.find (mId))->second, button);
}

void
unity::MT::GrabHandle::show ()
{
  mImpl->show ();
}

void
unity::MT::GrabHandle::hide ()
{
  mImpl->hide ();
}

void
unity::MT::GrabHandle::raise () const
{
  unity::MT::GrabHandleGroup::Ptr ghg = mOwner.lock ();
  std::shared_ptr <const unity::MT::GrabHandle> gh = shared_from_this ();
  ghg->raiseHandle (gh);
}

void
unity::MT::GrabHandle::reposition(int          x,
                                  int          y,
				  unsigned int flags)
{
  damage (mRect);

  if (flags & PositionSet)
  {
    mRect.x = x;
    mRect.y = y;
  }

  if (flags & PositionLock)
  {
    mImpl->lockPosition (x, y, flags);
  }

  damage (mRect);
}

void
unity::MT::GrabHandle::reposition(int x, int y, unsigned int flags) const
{
  if (flags & PositionLock)
  {
    mImpl->lockPosition (x, y, flags);
  }
}

unity::MT::TextureLayout
unity::MT::GrabHandle::layout()
{
  return TextureLayout(mTexture, mRect);
}

unity::MT::GrabHandle::GrabHandle(Texture::Ptr texture,
                                  unsigned int    width,
                                  unsigned int    height,
                                  const std::shared_ptr <GrabHandleGroup> &owner,
				  unsigned int    id) :
  mOwner(owner),
  mTexture (texture),
  mId(id),
  mRect (0, 0, width, height),
  mImpl (NULL)
{
}

unity::MT::GrabHandle::Ptr
unity::MT::GrabHandle::create (Texture::Ptr texture, unsigned int width, unsigned int height,
                               const std::shared_ptr <GrabHandleGroup> &owner,
                               unsigned int id)
{
  unity::MT::GrabHandle::Ptr p (new unity::MT::GrabHandle (texture, width, height, owner, id));
  p->mImpl = unity::MT::GrabHandle::ImplFactory::Default ()->create (p);

  return p;
}

unity::MT::GrabHandle::~GrabHandle()
{
  delete mImpl;
}
