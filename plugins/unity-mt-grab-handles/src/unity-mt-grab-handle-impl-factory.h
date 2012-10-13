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

#ifndef _UNITY_MT_GRAB_HANDLE_IMPL_FACTORY_H
#define _UNITY_MT_GRAB_HANDLE_IMPL_FACTORY_H

#include <Nux/Nux.h>
#include <glib.h>
#include <boost/noncopyable.hpp>
#include <memory>

#include "unity-mt-grab-handle.h"

namespace unity
{
namespace MT
{
class GrabHandle::ImplFactory
{
  public:

    virtual ~ImplFactory() {};

    static std::shared_ptr <ImplFactory>
    Default();

    static void
    SetDefault(ImplFactory *);

    virtual GrabHandle::Impl * create(const GrabHandle::Ptr &h) = 0;

  protected:

    static std::shared_ptr <ImplFactory> mDefault;

    ImplFactory() {};
};
};
};

#endif
