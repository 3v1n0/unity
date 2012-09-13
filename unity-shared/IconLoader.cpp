// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2010, 2011 Canonical Ltd
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
* Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
*/

#include "IconLoader.h"
#include "config.h"

#include <queue>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <unity-protocol.h>
#include <pango/pango.h>
#include <pango/pangocairo.h>

#include <Nux/Nux.h>
#include <NuxCore/Logger.h>
#include <NuxGraphics/CairoGraphics.h>
#include <UnityCore/GLibSource.h>
#include <UnityCore/GLibSignal.h>

#include "unity-shared/Timer.h"

namespace unity
{
namespace
{
nux::logging::Logger logger("unity.iconloader");
const int MIN_ICON_SIZE = 2;
const int RIBBON_PADDING = 2;
}

class IconLoader::Impl
{
public:
  // The Handle typedef is used to explicitly indicate which integers are
  // infact our opaque handles.
  typedef int Handle;
  static const int FONT_SIZE = 10;
  static const int MIN_FONT_SIZE = 6;

  Impl();

  Handle LoadFromIconName(std::string const& icon_name,
                          int max_width,
                          int max_height,
                          IconLoaderCallback slot);

  Handle LoadFromGIconString(std::string const& gicon_string,
                             int max_width,
                             int max_height,
                             IconLoaderCallback slot);

  Handle LoadFromFilename(std::string const& filename,
                          int max_width,
                          int max_height,
                          IconLoaderCallback slot);

  Handle LoadFromURI(std::string const& uri,
                     int max_width,
                     int max_height,
                     IconLoaderCallback slot);

  void DisconnectHandle(Handle handle);

  static void CalculateTextHeight(int* width, int* height);

private:

  enum IconLoaderRequestType
  {
    REQUEST_TYPE_ICON_NAME = 0,
    REQUEST_TYPE_GICON_STRING,
    REQUEST_TYPE_URI,
  };

  struct IconLoaderTask
  {
    typedef std::shared_ptr<IconLoaderTask> Ptr;

    IconLoaderRequestType type;
    std::string data;
    int max_width;
    int max_height;
    std::string key;
    IconLoaderCallback slot;
    Handle handle;
    Impl* impl;
    GtkIconInfo* icon_info;
    bool no_cache;
    int helper_handle;
    glib::Object<GdkPixbuf> result;
    glib::Error error;
    std::list<IconLoaderTask::Ptr> shadow_tasks;
    unsigned idle_id;

    IconLoaderTask(IconLoaderRequestType type_,
                   std::string const& data_,
                   int max_width_,
                   int max_height_,
                   std::string const& key_,
                   IconLoaderCallback slot_,
                   Handle handle_,
                   Impl* self_)
      : type(type_), data(data_), max_width(max_width_)
      , max_height(max_height_), key(key_)
      , slot(slot_), handle(handle_), impl(self_)
      , icon_info(nullptr), no_cache(false), helper_handle(0), idle_id(0)
      {}

    ~IconLoaderTask()
    {
      if (icon_info)
        ::gtk_icon_info_free(icon_info);
      if (helper_handle != 0)
        impl->DisconnectHandle(helper_handle);
      if (idle_id != 0)
        g_source_remove(idle_id);
    }

    void InvokeSlot()
    {
      if (slot)
        slot(data, max_width, max_height, result);

      // notify shadow tasks
      for (auto shadow_task : shadow_tasks)
      {
        if (shadow_task->slot)
          shadow_task->slot(shadow_task->data,
                            shadow_task->max_width,
                            shadow_task->max_height,
                            result);

        impl->task_map_.erase(shadow_task->handle);
      }

      shadow_tasks.clear();
    }

    bool Process()
    {
      // Check the cache again, as previous tasks might have wanted the same
      if (impl->CacheLookup(key, data, max_width, max_height, slot))
        return true;

      LOG_DEBUG(logger) << "Processing  " << data << " at size " << max_height;

      // Rely on the compiler to tell us if we miss a new type
      switch (type)
      {
        case REQUEST_TYPE_ICON_NAME:
          return ProcessIconNameTask();
        case REQUEST_TYPE_GICON_STRING:
          return ProcessGIconTask();
        case REQUEST_TYPE_URI:
          return ProcessURITask();
      }

      LOG_WARNING(logger) << "Request type " << type
                          << " is not supported (" << data
                          << " " << max_width << "x" << max_height << ")";
      result = nullptr;
      InvokeSlot();

      return true;
    }

