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
 * Authored by: Gordon Allott <gord.alott@canonical.com>
 */

#ifndef BGHASH_H
#define BGHASH_H

#include <sigc++/sigc++.h>
#include <Nux/Nux.h>
#include <libgnome-desktop/gnome-bg.h>
#include <unity-misc/gnome-bg-slideshow.h>
#include <UnityCore/GLibSignal.h>

namespace unity
{
  class BGHash
  {
  public:
    BGHash ();
    ~BGHash ();

    static gboolean ForceUpdate(BGHash *self);
    void LoadFileToHash (const std::string path);
    void LoadPixbufToHash (GdkPixbuf *pixbuf);
    GdkPixbuf *GetPixbufFromBG ();
    nux::Color CurrentColor ();
    void OnBackgroundChanged (GnomeBG *bg);
    void OnGSettingsChanged (GSettings *settings, gchar *key);

  private:
    static gboolean OnSlideshowTransition (BGHash *self);
    static gboolean OnTransitionCallback (BGHash *self);
    gboolean DoTransitionCallback ();
    void DoUbusColorEmit ();
    void TransitionToNewColor (nux::Color new_color);
    nux::Color InterpolateColor (nux::Color colora, nux::Color colorb, float value);
    nux::Color HashColor(GdkPixbuf *pixbuf);
    nux::Color MatchColor (nux::Color base_color);

  private:
    GnomeBG *background_monitor;
    GSettings *client;

    guint _transition_handler;

    SlideShow *_bg_slideshow;
    Slide     *_current_slide;
    guint      _slideshow_handler;

    nux::Color _current_color; // the current colour, including steps in transitions
    nux::Color _new_color;     // in transitions, the next colour, otherwise the current colour
    nux::Color _old_color;     // the last colour chosen, used for transitions

    guint64 _hires_time_start;
    guint64 _hires_time_end;
    glib::SignalManager signal_manager_; 
    uint _ubus_handle_request_colour;
  };
};

#endif // BGHASH_H
