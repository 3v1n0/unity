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
const unsigned MIN_ICON_SIZE = 2;
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
                          unsigned size,
                          IconLoaderCallback slot);

  Handle LoadFromGIconString(std::string const& gicon_string,
                             unsigned size,
                             IconLoaderCallback slot);

  Handle LoadFromFilename(std::string const& filename,
                          unsigned size,
                          IconLoaderCallback slot);

  Handle LoadFromURI(std::string const& uri,
                     unsigned size,
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
    unsigned int size;
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

    IconLoaderTask(IconLoaderRequestType type_,
                   std::string const& data_,
                   unsigned size_,
                   std::string const& key_,
                   IconLoaderCallback slot_,
                   Handle handle_,
                   Impl* self_)
      : type(type_), data(data_), size(size_), key(key_)
      , slot(slot_), handle(handle_), impl(self_)
      , icon_info(nullptr), no_cache(false), helper_handle(0)
      {}

    ~IconLoaderTask()
    {
      if (icon_info)
        ::gtk_icon_info_free(icon_info);
      if (helper_handle != 0)
        impl->DisconnectHandle(helper_handle);
    }

    void InvokeSlot()
    {
      if (slot)
        slot(data, size, result);

      // notify shadow tasks
      for (auto shadow_task : shadow_tasks)
      {
        if (shadow_task->slot)
          shadow_task->slot(shadow_task->data, shadow_task->size, result);

        impl->task_map_.erase(shadow_task->handle);
      }

      shadow_tasks.clear();
    }

    bool Process()
    {
      // Check the cache again, as previous tasks might have wanted the same
      if (impl->CacheLookup(key, data, size, slot))
        return true;

      LOG_DEBUG(logger) << "Processing  " << data << " at size " << size;

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
                          << " " << size << ")";
      result = nullptr;
      InvokeSlot();

      return true;
    }

    bool ProcessIconNameTask()
    {
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

      if (icon.IsType(UNITY_PROTOCOL_TYPE_ANNOTATED_ICON))
      {
        UnityProtocolAnnotatedIcon *anno;
        anno = UNITY_PROTOCOL_ANNOTATED_ICON(icon.RawPtr());
        GIcon* base_icon = unity_protocol_annotated_icon_get_icon(anno);
        glib::String gicon_string(g_icon_to_string(base_icon));

        no_cache = true;
        auto helper_slot = sigc::bind(sigc::mem_fun(this, &IconLoaderTask::BaseIconLoaded), glib::object_cast<UnityProtocolAnnotatedIcon>(icon));
        helper_handle = impl->LoadFromGIconString(gicon_string.Str(),
                                                  size, helper_slot);

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

    void CategoryIconLoaded(std::string const& base_icon_string, unsigned size,
                            glib::Object<GdkPixbuf> const& category_pixbuf,
                            glib::Object<UnityProtocolAnnotatedIcon> const& anno_icon)
    {
      helper_handle = 0;
      if (category_pixbuf)
      {
        // assuming the category pixbuf is smaller than result
        gdk_pixbuf_composite(category_pixbuf, result, // src, dest
                             0, 0, // dest_x, dest_y
                             gdk_pixbuf_get_width(category_pixbuf), // dest_w
                             gdk_pixbuf_get_height(category_pixbuf), // dest_h
                             0.0, 0.0, // offset_x, offset_y
                             1.0, 1.0, // scale_x, scale_y
                             GDK_INTERP_NEAREST, // interpolation
                             255); // src_alpha
      }

      const gchar* detail_text = unity_protocol_annotated_icon_get_ribbon(anno_icon);
      if (detail_text)
      {
        int icon_w = gdk_pixbuf_get_width(result);
        int icon_h = gdk_pixbuf_get_height(result);

        int max_font_height;
        CalculateTextHeight(nullptr, &max_font_height);

        max_font_height = max_font_height * 9 / 8; // let's have some padding on the stripe
        int pixbuf_size = static_cast<int>(
            sqrt(max_font_height*max_font_height*8));
        if (pixbuf_size > icon_w) pixbuf_size = icon_w;

        nux::CairoGraphics cairo_graphics(CAIRO_FORMAT_ARGB32,
                                          pixbuf_size, pixbuf_size);
        cairo_t* cr = cairo_graphics.GetContext();

        glib::Object<PangoLayout> layout;
        PangoFontDescription* desc = NULL;
        PangoContext* pango_context = NULL;
        GdkScreen* screen = gdk_screen_get_default(); // not ref'ed
        glib::String font;
        int dpi = -1;

        g_object_get(gtk_settings_get_default(), "gtk-font-name", &font, NULL);
        g_object_get(gtk_settings_get_default(), "gtk-xft-dpi", &dpi, NULL);
        cairo_set_font_options(cr, gdk_screen_get_font_options(screen));
        layout = pango_cairo_create_layout(cr);
        desc = pango_font_description_from_string(font);
        pango_font_description_set_weight(desc, PANGO_WEIGHT_BOLD);
        int font_size = FONT_SIZE;
        pango_font_description_set_size (desc, font_size * PANGO_SCALE);

        pango_layout_set_font_description(layout, desc);
        pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);

        double size_dbl = static_cast<double>(pixbuf_size);
        // we'll allow tiny bit of overflow since the text is rotated and there
        // is some space left... FIXME: 10/9? / 11/10?
        double max_text_width = sqrt(size_dbl*size_dbl / 2) * 9/8;

        pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
        pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);

        glib::String escaped_text(g_markup_escape_text(detail_text, -1));
        pango_layout_set_markup(layout, escaped_text, -1);

        pango_context = pango_layout_get_context(layout);  // is not ref'ed
        // FIXME: for reasons unknown, it looks better without this
        //pango_cairo_context_set_font_options(pango_context,
        //                                     gdk_screen_get_font_options(screen));
        pango_cairo_context_set_resolution(pango_context,
                                           dpi == -1 ? 96.0f : dpi/(float) PANGO_SCALE);
        pango_layout_context_changed(layout);

        // find proper font size (can we do this before the rotation?)
        int text_width, text_height;
        pango_layout_get_pixel_size(layout, &text_width, nullptr);
        while (text_width > max_text_width && font_size > MIN_FONT_SIZE)
        {
          font_size--;
          pango_font_description_set_size (desc, font_size * PANGO_SCALE);
          pango_layout_set_font_description(layout, desc);
          pango_layout_get_pixel_size(layout, &text_width, nullptr);
        }
        pango_layout_set_width(layout, static_cast<int>(max_text_width * PANGO_SCALE));

        cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
        cairo_paint(cr);

        cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

        // draw the trapezoid
        cairo_move_to(cr, 0.0, size_dbl);
        cairo_line_to(cr, size_dbl, 0.0);
        cairo_line_to(cr, size_dbl, size_dbl / 2.0);
        cairo_line_to(cr, size_dbl / 2.0, size_dbl);
        cairo_close_path(cr);

        cairo_set_source_rgba(cr, 0.196f, 0.086f, 0.1607f, 0.8825f);
        cairo_fill(cr);

        // draw the text (rotated!)
        cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 1.0f);
        cairo_move_to(cr, size_dbl * 0.25, size_dbl);
        cairo_rotate(cr, -G_PI_4); // rotate by -45 degrees

        pango_cairo_update_layout(cr, layout);
        pango_layout_get_pixel_size(layout, nullptr, &text_height);
        // current point is now in the middle of the stripe, need to translate
        // it, so that the text is centered
        cairo_rel_move_to(cr, 0.0, text_height / -2.0);
        double diagonal = sqrt(size_dbl*size_dbl*2);
        // x coordinate also needs to be shifted
        cairo_rel_move_to(cr, (diagonal - max_text_width) / 4, 0.0);
        pango_cairo_show_layout(cr, layout);

        // clean up
        pango_font_description_free(desc);
        cairo_destroy(cr);

        // FIXME: going from image_surface to pixbuf, and then to texture :(
        glib::Object<GdkPixbuf> detail_pb(
            gdk_pixbuf_get_from_surface(cairo_graphics.GetSurface(),
                                        0, 0,
                                        cairo_graphics.GetWidth(),
                                        cairo_graphics.GetHeight()));

        gdk_pixbuf_composite(detail_pb, result, // src, dest
                             icon_w - pixbuf_size, // dest_x
                             icon_h - pixbuf_size, // dest_y
                             pixbuf_size, // dest_w
                             pixbuf_size, // dest_h
                             icon_w - pixbuf_size, // offset_x
                             icon_h - pixbuf_size, // offset_y
                             1.0, 1.0, // scale_x, scale_y
                             GDK_INTERP_NEAREST, // interpolation
                             255); // src_alpha
      }

      g_idle_add(LoadIconComplete, this);
    }

    void BaseIconLoaded(std::string const& base_icon_string, unsigned size,
                        glib::Object<GdkPixbuf> const& base_pixbuf,
                        glib::Object<UnityProtocolAnnotatedIcon> const& anno_icon)
    {
      helper_handle = 0;
      if (base_pixbuf)
      {
        result = gdk_pixbuf_copy(base_pixbuf);
        // FIXME: can we composite the pixbuf in helper thread?
        UnityProtocolCategoryType category = unity_protocol_annotated_icon_get_category(anno_icon);
        auto helper_slot = sigc::bind(sigc::mem_fun(this, &IconLoaderTask::CategoryIconLoaded), anno_icon);
        unsigned cat_size = size / 4;
        // FIXME: where to find category assets?
        switch (category)
        {
          case UNITY_PROTOCOL_CATEGORY_TYPE_BOOK:
            helper_handle =
              impl->LoadFromIconName("emblem-favorite", cat_size, helper_slot);
            break;
          case UNITY_PROTOCOL_CATEGORY_TYPE_SONG:
            helper_handle =
              impl->LoadFromIconName("emblem-favorite", cat_size, helper_slot);
            break;
          case UNITY_PROTOCOL_CATEGORY_TYPE_MOVIE:
            helper_handle =
              impl->LoadFromIconName("emblem-favorite", cat_size, helper_slot);
            break;
          default:
            // rest of the processing is the CategoryIconLoaded, lets invoke it
            glib::Object<GdkPixbuf> null_pixbuf;
            helper_slot("", cat_size, null_pixbuf);
            break;
        }
      }
      else
      {
        result = nullptr;
        g_idle_add(LoadIconComplete, this);
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

        if (::g_file_load_contents(file, canc, &contents, &length,
                                 nullptr, &task->error))
        {
          glib::Object<GInputStream> stream(
              ::g_memory_input_stream_new_from_data(contents.Value(), length, nullptr));

          task->result = ::gdk_pixbuf_new_from_stream_at_scale(stream,
                                                               -1,
                                                               task->size,
                                                               TRUE,
                                                               canc,
                                                               &task->error);
          ::g_input_stream_close(stream, canc, nullptr);
        }
      }

      ::g_io_scheduler_job_send_to_mainloop_async (job, LoadIconComplete, task, nullptr);

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
                            << " at size " << task->size << ": " << task->error;
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
                             unsigned size,
                             IconLoaderCallback slot,
                             IconLoaderRequestType type);

  Handle QueueTask(std::string const& key,
                   std::string const& data,
                   unsigned size,
                   IconLoaderCallback slot,
                   IconLoaderRequestType type);

  std::string Hash(std::string const& data, unsigned size);

  bool CacheLookup(std::string const& key,
                   std::string const& data,
                   unsigned size,
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
}

