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

#ifndef PREVIEWINFOHINTWIDGET_H
#define PREVIEWINFOHINTWIDGET_H

#include <Nux/Nux.h>
#include <Nux/View.h>
#include <UnityCore/Preview.h>
#include "unity-shared/Introspectable.h"

namespace nux
{
class StaticCairoText;
}

namespace unity
{
namespace dash
{
namespace previews
{

class PreviewInfoHintWidget : public nux::View, public debug::Introspectable
{
public:
  typedef nux::ObjectPtr<PreviewInfoHintWidget> Ptr;
  NUX_DECLARE_OBJECT_TYPE(PreviewInfoHintWidget, nux::View);

  PreviewInfoHintWidget(dash::Preview::Ptr preview_model, int icon_size, bool visible_icons = true, bool condensed_format = false);
  virtual ~PreviewInfoHintWidget();

  void SetVisibleIcons(bool visible);
  void SetCondensedFormat(bool condensed);

  // From debug::Introspectable
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);
  
  void PreLayoutManagement();

protected:
  virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);

  void SetupBackground();
  void SetupViews();

  void IconLoaded(std::string const& texid,
                                    unsigned size,
                                    glib::Object<GdkPixbuf> const& pixbuf,
                                    std::string icon_name);

protected:
  int icon_size_;
  bool condensed_format_;
  bool visible_icons_;

  typedef nux::ObjectPtr<nux::StaticCairoText> StaticCairoTextPtr;
  typedef std::pair<StaticCairoTextPtr, StaticCairoTextPtr> InfoHint; 
  std::list<InfoHint> info_hints_;
  
  dash::Preview::Ptr preview_model_;
  typedef nux::ObjectPtr<nux::BaseTexture> BaseTexturePtr;
};

} // napespace prviews
} // namespace dash
} // namespace unity

#endif //PREVIEWINFOHINTWIDGET_H
