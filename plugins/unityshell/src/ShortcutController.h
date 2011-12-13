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

#include "Animator.h"
#include "ShortcutModel.h"
#include "ShortcutView.h"

namespace unity
{
namespace shortcut
{

class Controller
{
public:
  typedef std::shared_ptr<Controller> Ptr;
  
  // Ctor and dtor
  Controller(std::list<AbstractHint*>& hints);
  ~Controller();  
  
  // Public Methods
  void Show();
  void Hide();
  
  bool Visible();

  void SetWorkspace(nux::Geometry geo);

private:
  // Private Methods
  void ConstructView();
  static void OnBackgroundUpdate(GVariant* data, Controller* self);
  void OnFadeInUpdated(double opacity);
  void OnFadeInEnded();
  void OnFadeOutUpdated(double opacity);
  void OnFadeOutEnded();

  static gboolean OnShowTimer(gpointer data);
  
  // Private Members
  View::Ptr view_;
  Model::Ptr model_;
   
  nux::Geometry workarea_;
  nux::BaseWindow* view_window_;
  nux::HLayout* main_layout_;
  
  bool visible_;
  nux::Color bg_color_;
  guint show_timer_;
  guint bg_update_handle_;
  
  Animator* fade_in_animator_;
  Animator* fade_out_animator_;
}; 

} // namespace shortcut
} // namespace unity

#endif //UNITYSHELL_SHORTCUTHINTCONTROLLER_H

