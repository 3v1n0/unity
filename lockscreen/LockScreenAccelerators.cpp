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
 * Authored by: William Hua <william.hua@canonical.com>
 */

#include "LockScreenAccelerators.h"

#include <gtk/gtk.h>

namespace unity
{
namespace lockscreen
{

static unsigned int KeysymToModifier(unsigned int keysym)
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
  }

  return 0;
}

static unsigned int KeysymToPressedMask(unsigned int keysym)
{
  switch (keysym)
  {
  case GDK_KEY_Shift_L:
    return LeftShiftPressed;
  case GDK_KEY_Shift_R:
    return RightShiftPressed;
  case GDK_KEY_Control_L:
    return LeftControlPressed;
  case GDK_KEY_Control_R:
    return RightControlPressed;
  case GDK_KEY_Alt_L:
    return LeftAltPressed;
  case GDK_KEY_Alt_R:
    return RightAltPressed;
  case GDK_KEY_Super_L:
    return LeftSuperPressed;
  case GDK_KEY_Super_R:
    return RightSuperPressed;
  }

  return 0;
}

static unsigned int KeysymToMirrorKeysym(unsigned int keysym)
{
  switch (keysym)
  {
  case GDK_KEY_Shift_L:
    return GDK_KEY_Shift_R;
  case GDK_KEY_Shift_R:
    return GDK_KEY_Shift_L;
  case GDK_KEY_Control_L:
    return GDK_KEY_Control_R;
  case GDK_KEY_Control_R:
    return GDK_KEY_Control_L;
  case GDK_KEY_Alt_L:
    return GDK_KEY_Alt_R;
  case GDK_KEY_Alt_R:
    return GDK_KEY_Alt_L;
  case GDK_KEY_Super_L:
    return GDK_KEY_Super_R;
  case GDK_KEY_Super_R:
    return GDK_KEY_Super_L;
  }

  return 0;
}

Accelerator::Accelerator(unsigned int keysym,
                         unsigned int keycode,
                         unsigned int modifiers)
  : keysym_(keysym)
  , keycode_(keycode)
  , modifiers_(modifiers)
  , active_(true)
  , activated_(false)
{
}

Accelerator::Accelerator(std::string const& string)
  : keysym_(0)
  , keycode_(0)
  , modifiers_(0)
  , active_(true)
  , activated_(false)
{
  guint keysym;
  guint* keycodes;
  GdkModifierType modifiers;

  gtk_accelerator_parse_with_keycode(string.c_str(), &keysym, &keycodes, &modifiers);

  /* gtk_accelerator_parse_with_keycode() might fail if the key is not in the
   * default key map. gtk_accelerator_parse() might succeed in this case. */
  if (keysym == 0 && keycodes == NULL && modifiers == 0)
    gtk_accelerator_parse(string.c_str(), &keysym, &modifiers);

  keysym_ = keysym;

  if (keycodes != NULL)
  {
    keycode_ = keycodes[0];
    g_free(keycodes);
  }

  if (modifiers & GDK_SHIFT_MASK)
    modifiers_ |= nux::KEY_MODIFIER_SHIFT;
  if (modifiers & GDK_CONTROL_MASK)
    modifiers_ |= nux::KEY_MODIFIER_CTRL;
  if (modifiers & GDK_MOD1_MASK)
    modifiers_ |= nux::KEY_MODIFIER_ALT;
  if (modifiers & GDK_SUPER_MASK)
    modifiers_ |= nux::KEY_MODIFIER_SUPER;
}

bool Accelerator::operator==(Accelerator const& accelerator) const
{
  return keysym_    == accelerator.keysym_
      && keycode_   == accelerator.keycode_
      && modifiers_ == accelerator.modifiers_;
}

