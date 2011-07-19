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

#ifndef PLACES_EMPTY_VIEW_H
#define PLACES_EMPTY_VIEW_H

#include <Nux/Nux.h>
#include <Nux/View.h>
#include <NuxGraphics/GraphicsEngine.h>
#include <Nux/HLayout.h>
#include <Nux/VLayout.h>
#include "Introspectable.h"
#include "StaticCairoText.h"

class PlacesEmptyView : public nux::View, public unity::Introspectable
{
  NUX_DECLARE_OBJECT_TYPE (PlacesEmptyView, nux::View);
public:

  PlacesEmptyView ();
  ~PlacesEmptyView ();

  // nux::View overrides
  long ProcessEvent(nux::IEvent &ievent, long TraverseInfo, long ProcessEventInfo);
  void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  void DrawContent (nux::GraphicsEngine &GfxContext, bool force_draw);

  void SetText (const char *text);

protected:

  const gchar* GetName ();
  void AddProperties (GVariantBuilder *builder);

private:

  nux::StaticCairoText *_text;
};

#endif // PLACES_EMPTY_VIEW_H
