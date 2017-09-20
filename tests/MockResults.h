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
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 *              Marco Trevisan <marco.trevisan@canonical.com>
 */

#ifndef UNITY_MOCK_RESULTS_H
#define UNITY_MOCK_RESULTS_H

#include <UnityCore/Results.h>

namespace unity {
namespace dash {

struct MockResult : Result
{
  MockResult(std::size_t index)
    : Result(nullptr, nullptr, nullptr)
  {
    uri.SetGetterFunction([index] { return "proto://result-" + std::to_string(index); });
    icon_hint.SetGetterFunction([index] { return ""; });
    category_index.SetGetterFunction([index] { return 0; });
    result_type.SetGetterFunction([index] { return 0; });
    mimetype.SetGetterFunction([index] { return "mime-type-" + std::to_string(index); });
    name.SetGetterFunction([index] { return "MockResult " + std::to_string(index); });
    comment.SetGetterFunction([index] { return "Just a pointless result " + std::to_string(index); });
    dnd_uri.SetGetterFunction([index] { return "dnd://mock-result" + std::to_string(index); });
    hints.SetGetterFunction([index] { return glib::HintsMap(); });
  }
};

struct MockResults : Results
{
  MockResults(unsigned int count_)
    : Results(LOCAL)
  {
    count.SetGetterFunction([count_] { return count_; });
  }

  virtual const Result RowAtIndex(std::size_t index) const override
  {
    return MockResult(index);
  }
};

// Template specialization for Result in tests
template<> const Result Model<Result>::RowAtIndex(std::size_t index) const
{
  return MockResult(index);
}

}
}

#endif
