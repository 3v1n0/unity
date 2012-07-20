// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright 2012 Canonical Ltd.
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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 *
 */

#ifndef PREVIEWFACTORYITEM_H
#define PREVIEWFACTORYITEM_H

#include "unity-shared/Introspectable.h"
#include "PreviewFactoryItem.h"
#include <UnityCore/Preview.h>
#include "previews/Preview.h"

namespace unity
{
namespace dash
{

class PreviewFactoryCreator
{

private:
};

class IPreviewFactoryItem
{
public:
  virtual ~IPreviewFactoryItem() {} 
  
  virtual Preview::Ptr CreateModel(std::string const& uri, glib::Object<GObject> const& proto_obj) const = 0;
  virtual previews::Preview::Ptr CreateView(Preview::Ptr model) const = 0;
};

template <class MODEL, class VIEW>
class PreviewFactoryItem : public IPreviewFactoryItem
{
public:
  PreviewFactoryItem() {}

  Preview::Ptr CreateModel(std::string const& uri, glib::Object<GObject> const& proto_obj) const
  {
    Preview::Ptr preview(new MODEL(proto_obj));
    preview->preview_uri = uri;
    return preview;
  }
  previews::Preview::Ptr CreateView(Preview::Ptr model) const
  {
    return previews::Preview::Ptr(new VIEW(model));
  }
};

class PreviewFactoryOperator
{
public:
  ~PreviewFactoryOperator() {}

  virtual Preview::Ptr CreateModel(std::string const& uri) const
  {
    if (creator_ == nullptr)
      return nullptr;

    return creator_->CreateModel(uri, proto_obj_);
  }
  virtual previews::Preview::Ptr CreateView(Preview::Ptr model) const
  {
    if (creator_ == nullptr)
      return previews::Preview::Ptr(nullptr);

    return creator_->CreateView(model);
  }

private:
  PreviewFactoryOperator()
  : creator_(nullptr) {}

  PreviewFactoryOperator(glib::Object<GObject> const& proto_obj, IPreviewFactoryItem* creator)
  : proto_obj_(proto_obj)
  , creator_(creator)
  {    
  }

  glib::Object<GObject> proto_obj_;
  IPreviewFactoryItem* creator_;
  friend class PreviewFactory;
};

} // namespace dash
} // namespace unity

#endif //PREVIEWFACTORYITEM_H
