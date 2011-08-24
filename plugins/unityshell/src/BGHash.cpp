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

#include "BGHash.h"
#include <Nux/Nux.h>
#include <NuxCore/Logger.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libgnome-desktop/gnome-bg.h>
#include <unity-misc/gnome-bg-slideshow.h>
#include "ubus-server.h"
#include "UBusMessages.h"
#include "UnityCore/GLibWrapper.h"

namespace
{
  nux::logging::Logger logger("unity.BGHash");
}

namespace {
  int level_of_recursion;
  const int MAX_LEVEL_OF_RECURSION = 16;
  const int MIN_LEVEL_OF_RECURSION = 2;
}
namespace unity {

  BGHash::BGHash ()
    : _transition_handler (0),
      _bg_slideshow (NULL),
      _current_slide (NULL),
      _slideshow_handler(0),
      _current_color (unity::colors::Aubergine),
      _new_color (unity::colors::Aubergine),
      _old_color (unity::colors::Aubergine),
      _hires_time_start(10),
      _hires_time_end(20),
      _ubus_handle_request_colour(0)
  {
    g_warning ("BGHash: _current %f, %f, %f", _current_color.red, _current_color.green, _current_color.blue);
    g_warning ("BGHash: _new %f, %f, %f", _new_color.red, _new_color.green, _new_color.blue);
    g_warning ("BGHash: _old %f, %f, %f", _old_color.red, _old_color.green, _old_color.blue);

    background_monitor = gnome_bg_new ();
    client = g_settings_new ("org.gnome.desktop.background");

    signal_manager_.Add(
      new glib::Signal<void, GnomeBG*>(background_monitor,
                                       "changed",
                                       sigc::mem_fun(this, &BGHash::OnBackgroundChanged)));

    signal_manager_.Add(
      new glib::Signal<void, GSettings*, gchar*>(client,
                                                 "changed",
                                                 sigc::mem_fun(this, &BGHash::OnGSettingsChanged)));

    UBusServer *ubus = ubus_server_get_default ();

    gnome_bg_load_from_preferences (background_monitor, client);

    glib::Object<GdkPixbuf> pixbuf(GetPixbufFromBG());
    LoadPixbufToHash(pixbuf);

    g_timeout_add (0, (GSourceFunc)ForceUpdate, (gpointer)this);

    // avoids making a new object method when all we are doing is
    // calling a method with no logic
    auto request_lambda =  [](GVariant *data, gpointer self) {
      reinterpret_cast<BGHash*>(self)->DoUbusColorEmit();
    };
    _ubus_handle_request_colour = ubus_server_register_interest (ubus, UBUS_BACKGROUND_REQUEST_COLOUR_EMIT,
                                                                 (UBusCallback)request_lambda,
                                                                  this);



  }

  BGHash::~BGHash ()
  {
    g_object_unref (client);
    g_object_unref (background_monitor);
    UBusServer *ubus = ubus_server_get_default ();
    ubus_server_unregister_interest (ubus, _ubus_handle_request_colour);
  }

  gboolean BGHash::ForceUpdate (BGHash *self)
  {
    self->OnBackgroundChanged(self->background_monitor);
    return FALSE;
  }

  void BGHash::OnGSettingsChanged (GSettings *settings, gchar *key)
  {
    gnome_bg_load_from_preferences (background_monitor, settings);
  }

