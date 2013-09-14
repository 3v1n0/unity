/*
 * Copyright (C) 2012 Canonical Ltd
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

#ifndef UNITYSHELL_HUD_ABSTRACT_VIEW_H
#define UNITYSHELL_HUD_ABSTRACT_VIEW_H

#include <memory>
#include <string>

#include <Nux/Nux.h>
#include <Nux/View.h>
#include <UnityCore/Hud.h>

#include "unity-shared/Introspectable.h"

namespace unity
{
namespace hud
{

class AbstractView : public nux::View, public unity::debug::Introspectable
{
  NUX_DECLARE_OBJECT_TYPE(AbstractView, nux::View);
public:
  typedef nux::ObjectPtr<AbstractView> Ptr;

  AbstractView();

  virtual void AboutToShow() = 0;
  virtual void AboutToHide() = 0;
  virtual void ResetToDefault() = 0;
  virtual void SearchFinished() = 0;
  virtual void SetIcon(std::string const& icon_name, unsigned int tile_size, unsigned int size, unsigned int padding) = 0;
  virtual void SetQueries(Hud::Queries queries) = 0;
  virtual void SetMonitorOffset(int x, int y) = 0;
  virtual void ShowEmbeddedIcon(bool show) = 0;
  virtual nux::Geometry GetContentGeometry() = 0;

  virtual nux::View* default_focus() const = 0;

  // signals
  sigc::signal<void, std::string> search_changed;
  sigc::signal<void, std::string> search_activated;
  sigc::signal<void, Query::Ptr> query_activated;
  sigc::signal<void, Query::Ptr> query_selected;
  sigc::signal<void> layout_changed;
};

} // namespace hud
} // namespace unity

#endif // UNITYSHELL_HUD_ABSTRACT_VIEW_H
