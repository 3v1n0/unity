/*
 * Copyright 2011 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 **
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 *
 */

#include <pthread.h>
#include <NuxCore/Logger.h>
#include "UnityCore/GLibSource.h"
#include "UnityCore/DesktopUtilities.h"
#include "ThumbnailGenerator.h"
#include <glib/gstdio.h>
#include <unordered_map>

#include "TextureThumbnailProvider.h"
#include "DefaultThumbnailProvider.h"
#include "UserThumbnailProvider.h"
#include "config.h"

namespace unity
{
DECLARE_LOGGER(logger, "unity.dash.thumbnail");

namespace
{
  ThumbnailGenerator* thumbnail_instance = nullptr;

  const unsigned int CLEANUP_DURATION = 60*1000;         // 1 minute
  const unsigned int CLEANUP_PREVIEW_AGE = 6*60*60*1000; // 6 hours

  static std::multimap<std::string, std::string> thumbnail_content_map;
  static std::map<std::string, Thumbnailer::Ptr> thumbnailers_;

  pthread_mutex_t thumbnailers_mutex_ = PTHREAD_MUTEX_INITIALIZER;

  static std::string get_preview_dir()
  {
    return DesktopUtilities::GetUserDataDirectory().append("/previews");
  }
}


class Thumbnail
{
public:
  typedef std::shared_ptr<Thumbnail> Ptr;

  Thumbnail(std::string const& uri, unsigned int size, ThumbnailNotifier::Ptr const& notifier);

  std::string Generate(std::string& error_hint);

  std::string const uri_;
  unsigned int size_;
  ThumbnailNotifier::Ptr notifier_;
};

void ThumbnailNotifier::Cancel()
{
  cancel_.Cancel();
}

bool ThumbnailNotifier::IsCancelled() const
{
  return cancel_.IsCancelled();
}

class ThumbnailGeneratorImpl
{
public:
  ThumbnailGeneratorImpl(ThumbnailGenerator* parent)
  : parent_(parent)
  , thumbnails_mutex_(PTHREAD_MUTEX_INITIALIZER)
  , thumbnail_thread_is_running_(false)
  , thumbnail_thread_(0)
  {}

  ~ThumbnailGeneratorImpl()
  {
    pthread_join(thumbnail_thread_, NULL);
  }

  ThumbnailNotifier::Ptr GetThumbnail(std::string const& uri, int size);
  void DoCleanup();

  bool OnThumbnailComplete();

  static std::list<Thumbnailer::Ptr> GetThumbnailers(std::string const& content_type, std::string& error_hint);

  void RunGenerate();
  void RunManagement();

private:
  void StartCleanupTimer();

private:
  ThumbnailGenerator* parent_;

  glib::Source::UniquePtr thread_create_timer_;
  glib::Source::UniquePtr thread_return_timer_;

  /* Our mutex used when accessing data shared between the main thread and the
   thumbnail thread, i.e. the thumbnail_thread_is_running flag and the
   thumbnails_to_make list. */
  pthread_mutex_t thumbnails_mutex_;

  /* A flag to indicate whether a thumbnail thread is running, so we don't
   start more than one. Lock thumbnails_mutex when accessing this. */
  volatile bool thumbnail_thread_is_running_;
  pthread_t thumbnail_thread_;

  volatile bool management_thread_is_running_;
  pthread_t management_thread_;

  glib::Source::UniquePtr cleanup_timer_;

  std::queue<Thumbnail::Ptr> thumbnails_;