  void BGHash::OnBackgroundChanged (GnomeBG *bg)
  {
    const gchar *filename = gnome_bg_get_filename (bg);
    if (filename == NULL)
    {
      g_warning ("BGHash did not get a filename");
      // we might have a gradient instead
      GdkColor color_primary, color_secondary;
      GDesktopBackgroundShading shading_type;
      nux::Color parsed_color;

      gnome_bg_get_color (bg, &shading_type, &color_primary, &color_secondary);
      if (shading_type == G_DESKTOP_BACKGROUND_SHADING_HORIZONTAL ||
          shading_type == G_DESKTOP_BACKGROUND_SHADING_SOLID)
      {
        parsed_color = nux::Color(static_cast<float>(color_primary.red) / 65535,
                                  static_cast<float>(color_primary.green) / 65535,
                                  static_cast<float>(color_primary.blue) / 65535,
                                  1.0f);
      }
      else
      {
        nux::Color primary = nux::Color(static_cast<float>(color_primary.red) / 65535,
                                        static_cast<float>(color_primary.green) / 65535,
                                        static_cast<float>(color_primary.blue) / 65535,
                                        1.0f);

        nux::Color secondary = nux::Color(static_cast<float>(color_secondary.red) / 65535,
                                          static_cast<float>(color_secondary.green) / 65535,
                                          static_cast<float>(color_secondary.blue) / 65535,
                                          1.0f);

        parsed_color = (primary + secondary) * 0.5f;
      }
      g_warning ("got parsed colour %f, %f, %f", parsed_color.red, parsed_color.green, parsed_color.blue);
      nux::Color new_color = MatchColor (parsed_color);
      TransitionToNewColor (new_color);
    }
    else
    {
      g_warning ("BGHash got a filename: %s", filename);
      if (_bg_slideshow != NULL)
      {
        slideshow_unref (_bg_slideshow);
        _bg_slideshow = NULL;
        _current_slide = NULL;
      }

      if (_slideshow_handler)
      {
        g_source_remove (_slideshow_handler);
        _slideshow_handler = 0;
      }

      if (g_str_has_suffix (filename, "xml"))
      {
        GError *error = NULL;

        if (_bg_slideshow != NULL)
        {
          slideshow_unref (_bg_slideshow);
          _bg_slideshow = NULL;
        }

        _bg_slideshow = read_slideshow_file (filename, &error);

        if (error != NULL)
        {
          LOG_WARNING(logger) << "Could not load filename \"" << filename << "\": " << error->message;
          g_error_free (error);
        }
        else if (_bg_slideshow == NULL)
        {
          LOG_WARNING(logger) << "Could not load filename \"" << filename << "\"";
        }
        else
        {
          // we loaded fine, hook up to the slideshow
          time_t current_time = time(0);
          double now = (double) current_time;

          double time_diff = now - _bg_slideshow->start_time;
          double progress = fmod (time_diff, _bg_slideshow->total_duration);

          // progress now holds how many seconds we are in to this slideshow.
          // iterate over the slideshows until we get in to the current slideshow
          Slide *slide_iteration;
          Slide *slide_current = NULL;
          double elapsed = 0;
          double time_to_next_change = 0;
          GList *list;

          for (list = _bg_slideshow->slides->head; list != NULL; list = list->next)
          {
            slide_iteration = reinterpret_cast<Slide *>(list->data);
            if (elapsed + slide_iteration->duration > progress)
            {
              slide_current = slide_iteration;
              time_to_next_change = slide_current->duration- (progress - elapsed);
              break;
            }

            elapsed += slide_iteration->duration;
          }

          if (slide_current == NULL)
          {
            slide_current = reinterpret_cast<Slide *>(g_queue_peek_head(_bg_slideshow->slides));
            time_to_next_change = slide_current->duration;
          }

          // time_to_next_change now holds the seconds until the next slide change
          // the next slide change may or may not be a fixed slide.
          _slideshow_handler = g_timeout_add ((guint)(time_to_next_change * 1000),
                                                    (GSourceFunc)OnSlideshowTransition,
                                                    (gpointer)this);

          // find our current slide now
          if (slide_current->file1 == NULL)
          {
            LOG_WARNING(logger) << "Could not load filename \"" << filename << "\"";
          }
          else
          {
            FileSize *fs = reinterpret_cast<FileSize *>(slide_current->file1->data);
            filename = reinterpret_cast<gchar *>(fs->file);
          }

          _current_slide = slide_current;
        }
      }

      LoadFileToHash(filename);
    }
  }

  gboolean BGHash::OnSlideshowTransition (BGHash *self)
  {
    // the timing might be a bit weird, GnomeBG works in floating points
    // for the time value, so we might end up being a bit before the change
    // or a bit after, after is fine, but before is bad
    // so we need to check if the slide gnomebg tells us we are on is the
    // old slide or a new one
    double delta = 0.0;
    Slide *proposed_slide = get_current_slide (self->_bg_slideshow, &delta);
    if (proposed_slide == self->_current_slide)
    {
      // boo, so we arrived early, we need to get the next slide
      GList *list = g_queue_find (self->_bg_slideshow->slides, self->_current_slide);
      if (list->next == NULL)
      {
        // we hit the end of the slideshow, so we wrap around
        proposed_slide = reinterpret_cast<Slide *>(self->_bg_slideshow->slides->head->data);
      }
      else
      {
        proposed_slide = reinterpret_cast<Slide *>(list->next->data);
      }
    }

    // at this point proposed_slide contains the slide we currently have
    // if its a transition slide, we need to transition to the next image
    // if its fixed, we can stay still, the previous transition slide already
    // got us to this image.

    // quickly hook up the next transition

    self->_slideshow_handler = g_timeout_add ((guint)(proposed_slide->duration * 1000),
                                              (GSourceFunc)OnSlideshowTransition,
                                              (gpointer)self);

    if (proposed_slide->fixed == FALSE)
    {
      const gchar *filename = NULL;
      if (proposed_slide->file1 == NULL)
      {
        LOG_WARNING(logger) << "Could not load filename \"" << filename << "\"";
      }
      else
      {
        FileSize *fs = reinterpret_cast<FileSize *>(proposed_slide->file2->data);
        filename = reinterpret_cast<gchar *>(fs->file);
      }

      if (filename != NULL)
        self->LoadFileToHash (filename);
    }

    self->_current_slide = proposed_slide;
    return false;
  }

