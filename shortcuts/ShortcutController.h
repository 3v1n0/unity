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
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 */

#ifndef UNITYSHELL_SHORTCUT_CONTROLLER_H
#define UNITYSHELL_SHORTCUT_CONTROLLER_H

#include <memory>

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>
#include <Nux/HLayout.h>
#include <NuxCore/Color.h>
#include <NuxCore/Animation.h>
#include <UnityCore/Variant.h>
#include <UnityCore/GLibSource.h>

#include "BaseWindowRaiser.h"
#include "ShortcutModel.h"
#include "ShortcutView.h"
#include "unity-shared/Introspectable.h"
#include "unity-shared/UBusWrapper.h"

namespace unity
{
namespace shortcut
{

class Controller : public debug::Introspectable
{
public:
  typedef std::shared_ptr<Controller> Ptr;

  Controller(std::list<AbstractHint::Ptr> const& hints,
             BaseWindowRaiser::Ptr const& raiser);
  virtual ~Controller();

  bool Show();
  void Hide();

  bool Visible() const;
  bool IsEnabled() const;

  void SetAdjustment(int x, int y);
  void SetEnabled(bool enabled);
  virtual void SetOpacity(double value);

protected:
  // Introspectable
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

private:
  void ConstructView();
  void EnsureView();
  void OnBackgroundUpdate(GVariant* data);
  bool OnShowTimer();

  View::Ptr view_;
  Model::Ptr model_;
  BaseWindowRaiser::Ptr base_window_raiser_;

  nux::Geometry workarea_;
  nux::ObjectPtr<nux::BaseWindow> view_window_;
  nux::HLayout* main_layout_;

  bool visible_;
  bool enabled_;
  nux::Color bg_color_;

  nux::animation::AnimateValue<double> fade_animator_;

  glib::Source::UniquePtr show_timer_;
  UBusManager ubus_manager_;
};

}
}

#endif
