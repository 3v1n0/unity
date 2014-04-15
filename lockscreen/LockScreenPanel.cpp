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
const std::string INDICATOR_METHOD_ACTIVATE      = "Activate";

const std::string INDICATOR_KEYBOARD_ACTION_SCROLL = "locked_scroll";
const std::string INDICATOR_SOUND_ACTION_SCROLL    = "scroll";
const std::string INDICATOR_SOUND_ACTION_MUTE      = "mute";

const unsigned int MODIFIERS = nux::KEY_MODIFIER_SHIFT |
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

Panel::Accelerator::Accelerator(unsigned int keysym, unsigned int keycode, unsigned int modifiers)
  : keysym_(keysym)
  , keycode_(keycode)
  , modifiers_(modifiers)
  , active_(true)
  , activated_(false)
  , match_(false)
{
}

void Panel::Accelerator::Reset()
{
  active_ = true;
  activated_ = false;
  match_ = false;
}

Panel::Accelerator Panel::ParseAcceleratorString(std::string const& string) const
{
  Accelerator accelerator;

  guint gtk_keysym;
  guint* gtk_keycodes;
  GdkModifierType gtk_modifiers;
  gtk_accelerator_parse_with_keycode(string.c_str(), &gtk_keysym, &gtk_keycodes, &gtk_modifiers);

  /* gtk_accelerator_parse_with_keycode() might fail if the key isn't in the default key map.
   * In that case, try it again without looking for keycodes. */
  if (gtk_keysym == 0 && gtk_modifiers == 0)
  {
    g_free(gtk_keycodes);
    gtk_keycodes = NULL;
    gtk_accelerator_parse(string.c_str(), &gtk_keysym, &gtk_modifiers);
  }

  accelerator.keysym_ = gtk_keysym;

  if (gtk_keycodes != NULL)
    accelerator.keycode_ = gtk_keycodes[0];

  g_free(gtk_keycodes);

  if (gtk_modifiers & GDK_SHIFT_MASK)
    accelerator.modifiers_ |= nux::KEY_MODIFIER_SHIFT;
  if (gtk_modifiers & GDK_CONTROL_MASK)
    accelerator.modifiers_ |= nux::KEY_MODIFIER_CTRL;
  if (gtk_modifiers & GDK_MOD1_MASK)
    accelerator.modifiers_ |= nux::KEY_MODIFIER_ALT;
  if (gtk_modifiers & GDK_SUPER_MASK)
    accelerator.modifiers_ |= nux::KEY_MODIFIER_SUPER;

  return accelerator;
}

void Panel::ParseAccelerators()
{
  auto media_key_settings = glib::Object<GSettings>(g_settings_new(MEDIA_KEYS_SCHEMA.c_str()));
  auto input_switch_settings = glib::Object<GSettings>(g_settings_new(INPUT_SWITCH_SCHEMA.c_str()));
  auto activate_indicators_key = WindowManager::Default().activate_indicators_key();

  activate_indicator_ = Accelerator(activate_indicators_key.second, 0, activate_indicators_key.first);
  volume_mute_ = ParseAcceleratorString(glib::String(g_settings_get_string(media_key_settings, MEDIA_KEYS_VOLUME_MUTE.c_str())));
  volume_down_ = ParseAcceleratorString(glib::String(g_settings_get_string(media_key_settings, MEDIA_KEYS_VOLUME_DOWN.c_str())));
  volume_up_ = ParseAcceleratorString(glib::String(g_settings_get_string(media_key_settings, MEDIA_KEYS_VOLUME_UP.c_str())));

  auto variant = glib::Variant(g_settings_get_value(input_switch_settings, INPUT_SWITCH_PREVIOUS.c_str()), glib::StealRef());

  if (g_variant_n_children(variant) > 0)
  {
    const gchar *accelerator;
    g_variant_get_child(variant, 0, "&s", &accelerator);
    previous_source_ = ParseAcceleratorString(accelerator);
  }
  else
    previous_source_ = Accelerator();

  variant = glib::Variant(g_settings_get_value(input_switch_settings, INPUT_SWITCH_NEXT.c_str()), glib::StealRef());

  if (g_variant_n_children(variant) > 0)
  {
    const gchar *accelerator;
    g_variant_get_child(variant, 0, "&s", &accelerator);
    next_source_ = ParseAcceleratorString(accelerator);
  }
  else
    next_source_ = Accelerator();
}