int IconLoader::Impl::LoadFromIconName(std::string const& icon_name,
                                       unsigned size,
                                       IconLoaderCallback slot)
{
  if (no_load_ || icon_name.empty() || size < MIN_ICON_SIZE || !slot)
    return 0;

  // We need to check this because of legacy desktop files
  if (icon_name[0] == '/')
  {
    return LoadFromFilename(icon_name, size, slot);
  }

  return ReturnCachedOrQueue(icon_name, size, slot, REQUEST_TYPE_ICON_NAME);
}

int IconLoader::Impl::LoadFromGIconString(std::string const& gicon_string,
                                          unsigned size,
                                          IconLoaderCallback slot)
{
  if (no_load_ || gicon_string.empty() || size < MIN_ICON_SIZE || !slot)
    return 0;

  return ReturnCachedOrQueue(gicon_string, size, slot, REQUEST_TYPE_GICON_STRING);
}

int IconLoader::Impl::LoadFromFilename(std::string const& filename,
                                       unsigned size,
                                       IconLoaderCallback slot)
{
  if (no_load_ || filename.empty() || size < MIN_ICON_SIZE || !slot)
    return 0;

  glib::Object<GFile> file(::g_file_new_for_path(filename.c_str()));
  glib::String uri(::g_file_get_uri(file));

  return LoadFromURI(uri.Str(), size, slot);
}

