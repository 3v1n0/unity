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
#include <Nux/HLayout.h>
#include <Nux/View.h>

#include "unity-shared/UnityWindowView.h"
#include "unity-shared/BackgroundEffectHelper.h"
#include "ShortcutModel.h"

namespace unity
{
class StaticCairoText;

namespace shortcut
{

class View : public ui::UnityWindowView
{
  NUX_DECLARE_OBJECT_TYPE(View, ui::UnityWindowView);
public:
  typedef nux::ObjectPtr<View> Ptr;

  // Ctor
  View();

  // Public methods
  void SetModel(Model::Ptr model);
  Model::Ptr GetModel();

protected:
  // Protected methods
  void DrawOverlay(nux::GraphicsEngine& GfxContext, bool force_draw, nux::Geometry const& clip) override;
  nux::Geometry GetBackgroundGeometry() override;
  void PreLayoutManagement() override;

  // Introspectable methods
  std::string GetName() const;

private:
  // Private methods
  nux::LinearLayout* CreateSectionLayout(std::string const& section_name);
  nux::View* CreateShortKeyEntryView(AbstractHint::Ptr const&, StaticCairoText* shortkey, StaticCairoText* description);
  StaticCairoText* CreateShortcutTextView(std::string const& text, bool bold);
  nux::LinearLayout* CreateIntermediateLayout();

  void RenderColumns();

  // Private members
  Model::Ptr model_;
  nux::HLayout* columns_layout_;
  std::vector<std::vector<StaticCairoText*>> shortkeys_;
  std::vector<std::vector<StaticCairoText*>> descriptions_;

  friend class TestShortcutView;
};

} // namespace shortcut

} // namespace unity

#endif // UNITYSHELL_SHORTCUTVIEW_H