    bool ProcessIconNameTask()
    {
      int size = max_height < 0 ? max_width : (max_width < 0 ? max_height : MIN(max_height, max_width));
      GtkIconInfo* info = ::gtk_icon_theme_lookup_icon(impl->theme_, data.c_str(),
                                                       size, static_cast<GtkIconLookupFlags>(0));
      if (info)
      {
        icon_info = info;
        PushSchedulerJob();

        return false;
      }
      else
      {
        LOG_WARNING(logger) << "Unable to load icon " << data
                            << " at size " << size;
      }

      result = nullptr;
      InvokeSlot();

      return true;
    }

    bool ProcessGIconTask()
    {
      glib::Error error;
      glib::Object<GIcon> icon(::g_icon_new_for_string(data.c_str(), &error));
      int size = max_height < 0 ? max_width : (max_width < 0 ? max_height : MIN(max_height, max_width));

      if (icon.IsType(UNITY_PROTOCOL_TYPE_ANNOTATED_ICON))
      {
        UnityProtocolAnnotatedIcon *anno;
        anno = UNITY_PROTOCOL_ANNOTATED_ICON(icon.RawPtr());
        GIcon* base_icon = unity_protocol_annotated_icon_get_icon(anno);
        glib::String gicon_string(g_icon_to_string(base_icon));

        no_cache = true;
        auto helper_slot = sigc::bind(sigc::mem_fun(this, &IconLoaderTask::BaseIconLoaded), glib::object_cast<UnityProtocolAnnotatedIcon>(icon));
        int base_icon_width = max_width > 0 ? max_width - RIBBON_PADDING * 2 : -1;
        int base_icon_height = base_icon_width < 0 ? max_height - RIBBON_PADDING *2 : max_height;
        helper_handle = impl->LoadFromGIconString(gicon_string.Str(),
                                                  base_icon_width,
                                                  base_icon_height,
                                                  helper_slot);

        return false;
      }
      else if (icon.IsType(G_TYPE_FILE_ICON))
      {
        // [trasfer none]
        GFile* file = ::g_file_icon_get_file(G_FILE_ICON(icon.RawPtr()));
        glib::String uri(::g_file_get_uri(file));

        type = REQUEST_TYPE_URI;
        data = uri.Str();

        return ProcessURITask();
      }
      else if (icon.IsType(G_TYPE_ICON))
      {
        GtkIconInfo* info = ::gtk_icon_theme_lookup_by_gicon(impl->theme_, icon, size,
                                                             static_cast<GtkIconLookupFlags>(0));
        if (info)
        {
          icon_info = info;
          PushSchedulerJob();

          return false;
        }
        else
        {
          // There is some funkiness in some programs where they install
          // their icon to /usr/share/icons/hicolor/apps/, but they
          // name the Icon= key as `foo.$extension` which breaks loading
          // So we can try and work around that here.

          if (boost::iends_with(data, ".png") ||
              boost::iends_with(data, ".xpm") ||
              boost::iends_with(data, ".gif") ||
              boost::iends_with(data, ".jpg"))
          {
            data = data.substr(0, data.size() - 4);
            return ProcessIconNameTask();
          }
          else
          {
            LOG_WARNING(logger) << "Unable to load icon " << data
                                << " at size " << size;
          }
        }
      }
      else
      {
        LOG_WARNING(logger) << "Unable to load icon " << data
                            << " at size " << size << ": " << error;
      }

      InvokeSlot();
      return true;
    }

    bool ProcessURITask()
    {
      PushSchedulerJob();

      return false;
    }

    void CategoryIconLoaded(std::string const& base_icon_string,
                            int max_width, int max_height,
                            glib::Object<GdkPixbuf> const& category_pixbuf,
                            glib::Object<UnityProtocolAnnotatedIcon> const& anno_icon)
    {
      helper_handle = 0;
      bool has_emblem = category_pixbuf;

      const gchar* detail_text = unity_protocol_annotated_icon_get_ribbon(anno_icon);
      if (detail_text)
      {
        const int SHADOW_BOTTOM_PADDING = 2;
        const int SHADOW_SIDE_PADDING = 1;
        int icon_w = gdk_pixbuf_get_width(result);
        int icon_h = gdk_pixbuf_get_height(result);

        int max_font_height;
        CalculateTextHeight(nullptr, &max_font_height);

        // FIXME: design wants the tags 2px wider than the original icon
        int pixbuf_width = icon_w;
        int pixbuf_height = max_font_height * 5 / 4 + SHADOW_BOTTOM_PADDING;
        if (pixbuf_height > icon_h) pixbuf_height = icon_h;

        nux::CairoGraphics cairo_graphics(CAIRO_FORMAT_ARGB32,
                                          pixbuf_width, pixbuf_height);
        std::shared_ptr<cairo_t> cr(cairo_graphics.GetContext(), cairo_destroy);

        glib::Object<PangoLayout> layout;
        PangoContext* pango_context = NULL;
        GdkScreen* screen = gdk_screen_get_default(); // not ref'ed
        glib::String font;
        int dpi = -1;

        g_object_get(gtk_settings_get_default(), "gtk-font-name", &font, NULL);
        g_object_get(gtk_settings_get_default(), "gtk-xft-dpi", &dpi, NULL);
        cairo_set_font_options(cr.get(), gdk_screen_get_font_options(screen));
        layout = pango_cairo_create_layout(cr.get());
        std::shared_ptr<PangoFontDescription> desc(pango_font_description_from_string(font), pango_font_description_free);
        pango_font_description_set_weight(desc.get(), PANGO_WEIGHT_BOLD);
        int font_size = FONT_SIZE;
        pango_font_description_set_size (desc.get(), font_size * PANGO_SCALE);

        pango_layout_set_font_description(layout, desc.get());
        pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);

        // we'll allow tiny bit of overflow since the text is rotated and there
        // is some space left... FIXME: 10/9? / 11/10?
        double max_text_width = has_emblem ?
          pixbuf_width * 0.73 : pixbuf_width;

        pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
        pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);