bool Accelerator::HandleKeyPress(unsigned int keysym,
                                 unsigned int modifiers,
                                 unsigned int press_state)
{
  auto is_modifier_only = keysym_ == 0 && keycode_ == 0 && modifiers_ != 0;
  auto is_modifier_keysym = KeysymToModifier(keysym_);
  auto modifier = KeysymToModifier(keysym);

  if (modifiers == 0)
  {
    /* We're pressing a key when no other key is pressed. We can reset this
     * accelerator back to its original enabled state. */
    active_ = true;
    activated_ = false;
  }

  if (!active_)
    return false;

  /* We need to cancel modifier-only accelerators in some cases. For example,
   * we should cancel a Ctrl+Alt accelerator if Ctrl+Alt+T is pressed. */
  if (is_modifier_only || is_modifier_keysym)
  {
    if (!modifier)
    {
      /* We pressed a non-modifier key for a modifier-only accelerator. */
      active_ = false;
      return false;
    }
    else if (keysym != keysym_ && (modifiers_ & modifier) == 0)
    {
      /* We pressed a modifier key that isn't the keysym and isn't one of the
       * modifiers. */
      active_ = false;
      return false;
    }
  }
  else if (!modifier)
  {
    /* We expect a non-modifier key to activate and one was pressed. */
    if (modifiers == modifiers_)
    {
      /* The modifiers match. Check if the keysyms match. */
      if (keysym == keysym_)
        goto activate;
      else
      {
        /* Otherwise, check if the keycodes match. Maybe the accelerator
         * specifies a particular key code, or the keyboard layout changed. For
         * example, if the accelerator is Ctrl+A and the user switches from a
         * QWERTY to an AZERTY layout, we should accept Ctrl+Q so the user can
         * cycle through the entire list of keyboard layouts. */

        GdkKeymapKey* key;
        gint keys;

        if (gdk_keymap_get_entries_for_keyval(gdk_keymap_get_default(), keysym, &key, &keys))
        {
          for (auto i = 0; i < keys; i++)
          {
            if (key[i].keycode == keycode_)
            {
              g_free(key);
              goto activate;
            }
          }

          g_free(key);
        }
      }
    }
  }

  return false;

activate:
  Activate.emit();
  activated_ = true;

  return true;
}

bool Accelerator::HandleKeyRelease(unsigned int keysym,
                                   unsigned int modifiers,
                                   unsigned int press_state)
{
  auto is_modifier_only = keysym_ == 0 && keycode_ == 0 && modifiers_ != 0;
  auto is_modifier_keysym = KeysymToModifier(keysym_);
  auto modifier = KeysymToModifier(keysym);

  /* Don't activate on key release if we were activated on a key press. */
  if (!active_ || activated_)
    return false;

  /* Check if the keysyms match. */
  if (keysym == keysym_)
  {
    if (KeysymToModifier(keysym) == 0)
    {
      /* We released a non-modifier key. */
      if (modifiers == modifiers_)
        goto activate;
    }
    else
    {
      /* We released a modifier key. */
      auto mirror_keysym = KeysymToMirrorKeysym(keysym);
      auto is_mirror_pressed = press_state & KeysymToPressedMask(mirror_keysym);

      /* Ctrl+Shift_R is different from Ctrl+Shift+Shift_R, so we must detect
       * if the mirror key was pressed or not. */
      if (is_mirror_pressed)
      {
        /* The mirrored modifier is pressed. */
        if (modifiers == modifiers_)
          goto activate;
      }
      else
      {
        /* The mirrored modifier wasn't pressed. Compare modifiers without it. */
        if ((modifiers & ~KeysymToModifier(mirror_keysym)) == modifiers_)
          goto activate;
      }
    }
  }

  if (is_modifier_only || is_modifier_keysym)
  {
    if (modifier)
    {
      /* We released a modifier key for a modifier-only accelerator. */

      if (is_modifier_only)
      {
        /* The accelerator has no keysym or keycode. */

        /* TODO: Normally we would goto activate here, but compiz is
         * intercepting this case and handling it. This is bad because now we
         * can't do anything here. Otherwise we'll do the same action twice. */
        if (modifiers == modifiers_)
          return false;
      }
      else
      {
        /* The accelerator has a modifier keysym. */
        auto is_keysym_pressed = press_state & KeysymToPressedMask(keysym_);

        if (is_keysym_pressed)
        {
          auto mirror_keysym = KeysymToMirrorKeysym(keysym_);
          auto is_mirror_pressed = press_state & KeysymToPressedMask(mirror_keysym);

          /* Ctrl+Shift_R is different from Ctrl+Shift+Shift_R, so we must detect
           * if the mirror key was pressed or not. */
          if (is_mirror_pressed)
          {
            /* The mirrored modifier is pressed. */
            if (modifiers == modifiers_)
              goto activate;
          }
          else
          {
            /* The mirrored modifier wasn't pressed. Compare modifiers without it. */
            if ((modifiers & ~KeysymToModifier(mirror_keysym)) == modifiers_)
              goto activate;
          }
        }
      }
    }
  }
  else if (keycode_ != 0 && modifiers == modifiers_)
  {
    /* Otherwise, check if the keycodes match. Maybe the accelerator
     * specifies a particular key code, or the keyboard layout changed. For
     * example, if the accelerator is Ctrl+A and the user switches from a
     * QWERTY to an AZERTY layout, we should accept Ctrl+Q so the user can
     * cycle through the entire list of keyboard layouts. */

    GdkKeymapKey* key;
    gint keys;

    if (gdk_keymap_get_entries_for_keyval(gdk_keymap_get_default(), keysym, &key, &keys))
    {
      for (auto i = 0; i < keys; i++)
      {
        if (key[i].keycode == keycode_)
        {
          g_free(key);
          goto activate;
        }
      }

      g_free(key);
    }
  }

  return false;

activate:
  Activate.emit();
  activated_ = false;

  return true;
}

