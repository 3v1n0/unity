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

#include "Results.h"

namespace unity
{
namespace dash
{

Results::Results()
: Model<Result>::Model(ModelType::REMOTE_SHARED)
{
  row_added.connect(sigc::mem_fun(this, &Results::OnRowAdded));
  row_changed.connect(sigc::mem_fun(this, &Results::OnRowChanged));
  row_removed.connect(sigc::mem_fun(this, &Results::OnRowRemoved));
}

Results::Results(ModelType model_type)
  : Model<Result>::Model(model_type)
{
  row_added.connect(sigc::mem_fun(this, &Results::OnRowAdded));
  row_changed.connect(sigc::mem_fun(this, &Results::OnRowChanged));
  row_removed.connect(sigc::mem_fun(this, &Results::OnRowRemoved));
}

ResultIterator Results::begin()
{
  return ResultIterator(model(), dee_model_get_first_iter(model()), GetTag());
}

ResultIterator Results::end()
{
  return ResultIterator(model(), dee_model_get_last_iter(model()), GetTag());
}

void Results::OnRowAdded(Result& result)
{
  result_added.emit(result);
}

void Results::OnRowChanged(Result& result)
{
  result_changed.emit(result);
}

void Results::OnRowRemoved(Result& result)
{
  result_removed.emit(result);
}

}
}