bool Panel::WillHandleKeyEvent(unsigned int event_type, unsigned long keysym, unsigned long modifiers)
{
  auto is_press = event_type == nux::EVENT_KEY_DOWN;

  /* Update modifier states on key press. */
  if (is_press)
  {
    switch (keysym)
    {
    case GDK_KEY_Shift_L:
      left_shift = is_press;
      break;
    case GDK_KEY_Shift_R:
      right_shift = is_press;
      break;
    case GDK_KEY_Control_L:
      left_control = is_press;
      break;
    case GDK_KEY_Control_R:
      right_control = is_press;
      break;
    case GDK_KEY_Alt_L:
      left_alt = is_press;
      break;
    case GDK_KEY_Alt_R:
      right_alt = is_press;
      break;
    case GDK_KEY_Super_L:
      left_super = is_press;
      break;
    case GDK_KEY_Super_R:
      right_super = is_press;
      break;
    }
  }

  /* If we're just pressing a key and no modifiers are pressed,
   * then we can start accepting new actions again. */
  if (is_press && (modifiers & MODIFIERS) == 0)
  {
    activate_indicator_.Reset();
    volume_mute_.Reset();
    volume_down_.Reset();
    volume_up_.Reset();
    previous_source_.Reset();
    next_source_.Reset();
  }

  /* We may have to disable the accelerator if this press invalidates it.
   * An example is pressing Ctrl+Alt+T which should cancel a Ctrl+Alt
   * accelerator. */
  MaybeDisableAccelerator(is_press, keysym, modifiers, activate_indicator_);
  MaybeDisableAccelerator(is_press, keysym, modifiers, volume_mute_);
  MaybeDisableAccelerator(is_press, keysym, modifiers, volume_down_);
  MaybeDisableAccelerator(is_press, keysym, modifiers, volume_up_);
  MaybeDisableAccelerator(is_press, keysym, modifiers, previous_source_);
  MaybeDisableAccelerator(is_press, keysym, modifiers, next_source_);

  /* We store the match here because IsMatch() is only valid here,
   * and not in the OnKeyDown()/OnKeyUp() functions. */
  activate_indicator_.match_ = IsMatch(is_press, keysym, modifiers, activate_indicator_);
  volume_mute_.match_ = IsMatch(is_press, keysym, modifiers, volume_mute_);
  volume_down_.match_ = IsMatch(is_press, keysym, modifiers, volume_down_);
  volume_up_.match_ = IsMatch(is_press, keysym, modifiers, volume_up_);
  previous_source_.match_ = IsMatch(is_press, keysym, modifiers, previous_source_);
  next_source_.match_ = IsMatch(is_press, keysym, modifiers, next_source_);

  /* Update modifier states on key release. */
  if (!is_press)
  {
    switch (keysym)
    {
    case GDK_KEY_Shift_L:
      left_shift = is_press;
      break;
    case GDK_KEY_Shift_R:
      right_shift = is_press;
      break;
    case GDK_KEY_Control_L:
      left_control = is_press;
      break;
    case GDK_KEY_Control_R:
      right_control = is_press;
      break;
    case GDK_KEY_Alt_L:
      left_alt = is_press;
      break;
    case GDK_KEY_Alt_R:
      right_alt = is_press;
      break;
    case GDK_KEY_Super_L:
      left_super = is_press;
      break;
    case GDK_KEY_Super_R:
      right_super = is_press;
      break;
    }
  }

  return activate_indicator_.match_ ||
         volume_mute_.match_        ||
         volume_down_.match_        ||
         volume_up_.match_          ||
         previous_source_.match_    ||
         next_source_.match_;
}

bool Panel::InspectKeyEvent(unsigned int event_type, unsigned int keysym, const char*)
{
  return true;
}

bool Panel::IsModifier(unsigned int keysym) const
{
  return ToModifier(keysym);
}

unsigned int Panel::ToModifier(unsigned int keysym) const
{
  switch (keysym)
  {
  case GDK_KEY_Shift_L:
  case GDK_KEY_Shift_R:
    return nux::KEY_MODIFIER_SHIFT;
  case GDK_KEY_Control_L:
  case GDK_KEY_Control_R:
    return nux::KEY_MODIFIER_CTRL;
  case GDK_KEY_Alt_L:
  case GDK_KEY_Alt_R:
    return nux::KEY_MODIFIER_ALT;
  case GDK_KEY_Super_L:
  case GDK_KEY_Super_R:
    return nux::KEY_MODIFIER_SUPER;
  default:
    return 0;
  }
}