int IconLoader::Impl::LoadFromURI(std::string const& uri,
                                  unsigned size,
                                  IconLoaderCallback slot)
{
  if (no_load_ || uri.empty() || size < MIN_ICON_SIZE || !slot)
    return 0;

  return ReturnCachedOrQueue(uri, size, slot, REQUEST_TYPE_URI);
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
  PangoFontDescription* desc = pango_font_description_from_string(font);
  pango_font_description_set_weight(desc, PANGO_WEIGHT_BOLD);
  pango_font_description_set_size(desc, FONT_SIZE * PANGO_SCALE);

  glib::Object<PangoLayout> layout(pango_cairo_create_layout(cr));
  pango_layout_set_font_description(layout, desc);
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

  pango_font_description_free(desc);
}

//
// Private Methods
//

int IconLoader::Impl::ReturnCachedOrQueue(std::string const& data,
                                          unsigned size,
                                          IconLoaderCallback slot,
                                          IconLoaderRequestType type)
{
  Handle result = 0;
  std::string key(Hash(data, size));

  if (!CacheLookup(key, data, size, slot))
  {
    result = QueueTask(key, data, size, slot, type);
  }
  return result;
}


int IconLoader::Impl::QueueTask(std::string const& key,
                                std::string const& data,
                                unsigned size,
                                IconLoaderCallback slot,
                                IconLoaderRequestType type)
{
  auto task = std::make_shared<IconLoaderTask>(type, data, size, key, slot, ++handle_counter_, this);
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

  LOG_DEBUG(logger) << "Pushing task  " << data << " at size " << size
                    << ", queue size now at " << tasks_.size();

  if (!idle_)
  {
    idle_.reset(new glib::Idle(sigc::mem_fun(this, &Impl::Iteration), glib::Source::Priority::LOW));
  }
  return task->handle;
}

