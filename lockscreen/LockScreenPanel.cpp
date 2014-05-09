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

#include "LockScreenPanel.h"

#include <boost/algorithm/string/trim.hpp>
#include <Nux/HLayout.h>
#include <UnityCore/Variant.h>

#include "LockScreenSettings.h"
#include "panel/PanelIndicatorsView.h"
#include "unity-shared/CairoTexture.h"
#include "unity-shared/StaticCairoText.h"
#include "unity-shared/PanelStyle.h"
#include "unity-shared/RawPixel.h"
#include "unity-shared/UnitySettings.h"
#include "unity-shared/WindowManager.h"

namespace unity
{
namespace lockscreen
{
namespace
{
const RawPixel PADDING = 5_em;

const std::string MEDIA_KEYS_SCHEMA      = "org.gnome.settings-daemon.plugins.media-keys";
const std::string MEDIA_KEYS_VOLUME_MUTE = "volume-mute";
const std::string MEDIA_KEYS_VOLUME_DOWN = "volume-down";
const std::string MEDIA_KEYS_VOLUME_UP   = "volume-up";
const std::string INPUT_SWITCH_SCHEMA    = "org.gnome.desktop.wm.keybindings";
const std::string INPUT_SWITCH_PREVIOUS  = "switch-input-source-backward";
const std::string INPUT_SWITCH_NEXT      = "switch-input-source";

const std::string INDICATOR_KEYBOARD_BUS_NAME    = "com.canonical.indicator.keyboard";
const std::string INDICATOR_KEYBOARD_OBJECT_PATH = "/com/canonical/indicator/keyboard";
const std::string INDICATOR_SOUND_BUS_NAME       = "com.canonical.indicator.sound";
const std::string INDICATOR_SOUND_OBJECT_PATH    = "/com/canonical/indicator/sound";
const std::string INDICATOR_ACTION_INTERFACE     = "org.gtk.Actions";

const std::string INDICATOR_KEYBOARD_ACTION_SCROLL = "locked_scroll";
const std::string INDICATOR_SOUND_ACTION_SCROLL    = "scroll";
const std::string INDICATOR_SOUND_ACTION_MUTE      = "mute";

const unsigned int MODIFIERS = nux::KEY_MODIFIER_SHIFT |
                               nux::KEY_MODIFIER_CAPS_LOCK |
                               nux::KEY_MODIFIER_CTRL |
                               nux::KEY_MODIFIER_ALT |
                               nux::KEY_MODIFIER_SUPER;
}

using namespace indicator;
using namespace panel;

Panel::Panel(int monitor_, Indicators::Ptr const& indicators, session::Manager::Ptr const& session_manager)
  : nux::View(NUX_TRACKER_LOCATION)
  , active(false)
  , monitor(monitor_)
  , indicators_(indicators)
  , needs_geo_sync_(true)
  , media_key_settings_(g_settings_new(MEDIA_KEYS_SCHEMA.c_str()))
  , input_switch_settings_(g_settings_new(INPUT_SWITCH_SCHEMA.c_str()))
{
  double scale = unity::Settings::Instance().em(monitor)->DPIScale();
  auto* layout = new nux::HLayout();
  layout->SetLeftAndRightPadding(PADDING.CP(scale), 0);
  SetLayout(layout);

  BuildTexture();

  // Add setting
  auto *hostname = new StaticCairoText(session_manager->HostName());
  hostname->SetFont(Settings::Instance().font_name());
  hostname->SetTextColor(nux::color::White);
  hostname->SetInputEventSensitivity(false);
  hostname->SetScale(scale);
  hostname->SetVisible(Settings::Instance().show_hostname());
  layout->AddView(hostname, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);

  indicators_view_ = new PanelIndicatorsView();
  indicators_view_->SetMonitor(monitor);
  indicators_view_->OverlayShown();
  indicators_view_->on_indicator_updated.connect(sigc::mem_fun(this, &Panel::OnIndicatorViewUpdated));
  layout->AddView(indicators_view_, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);

  for (auto const& indicator : indicators_->GetIndicators())
    AddIndicator(indicator);

  indicators_->on_object_added.connect(sigc::mem_fun(this, &Panel::AddIndicator));
  indicators_->on_object_removed.connect(sigc::mem_fun(this, &Panel::RemoveIndicator));
  indicators_->on_entry_show_menu.connect(sigc::mem_fun(this, &Panel::OnEntryShowMenu));
  indicators_->on_entry_activated.connect(sigc::mem_fun(this, &Panel::OnEntryActivated));
  indicators_->on_entry_activate_request.connect(sigc::mem_fun(this, &Panel::OnEntryActivateRequest));

  monitor.changed.connect([this, hostname] (int monitor) {
    hostname->SetScale(unity::Settings::Instance().em(monitor)->DPIScale());
    indicators_view_->SetMonitor(monitor);
    BuildTexture();
    QueueRelayout();
  });

  ParseAccelerators();

  key_down.connect(sigc::mem_fun(this, &Panel::OnKeyDown));
  key_up.connect(sigc::mem_fun(this, &Panel::OnKeyUp));
}

void Panel::BuildTexture()
{
  int height = panel::Style::Instance().PanelHeight(monitor);
  nux::CairoGraphics context(CAIRO_FORMAT_ARGB32, 1, height);
  auto* cr = context.GetInternalContext();
  cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
  cairo_paint_with_alpha(cr, 0.4);
  bg_texture_ = texture_ptr_from_cairo_graphics(context);

  view_layout_->SetMinimumHeight(height);
  view_layout_->SetMaximumHeight(height);
}

void Panel::AddIndicator(Indicator::Ptr const& indicator)
{
  if (indicator->IsAppmenu())
    return;

  indicators_view_->AddIndicator(indicator);
  QueueRelayout();
  QueueDraw();
}

void Panel::RemoveIndicator(indicator::Indicator::Ptr const& indicator)
{
  indicators_view_->RemoveIndicator(indicator);
  QueueRelayout();
  QueueDraw();
}

std::string Panel::GetPanelName() const
{
  return "LockScreenPanel" + std::to_string(monitor);
}

void Panel::OnIndicatorViewUpdated()
{
  needs_geo_sync_ = true;
  QueueRelayout();
  QueueDraw();
}

void Panel::OnEntryShowMenu(std::string const& entry_id, unsigned xid, int x, int y, unsigned button)
{
  if (!GetInputEventSensitivity())
    return;

  // This is ugly... But Nux fault!
  WindowManager::Default().UnGrabMousePointer(CurrentTime, button, x, y);

  active = true;
}

void Panel::OnEntryActivateRequest(std::string const& entry_id)
{
  if (GetInputEventSensitivity())
    indicators_view_->ActivateEntry(entry_id, 0);
}

void Panel::ActivateFirst()
{
  if (GetInputEventSensitivity())
    indicators_view_->ActivateIfSensitive();
}

void Panel::OnEntryActivated(std::string const& panel, std::string const& entry_id, nux::Rect const&)
{
  if (!GetInputEventSensitivity() || (!panel.empty() && panel != GetPanelName()))
    return;

  bool active = !entry_id.empty();

  if (active && !WindowManager::Default().IsScreenGrabbed())
  {
    // The menu didn't grab the keyboard, let's take it back.
    nux::GetWindowCompositor().GrabKeyboardAdd(static_cast<nux::BaseWindow*>(GetTopLevelViewWindow()));
  }

  if (active && !track_menu_pointer_timeout_)
  {
    track_menu_pointer_timeout_.reset(new glib::Timeout(16));
    track_menu_pointer_timeout_->Run([this] {
      nux::Point const& mouse = nux::GetGraphicsDisplay()->GetMouseScreenCoord();
      if (tracked_pointer_pos_ != mouse)
      {
        if (GetAbsoluteGeometry().IsPointInside(mouse.x, mouse.y))
          indicators_view_->ActivateEntryAt(mouse.x, mouse.y);

        tracked_pointer_pos_ = mouse;
      }

      return true;
    });
  }
  else if (!active)
  {
    track_menu_pointer_timeout_.reset();
    tracked_pointer_pos_ = {-1, -1};
    this->active = false;
  }
}

void Panel::Draw(nux::GraphicsEngine& graphics_engine, bool force_draw)
{
  auto const& geo = GetGeometry();

  unsigned int alpha, src, dest = 0;
  graphics_engine.GetRenderStates().GetBlend(alpha, src, dest);
  graphics_engine.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  graphics_engine.PushClippingRectangle(geo);
  nux::GetPainter().PaintBackground(graphics_engine, geo);

  nux::TexCoordXForm texxform;
  texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_CLAMP);
  graphics_engine.QRP_1Tex(geo.x, geo.y, geo.width, geo.height,
                           bg_texture_->GetDeviceTexture(), texxform,
                           nux::color::White);