        glib::String escaped_text(g_markup_escape_text(detail_text, -1));
        pango_layout_set_markup(layout, escaped_text, -1);

        pango_context = pango_layout_get_context(layout);  // is not ref'ed
        pango_cairo_context_set_font_options(pango_context,
                                             gdk_screen_get_font_options(screen));
        pango_cairo_context_set_resolution(pango_context,
                                           dpi == -1 ? 96.0f : dpi/(float) PANGO_SCALE);
        pango_layout_context_changed(layout);

        // find proper font size (can we do this before the rotation?)
        int text_width, text_height;
        pango_layout_get_pixel_size(layout, &text_width, nullptr);
        while (text_width > max_text_width && font_size > MIN_FONT_SIZE)
        {
          font_size--;
          pango_font_description_set_size (desc.get(), font_size * PANGO_SCALE);
          pango_layout_set_font_description(layout, desc.get());
          pango_layout_get_pixel_size(layout, &text_width, nullptr);
        }
        pango_layout_set_width(layout, static_cast<int>(max_text_width * PANGO_SCALE));

        cairo_set_operator(cr.get(), CAIRO_OPERATOR_CLEAR);
        cairo_paint(cr.get());

        cairo_set_operator(cr.get(), CAIRO_OPERATOR_OVER);

        // this should be #dd4814
        const double ORANGE_R = 0.86666;
        const double ORANGE_G = 0.28235;
        const double ORANGE_B = 0.07843;

        double belt_w = static_cast<double>(pixbuf_width - SHADOW_SIDE_PADDING * 2);
        double belt_h = static_cast<double>(pixbuf_height - SHADOW_BOTTOM_PADDING);

        // translate to make space for the shadow
        cairo_save(cr.get());
        cairo_translate(cr.get(), 1.0, 1.0);

        cairo_set_source_rgba(cr.get(), ORANGE_R, ORANGE_G, ORANGE_B, 1.0);

        cairo_rectangle(cr.get(), 0.0, 0.0, belt_w, belt_h);
        cairo_fill_preserve(cr.get());

        std::shared_ptr<cairo_pattern_t> pattern(
            cairo_pattern_create_linear(0.0, 0.0, belt_w, 0.0),
            cairo_pattern_destroy);
        cairo_pattern_add_color_stop_rgba(pattern.get(), 0.0, 1.0, 1.0, 1.0, 0.235294);
        cairo_pattern_add_color_stop_rgba(pattern.get(), 0.02, 1.0, 1.0, 1.0, 0.0);
        if (!has_emblem)
        {
          cairo_pattern_add_color_stop_rgba(pattern.get(), 0.98, 1.0, 1.0, 1.0, 0.0);
          cairo_pattern_add_color_stop_rgba(pattern.get(), 1.0, 1.0, 1.0, 1.0, 0.235294);
        }
        cairo_pattern_add_color_stop_rgba(pattern.get(), 1.0, 1.0, 1.0, 1.0, 0.0);

        cairo_set_source(cr.get(), pattern.get());
        cairo_fill(cr.get());

