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
#include "unity-shared/UBusMessages.h"

#ifndef XA_STRING
#define XA_STRING ((Atom) 31)
#endif

DECLARE_LOGGER(logger, "unity.bghash");

namespace unity
{

BGHash::BGHash()
  : current_color_(unity::colors::Aubergine)
{
  transition_animator_.SetDuration(500);
  override_color_.alpha = 0.0f;

  transition_animator_.updated.connect(sigc::mem_fun(this, &BGHash::OnTransitionUpdated));
  ubus_manager_.RegisterInterest(UBUS_BACKGROUND_REQUEST_COLOUR_EMIT, [&](GVariant *) { DoUbusColorEmit(); } );

  RefreshColor();
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

  Atom         real_type;
  gint         result;
  gint         real_format;
  gulong       items_read;
  gulong       items_left;
  gchar*       colors;
  Atom         representative_colors_atom;
  Display*     display;
  GdkRGBA      color_gdk;

  representative_colors_atom = gdk_x11_get_xatom_by_name("_GNOME_BACKGROUND_REPRESENTATIVE_COLORS");
  display = gdk_x11_display_get_xdisplay(gdk_display_get_default ());

  gdk_error_trap_push();
  result = XGetWindowProperty (display,
             GDK_ROOT_WINDOW(),
             representative_colors_atom,
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
    nux::Color new_color(color_gdk.red,
                         color_gdk.green,
                         color_gdk.blue,
                         1.0f);
    TransitionToNewColor(MatchColor(new_color));
    XFree (colors);
  }
}

void BGHash::TransitionToNewColor(nux::color::Color const& new_color)
{
  if (new_color == current_color_)
    return;

  LOG_DEBUG(logger) << "transitioning from: " << current_color_.red << " to " << new_color.red;

  transition_animator_.SetStartValue(current_color_);
  transition_animator_.SetFinishValue(new_color);
  transition_animator_.Stop();
  transition_animator_.Start();
}

void BGHash::OnTransitionUpdated(nux::Color const& new_color)
{
  current_color_ = new_color;
  DoUbusColorEmit();
}

void BGHash::DoUbusColorEmit()
{
  ubus_manager_.SendMessage(UBUS_BACKGROUND_COLOR_CHANGED,
                            g_variant_new ("(dddd)",
                                           current_color_.red,
                                           current_color_.green,
                                           current_color_.blue,
                                           current_color_.alpha));
}

nux::Color BGHash::MatchColor(nux::Color const& base_color) const
{
  nux::Color colors[12];

  colors[ 0] = nux::Color (0x540e44);
  colors[ 1] = nux::Color (0x6e0b2a);
  colors[ 2] = nux::Color (0x841617);
  colors[ 3] = nux::Color (0x84371b);
  colors[ 4] = nux::Color (0x864d20);
  colors[ 5] = nux::Color (0x857f31);
  colors[ 6] = nux::Color (0x1d6331);
  colors[ 7] = nux::Color (0x11582e);
  colors[ 8] = nux::Color (0x0e5955);
  colors[ 9] = nux::Color (0x192b59);
  colors[10] = nux::Color (0x1b134c);
  colors[11] = nux::Color (0x2c0d46);

  nux::Color chosen_color;
  nux::color::HueSaturationValue base_hsv (base_color);

  if (base_hsv.saturation < 0.08)
  {
    // grayscale image
    LOG_DEBUG (logger) << "got a grayscale image";
    chosen_color = nux::Color (46 , 52 , 54 );
    chosen_color.alpha = 0.72f;
  }
  else
  {
    float closest_diff = 200.0f;
    LOG_DEBUG (logger) << "got a colour image";
    // full colour image
    for (int i = 0; i < 11; i++)
    {
      nux::color::HueSaturationValue comparison_hsv (colors[i]);
      float color_diff = fabs(base_hsv.hue - comparison_hsv.hue);

      if (color_diff < closest_diff)
      {
        chosen_color = colors[i];
        closest_diff = color_diff;
      }
    }

    nux::color::HueSaturationValue hsv_color (chosen_color);
    hsv_color.saturation = std::min(base_hsv.saturation, hsv_color.saturation);
    hsv_color.saturation *= (2.0f - hsv_color.saturation);
    hsv_color.value = std::min(std::min(base_hsv.value, hsv_color.value), 0.26f);

    chosen_color = nux::Color (nux::color::RedGreenBlue(hsv_color));
    chosen_color.alpha = 0.72f;
  }

  LOG_DEBUG(logger) << "eventually chose "
                    << chosen_color.red << ", "
                    << chosen_color.green << ", "
                    << chosen_color.blue;
  return chosen_color;
}

nux::Color const& BGHash::CurrentColor() const
{
  return current_color_;
}

}