Accelerators::Accelerators()
  : press_state_(0)
{
}

void Accelerators::Clear()
{
  accelerators_.clear();
}

void Accelerators::Add(Accelerator const& accelerator)
{
  accelerators_.push_back(accelerator);
}

void Accelerators::Remove(Accelerator const& accelerator)
{
  accelerators_.remove(accelerator);
}

bool Accelerators::HandleKeyPress(unsigned int keysym,
                                  unsigned int modifiers)
{
  modifiers &= nux::KEY_MODIFIER_SHIFT
             | nux::KEY_MODIFIER_CTRL
             | nux::KEY_MODIFIER_ALT
             | nux::KEY_MODIFIER_SUPER;

  switch (keysym)
  {
  case GDK_KEY_Shift_L:
    press_state_ |= LeftShiftPressed;
    break;
  case GDK_KEY_Shift_R:
    press_state_ |= RightShiftPressed;
    break;
  case GDK_KEY_Control_L:
    press_state_ |= LeftControlPressed;
    break;
  case GDK_KEY_Control_R:
    press_state_ |= RightControlPressed;
    break;
  case GDK_KEY_Alt_L:
    press_state_ |= LeftAltPressed;
    break;
  case GDK_KEY_Alt_R:
    press_state_ |= RightAltPressed;
    break;
  case GDK_KEY_Super_L:
    press_state_ |= LeftSuperPressed;
    break;
  case GDK_KEY_Super_R:
    press_state_ |= RightSuperPressed;
    break;
  }

  auto handled = false;

  for (auto& accelerator : accelerators_)
    handled = accelerator.HandleKeyPress(keysym, modifiers, press_state_) || handled;

  return handled;
}

bool Accelerators::HandleKeyRelease(unsigned int keysym,
                                    unsigned int modifiers)
{
  modifiers &= nux::KEY_MODIFIER_SHIFT
             | nux::KEY_MODIFIER_CTRL
             | nux::KEY_MODIFIER_ALT
             | nux::KEY_MODIFIER_SUPER;

  auto handled = false;

  for (auto& accelerator : accelerators_)
    handled = accelerator.HandleKeyRelease(keysym, modifiers, press_state_) || handled;

  switch (keysym)
  {
  case GDK_KEY_Shift_L:
    press_state_ &= ~LeftShiftPressed;
    break;
  case GDK_KEY_Shift_R:
    press_state_ &= ~RightShiftPressed;
    break;
  case GDK_KEY_Control_L:
    press_state_ &= ~LeftControlPressed;
    break;
  case GDK_KEY_Control_R:
    press_state_ &= ~RightControlPressed;
    break;
  case GDK_KEY_Alt_L:
    press_state_ &= ~LeftAltPressed;
    break;
  case GDK_KEY_Alt_R:
    press_state_ &= ~RightAltPressed;
    break;
  case GDK_KEY_Super_L:
    press_state_ &= ~LeftSuperPressed;
    break;
  case GDK_KEY_Super_R:
    press_state_ &= ~RightSuperPressed;
    break;
  }

  return handled;
}

} // lockscreen namespace
} // unity namespace