std::string IconLoader::Impl::Hash(std::string const& data, unsigned size)
{
  std::ostringstream sout;
  sout << data << ":" << size;
  return sout.str();
}

bool IconLoader::Impl::CacheLookup(std::string const& key,
                                   std::string const& data,
                                   unsigned size,
                                   IconLoaderCallback slot)
{
  auto iter = cache_.find(key);
  bool found = iter != cache_.end();

  if (found && slot)
  {
    glib::Object<GdkPixbuf> const& pixbuf = iter->second;
    slot(data, size, pixbuf);
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
                                 unsigned size,
                                 IconLoaderCallback slot)
{
  return pimpl->LoadFromIconName(icon_name, size, slot);
}

int IconLoader::LoadFromGIconString(std::string const& gicon_string,
                                    unsigned size,
                                    IconLoaderCallback slot)
{
  return pimpl->LoadFromGIconString(gicon_string, size, slot);
}

int IconLoader::LoadFromFilename(std::string const& filename,
                                 unsigned size,
                                 IconLoaderCallback slot)
{
  return pimpl->LoadFromFilename(filename, size, slot);
}

int IconLoader::LoadFromURI(std::string const& uri,
                            unsigned size,
                            IconLoaderCallback slot)
{
  return pimpl->LoadFromURI(uri, size, slot);
}

void IconLoader::DisconnectHandle(int handle)
{
  pimpl->DisconnectHandle(handle);
}


}
