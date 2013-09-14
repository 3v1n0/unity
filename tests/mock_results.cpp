// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2013 Canonical Ltd
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
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

#include "MockResults.h"

namespace unity {
namespace dash {

// Template specialization for Result in tests
template<>
const Result Model<Result>::RowAtIndex(std::size_t index) const
{
  Result mock_result(nullptr, nullptr, nullptr);
  mock_result.uri.SetGetterFunction([index] { return "uri"+std::to_string(index); });
  mock_result.icon_hint.SetGetterFunction([index] { return "icon"+std::to_string(index); });
  mock_result.category_index.SetGetterFunction([] { return 0; });
  mock_result.result_type.SetGetterFunction([] { return 0; });
  mock_result.mimetype.SetGetterFunction([index] { return "mimetype"+std::to_string(index); });
  mock_result.name.SetGetterFunction([index] { return "result"+std::to_string(index); });
  mock_result.comment.SetGetterFunction([index] { return "comment"+std::to_string(index); });
  mock_result.dnd_uri.SetGetterFunction([index] { return "dnduri"+std::to_string(index); });
  mock_result.hints.SetGetterFunction([] { return glib::HintsMap(); });
  return mock_result;
}

}
}
