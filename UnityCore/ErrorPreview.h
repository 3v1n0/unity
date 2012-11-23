// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011-2012 Canonical Ltd
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
 * Authored by: Diego Sarmentero <diego.sarmentero@canonical.com>
 */

#ifndef ERROR_PREVIEW_PREVIEW_H
#define ERROR_PREVIEW_PREVIEW_H

#include <memory>

#include <sigc++/trackable.h>

#include "PaymentPreview.h"

namespace unity
{
namespace dash
{

class ErrorPreview : public PaymentPreview
{
public:
  typedef std::shared_ptr<ErrorPreview> Ptr;

  ErrorPreview(unity::glib::Object<GObject> const& proto_obj);
  ~ErrorPreview();
};

}
}

#endif
