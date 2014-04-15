// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2014 Canonical Ltd
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
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

#ifndef UNITY_LOCKSCREEN_PANEL
#define UNITY_LOCKSCREEN_PANEL

#include <Nux/Nux.h>
#include <Nux/View.h>
#include <UnityCore/GLibDBusProxy.h>
#include <UnityCore/GLibSource.h>
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/Indicators.h>
#include <UnityCore/SessionManager.h>

namespace unity
{
namespace panel
{
class PanelIndicatorsView;
}

namespace lockscreen
{

class Panel : public nux::View
{
public:
  Panel(int monitor, indicator::Indicators::Ptr const&, session::Manager::Ptr const&);

  nux::Property<bool> active;
  nux::Property<int> monitor;

  bool WillHandleKeyEvent(unsigned int event_type, unsigned long keysym, unsigned long modifiers);

protected:
  void Draw(nux::GraphicsEngine& GfxContext, bool force_draw) override;
  bool InspectKeyEvent(unsigned int event_type, unsigned int keysym, const char*) override;

private:
  void ActivateFirst();
  void AddIndicator(indicator::Indicator::Ptr const&);
  void RemoveIndicator(indicator::Indicator::Ptr const&);
  void OnIndicatorViewUpdated();
  void OnEntryActivated(std::string const& panel, std::string const& entry_id, nux::Rect const& geo);
  void OnEntryShowMenu(std::string const& entry_id, unsigned xid, int x, int y, unsigned button);
  void OnEntryActivateRequest(std::string const& entry_id);

  void BuildTexture();
  std::string GetPanelName() const;

  indicator::Indicators::Ptr indicators_;
  panel::PanelIndicatorsView* indicators_view_;
  nux::ObjectPtr<nux::BaseTexture> bg_texture_;

  bool needs_geo_sync_;
  nux::Point tracked_pointer_pos_;
  glib::Source::UniquePtr track_menu_pointer_timeout_;

  unsigned int left_shift : 1;
  unsigned int right_shift : 1;
  unsigned int left_control : 1;
  unsigned int right_control : 1;
  unsigned int left_alt : 1;
  unsigned int right_alt : 1;
  unsigned int left_super : 1;
  unsigned int right_super : 1;

  class Accelerator
  {
  public:
    unsigned int keysym_;
    unsigned int keycode_;
    unsigned int modifiers_;
    bool active_;
    bool activated_;
    bool match_;

    explicit Accelerator(unsigned int keysym = 0, unsigned int keycode = 0, unsigned int modifiers = 0);

    void Reset();
  };

  Accelerator ParseAcceleratorString(std::string const& string) const;

  void ParseAccelerators();

  Accelerator activate_indicator_;
  Accelerator volume_mute_;
  Accelerator volume_down_;
  Accelerator volume_up_;
  Accelerator previous_source_;
  Accelerator next_source_;

  bool IsModifier(unsigned int keysym) const;
  unsigned int ToModifier(unsigned int keysym) const;

  void MaybeDisableAccelerator(bool is_press,
                               unsigned int keysym,
                               unsigned int modifiers,
                               Accelerator& accelerator) const;

  bool IsMatch(bool is_press,
               unsigned int keysym,
               unsigned int modifiers,
               Accelerator const& accelerator) const;

  void OnKeyDown(unsigned long event,
                 unsigned long keysym,
                 unsigned long state,
                 const char* text,
                 unsigned short repeat);

  void OnKeyUp(unsigned int keysym,
               unsigned long keycode,
               unsigned long state);

  /* This is just for telling an indicator to do something. */
  void ActivateIndicatorAction(std::string const& bus_name,
                               std::string const& object_path,
                               std::string const& action,
                               glib::Variant const& parameter = glib::Variant()) const;

  void ActivateKeyboardAction(std::string const& action, glib::Variant const& parameter = glib::Variant()) const;
  void ActivateSoundAction(std::string const& action, glib::Variant const& parameter = glib::Variant()) const;
};

} // lockscreen namespace
} // unity namespace

#endif // UNITY_LOCKSCREEN_PANEL
