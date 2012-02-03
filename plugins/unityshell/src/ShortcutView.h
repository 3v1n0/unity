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

#include "UnityWindowView.h"
#include "BackgroundEffectHelper.h"
#include "ShortcutModel.h"

namespace unity
{
namespace shortcut
{

class View : public ui::UnityWindowView
{
  NUX_DECLARE_OBJECT_TYPE(View, ui::UnityWindowView);
public:
  typedef nux::ObjectPtr<View> Ptr;

  // Ctor and dtor
  View();
  ~View();
  
  // Public methods
  void SetModel(Model::Ptr model);
  Model::Ptr GetModel();
  
protected:
  // Protected methods
  void DrawOverlay(nux::GraphicsEngine& GfxContext, bool force_draw, nux::Geometry clip);
  nux::Geometry GetBackgroundGeometry();

private:
  // Private methods
  nux::LinearLayout* CreateSectionLayout(const char* section_name);
  nux::LinearLayout* CreateShortKeyEntryLayout(AbstractHint* hint);
  nux::LinearLayout* CreateIntermediateLayout();

  void RenderColumns();
    
  // Private members
  Model::Ptr model_;

  nux::VLayout* layout_;
  nux::HLayout* columns_layout_;
  std::vector<nux::VLayout*> columns_;
};

} // namespace shortcut

} // namespace unity

#endif // UNITYSHELL_SHORTCUTVIEW_H

