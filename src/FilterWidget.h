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

#include <Nux/View.h>

#ifndef FILTERWIDGET_H
#define FILTERWIDGET_H

namespace unity {
  class FilterWidget
  {
  public:
    virtual ~FilterWidget () {}
    virtual void SetFilter (void *) = 0; // FIXME - a void pointer for now, we don't have filters
    virtual std::string GetFilterType () = 0;
  };
}

#endif // FILTERWIDGET_H
