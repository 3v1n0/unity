// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011-2013 Canonical Ltd
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
 * Authored by: Manuel de la Pena <manuel.delapena@canonical.com>
 */

#ifndef UNITY_PAYMENT_PREVIEW_PREVIEW_H
#define UNITY_PAYMENT_PREVIEW_PREVIEW_H

#include <memory>

#include <sigc++/trackable.h>

#include "Preview.h"

namespace unity
{
namespace dash
{

class PaymentPreview : public Preview
{
public:
  enum PreviewType { APPLICATION, MUSIC, ERROR };

  typedef std::shared_ptr<PaymentPreview> Ptr;

  PaymentPreview(unity::glib::Object<GObject> const& proto_obj);
  ~PaymentPreview();

  // properties that contain the data
  nux::RWProperty<std::string> title;
  nux::RWProperty<std::string> subtitle;
  nux::RWProperty<std::string> header;
  nux::RWProperty<std::string> email;
  nux::RWProperty<std::string> payment_method;
  nux::RWProperty<std::string> purchase_prize;
  nux::RWProperty<std::string> purchase_type;
  nux::RWProperty<PaymentPreview::PreviewType> preview_type;

private:
  class Impl;
  std::unique_ptr<Impl> pimpl;
};

}
}

#endif