        if (has_emblem)
        {
          // paint the curve
          const double CURVE_START_X = 0.651163 * belt_w;

          cairo_set_source_rgba(cr.get(), 1.0, 1.0, 1.0, 1.0);

          cairo_move_to(cr.get(), CURVE_START_X, belt_h);
          cairo_curve_to(cr.get(), 0.719186 * belt_w, belt_h,
                                   0.721163 * belt_w, 0.9825 * belt_h,
                                   0.754128 * belt_w, 0.72725 * belt_h);
          cairo_line_to(cr.get(), 0.812674 * belt_w, 0.27275 * belt_h);
          cairo_curve_to(cr.get(), 0.848256 * belt_w, 0.0,
                                   0.848256 * belt_w, 0.0,
                                   0.916942 * belt_w, 0.0);
          cairo_line_to(cr.get(), belt_w, 0.0);
          cairo_line_to(cr.get(), belt_w, belt_h);
          cairo_close_path(cr.get());
          cairo_fill(cr.get());

          // and the highlight
          pattern.reset(cairo_pattern_create_linear(CURVE_START_X, 0.0, belt_w, 0.0),
                        cairo_pattern_destroy);
          cairo_pattern_add_color_stop_rgba(pattern.get(), 0.0, 1.0, 1.0, 1.0, 0.0);
          cairo_pattern_add_color_stop_rgba(pattern.get(), 0.95, 1.0, 1.0, 1.0, 0.0);
          cairo_pattern_add_color_stop_rgba(pattern.get(), 1.0, 0.0, 0.0, 0.0, 0.235294);
          cairo_set_source(cr.get(), pattern.get());
          cairo_rectangle(cr.get(), CURVE_START_X, 0.0, belt_w - CURVE_START_X, belt_h);
          cairo_fill(cr.get());

          // paint the emblem
          int category_pb_h = gdk_pixbuf_get_height(category_pixbuf);
          gdk_cairo_set_source_pixbuf(cr.get(), category_pixbuf,
                                      belt_w * 0.812674, (belt_h - category_pb_h) / 2);
          cairo_paint(cr.get());
        }

        cairo_set_source_rgba(cr.get(), 1.0, 1.0, 1.0, 1.0);
        cairo_move_to(cr.get(), 0.0, belt_h / 2);
        pango_layout_get_pixel_size(layout, nullptr, &text_height);
        // current point is now in the middle of the stripe, need to translate
        // it, so that the text is centered
        cairo_rel_move_to(cr.get(), 0.0, text_height / -2.0);
        pango_cairo_show_layout(cr.get(), layout);

        // paint the shadow
        cairo_restore(cr.get());

        pattern.reset(cairo_pattern_create_linear(0.0, belt_h, 0.0, belt_h + SHADOW_BOTTOM_PADDING),
                      cairo_pattern_destroy);
        cairo_pattern_add_color_stop_rgba(pattern.get(), 0.0, 0.0, 0.0, 0.0, 0.2);
        cairo_pattern_add_color_stop_rgba(pattern.get(), 1.0, 0.0, 0.0, 0.0, 0.0);

        cairo_set_source(cr.get(), pattern.get());

        cairo_rectangle(cr.get(), 0.0, belt_h, belt_w, SHADOW_BOTTOM_PADDING);
        cairo_fill(cr.get());

        cairo_set_source_rgba(cr.get(), 0.0, 0.0, 0.0, 0.1);
        cairo_move_to(cr.get(), 0.0, 1.0);
        cairo_line_to(cr.get(), 0.0, belt_h);
        cairo_stroke(cr.get());

        cairo_move_to(cr.get(), belt_w, 1.0);
        cairo_line_to(cr.get(), belt_w, belt_h);
        cairo_stroke(cr.get());

        // FIXME: going from image_surface to pixbuf, and then to texture :(
        glib::Object<GdkPixbuf> detail_pb(
            gdk_pixbuf_get_from_surface(cairo_graphics.GetSurface(),
                                        0, 0,
                                        cairo_graphics.GetWidth(),
                                        cairo_graphics.GetHeight()));

        int y_pos = icon_h - pixbuf_height - max_font_height / 2;
        gdk_pixbuf_composite(detail_pb, result, // src, dest
                             0, // dest_x
                             y_pos, // dest_y
                             pixbuf_width, // dest_w
                             pixbuf_height, // dest_h
                             0, // offset_x
                             y_pos, // offset_y
                             1.0, 1.0, // scale_x, scale_y
                             GDK_INTERP_NEAREST, // interpolation
                             255); // src_alpha
      }

      idle_id = g_idle_add(LoadIconComplete, this);
    }

