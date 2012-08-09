/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 *
 */
#include <gtk/gtk.h>

#include "Nux/Nux.h"
#include "Nux/VLayout.h"
#include "Nux/SystemThread.h"
#include <Nux/Layout.h>
#include <NuxCore/Logger.h>
#include <UnityCore/Variant.h>

#include "DBusTestRunner.h"


#define WIDTH 500
#define HEIGHT 500

using namespace unity;
using namespace unity::dash;

namespace
{
nux::logging::Logger logger("unity.dash.StandaloneThumbnailer");
}


class TestRunner :  public previews::DBusTestRunner 
{
public:
  TestRunner(std::string const& thumnail_uri);
  ~TestRunner ();

  static void InitWindowThread (nux::NThread* thread, void* InitData);
  void Init ();

  void OnProxyConnectionChanged();

  void OnGetSupported(GVariant* parameters);
  void OnThumbnailReady(GVariant* parameters);
  void OnThumbnailStarted(GVariant* parameters);
  void OnThumbnailFinished(GVariant* parameters);
  void OnThumbnailError(GVariant* parameters);

  std::string thumnail_uri_;
};

TestRunner::TestRunner (std::string const& thumnail_uri)
: DBusTestRunner("org.freedesktop.thumbnails.Thumbnailer1","/org/freedesktop/thumbnails/Thumbnailer1", "org.freedesktop.thumbnails.Thumbnailer1")
, thumnail_uri_(thumnail_uri)
{
    proxy_->Connect("Ready", sigc::mem_fun(this, &TestRunner::OnThumbnailReady));
    proxy_->Connect("Started", sigc::mem_fun(this, &TestRunner::OnThumbnailStarted));
    proxy_->Connect("Finished", sigc::mem_fun(this, &TestRunner::OnThumbnailFinished));
    proxy_->Connect("Error", sigc::mem_fun(this, &TestRunner::OnThumbnailError));
}

TestRunner::~TestRunner ()
{
}

void TestRunner::Init ()
{
}

void TestRunner::OnProxyConnectionChanged()
{
  DBusTestRunner::OnProxyConnectionChanged();

  if (proxy_->IsConnected())
  {
    GVariantBuilder *builder_uri = g_variant_builder_new (G_VARIANT_TYPE ("as"));
    g_variant_builder_add (builder_uri, "s", thumnail_uri_.c_str());
    GVariantBuilder *builder_mime = g_variant_builder_new (G_VARIANT_TYPE ("as"));
    g_variant_builder_add (builder_mime, "s", "video/ogg");
    GVariant *value = g_variant_new ("(asasssu)", builder_uri, builder_mime, "xxlarge", "default", 0);

    printf("type: %s\n", g_variant_get_type_string(value));

    g_variant_builder_unref (builder_uri);
    g_variant_builder_unref (builder_mime);

    proxy_->Call("Queue", value,
          sigc::mem_fun(this, &TestRunner::OnGetSupported), NULL);
  }
}

void TestRunner::OnThumbnailReady(GVariant* parameters)
{
  LOG_INFO(logger) << "Thumbnail ready";

  GVariantIter *thumbnail_iter;
  guint handle;
  g_variant_get(parameters, "(uas)", &handle, &thumbnail_iter);

  gchar *str;
  while (g_variant_iter_loop (thumbnail_iter, "s", &str))
    g_print ("thumbnail @ %s\n", str);
  g_variant_iter_free (thumbnail_iter);
}

void TestRunner::OnThumbnailStarted(GVariant* parameters)
{
  LOG_INFO(logger) << "Thumbnail started";
}

void TestRunner::OnThumbnailFinished(GVariant* parameters)
{
  LOG_INFO(logger) << "Thumbnail finished";
}

void TestRunner::OnThumbnailError(GVariant* parameters)
{
  GVariantIter *thumbnail_iter;
  guint handle;
  gint error_code;
  glib::String message;
  g_variant_get(parameters, "(uasis)", &handle, &thumbnail_iter, &error_code, &message);
  g_variant_iter_free (thumbnail_iter);


  LOG_INFO(logger) << "Thumbnail error: " << message;
}

void TestRunner::OnGetSupported(GVariant* parameters)
{
  LOG_INFO(logger) << "OnGetSupported return";

  printf("return type: %s\n", g_variant_get_type_string(parameters));
}


void TestRunner::InitWindowThread(nux::NThread* thread, void* InitData)
{
  TestRunner *self =  (TestRunner *) InitData;
  self->Init ();
}

int main(int argc, char **argv)
{
  nux::WindowThread* wt = NULL;

  gtk_init (&argc, &argv);

  if (argc < 2)
  {
    printf("Usage: thumbnailer URI");
    return 1;
  }

  nux::NuxInitialize(0);
  nux::logging::configure_logging(::getenv("UNITY_LOG_SEVERITY"));
  nux::logging::Logger("unity").SetLogLevel(nux::logging::Trace);

  TestRunner *test_runner = new TestRunner (argv[1]);
  wt = nux::CreateGUIThread(TEXT("Unity Preview"),
                            WIDTH, HEIGHT,
                            0,
                            &TestRunner::InitWindowThread,
                            test_runner);

  wt->Run (NULL);
  delete wt;
  return 0;
}