  nux::Color BGHash::InterpolateColor (nux::Color colora, nux::Color colorb, float value)
  {
    // takes two colours, transitions between them, we can do it linearly or whatever
    // i don't think it will matter that much
    // it doesn't happen too often
    g_warning ("asked to interpolate between %f, %f, %f and %f, %f, %f at value %f",
              colora.red, colora.green, colora.blue, colorb.red, colorb.green, colorb.blue, value);
    return colora + ((colorb - colora) * value);
  }

  void BGHash::TransitionToNewColor(nux::color::Color new_color)
  {
    if (new_color == _current_color)
      return;

    if (_transition_handler)
    {
      // we are currently in a transition
      g_source_remove (_transition_handler);
    }

    _old_color = _current_color;
    _new_color = new_color;

    _hires_time_start = g_get_monotonic_time();
    _hires_time_end = 500 * 1000; // 500 milliseconds
    _transition_handler = g_timeout_add (1000/60, (GSourceFunc)BGHash::OnTransitionCallback, this);

  }

  gboolean BGHash::OnTransitionCallback(BGHash *self)
  {
    return self->DoTransitionCallback();
  }

  gboolean BGHash::DoTransitionCallback ()
  {
    guint64 current_time = g_get_monotonic_time();
    float timediff = ((float)current_time - _hires_time_start) / _hires_time_end;

    timediff = std::max(std::min(timediff, 1.0f), 0.0f);

    _current_color = InterpolateColor(_old_color,
                                      _new_color,
                                      timediff);
    DoUbusColorEmit ();

    if (current_time > _hires_time_start + _hires_time_end)
    {
      _transition_handler = 0;
      return FALSE;
    }
    else
    {
      return TRUE;
    }
  }

  void BGHash::DoUbusColorEmit()
  {
    g_warning ("BGHash emitting colour %f, %f, %f", _current_color.red, _current_color.green, _current_color.blue);
    ubus_server_send_message(ubus_server_get_default(),
                             UBUS_BACKGROUND_COLOR_CHANGED,
                             g_variant_new ("(dddd)",
                                            _current_color.red * 0.7f,
                                            _current_color.green * 0.7f,
                                            _current_color.blue * 0.7f,
                                            0.5)
                            );
  }

  GdkPixbuf *BGHash::GetPixbufFromBG ()
  {
    GnomeDesktopThumbnailFactory *factory;
    GdkPixbuf *pixbuf;
    GdkScreen *screen = gdk_screen_get_default ();
    int width = 0, height = 0;

    factory = gnome_desktop_thumbnail_factory_new (GNOME_DESKTOP_THUMBNAIL_SIZE_LARGE);
    gnome_bg_get_image_size (background_monitor, factory,
                             gdk_screen_get_height(screen),
                             gdk_screen_get_width (screen),
                             &width, &height);

    pixbuf = gnome_bg_create_thumbnail (background_monitor, factory,
                                        gdk_screen_get_default (),
                                        width, height);

    return pixbuf;
  }

  void BGHash::LoadPixbufToHash(GdkPixbuf *pixbuf)
  {
    nux::Color new_color;
    if (pixbuf == NULL)
    {
      LOG_WARNING(logger) << "Passed in a bad pixbuf, defaulting colour";
      new_color = unity::colors::Aubergine;
    }
    else
    {
      new_color = HashColor (pixbuf);
    }

    TransitionToNewColor (new_color);
  }

  void BGHash::LoadFileToHash(const std::string path)
  {
    g_warning ("BGHash - loading file %s to hash", path.c_str());
    glib::Error error;
    glib::Object<GdkPixbuf> pixbuf(gdk_pixbuf_new_from_file (path.c_str (), &error));

    if (error)
    {
      LOG_WARNING(logger) << "Could not load filename \"" << path << "\": " << error.Message();
      g_warning ("got an error :(");
      _current_color = unity::colors::Aubergine;

      // try and get a colour from gnome-bg, for various reasons, gnome bg might not
      // return a correct image which sucks =\ but this is a fallback
      pixbuf = GetPixbufFromBG();
    }

    LoadPixbufToHash (pixbuf);
  }

  inline nux::Color GetPixbufSample (GdkPixbuf *pixbuf, int x, int y)
  {
    guchar *img = gdk_pixbuf_get_pixels (pixbuf);
    guchar *pixel = img + ((y * gdk_pixbuf_get_rowstride(pixbuf)) + (x * gdk_pixbuf_get_n_channels (pixbuf)));
    return nux::Color ((float)(*(pixel + 0)), (float)(*(pixel + 1)), (float)(*(pixel + 2)));
  }

