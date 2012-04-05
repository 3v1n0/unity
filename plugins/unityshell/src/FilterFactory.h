/*
 * Copyright 2011 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 *
 */

#ifndef UNITYSHELL_FILTERFACTORY_H
#define UNITYSHELL_FILTERFACTORY_H

#include <Nux/View.h>
#include <UnityCore/Filter.h>

namespace unity
{
namespace dash
{

class FilterExpanderLabel;

class FilterFactory
{
public:
  FilterExpanderLabel* WidgetForFilter(Filter::Ptr const& filter);
};

} // namespace dash
} // namespace unity

#endif // UNITYSHELL_FILTERFACTORY_H

