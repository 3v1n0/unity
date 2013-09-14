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

#ifndef _UNITY_MT_GRAB_HANDLES_TEXTURE_H
#define _UNITY_MT_GRAB_HANDLES_TEXTURE_H

#include <Nux/Nux.h>
#include <boost/noncopyable.hpp>

namespace unity
{
namespace MT
{
class Texture
{
  public:

    typedef std::shared_ptr <Texture> Ptr;

    virtual ~Texture ();

    class Factory :
      boost::noncopyable
    {
    public:

      virtual ~Factory ();

      virtual unity::MT::Texture::Ptr
      create () = 0;

      static void
      SetDefault (Factory *);

      static std::shared_ptr <Factory>
      Default ();

    protected:

      Factory ();

    private:

      static std::shared_ptr <unity::MT::Texture::Factory> mDefault;
    };

  protected:

    Texture ();
};


typedef std::pair <Texture::Ptr, nux::Geometry> TextureSize;
typedef std::pair <Texture::Ptr, nux::Geometry> TextureLayout;

};
};

#endif