    void BaseIconLoaded(std::string const& base_icon_string,
                        int max_width, int max_height,
                        glib::Object<GdkPixbuf> const& base_pixbuf,
                        glib::Object<UnityProtocolAnnotatedIcon> const& anno_icon)
    {
      helper_handle = 0;
      if (base_pixbuf)
      {
        LOG_DEBUG(logger) << "Base icon loaded: '" << base_icon_string << 
          "' size: " << gdk_pixbuf_get_width(base_pixbuf) << 'x' <<
                        gdk_pixbuf_get_height(base_pixbuf);

        // we need extra space for the ribbon
        result = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 
                                gdk_pixbuf_get_width(base_pixbuf) + RIBBON_PADDING * 2,
                                gdk_pixbuf_get_height(base_pixbuf));
        gdk_pixbuf_fill(result, 0x0);
        gdk_pixbuf_copy_area(base_pixbuf, 0, 0,
                             gdk_pixbuf_get_width(base_pixbuf),
                             gdk_pixbuf_get_height(base_pixbuf),
                             result,
                             RIBBON_PADDING, 0);
        // FIXME: can we composite the pixbuf in helper thread?
        UnityProtocolCategoryType category = unity_protocol_annotated_icon_get_category(anno_icon);
        auto helper_slot = sigc::bind(sigc::mem_fun(this, &IconLoaderTask::CategoryIconLoaded), anno_icon);
        int max_font_height;
        CalculateTextHeight(nullptr, &max_font_height);
        unsigned cat_size = max_font_height * 8 / 9;
        switch (category)
        {
          case UNITY_PROTOCOL_CATEGORY_TYPE_BOOK:
            helper_handle =
              impl->LoadFromFilename(PKGDATADIR"/emblem_books.svg", -1, cat_size, helper_slot);
            break;
          case UNITY_PROTOCOL_CATEGORY_TYPE_MUSIC:
            helper_handle =
              impl->LoadFromFilename(PKGDATADIR"/emblem_music.svg", -1, cat_size, helper_slot);
            break;
          case UNITY_PROTOCOL_CATEGORY_TYPE_MOVIE:
            helper_handle =
              impl->LoadFromFilename(PKGDATADIR"/emblem_video.svg", -1, cat_size, helper_slot);
            break;
          case UNITY_PROTOCOL_CATEGORY_TYPE_CLOTHES:
          case UNITY_PROTOCOL_CATEGORY_TYPE_SHOES:
            helper_handle =
              impl->LoadFromFilename(PKGDATADIR"/emblem_clothes.svg", -1, cat_size, helper_slot);
            break;
          case UNITY_PROTOCOL_CATEGORY_TYPE_GAMES:
          case UNITY_PROTOCOL_CATEGORY_TYPE_ELECTRONICS:
          case UNITY_PROTOCOL_CATEGORY_TYPE_COMPUTERS:
          case UNITY_PROTOCOL_CATEGORY_TYPE_OFFICE:
          case UNITY_PROTOCOL_CATEGORY_TYPE_HOME:
          case UNITY_PROTOCOL_CATEGORY_TYPE_GARDEN:
          case UNITY_PROTOCOL_CATEGORY_TYPE_PETS:
          case UNITY_PROTOCOL_CATEGORY_TYPE_TOYS:
          case UNITY_PROTOCOL_CATEGORY_TYPE_CHILDREN:
          case UNITY_PROTOCOL_CATEGORY_TYPE_BABY:
          case UNITY_PROTOCOL_CATEGORY_TYPE_WATCHES:
          case UNITY_PROTOCOL_CATEGORY_TYPE_SPORTS:
          case UNITY_PROTOCOL_CATEGORY_TYPE_OUTDOORS:
          case UNITY_PROTOCOL_CATEGORY_TYPE_GROCERY:
          case UNITY_PROTOCOL_CATEGORY_TYPE_HEALTH:
          case UNITY_PROTOCOL_CATEGORY_TYPE_BEAUTY:
          case UNITY_PROTOCOL_CATEGORY_TYPE_DIY:
          case UNITY_PROTOCOL_CATEGORY_TYPE_TOOLS:
          case UNITY_PROTOCOL_CATEGORY_TYPE_CAR:
          default:
            // rest of the processing is the CategoryIconLoaded, lets invoke it
            glib::Object<GdkPixbuf> null_pixbuf;
            helper_slot("", -1, cat_size, null_pixbuf);
            break;
        }
      }
      else
      {
        result = nullptr;
        idle_id = g_idle_add(LoadIconComplete, this);
      }
    }

    void PushSchedulerJob()
    {
      ::g_io_scheduler_push_job (LoaderJobFunc, this, nullptr, G_PRIORITY_HIGH_IDLE, nullptr);
    }

    // Loading/rendering of pixbufs is done in a separate thread
    static gboolean LoaderJobFunc(GIOSchedulerJob* job, GCancellable *canc, gpointer data)
    {
      auto task = static_cast<IconLoaderTask*>(data);

      // careful here this is running in non-main thread
      if (task->icon_info)
      {
        task->result = ::gtk_icon_info_load_icon(task->icon_info, &task->error);
      }
      else if (task->type == REQUEST_TYPE_URI)
      {
        glib::Object<GFile> file(::g_file_new_for_uri(task->data.c_str()));
        glib::String contents;
        gsize length = 0;

        if (g_file_load_contents(file, canc, &contents, &length,
                                 nullptr, &task->error))
        {
          glib::Object<GInputStream> stream(
              g_memory_input_stream_new_from_data(contents.Value(), length, nullptr));

          task->result = gdk_pixbuf_new_from_stream_at_scale(stream,
                                                             task->max_width,
                                                             task->max_height,
                                                             TRUE,
                                                             canc,
                                                             &task->error);
          g_input_stream_close(stream, canc, nullptr);
        }
      }

      g_io_scheduler_job_send_to_mainloop_async (job, LoadIconComplete, task, nullptr);

      return FALSE;
    }

    // this will be invoked back in the thread from which push_job was called
    static gboolean LoadIconComplete(gpointer data)
    {
      auto task = static_cast<IconLoaderTask*>(data);
      auto impl = task->impl;

      if (task->result.IsType(GDK_TYPE_PIXBUF))
      {
        if (!task->no_cache) impl->cache_[task->key] = task->result;
      }
      else
      {
        if (task->result)
          task->result = nullptr;

        LOG_WARNING(logger) << "Unable to load icon " << task->data
                            << " at size "
                            << task->max_width << "x" << task->max_height
                            << ": " << task->error;
      }

      impl->finished_tasks_.push_back(task);

      if (!impl->coalesce_timeout_)
      {
        // we're using lower priority than the GIOSchedulerJob uses to deliver
        // results to the mainloop
        auto prio = static_cast<glib::Source::Priority>(glib::Source::Priority::DEFAULT_IDLE + 40);
        impl->coalesce_timeout_.reset(new glib::Timeout(40, prio));
        impl->coalesce_timeout_->Run(sigc::mem_fun(impl, &Impl::CoalesceTasksCb));
      }

      return FALSE;
    }
  };

  Handle ReturnCachedOrQueue(std::string const& data,
                             int max_width,
                             int max_height,
                             IconLoaderCallback slot,
                             IconLoaderRequestType type);

  Handle QueueTask(std::string const& key,
                   std::string const& data,
                   int max_width,
                   int max_height,
                   IconLoaderCallback slot,
                   IconLoaderRequestType type);

  std::string Hash(std::string const& data, int max_width, int max_height);

  bool CacheLookup(std::string const& key,
                   std::string const& data,
                   int max_width,
                   int max_height,
                   IconLoaderCallback slot);

  // Looping idle callback function
  bool Iteration();
  bool CoalesceTasksCb();

