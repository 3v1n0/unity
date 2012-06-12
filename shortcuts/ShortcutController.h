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

#ifndef UNITYSHELL_SHORTCUTCONTROLLER_H
#define UNITYSHELL_SHORTCUTCONTROLLER_H

#include <boost/shared_ptr.hpp>

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>
#include <Nux/HLayout.h>
#include <NuxCore/Color.h>
#include <UnityCore/Variant.h>
#include <UnityCore/GLibSource.h>

#include "unity-shared/Animator.h"
#include "unity-shared/Introspectable.h"
#include "ShortcutModel.h"
#include "ShortcutView.h"
#include "unity-shared/UBusWrapper.h"

namespace unity
{
namespace shortcut
{

class Controller : public debug::Introspectable
{
public:
  typedef std::shared_ptr<Controller> Ptr;

  // Ctor
  Controller(std::list<AbstractHint::Ptr>& hints);

  // Public Methods
  bool Show();
  void Hide();

  bool Visible();
  bool IsEnabled();

  void SetAdjustment(int x, int y);
  void SetEnabled(bool enabled);

protected:
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

private:
  // Private Methods
  void ConstructView();
  void EnsureView();
  void OnBackgroundUpdate(GVariant* data);
  void OnFadeInUpdated(double opacity);
  void OnFadeInEnded();
  void OnFadeOutUpdated(double opacity);
  void OnFadeOutEnded();
  bool OnShowTimer();

  // Private Members
  View::Ptr view_;
  Model::Ptr model_;

  nux::Geometry workarea_;
  nux::ObjectPtr<nux::BaseWindow> view_window_;
  nux::HLayout* main_layout_;

  bool visible_;
  bool enabled_;
  nux::Color bg_color_;

  Animator fade_in_animator_;
  Animator fade_out_animator_;

  glib::Source::UniquePtr show_timer_;
  UBusManager ubus_manager_;
};

} // namespace shortcut
} // namespace unity

#endif //UNITYSHELL_SHORTCUTHINTCONTROLLER_H

