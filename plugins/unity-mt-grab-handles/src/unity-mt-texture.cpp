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

#include "unity-mt-texture.h"

std::shared_ptr <unity::MT::Texture::Factory> unity::MT::Texture::Factory::mDefault;

unity::MT::Texture::Factory::Factory ()
{
}

unity::MT::Texture::Factory::~Factory ()
{
}

void
unity::MT::Texture::Factory::SetDefault (Factory *f)
{
  mDefault.reset (f);
}

std::shared_ptr <unity::MT::Texture::Factory>
unity::MT::Texture::Factory::Default ()
{
  return mDefault;
}

unity::MT::Texture::Texture ()
{
}

unity::MT::Texture::~Texture ()
{
}
