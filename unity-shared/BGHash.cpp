/*
 * Copyright (C) 2011-2012 Canonical Ltd
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
 * Authored by: Gordon Allott <gord.alott@canonical.com>
 */


#include "BGHash.h"
#include <gdk/gdkx.h>
#include <NuxCore/Logger.h>
#include "unity-shared/WindowManager.h"

#ifndef XA_STRING
#define XA_STRING ((Atom) 31)
#endif

DECLARE_LOGGER(logger, "unity.bghash");

namespace unity
{

namespace
{
  Atom COLORS_ATOM = 0;
  const unsigned TRANSITION_DURATION = 500;
}

BGHash::BGHash()
  : transition_animator_(TRANSITION_DURATION)
  , override_color_(nux::color::Transparent)
{
  COLORS_ATOM = gdk_x11_get_xatom_by_name("_GNOME_BACKGROUND_REPRESENTATIVE_COLORS");
  transition_animator_.updated.connect(sigc::mem_fun(this, &BGHash::OnTransitionUpdated));
  WindowManager::Default().average_color = unity::colors::Aubergine;
  RefreshColor();
}

uint64_t BGHash::ColorAtomId() const
{
  return COLORS_ATOM;
}

void BGHash::OverrideColor(nux::Color const& color)
{
  override_color_ = color;
  RefreshColor();
}

void BGHash::RefreshColor()
{
  if (override_color_.alpha > 0.0f)
  {
    TransitionToNewColor(override_color_);
    return;
  }

  // XXX: move this part to PluginAdapter
  Atom         real_type;
  gint         result;
  gint         real_format;
  gulong       items_read;
  gulong       items_left;
  gchar*       colors;
  Display*     display;
  GdkRGBA      color_gdk;

  colors = nullptr;
  display = gdk_x11_display_get_xdisplay(gdk_display_get_default());

  gdk_error_trap_push();
  result = XGetWindowProperty (display,
             GDK_ROOT_WINDOW(),
             COLORS_ATOM,
             0L,
             G_MAXLONG,
             False,
             XA_STRING,
             &real_type,
             &real_format,
             &items_read,
             &items_left,
             (guchar **) &colors);
  gdk_flush ();
  gdk_error_trap_pop_ignored ();

  if (result == Success && items_read)
  {
    gdk_rgba_parse(&color_gdk, colors);
    nux::Color new_color(color_gdk.red, color_gdk.green, color_gdk.blue, 1.0f);
    TransitionToNewColor(MatchColor(new_color));
  }

  XFree(colors);
}

void BGHash::TransitionToNewColor(nux::color::Color const& new_color)
{
  auto const& current_color = WindowManager::Default().average_color();
  LOG_DEBUG(logger) << "transitioning from: " << current_color.red << " to " << new_color.red;

  transition_animator_.Stop();
  transition_animator_.SetStartValue(current_color).SetFinishValue(new_color).Start();
}

void BGHash::OnTransitionUpdated(nux::Color const& new_color)
{
  WindowManager::Default().average_color = new_color;
}

nux::Color BGHash::MatchColor(nux::Color const& base_color) const
{
  nux::Color chosen_color;
  nux::color::HueSaturationValue base_hsv(base_color);

  if (base_hsv.saturation < 0.08)
  {
    // grayscale image
    LOG_DEBUG (logger) << "got a grayscale image";
    chosen_color = nux::Color (46 , 52 , 54 );
    chosen_color.alpha = 0.72f;
  }
  else
  {
    std::vector<nux::Color> colors = {
      nux::Color(0x540e44), nux::Color(0x6e0b2a), nux::Color(0x841617),
      nux::Color(0x84371b), nux::Color(0x864d20), nux::Color(0x857f31),
      nux::Color(0x1d6331), nux::Color(0x11582e), nux::Color(0x0e5955),
      nux::Color(0x192b59), nux::Color(0x1b134c), nux::Color(0x2c0d46),
    };

    float closest_diff = 200.0f;
    LOG_DEBUG (logger) << "got a colour image";
    // full colour image
    for (auto const& color : colors)
    {
      nux::color::HueSaturationValue comparison_hsv(color);
      float color_diff = fabs(base_hsv.hue - comparison_hsv.hue);

      if (color_diff < closest_diff)
      {
        chosen_color = color;
        closest_diff = color_diff;
      }
    }

    nux::color::HueSaturationValue hsv_color(chosen_color);
    hsv_color.saturation = std::min(base_hsv.saturation, hsv_color.saturation);
    hsv_color.saturation *= (2.0f - hsv_color.saturation);
    hsv_color.value = std::min({base_hsv.value, hsv_color.value, 0.26f});

    chosen_color = nux::Color(nux::color::RedGreenBlue(hsv_color));
    chosen_color.alpha = 0.72f;
  }

  LOG_DEBUG(logger) << "eventually chose "
                    << chosen_color.red << ", "
                    << chosen_color.green << ", "
                    << chosen_color.blue;
  return chosen_color;
}

nux::Color BGHash::CurrentColor() const
{
  return WindowManager::Default().average_color();
}

}