  struct CompleteThumbnail
  {
    std::string thubnail_uri;
    std::string error_hint;
    ThumbnailNotifier::Ptr notifier;
  };
  std::list<CompleteThumbnail> complete_thumbnails_;
};

static void* thumbnail_thread_start (void* data)
{
  ((ThumbnailGeneratorImpl*)data)->RunGenerate();
  return NULL;
}

bool CheckCache(std::string const& uri_in, std::string& filename_out)
{
  // Check Cache.
  filename_out = get_preview_dir() + "/";
  filename_out += std::to_string(std::hash<std::string>()(uri_in)) + ".png";

  glib::Object<GFile> cache_file(g_file_new_for_path(filename_out.c_str()));
  return g_file_query_exists(cache_file, NULL);
}

ThumbnailNotifier::Ptr ThumbnailGeneratorImpl::GetThumbnail(std::string const& uri, int size)
{
  std::string cache_filename;
  if (CheckCache(uri, cache_filename))
  {
    pthread_mutex_lock (&thumbnails_mutex_);

    CompleteThumbnail complete_thumb;
    complete_thumb.thubnail_uri = cache_filename;
    complete_thumb.notifier = std::make_shared<ThumbnailNotifier>();
    complete_thumbnails_.push_back(complete_thumb);

    // Delay the thumbnail update until after this method has returned with the notifier
    if (!thread_return_timer_)
    {
      thread_return_timer_.reset(new glib::Timeout(0, sigc::mem_fun(this, &ThumbnailGeneratorImpl::OnThumbnailComplete), glib::Source::Priority::LOW));
    }

    pthread_mutex_unlock (&thumbnails_mutex_);

    StartCleanupTimer();

    return complete_thumb.notifier;
  }

  pthread_mutex_lock (&thumbnails_mutex_);
  /*********************************
   * MUTEX LOCKED
   *********************************/

  if (!thread_create_timer_ && thumbnail_thread_is_running_ == false)
  {
    thread_create_timer_.reset(new glib::Timeout(0, [this]()
    {
      thumbnail_thread_is_running_ = true;
      pthread_create (&thumbnail_thread_, NULL, thumbnail_thread_start, this);
      thread_create_timer_.reset();
      return false;
    }, glib::Source::Priority::LOW));
  }

  auto notifier = std::make_shared<ThumbnailNotifier>();
  auto thumb = std::make_shared<Thumbnail>(uri, size, notifier);
  thumbnails_.push(thumb);

  pthread_mutex_unlock (&thumbnails_mutex_);
  /*********************************
   * MUTEX UNLOCKED
   *********************************/

  StartCleanupTimer();


  return notifier;
}

void ThumbnailGeneratorImpl::StartCleanupTimer()
{
  if (!cleanup_timer_)
      cleanup_timer_.reset(new glib::Timeout(CLEANUP_DURATION, [this]() { DoCleanup(); return false; }));
}


void ThumbnailGeneratorImpl::RunGenerate()
{
  for (;;)
  {
    /*********************************
     * MUTEX LOCKED
     *********************************/
    pthread_mutex_lock (&thumbnails_mutex_);

    if (thumbnails_.empty())
    {
      thumbnail_thread_is_running_ = FALSE;
      pthread_mutex_unlock (&thumbnails_mutex_);
      pthread_exit (NULL);
    }

    Thumbnail::Ptr thumb = thumbnails_.front();
    thumbnails_.pop();

    pthread_mutex_unlock (&thumbnails_mutex_);
    /*********************************
     * MUTEX UNLOCKED
     *********************************/

    if (thumb->notifier_->IsCancelled())
      continue;

    std::string error_hint;
    std::string uri_result = thumb->Generate(error_hint);

    /*********************************
     * MUTEX LOCKED
     *********************************/
    pthread_mutex_lock (&thumbnails_mutex_);

    CompleteThumbnail complete_thumb;
    complete_thumb.thubnail_uri = uri_result;
    complete_thumb.error_hint = error_hint;
    complete_thumb.notifier = thumb->notifier_;

    complete_thumbnails_.push_back(complete_thumb);

    if (!thread_return_timer_)
    {
      thread_return_timer_.reset(new glib::Timeout(0, sigc::mem_fun(this, &ThumbnailGeneratorImpl::OnThumbnailComplete), glib::Source::Priority::LOW));
    }

    pthread_mutex_unlock (&thumbnails_mutex_);
    /*********************************
     * MUTEX UNLOCKED
     *********************************/
   }
}

bool ThumbnailGeneratorImpl::OnThumbnailComplete()
{
  for (;;)
  {
    pthread_mutex_lock (&thumbnails_mutex_);

    if (complete_thumbnails_.empty())
    {
      thread_return_timer_.reset();
      pthread_mutex_unlock (&thumbnails_mutex_);
      return false;
    }
    CompleteThumbnail complete_thumbnail = complete_thumbnails_.front();
    complete_thumbnails_.pop_front();

    pthread_mutex_unlock (&thumbnails_mutex_);

    if (complete_thumbnail.notifier->IsCancelled())
      continue;

    if (complete_thumbnail.error_hint.empty())
      complete_thumbnail.notifier->ready.emit(complete_thumbnail.thubnail_uri);
    else
      complete_thumbnail.notifier->error.emit(complete_thumbnail.error_hint);
  }
  return false;
}

std::list<Thumbnailer::Ptr> ThumbnailGeneratorImpl::GetThumbnailers(std::string const& content_type, std::string& error_hint)
{
  std::list<Thumbnailer::Ptr> thumbnailer_list;

  gchar** content_split = g_strsplit(content_type.c_str(), "/", -1);

  std::vector<std::string> content_list;
  int i = 0;
  while(content_split[i] != NULL)
  {
    if (g_strcmp0(content_split[i], "") != 0)
      content_list.push_back(content_split[i]);
    i++;
  }

  unsigned int content_last = content_list.size();
  for (unsigned int i = 0; i < content_list.size()+1; ++i)
  {
    std::stringstream ss_content_type;
    for (unsigned int j = 0; j < content_last; ++j)
    {
      ss_content_type << content_list[j];
      if (j < content_last-1)
        ss_content_type << "/";
    }

    if (content_last == 0)
      ss_content_type << "*";
    else if (content_last < content_list.size())
      ss_content_type << "/*";

    content_last--;

    /*********************************
     * FIND THUMBNAILER
     *********************************/
    pthread_mutex_lock (&thumbnailers_mutex_);

    // have already got this content type?
    auto range = thumbnail_content_map.equal_range(ss_content_type.str());

    for (; range.first != range.second; range.first++)
    {
      // find the thumbnailer.
      auto iter_tumbnailers = thumbnailers_.find(range.first->second);
      if (iter_tumbnailers != thumbnailers_.end())
      {
        thumbnailer_list.push_back(iter_tumbnailers->second);
      }
    }
    pthread_mutex_unlock (&thumbnailers_mutex_);
  }

  return thumbnailer_list;
}

static void* management_thread_start (void* data)
{
  ((ThumbnailGeneratorImpl*)data)->RunManagement();
  return NULL;
}

void ThumbnailGeneratorImpl::DoCleanup()
{
  cleanup_timer_.reset();

  if (!management_thread_is_running_)
  {
    management_thread_is_running_ = true;
    pthread_create (&management_thread_, NULL, management_thread_start, this);
  }
}

void ThumbnailGeneratorImpl::RunManagement()
{
  guint64 time = std::time(NULL) - CLEANUP_PREVIEW_AGE;
  std::string thumbnail_folder_name = get_preview_dir();

  glib::Error err;
  GDir* thumbnailer_dir = g_dir_open(thumbnail_folder_name.c_str(), 0, &err);
  if (err)
  {
    LOG_ERROR(logger) << "Impossible to open directory: " << err;
    return;
  }

  const gchar* file_basename = NULL;;
  while ((file_basename = g_dir_read_name(thumbnailer_dir)) != NULL)
  {
    std::string filename = g_build_filename (thumbnail_folder_name.c_str(), file_basename, NULL);

    glib::Object<GFile> file(g_file_new_for_path(filename.c_str()));

    glib::Error err;
    glib::Object<GFileInfo> file_info(g_file_query_info(file, G_FILE_ATTRIBUTE_TIME_CREATED, G_FILE_QUERY_INFO_NONE, NULL, &err));
    if (err)
    {
      LOG_ERROR(logger) << "Impossible to get file info: " << err;
      return;
    }

    guint64 mtime = g_file_info_get_attribute_uint64(file_info, G_FILE_ATTRIBUTE_TIME_CREATED);

    if (mtime < time)
    {
      g_unlink(filename.c_str());
    }
  }

  thumbnail_thread_is_running_ = false;
}

ThumbnailGenerator::ThumbnailGenerator()
: pimpl(new ThumbnailGeneratorImpl(this))
{
  if (thumbnail_instance)
  {
    LOG_ERROR(logger) << "More than one thumbnail generator created.";
  }
  else
  {
    thumbnail_instance = this;

    UserThumbnailProvider::Initialise();
    TextureThumbnailProvider::Initialise();
    DefaultThumbnailProvider::Initialise();
  }

}

ThumbnailGenerator::~ThumbnailGenerator()
{
  if (this == thumbnail_instance)
    thumbnail_instance = NULL;
}

ThumbnailGenerator& ThumbnailGenerator::Instance()
{
  if (!thumbnail_instance)
  {
    LOG_ERROR(logger) << "No thumbnail generator created yet.";
  }

  return *thumbnail_instance;
}

ThumbnailNotifier::Ptr ThumbnailGenerator::GetThumbnail(std::string const& uri, int size)
{
  if (uri.empty())
    return nullptr;

  return pimpl->GetThumbnail(uri, size);
}

void ThumbnailGenerator::RegisterThumbnailer(std::list<std::string> mime_types, Thumbnailer::Ptr const& thumbnailer)
{
  pthread_mutex_lock (&thumbnailers_mutex_);

  thumbnailers_[thumbnailer->GetName()] = thumbnailer;

  for (std::string mime_type : mime_types)
  {
    thumbnail_content_map.insert(std::pair<std::string,std::string>(mime_type, thumbnailer->GetName()));
  }

  pthread_mutex_unlock (&thumbnailers_mutex_);
}

void ThumbnailGenerator::DoCleanup()
{
  pimpl->DoCleanup();
}

Thumbnail::Thumbnail(std::string const& uri, unsigned int size, ThumbnailNotifier::Ptr const& notifier)
: uri_(uri)
, size_(size)
, notifier_(notifier)
{}

std::string Thumbnail::Generate(std::string& error_hint)
{
  glib::Object<GFile> file(::g_file_new_for_uri(uri_.c_str()));

  glib::Error err;
  glib::Object<GFileInfo> file_info(g_file_query_info(file, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, G_FILE_QUERY_INFO_NONE, NULL, &err));
  if (err)
  {
    LOG_ERROR(logger) << "Could not retrieve file info for '" << uri_ << "': " << err;
    error_hint = err.Message();
    return "";
  }

  mkdir(get_preview_dir().c_str(), S_IRWXU);

  std::string file_type = g_file_info_get_attribute_string(file_info, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE);

  std::list<Thumbnailer::Ptr> const& thumbnailers = ThumbnailGeneratorImpl::GetThumbnailers(file_type, error_hint);

  std::string output_file = get_preview_dir() + "/";
  output_file += std::to_string(std::hash<std::string>()(uri_)) + ".png";

  for (Thumbnailer::Ptr const& thumbnailer : thumbnailers)
  {
    LOG_TRACE(logger) << "Attempting to generate thumbnail using '" << thumbnailer->GetName() << "' thumbnail provider";

    if (thumbnailer->Run(size_, uri_, output_file, error_hint))
    {
      error_hint.clear();
      return output_file;
    }
  }
  if (error_hint.empty())
    error_hint = "Could not find thumbnailer";
  return "";
}



} // namespace unity
