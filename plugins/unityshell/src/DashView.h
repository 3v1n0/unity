/*
 * Copyright (C) 2010 Canonical Ltd
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

#ifndef UNITY_DASH_VIEW_H_
#define UNITY_DASH_VIEW_H_

#include <NuxGraphics/GraphicsEngine.h>
#include <Nux/Nux.h>
#include <Nux/View.h>
#include <Nux/VLayout.h>
#include <UnityCore/FilesystemLenses.h>

#include "Introspectable.h"
#include "UBusWrapper.h"

namespace unity
{
namespace dash
{

class DashView : public nux::View, public unity::Introspectable
{
  NUX_DECLARE_OBJECT_TYPE(DashView, nux::View);

public:
  DashView();
  ~DashView();

protected:
  const gchar* GetName();
  void AddProperties(GVariantBuilder* builder);

private:
  long ProcessEvent(nux::IEvent& ievent, long traverse_info, long event_info);
  void Draw(nux::GraphicsEngine& gfx_context, bool force_draw);
  void DrawContent(nux::GraphicsEngine& gfx_context, bool force_draw);

private:
  UBusManager ubus_manager_;
  FilesystemLenses lenses_;

  nux::VLayout* layout_;
};


}
}
#endif
