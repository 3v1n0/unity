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


#define WIDTH 972
#define HEIGHT 452

using namespace unity;
using namespace unity::dash;

namespace
{
nux::logging::Logger logger("unity.dash.StandaloneThumbnailer");
}


class TestRunner :  public previews::DBusTestRunner 
{
public:
  TestRunner(std::string const& search_string);
  ~TestRunner ();

  static void InitWindowThread (nux::NThread* thread, void* InitData);
  void Init ();
};

TestRunner::TestRunner (std::string const& search_string)
: DBusTestRunner("org.freedesktop.thumbnails.Thumbnailer1","/org/freedesktop/thumbnails/Thumbnailer1", "org.freedesktop.thumbnails")
{
}

TestRunner::~TestRunner ()
{
}

void TestRunner::Init ()
{
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
    printf("Usage: music_previews SEARCH_STRING");
    return 1;
  }

  nux::NuxInitialize(0);
  nux::logging::configure_logging(::getenv("UNITY_LOG_SEVERITY"));
  nux::logging::Logger("unity").SetLogLevel(nux::logging::Trace);

  TestRunner *test_runner = new TestRunner (argv[1]);
  nux::SystemThread* test_thread = nux::CreateSystemThread(NULL, &TestRunner::InitWindowThread,
                            test_runner);

  test_thread->Start (NULL);
  delete wt;
  return 0;
}


