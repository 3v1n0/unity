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

#include "unity-mt-grab-handle-impl-factory.h"
#include "unity-mt-grab-handle.h"

std::shared_ptr <unity::MT::GrabHandle::ImplFactory> unity::MT::GrabHandle::ImplFactory::mDefault;

std::shared_ptr <unity::MT::GrabHandle::ImplFactory>
unity::MT::GrabHandle::ImplFactory::Default()
{
  return mDefault;
}

void
unity::MT::GrabHandle::ImplFactory::SetDefault (ImplFactory *factory)
{
  mDefault.reset (factory);
}