private:
  std::map<std::string, glib::Object<GdkPixbuf>> cache_;
  /* FIXME: the reference counting of IconLoaderTasks with shared pointers
   * is currently somewhat broken, and the queued_tasks_ member is what keeps
   * it from crashing randomly.
   * The IconLoader instance is assuming that it is the only owner of the loader
   * tasks, but when they are being completed in a worker thread, the thread
   * should own them as well (yet it doesn't), this could cause trouble
   * in the future... You've been warned! */
  std::map<std::string, IconLoaderTask::Ptr> queued_tasks_;
  std::queue<IconLoaderTask::Ptr> tasks_;
  std::map<Handle, IconLoaderTask::Ptr> task_map_;
  std::vector<IconLoaderTask*> finished_tasks_;

  bool no_load_;
  GtkIconTheme* theme_; // Not owned.
  Handle handle_counter_;
  glib::Source::UniquePtr idle_;
  glib::Source::UniquePtr coalesce_timeout_;
  glib::Signal<void, GtkIconTheme*> theme_changed_signal_;
};


IconLoader::Impl::Impl()
  : // Option to disable loading, if you're testing performance of other things
    no_load_(::getenv("UNITY_ICON_LOADER_DISABLE"))
  , theme_(::gtk_icon_theme_get_default())
  , handle_counter_(0)
{
  theme_changed_signal_.Connect(theme_, "changed", [&] (GtkIconTheme*) {
    /* Since the theme has been changed we can clear the cache, however we
     * could include two improvements here:
     *  1) clear only the themed icons in cache
     *  2) make the clients of this class to update their icons forcing them
     *     to reload the pixbufs and erase the cached textures, to make this
     *     apply immediately. */
    cache_.clear();
  });

  // make sure the AnnotatedIcon type is registered, so we can deserialize it
#if GLIB_CHECK_VERSION(2, 34, 0)
  g_type_ensure(unity_protocol_annotated_icon_get_type());
#else
  // we need to fool the compiler cause get_type is marked as G_GNUC_CONST,
  // which isn't exactly true
  volatile GType proto_icon = unity_protocol_annotated_icon_get_type();
  g_type_name(proto_icon);
#endif
}