void Panel::MaybeDisableAccelerator(bool is_press,
                                    unsigned int keysym,
                                    unsigned int state,
                                    Accelerator& accelerator) const
{
  auto is_modifier_only = accelerator.keysym_ == 0 && accelerator.keycode_ == 0 && accelerator.modifiers_ != 0;
  auto keysym_modifier = ToModifier(accelerator.keysym_);

  if (is_modifier_only || keysym_modifier)
  {
    if (is_press)
    {
      /* We may have to disable the accelerator if this press invalidates it.
       * An example is pressing Ctrl+Alt+T for a Ctrl+Alt accelerator. */

      if (!IsModifier(keysym))
      {
        /* We pressed a non-modifier key: disable the accelerator. */
        accelerator.active_ = false;
      }
      else if (keysym != accelerator.keysym_ && (ToModifier(keysym) & accelerator.modifiers_) == 0)
      {
        /* We pressed a modifier key that isn't the keysym or one of the modifiers: disable the accelerator. */
        accelerator.active_ = false;
      }
    }
  }
}

/* This function is only valid in WillHandleKeyEvent because
 * that's the only place the modifier key state is valid. */

bool Panel::IsMatch(bool is_press,
                    unsigned int keysym,
                    unsigned int state,
                    Accelerator const& accelerator) const
{
  state &= MODIFIERS;

  /* Inactive accelerators never match. */
  if (!accelerator.active_)
    return false;

  /* Do the easiest check and compare keysyms.
   * But we must be careful with modifier-only accelerators. */
  if (keysym == accelerator.keysym_)
  {
    if (!IsModifier(keysym))
    {
      /* A non-modifier key was pressed/released, so just compare modifiers. */
      if (state == accelerator.modifiers_)
        return true;
    }
    else if (!is_press)
    {
      /* A modifier key was released, so compare modifiers ignoring that key. */

      auto is_mirror_pressed = false;

      switch (keysym)
      {
        case GDK_KEY_Shift_L:
          is_mirror_pressed = right_shift;
          break;
        case GDK_KEY_Shift_R:
          is_mirror_pressed = left_shift;
          break;
        case GDK_KEY_Control_L:
          is_mirror_pressed = right_control;
          break;
        case GDK_KEY_Control_R:
          is_mirror_pressed = left_control;
          break;
        case GDK_KEY_Alt_L:
          is_mirror_pressed = right_alt;
          break;
        case GDK_KEY_Alt_R:
          is_mirror_pressed = left_alt;
          break;
        case GDK_KEY_Super_L:
          is_mirror_pressed = right_super;
          break;
        case GDK_KEY_Super_R:
          is_mirror_pressed = left_super;
          break;
      }

      if (is_mirror_pressed)
      {
        /* We can just compare the state directly. */
        if (state == accelerator.modifiers_)
          return true;
      }
      else
      {
        /* We must pretend the state doesn't include keysym's modifier. */
        if ((state & ~ToModifier(keysym)) == accelerator.modifiers_)
          return true;
      }
    }
  }

  auto is_modifier_only = accelerator.keysym_ == 0 && accelerator.keycode_ == 0 && accelerator.modifiers_ != 0;
  auto keysym_modifier = ToModifier(accelerator.keysym_);

  if (is_modifier_only || keysym_modifier)
  {
    /* The accelerator consists of only modifier keys. */

    if (!is_press && IsModifier(keysym))
    {
      /* We're releasing a modifier key. */

      if (is_modifier_only)
      {
        /* Just ensure the states match in this case. */
        if (state == accelerator.modifiers_)
        {
          /* TODO: We would normally return true here, but for some reason
           * compiz handles it. This is bad because it means we have to do
           * nothing in this case where we would normally handle it.... */
          return false;
        }
      }
      else if (keysym_modifier)
      {
        auto is_keysym_pressed = false;
        auto is_mirror_pressed = false;

        /* Check that accelerator.keysym_ is pressed. */
        switch (accelerator.keysym_)
        {
        case GDK_KEY_Shift_L:
          is_keysym_pressed = left_shift;
          is_mirror_pressed = right_shift;
          break;
        case GDK_KEY_Shift_R:
          is_keysym_pressed = right_shift;
          is_mirror_pressed = left_shift;
          break;
        case GDK_KEY_Control_L:
          is_keysym_pressed = left_control;
          is_mirror_pressed = right_control;
          break;
        case GDK_KEY_Control_R:
          is_keysym_pressed = right_control;
          is_mirror_pressed = left_control;
          break;
        case GDK_KEY_Alt_L:
          is_keysym_pressed = left_alt;
          is_mirror_pressed = right_alt;
          break;
        case GDK_KEY_Alt_R:
          is_keysym_pressed = right_alt;
          is_mirror_pressed = left_alt;
          break;
        case GDK_KEY_Super_L:
          is_keysym_pressed = left_super;
          is_mirror_pressed = right_super;
          break;
        case GDK_KEY_Super_R:
          is_keysym_pressed = right_super;
          is_mirror_pressed = left_super;
          break;
        }

        if (is_keysym_pressed)
        {
          /* If the mirror key is not pressed, clear it from the state. */
          if (!is_mirror_pressed)
            state &= ~ToModifier(accelerator.keysym_);

          /* Check that the states are matching. */
          if (state == accelerator.modifiers_)
            return true;
        }
      }
    }
  }
  else if (accelerator.keycode_ != 0 && state == accelerator.modifiers_)
  {
    /* The keysyms might be different, but the keycodes might be the same.
     * For example, if the switching shortcut is Ctrl+A, we want Ctrl+Q to
     * match on an AZERTY layout. Otherwise, you can't cycle through a full
     * list of keyboard layouts with one accelerator. Or the accelerator may
     * not even have a keysym at all. Either way, check for both these cases.
     * Note that we don't want to check this for the modifier-only case, as
     * it's already handled specially above. */

    gint n_keys;
    GdkKeymapKey *keys;

    if (gdk_keymap_get_entries_for_keyval(gdk_keymap_get_default(), keysym, &keys, &n_keys))
    {
      auto is_match = false;

      for (auto i = 0; i < n_keys && !is_match; i++)
        is_match = keys[i].keycode == accelerator.keycode_;

      g_free(keys);

      if (is_match)
        return true;
    }
  }

  return false;
}

