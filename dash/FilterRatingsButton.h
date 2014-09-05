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

#ifndef UNITYSHELL_FILTERRATINGSBUTTONWIDGET_H
#define UNITYSHELL_FILTERRATINGSBUTTONWIDGET_H

#include <memory>

#include <Nux/Nux.h>
#include <UnityCore/RatingsFilter.h>
#include "unity-shared/RatingsButton.h"

namespace unity
{
namespace dash
{

class FilterRatingsButton : public RatingsButton
{
  NUX_DECLARE_OBJECT_TYPE(FilterRatingsButton, RatingsButton);
public:
  FilterRatingsButton(NUX_FILE_LINE_PROTO);

  void SetFilter(Filter::Ptr const& filter);
  RatingsFilter::Ptr GetFilter() const;
  std::string GetFilterType();

protected:
  // Introspectable methods
  std::string GetName() const;

  void SetRating(float rating) override;
  float GetRating() const override;

private:
  dash::RatingsFilter::Ptr filter_;
};

} // namespace dash
} // namespace unity

#endif // UNITYSHELL_FILTERRATINGSBUTTONWIDGET_H

