// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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
 * Authored by: Andrea Azzarone <azzaronea@gmail.com>
 *
 */

#ifndef UNITYSHELL_FILTERALLBUTTON_H
#define UNITYSHELL_FILTERALLBUTTON_H

#include <sigc++/connection.h>
#include <UnityCore/ConnectionManager.h>
#include <UnityCore/Filter.h>

#include "FilterBasicButton.h"

namespace unity
{
namespace dash
{

class FilterAllButton : public FilterBasicButton
{
  NUX_DECLARE_OBJECT_TYPE(FilterAllButton, FilterBasicButton);
public:
   FilterAllButton(NUX_FILE_LINE_PROTO);

   void SetFilter(Filter::Ptr const& filter);

private:
  void OnFilteringChanged(bool filtering);
  void OnStateChanged(nux::View* view);

  Filter::Ptr filter_;
  connection::Wrapper filtering_connection_;
};

} // namespace dash
} // namespace unity

#endif // UNITYSHELL_FILTERALLBUTTON_H

