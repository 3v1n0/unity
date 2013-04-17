// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010 Canonical Ltd
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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 */

#ifndef UNITY_GRAPHICS_ADAPTER
#define UNITY_GRAPHICS_ADAPTER

#include <Nux/Nux.h>

namespace unity
{
namespace graphics
{

void PushOffscreenRenderTarget(nux::ObjectPtr<nux::IOpenGLBaseTexture> const& texture);
void PopOffscreenRenderTarget();

void ClearGeometry(nux::Geometry const& geo, nux::Color const& color = nux::Color(0.0f, 0.0f, 0.0f, 0.0f));

}
}

#endif // UNITY_GRAPHICS_ADAPTER