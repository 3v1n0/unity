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

#ifndef UNITY_RESULTS_H
#define UNITY_RESULTS_H

#include <memory>

#include "Model.h"
#include "Result.h"

namespace unity
{
namespace dash
{

class Results : public Model<Result>
{
public:
  typedef std::shared_ptr<Results> Ptr;

  Results();
  Results(ModelType model_type);

  sigc::signal<void, Result const&> result_added;
  sigc::signal<void, Result const&> result_changed;
  sigc::signal<void, Result const&> result_removed;

private:
  void OnRowAdded(Result& result);
  void OnRowChanged(Result& result);
  void OnRowRemoved(Result& result);
};

}
}

#endif