int IconLoader::Impl::LoadFromIconName(std::string const& icon_name,
                                       int max_width,
                                       int max_height,
                                       IconLoaderCallback slot)
{
  if (no_load_ || icon_name.empty() || !slot ||
      ((max_width >= 0 && max_width < MIN_ICON_SIZE) ||
       (max_height >= 0 && max_height < MIN_ICON_SIZE)))
    return 0;

  // We need to check this because of legacy desktop files
  if (icon_name[0] == '/')
  {
    return LoadFromFilename(icon_name, max_width, max_height, slot);
  }

  return ReturnCachedOrQueue(icon_name, max_width, max_height, slot,
                             REQUEST_TYPE_ICON_NAME);
}

int IconLoader::Impl::LoadFromGIconString(std::string const& gicon_string,
                                          int max_width,
                                          int max_height,
                                          IconLoaderCallback slot)
{
  if (no_load_ || gicon_string.empty() || !slot ||
      ((max_width >= 0 && max_width < MIN_ICON_SIZE) ||
       (max_height >= 0 && max_height < MIN_ICON_SIZE)))
    return 0;

  return ReturnCachedOrQueue(gicon_string, max_width, max_height, slot,
                             REQUEST_TYPE_GICON_STRING);
}

int IconLoader::Impl::LoadFromFilename(std::string const& filename,
                                       int max_width,
                                       int max_height,
                                       IconLoaderCallback slot)
{
  if (no_load_ || filename.empty() || !slot ||
      ((max_width >= 0 && max_width < MIN_ICON_SIZE) ||
       (max_height >= 0 && max_height < MIN_ICON_SIZE)))
    return 0;

  glib::Object<GFile> file(::g_file_new_for_path(filename.c_str()));
  glib::String uri(::g_file_get_uri(file));

  return LoadFromURI(uri.Str(), max_width, max_height, slot);
}

int IconLoader::Impl::LoadFromURI(std::string const& uri,
                                  int max_width,
                                  int max_height,
                                  IconLoaderCallback slot)
{
  if (no_load_ || uri.empty() || !slot ||
      ((max_width >= 0 && max_width < MIN_ICON_SIZE) ||
       (max_height >= 0 && max_height < MIN_ICON_SIZE)))
    return 0;

  return ReturnCachedOrQueue(uri, max_width, max_height, slot,
                             REQUEST_TYPE_URI);
}

void IconLoader::Impl::DisconnectHandle(Handle handle)
{
  auto iter = task_map_.find(handle);

  if (iter != task_map_.end())
  {
    iter->second->slot = nullptr;
  }
}

void IconLoader::Impl::CalculateTextHeight(int* width, int* height)
{
  // FIXME: what about CJK?
  const char* const SAMPLE_MAX_TEXT = "Chromium Web Browser";
  GtkSettings* settings = gtk_settings_get_default();

  nux::CairoGraphics util_cg(CAIRO_FORMAT_ARGB32, 1, 1);
  cairo_t* cr = util_cg.GetInternalContext();

  glib::String font;
  int dpi = 0;
  g_object_get(settings,
                 "gtk-font-name", &font,
                 "gtk-xft-dpi", &dpi,
                 NULL);
  std::shared_ptr<PangoFontDescription> desc(pango_font_description_from_string(font), pango_font_description_free);
  pango_font_description_set_weight(desc.get(), PANGO_WEIGHT_BOLD);
  pango_font_description_set_size(desc.get(), FONT_SIZE * PANGO_SCALE);

  glib::Object<PangoLayout> layout(pango_cairo_create_layout(cr));
  pango_layout_set_font_description(layout, desc.get());
  pango_layout_set_text(layout, SAMPLE_MAX_TEXT, -1);

  PangoContext* cxt = pango_layout_get_context(layout);
  GdkScreen* screen = gdk_screen_get_default();
  pango_cairo_context_set_font_options(cxt, gdk_screen_get_font_options(screen));
  pango_cairo_context_set_resolution(cxt, dpi / (double) PANGO_SCALE);
  pango_layout_context_changed(layout);

  PangoRectangle log_rect;
  pango_layout_get_extents(layout, NULL, &log_rect);

  if (width) *width = log_rect.width / PANGO_SCALE;
  if (height) *height = log_rect.height / PANGO_SCALE;
}

//
// Private Methods
//

int IconLoader::Impl::ReturnCachedOrQueue(std::string const& data,
                                          int max_width,
                                          int max_height,
                                          IconLoaderCallback slot,
                                          IconLoaderRequestType type)
{
  Handle result = 0;
  std::string key(Hash(data, max_width, max_height));

  if (!CacheLookup(key, data, max_width, max_height, slot))
  {
    result = QueueTask(key, data, max_width, max_height, slot, type);
  }
  return result;
}