  inline bool is_color_different (const nux::Color color_a, const nux::Color color_b)
  {
    nux::Color diff = color_a - color_b;
    if (fabs (diff.red) > 0.15 || fabs (diff.green) > 0.15 || fabs (diff.blue) > 0.15)
      return true;

    return false;
  }

  nux::Color GetQuadAverage (int x, int y, int width, int height, GdkPixbuf *pixbuf)
  {
    level_of_recursion++;
    // samples four corners
    // c1-----c2
    // |       |
    // c3-----c4

    nux::Color corner1 = GetPixbufSample(pixbuf, x        , y         );
    nux::Color corner2 = GetPixbufSample(pixbuf, x + width, y         );
    nux::Color corner3 = GetPixbufSample(pixbuf, x        , y + height);
    nux::Color corner4 = GetPixbufSample(pixbuf, x + width, y + height);

    nux::Color centre = GetPixbufSample(pixbuf, x + (width/2), y + (height / 2));

    // corner 1
    if ((is_color_different(corner1, centre) ||
         level_of_recursion < MIN_LEVEL_OF_RECURSION) &&
        level_of_recursion < MAX_LEVEL_OF_RECURSION)
    {
      corner1 = GetQuadAverage(x, y, width/2, height/2, pixbuf);
    }

    // corner 2
    if ((is_color_different(corner2, centre) ||
         level_of_recursion < MIN_LEVEL_OF_RECURSION) &&
        level_of_recursion < MAX_LEVEL_OF_RECURSION)
    {
      corner2 = GetQuadAverage(x + width/2, y, width/2, height/2, pixbuf);
    }

    // corner 3
    if ((is_color_different(corner3, centre) ||
         level_of_recursion < MIN_LEVEL_OF_RECURSION) &&
        level_of_recursion < MAX_LEVEL_OF_RECURSION)
    {
      corner3 = GetQuadAverage(x, y + height/2, width/2, height/2, pixbuf);
    }

    // corner 4
    if ((is_color_different(corner4, centre) ||
         level_of_recursion < MIN_LEVEL_OF_RECURSION) &&
        level_of_recursion < MAX_LEVEL_OF_RECURSION)
    {
      corner4 = GetQuadAverage(x + width/2, y + height/2, width/2, height/2, pixbuf);
    }

    nux::Color average = (  (corner1 * 3)
                          + (corner3 * 3)
                          + (centre  * 2)
                          + corner2
                          + corner4) * 0.1;
    level_of_recursion--;
    return average;
  }

  nux::Color BGHash::HashColor(GdkPixbuf *pixbuf)
  {
    level_of_recursion = 0;
    nux::Color average = GetQuadAverage (0, 0,
                                         gdk_pixbuf_get_width (pixbuf) - 1,
                                         gdk_pixbuf_get_height (pixbuf) - 1,
                                         pixbuf);

    g_warning ("got average colour %f, %f, %f", average.red, average.green, average.blue);
    nux::Color matched_color = MatchColor (average);
    return matched_color;
  }

  nux::Color BGHash::MatchColor (const nux::Color base_color)
  {
    g_warning ("trying to match colour %f, %f, %f", base_color.red, base_color.green, base_color.blue);
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

    nux::Color bw_colors[2];
    //bw_colors[0] = nux::Color (211, 215, 207); //Aluminium 2
    bw_colors[0] = nux::Color (136, 138, 133); //Aluminium 4
    bw_colors[1] = nux::Color (46 , 52 , 54 ); //Aluminium 6

    float closest_diff = 200.0f;
    nux::Color chosen_color;
    nux::color::HueSaturationValue base_hsv (base_color);

    if (base_hsv.saturation < 0.08)
    {
      // grayscale image
      for (int i = 0; i < 3; i++)
      {
        nux::color::HueSaturationValue comparison_hsv (bw_colors[i]);
        float color_diff = fabs(base_hsv.value - comparison_hsv.value);
        if (color_diff < closest_diff)
        {
          chosen_color = bw_colors[i];
          closest_diff = color_diff;
        }
      }
    }
    else
    {
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

      nux::color::HueLightnessSaturation hsv_color (chosen_color);

      hsv_color.saturation = base_hsv.saturation;
      hsv_color.lightness = 0.2;
      chosen_color = nux::Color (nux::color::RedGreenBlue(hsv_color));
    }

    // apply design to the colour
    chosen_color.alpha = 0.5f;

    g_warning ("resulting in colour %f, %f, %f", chosen_color.red, chosen_color.green, chosen_color.blue);

    return chosen_color;
  }

  nux::Color BGHash::CurrentColor ()
  {
    return _current_color;
  }
}
