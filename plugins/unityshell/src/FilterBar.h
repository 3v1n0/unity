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

#ifndef FILTERBAR_H
#define FILTERBAR_H

#include <Nux/View.h>

#include <UnityCore/Filters.h>

#include "FilterFactory.h"

namespace unity {

  class FilterBar : public nux::View
  {
    NUX_DECLARE_OBJECT_TYPE(FilterBar, nux::View);
  public:
    FilterBar(NUX_FILE_LINE_PROTO);
    ~FilterBar();

    void SetFilters (dash::Filters::Ptr filters);

    void AddFilter (dash::Filter::Ptr filter);
    void RemoveFilter (dash::Filter::Ptr filter);

  protected:
    virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
    virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);
    virtual void PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw);

  private:
    void Init ();

    FilterFactory factory_;
    dash::Filters::Ptr filters_;
    std::map <dash::Filter::Ptr, nux::View *> filter_map_;
  };
}

#endif // FILTERBAR_H
