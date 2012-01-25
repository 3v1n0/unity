// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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

#ifndef UNITYSHELL_FILTERBAR_H
#define UNITYSHELL_FILTERBAR_H

#include <Nux/View.h>

#include <UnityCore/Filters.h>

#include "FilterFactory.h"

namespace unity
{
namespace dash
{

class FilterBar : public nux::View
{
  NUX_DECLARE_OBJECT_TYPE(FilterBar, nux::View);
public:
  FilterBar(NUX_FILE_LINE_PROTO);
  ~FilterBar();

  void SetFilters(Filters::Ptr const& filters);

  void AddFilter(Filter::Ptr const& filter);
  void RemoveFilter(Filter::Ptr const& filter);

protected:
  virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);

private:
  void Init();

  FilterFactory factory_;
  Filters::Ptr filters_;
  std::map<Filter::Ptr, nux::View*> filter_map_;
  nux::Texture2D* separator_;
};

} // namespace dash
} // namespace unity

#endif // UNITYSHELL_FILTERBAR_H