  view_layout_->ProcessDraw(graphics_engine, force_draw);

  graphics_engine.PopClippingRectangle();
  graphics_engine.GetRenderStates().SetBlend(alpha, src, dest);

  if (needs_geo_sync_)
  {
    EntryLocationMap locations;
    indicators_view_->GetGeometryForSync(locations);
    indicators_->SyncGeometries(GetPanelName(), locations);
    needs_geo_sync_ = false;
  }
}

Panel::Accelerator Panel::ParseAcceleratorString(std::string const& string) const
{
  guint gtk_key;
  GdkModifierType gtk_modifiers;
  gtk_accelerator_parse(string.c_str(), &gtk_key, &gtk_modifiers);

  unsigned int nux_key = gtk_key;
  unsigned int nux_modifiers = 0;

  if (gtk_modifiers & GDK_SHIFT_MASK)
    nux_modifiers |= nux::KEY_MODIFIER_SHIFT;
  if (gtk_modifiers & GDK_LOCK_MASK)
    nux_modifiers |= nux::KEY_MODIFIER_CAPS_LOCK;
  if (gtk_modifiers & GDK_CONTROL_MASK)
    nux_modifiers |= nux::KEY_MODIFIER_CTRL;
  if (gtk_modifiers & GDK_MOD1_MASK)
    nux_modifiers |= nux::KEY_MODIFIER_ALT;
  if (gtk_modifiers & GDK_SUPER_MASK)
    nux_modifiers |= nux::KEY_MODIFIER_SUPER;

  return std::make_pair(nux_modifiers, nux_key);
}

void Panel::ParseAccelerators()
{
  activate_indicator_ = WindowManager::Default().activate_indicators_key();
  volume_mute_ = ParseAcceleratorString(glib::String(g_settings_get_string(media_key_settings_, MEDIA_KEYS_VOLUME_MUTE.c_str())));
  volume_down_ = ParseAcceleratorString(glib::String(g_settings_get_string(media_key_settings_, MEDIA_KEYS_VOLUME_DOWN.c_str())));
  volume_up_ = ParseAcceleratorString(glib::String(g_settings_get_string(media_key_settings_, MEDIA_KEYS_VOLUME_UP.c_str())));

  auto variant = glib::Variant(g_settings_get_value(input_switch_settings_, INPUT_SWITCH_PREVIOUS.c_str()), glib::StealRef());

  if (g_variant_n_children(variant) > 0)
  {
    const gchar *accelerator;
    g_variant_get_child(variant, 0, "&s", &accelerator);
    previous_source_ = ParseAcceleratorString(accelerator);
  }
  else
    previous_source_ = std::make_pair(0, 0);

  variant = glib::Variant(g_settings_get_value(input_switch_settings_, INPUT_SWITCH_NEXT.c_str()), glib::StealRef());

  if (g_variant_n_children(variant) > 0)
  {
    const gchar *accelerator;
    g_variant_get_child(variant, 0, "&s", &accelerator);
    next_source_ = ParseAcceleratorString(accelerator);
  }
  else
    next_source_ = std::make_pair(0, 0);
}

bool Panel::WillHandleKeyEvent(unsigned int event_type, unsigned long key_sym, unsigned long modifiers)
{
  auto is_press = event_type == nux::EVENT_KEY_DOWN;

  /* If we're just pressing a key, and no modifiers are pressed, then
   * we can start accepting new actions again. */
  if (is_press && (modifiers & MODIFIERS) == 0)
    last_action_ = std::make_pair(0, 0);

  return IsMatch(is_press, key_sym, modifiers, activate_indicator_) ||
         IsMatch(is_press, key_sym, modifiers, volume_mute_) ||
         IsMatch(is_press, key_sym, modifiers, volume_down_) ||
         IsMatch(is_press, key_sym, modifiers, volume_up_) ||
         IsMatch(is_press, key_sym, modifiers, previous_source_) ||
         IsMatch(is_press, key_sym, modifiers, next_source_);
}

bool Panel::InspectKeyEvent(unsigned int event_type, unsigned int keysym, const char*)
{
  return true;
}

bool Panel::IsMatch(bool is_press,
                    unsigned int key_sym,
                    unsigned int state,
                    Accelerator const& accelerator) const
{
  /* Do the easy check, just compare key codes and modifiers.
   * TODO: Check permutations of modifier-only shortcuts. */
  return key_sym == accelerator.second && (state & MODIFIERS) == accelerator.first;
}

void Panel::OnKeyDown(unsigned long event,
                      unsigned long key_sym,
                      unsigned long state,
                      const char* text,
                      unsigned short repeat)
{
  if (IsMatch(true, key_sym, state, activate_indicator_))
  {
    ActivateFirst();
    last_action_ = activate_indicator_;
  }
  else if (IsMatch(true, key_sym, state, volume_mute_))
  {
    ActivateSoundAction(INDICATOR_SOUND_ACTION_MUTE);
    last_action_ = volume_mute_;
  }
  else if (IsMatch(true, key_sym, state, volume_down_))
  {
    ActivateSoundAction(INDICATOR_SOUND_ACTION_SCROLL, g_variant_new_int32(-1));
    last_action_ = volume_down_;
  }
  else if (IsMatch(true, key_sym, state, volume_up_))
  {
    ActivateSoundAction(INDICATOR_SOUND_ACTION_SCROLL, g_variant_new_int32(+1));
    last_action_ = volume_up_;
  }
  else if (IsMatch(true, key_sym, state, previous_source_))
  {
    ActivateKeyboardAction(INDICATOR_KEYBOARD_ACTION_SCROLL, g_variant_new_int32(-1));
    last_action_ = previous_source_;
  }
  else if (IsMatch(true, key_sym, state, next_source_))
  {
    ActivateKeyboardAction(INDICATOR_KEYBOARD_ACTION_SCROLL, g_variant_new_int32(+1));
    last_action_ = next_source_;
  }
}

void Panel::OnKeyUp(unsigned int key_sym,
                    unsigned long key_code,
                    unsigned long state)
{
  /* We only want to act if we didn't activate the action on key
   * down. Once we see the key up, we can start accepting actions
   * again. */

  if (IsMatch(false, key_sym, state, activate_indicator_))
  {
    if (last_action_ != activate_indicator_)
      ActivateFirst();

    last_action_ = std::make_pair(0, 0);
  }
  else if (IsMatch(false, key_sym, state, volume_mute_))
  {
    if (last_action_ != volume_mute_)
      ActivateSoundAction(INDICATOR_SOUND_ACTION_MUTE);

    last_action_ = std::make_pair(0, 0);
  }
  else if (IsMatch(false, key_sym, state, volume_down_))
  {
    if (last_action_ != volume_down_)
      ActivateSoundAction(INDICATOR_SOUND_ACTION_SCROLL, g_variant_new_int32(-1));

    last_action_ = std::make_pair(0, 0);
  }
  else if (IsMatch(false, key_sym, state, volume_up_))
  {
    if (last_action_ != volume_up_)
      ActivateSoundAction(INDICATOR_SOUND_ACTION_SCROLL, g_variant_new_int32(+1));

    last_action_ = std::make_pair(0, 0);
  }
  else if (IsMatch(false, key_sym, state, previous_source_))
  {
    if (last_action_ != previous_source_)
      ActivateKeyboardAction(INDICATOR_KEYBOARD_ACTION_SCROLL, g_variant_new_int32(-1));

    last_action_ = std::make_pair(0, 0);
  }
  else if (IsMatch(false, key_sym, state, next_source_))
  {
    if (last_action_ != next_source_)
      ActivateKeyboardAction(INDICATOR_KEYBOARD_ACTION_SCROLL, g_variant_new_int32(+1));

    last_action_ = std::make_pair(0, 0);
  }
}

void Panel::ActivateIndicatorAction(std::string const& bus_name,
                                    std::string const& object_path,
                                    std::string const& action,
                                    glib::Variant const& parameter) const
{
  GVariantBuilder builder;

  g_variant_builder_init(&builder, G_VARIANT_TYPE("(sava{sv})"));
  g_variant_builder_add(&builder, "s", action.c_str());

  if (parameter)
    g_variant_builder_add_parsed(&builder, "[%v]", (GVariant*) parameter);
  else
    g_variant_builder_add_parsed(&builder, "@av []");

  g_variant_builder_add_parsed(&builder, "@a{sv} []");

  auto proxy = std::make_shared<glib::DBusProxy>(bus_name, object_path, INDICATOR_ACTION_INTERFACE, G_BUS_TYPE_SESSION);
  proxy->CallBegin("Activate", g_variant_builder_end(&builder), [proxy] (GVariant*, glib::Error const&) {});
}

void Panel::ActivateKeyboardAction(std::string const& action, glib::Variant const& parameter) const
{
  ActivateIndicatorAction(INDICATOR_KEYBOARD_BUS_NAME, INDICATOR_KEYBOARD_OBJECT_PATH, action, parameter);
}

void Panel::ActivateSoundAction(std::string const& action, glib::Variant const& parameter) const
{
  ActivateIndicatorAction(INDICATOR_SOUND_BUS_NAME, INDICATOR_SOUND_OBJECT_PATH, action, parameter);
}

}
}
