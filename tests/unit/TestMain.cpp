/*
 * Copyright 2010 Canonical Ltd.
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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */

#include "config.h"
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include "Nux/Nux.h"
#include "Nux/WindowThread.h"

void TestPanelServiceCreateSuite();
void TestUBusCreateSuite();
void TestStaticCairoTextCreateSuite();

nux::WindowThread*
createThread()
{
  nux::WindowThread* thread = NULL;

  nux::NuxInitialize(0);
  thread = nux::CreateGUIThread(TEXT("Unit-Test Dummy Window"),
                                320,
                                240,
                                0,
                                NULL,
                                0);
  return thread;
}

void
runThread(nux::WindowThread* thread)
{
  thread->Run(NULL);
}

void
stopThread(nux::WindowThread* thread)
{
  thread->ExitMainLoop();
  delete thread;
}

int
main(int argc, char** argv)
{
  g_setenv("GSETTINGS_SCHEMA_DIR", BUILDDIR"/settings/", TRUE);

  g_type_init();
  
  gtk_init(&argc, &argv);

  g_test_init(&argc, &argv, NULL);

  //Keep alphabetical please
  TestPanelServiceCreateSuite();
  TestStaticCairoTextCreateSuite();
  TestUBusCreateSuite();

  nux::WindowThread* thread = createThread();

  int ret = g_test_run();

  stopThread(thread);

  return ret;
}

