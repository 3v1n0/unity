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

#ifndef _UNITY_MT_GRAB_HANDLE_LAYOUT_H
#define _UNITY_MT_GRAB_HANDLE_LAYOUT_H

namespace unity
{
namespace MT
{
extern unsigned int MaximizedVertMask;
extern unsigned int MaximizedHorzMask;
extern unsigned int MoveMask;
extern unsigned int ResizeMask;

unsigned int getLayoutForMask (unsigned int state,
                               unsigned int actions);
};
};

#endif