int IconLoader::Impl::QueueTask(std::string const& key,
                                std::string const& data,
                                int max_width,
                                int max_height,
                                IconLoaderCallback slot,
                                IconLoaderRequestType type)
{
  auto task = std::make_shared<IconLoaderTask>(type, data,
                                               max_width, max_height,
                                               key, slot,
                                               ++handle_counter_, this);
  auto iter = queued_tasks_.find(key);

  if (iter != queued_tasks_.end())
  {
    IconLoaderTask::Ptr const& running_task = iter->second;
    running_task->shadow_tasks.push_back(task);
    // do NOT push the task into the tasks queue,
    // the parent task (which is in the queue) will handle it
    task_map_[task->handle] = task;

    LOG_DEBUG(logger) << "Appending shadow task  " << data
                      << ", queue size now at " << tasks_.size();

    return task->handle;
  }
  else
  {
    queued_tasks_[key] = task;
  }

  tasks_.push(task);
  task_map_[task->handle] = task;

  LOG_DEBUG(logger) << "Pushing task  " << data << " at size " 
                    << max_width << "x" << max_height
                    << ", queue size now at " << tasks_.size();

  if (!idle_)
  {
    idle_.reset(new glib::Idle(sigc::mem_fun(this, &Impl::Iteration), glib::Source::Priority::LOW));
  }
  return task->handle;
}

std::string IconLoader::Impl::Hash(std::string const& data, int max_width, int max_height)
{
  std::ostringstream sout;
  sout << data << ":" << max_width << "x" << max_height;
  return sout.str();
}

bool IconLoader::Impl::CacheLookup(std::string const& key,
                                   std::string const& data,
                                   int max_width,
                                   int max_height,
                                   IconLoaderCallback slot)
{
  auto iter = cache_.find(key);
  bool found = iter != cache_.end();

  if (found && slot)
  {
    glib::Object<GdkPixbuf> const& pixbuf = iter->second;
    slot(data, max_width, max_height, pixbuf);
  }

  return found;
}

bool IconLoader::Impl::CoalesceTasksCb()
{
  for (auto task : finished_tasks_)
  {
    task->InvokeSlot();

    // this was all async, we need to erase the task from the task_map
    task_map_.erase(task->handle);
    queued_tasks_.erase(task->key);
  }

  finished_tasks_.clear();
  coalesce_timeout_.reset();

  return false;
}

bool IconLoader::Impl::Iteration()
{
  static const int MAX_MICRO_SECS = 1000;
  util::Timer timer;

  bool queue_empty = tasks_.empty();

  // always do at least one iteration if the queue isn't empty
  while (!queue_empty)
  {
    IconLoaderTask::Ptr const& task = tasks_.front();

    if (task->Process())
    {
      task_map_.erase(task->handle);
      queued_tasks_.erase(task->key);
    }

    tasks_.pop();
    queue_empty = tasks_.empty();

    if (timer.ElapsedMicroSeconds() >= MAX_MICRO_SECS) break;
  }

  LOG_DEBUG(logger) << "Iteration done, queue size now at " << tasks_.size();

  if (queue_empty)
  {
    if (task_map_.empty())
      handle_counter_ = 0;

    idle_.reset();
  }

  return !queue_empty;
}

IconLoader::IconLoader()
  : pimpl(new Impl())
{
}

IconLoader::~IconLoader()
{
}

IconLoader& IconLoader::GetDefault()
{
  static IconLoader default_loader;
  return default_loader;
}

int IconLoader::LoadFromIconName(std::string const& icon_name,
                                 int max_width,
                                 int max_height,
                                 IconLoaderCallback slot)
{
  return pimpl->LoadFromIconName(icon_name, max_width, max_height, slot);
}

int IconLoader::LoadFromGIconString(std::string const& gicon_string,
                                    int max_width,
                                    int max_height,
                                    IconLoaderCallback slot)
{
  return pimpl->LoadFromGIconString(gicon_string, max_width, max_height, slot);
}

int IconLoader::LoadFromFilename(std::string const& filename,
                                 int max_width,
                                 int max_height,
                                 IconLoaderCallback slot)
{
  return pimpl->LoadFromFilename(filename, max_width, max_height, slot);
}

int IconLoader::LoadFromURI(std::string const& uri,
                            int max_width,
                            int max_height,
                            IconLoaderCallback slot)
{
  return pimpl->LoadFromURI(uri, max_width, max_height, slot);
}

void IconLoader::DisconnectHandle(int handle)
{
  pimpl->DisconnectHandle(handle);
}


}