void Panel::OnKeyDown(unsigned long event,
                      unsigned long keysym,
                      unsigned long state,
                      const char* text,
                      unsigned short repeat)
{
  if (activate_indicator_.match_)
  {
    ActivateFirst();
    activate_indicator_.activated_ = true;
  }
  else if (volume_mute_.match_)
  {
    ActivateSoundAction(INDICATOR_SOUND_ACTION_MUTE);
    volume_mute_.activated_ = true;
  }
  else if (volume_down_.match_)
  {
    ActivateSoundAction(INDICATOR_SOUND_ACTION_SCROLL, g_variant_new_int32(-1));
    volume_down_.activated_ = true;
  }
  else if (volume_up_.match_)
  {
    ActivateSoundAction(INDICATOR_SOUND_ACTION_SCROLL, g_variant_new_int32(+1));
    volume_up_.activated_ = true;
  }
  else if (previous_source_.match_)
  {
    ActivateKeyboardAction(INDICATOR_KEYBOARD_ACTION_SCROLL, g_variant_new_int32(+1));
    previous_source_.activated_ = true;
  }
  else if (next_source_.match_)
  {
    ActivateKeyboardAction(INDICATOR_KEYBOARD_ACTION_SCROLL, g_variant_new_int32(-1));
    next_source_.activated_ = true;
  }
}

void Panel::OnKeyUp(unsigned int keysym,
                    unsigned long keycode,
                    unsigned long state)
{
  /* We only want to act if we didn't activate the action on key
   * down. Once we see the key up, we can start accepting actions
   * again. */

  if (activate_indicator_.match_)
  {
    if (!activate_indicator_.activated_)
      ActivateFirst();

    activate_indicator_.Reset();
  }
  else if (volume_mute_.match_)
  {
    if (!volume_mute_.activated_)
      ActivateSoundAction(INDICATOR_SOUND_ACTION_MUTE);

    volume_mute_.Reset();
  }
  else if (volume_down_.match_)
  {
    if (!volume_down_.activated_)
      ActivateSoundAction(INDICATOR_SOUND_ACTION_SCROLL, g_variant_new_int32(-1));

    volume_down_.Reset();
  }
  else if (volume_up_.match_)
  {
    if (!volume_up_.activated_)
      ActivateSoundAction(INDICATOR_SOUND_ACTION_SCROLL, g_variant_new_int32(+1));

    volume_up_.Reset();
  }
  else if (previous_source_.match_)
  {
    if (!previous_source_.activated_)
      ActivateKeyboardAction(INDICATOR_KEYBOARD_ACTION_SCROLL, g_variant_new_int32(+1));

    previous_source_.Reset();
  }
  else if (next_source_.match_)
  {
    if (!next_source_.activated_)
      ActivateKeyboardAction(INDICATOR_KEYBOARD_ACTION_SCROLL, g_variant_new_int32(-1));

    next_source_.Reset();
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
  proxy->CallBegin(INDICATOR_METHOD_ACTIVATE, g_variant_builder_end(&builder), [proxy] (GVariant*, glib::Error const&) {});
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
