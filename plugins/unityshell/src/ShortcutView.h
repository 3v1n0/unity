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
 * Authored by: Andrea Azzarone <azzaronea@gmail.com>
 */

#ifndef UNITYSHELL_SHORTCUTVIEW_H
#define UNITYSHELL_SHORTCUTVIEW_H

#include <Nux/Nux.h>
#include <Nux/GridHLayout.h>
#include <Nux/HLayout.h>
#include <NuxCore/ObjectPtr.h>
#include <NuxCore/Property.h>
#include <Nux/StaticText.h>
#include <Nux/View.h>
#include <Nux/VLayout.h>

#include "BackgroundEffectHelper.h"
#include "ShortcutModel.h"

namespace unity
{
namespace shortcut
{

class View : public nux::View
{
  NUX_DECLARE_OBJECT_TYPE(View, nux::View);
public:
  typedef nux::ObjectPtr<View> Ptr;

  // Ctor and dtor
  View(NUX_FILE_LINE_PROTO);
  ~View();
  
  // Public methods
  void SetModel(Model::Ptr model);
  Model::Ptr GetModel();

  void SetupBackground(bool enabled);

  // Properties  
  nux::Property<nux::Color> background_color;
  
protected:
  // Protected methods
  void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);

private:
  // Private methods
  nux::LinearLayout* CreateSectionLayout(const char* section_name);
  nux::LinearLayout* CreateShortKeyEntryLayout(AbstractHint* hint);
  nux::LinearLayout* CreateIntermediateLayout();

  void DrawBackground(nux::GraphicsEngine& GfxContext, nux::Geometry const& geo);
  void RenderColumns();
    
  // Private members
  Model::Ptr model_;

  nux::BaseTexture* background_top_;
  nux::BaseTexture* background_left_;
  nux::BaseTexture* background_corner_;
  nux::BaseTexture* rounding_texture_;

  nux::VLayout* layout_;
  nux::HLayout* columns_layout_;
  std::vector<nux::VLayout*> columns_;
  
  BackgroundEffectHelper bg_effect_helper_;
  
};

} // namespace shortcut

} // namespace unity

#endif // UNITYSHELL_SHORTCUTVIEW_H

