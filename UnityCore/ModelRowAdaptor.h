// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#ifndef UNITY_MODEL_ROW_H
#define UNITY_MODEL_ROW_H

#include <string>

#include <dee.h>
#include "Variant.h"

namespace unity
{
namespace dash
{

/* RowAdaptors represent a row of a Model<RowAdaptorType> in a form that's easily
 * accessible for consumers of the Model. It means you can convert the
 * added/removed/changed signals from a pointer to the DeeModelIter to, for example,
 * a Dash Category, which represents the data in that DeeModelIter in an easy to use
 * manner.
 *
 * You should not expect to keep a RowAdaptor class around, rather you should use
 * the set_renderer() and renderer() functions to store a pointer to your view on
 * the row on the added signal. That means that you can easily access the matching
 * view to a RowAdaptor that you receive via the changed or removed signals.
 *
 * Finally, RowAdaptors are expected to be stack-allocated and extremly quick to
 * initialize and use (there might be thousands of results to display), therefore
 * it is expected that sub-classes continue the trend of a very light
 * construction, ideally involving no memory allocation.
 */
class RowAdaptorBase
{
public:
  RowAdaptorBase(DeeModel* model=0, DeeModelIter* iter=0, DeeModelTag* tag=0);
  RowAdaptorBase(RowAdaptorBase const& other);
  virtual ~RowAdaptorBase();

  RowAdaptorBase& operator=(RowAdaptorBase const& other);

  virtual std::string GetStringAt(int position) const;
  virtual bool GetBoolAt(int position) const;
  virtual int GetIntAt(int position) const;
  virtual unsigned int GetUIntAt(int position) const;
  virtual float GetFloatAt(int position) const;
  virtual glib::Variant GetVariantAt(int position) const;

  void SetTarget(DeeModel* model, DeeModelIter* iter, DeeModelTag* tag);

  template<typename T>
  void set_renderer(T renderer);

  template<typename T>
  T renderer() const;

  DeeModel* model() { return model_; }

protected:
  virtual void set_model_tag(gpointer value);
  virtual gpointer get_model_tag() const;

  DeeModel* model_;
  DeeModelIter* iter_;
  DeeModelTag* tag_;
};

}
}

#include "ModelRowAdaptor-inl.h"

#endif
