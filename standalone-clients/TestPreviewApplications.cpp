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
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 *
 */

#include "Nux/Nux.h"
#include "Nux/VLayout.h"
#include "Nux/Button.h"
#include "Nux/TextureArea.h"
#include "Nux/WindowThread.h"
#include "NuxGraphics/GraphicsEngine.h"
#include <gtk/gtk.h>

#include "PreviewBasicButton.h"
#include "PreviewApplications.h"
#include "DashStyle.h"

class TestRunner
{
public:
  TestRunner ();
  ~TestRunner ();

  static void InitWindowThread (nux::NThread* thread, void* InitData);
  void Init ();
  nux::Layout *layout;

private:

};

TestRunner::TestRunner ()
{
}

TestRunner::~TestRunner ()
{
}

void TestRunner::Init ()
{
  unity::dash::Preview::Properties properties;
  unity::dash::ApplicationPreview* application_preview = new unity::dash::ApplicationPreview (properties);

  application_preview->name = "Shotwell";
  application_preview->version = "Version 0.9.3";
  application_preview->size = "Size 56 MB";
  application_preview->license = "Open Source (GPL)";
  application_preview->last_updated = "Last updated 11th Apr 2011";
  application_preview->rating = 0.5;
  application_preview->n_ratings = 17;
  application_preview->description = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Aliquam nec fringilla ligula. Mauris posuere tempor fermentum. Maecenas aliquet elementum orci, porta molestie mauris blandit nec. Fusce ac tellus eu elit porta placerat ut ut nulla. Sed viverra magna id nibh egestas sodales. Fusce ut massa leo, a varius massa. Vivamus venenatis pretium diam. Nam dictum eleifend pulvinar. Aenean consequat diam sed enim mollis non adipiscing odio commodo. Donec orci lorem, viverra in convallis eget, volutpat nec tellus. Nulla at lobortis odio. Pellentesque porta, ante id convallis tincidunt, tellus nunc feugiat massa, sit amet porta mauris turpis ac enim. Integer porttitor bibendum justo, ut posuere mi commodo non. Nullam volutpat eros vel magna congue non lobortis urna pulvinar. Vivamus quis ante leo. Fusce mauris tellus, tincidunt quis rutrum vitae, feugiat venenatis nulla. Sed quis lorem vitae augue luctus sodales. Proin egestas nibh quam. Nulla cursus erat arcu, et mollis tortor. Maecenas ut scelerisque leo.\n\nNulla varius, neque at laoreet sodales, mauris risus ultricies risus, at convallis neque purus sodales purus. Vestibulum non ligula sit amet dui suscipit egestas. Aliquam lobortis tellus nec est pretium non accumsan turpis bibendum. Nunc condimentum dolor urna. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos. Suspendisse eros ante, rutrum sed tincidunt id, tempor vel arcu. Nullam in dui leo. Mauris orci metus, convallis vel cursus eget, mattis ut erat. Praesent condimentum metus cursus nulla feugiat eleifend. Fusce mattis congue sapien quis sagittis. Phasellus volutpat condimentum mauris, a rutrum enim pretium id. Phasellus non laoreet est. Mauris et dictum leo. Nunc interdum lacinia elit posuere imperdiet. In hac habitasse platea dictumst. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Donec venenatis dolor et nisl faucibus vitae tristique erat vehicula. Proin vitae neque non magna tempus ultrices id fermentum lorem. Proin sodales mauris nisl.\n\nDuis tempus, dui quis viverra vehicula, massa tellus pulvinar libero, vitae varius tortor velit vel turpis. Aliquam ac massa lectus, ut ultrices turpis. Sed et mi nisl, id laoreet mi. Aenean et felis est. Sed ut libero ipsum, a tincidunt nibh. Integer viverra dictum urna, eget convallis est accumsan et. Praesent posuere orci vel nibh iaculis congue. Proin eget congue dolor. Aliquam mattis laoreet tortor, sit amet porttitor turpis fermentum at. Nulla iaculis dignissim laoreet. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Integer tincidunt tristique egestas. Phasellus ut leo nec neque fringilla vestibulum. Donec id facilisis elit.\n\nVivamus ut vehicula mi. Curabitur est velit, laoreet sodales ornare eget, semper sed felis. Pellentesque nisi velit, faucibus vitae auctor sed, lacinia id leo. Quisque mattis laoreet nisi ut facilisis. Nulla lacus ligula, dictum a malesuada eu, dictum vitae lacus. In a ante sodales dui semper vestibulum mattis non lectus. Sed ligula risus, porttitor eget ultricies ac, imperdiet at quam. Vivamus vulputate, sapien nec congue semper, velit augue bibendum purus, et sollicitudin velit tellus sed mauris. Praesent non purus id odio vehicula facilisis. Integer bibendum porta felis eget suscipit. Mauris elementum suscipit ante, at sagittis libero pharetra sed. Nulla non lectus diam. Vestibulum nisi mi, tincidunt quis convallis a, elementum et justo. Curabitur odio est, fringilla vel consectetur eget, placerat nec risus.\n\nDuis ac lectus enim. Donec interdum enim vulputate dui adipiscing iaculis. Duis sit amet consectetur sapien. Etiam ut leo sed lectus interdum vestibulum et vel mauris. Vestibulum vitae neque vel mi rhoncus condimentum ut vitae libero. Proin pulvinar pretium mi sit amet fermentum. Quisque consectetur felis ac libero molestie interdum. Aliquam erat volutpat. Proin venenatis est sed metus volutpat id egestas turpis congue. Nulla euismod massa euismod ante gravida porttitor. Aenean ultricies fermentum sem vehicula aliquam. Donec mollis augue id ligula sagittis et tempus eros euismod. Etiam euismod tempus velit, non varius purus eleifend sit amet. Fusce sed nunc vitae quam tempus porttitor vel a nibh. Suspendisse potenti. Praesent facilisis justo id est ullamcorper accumsan. Ut elementum, dolor ut commodo aliquam, sem libero elementum tellus, nec iaculis ipsum turpis et sem. ";
  application_preview->icon_hint = "firefox.png";
  application_preview->screenshot_icon_hint = "file:///usr/share/backgrounds/warty-final-ubuntu.png";
  application_preview->primary_action_name = "Launch";
  application_preview->primary_action_icon_hint = "";
  application_preview->primary_action_uri = "/notreal/buthey/starwars/is/pretty/great/beep";

  unity::dash::Preview::Ptr preview = unity::dash::Preview::Ptr(application_preview);
  unity::PreviewApplications* preview_view = new unity::PreviewApplications (preview, NUX_TRACKER_LOCATION);

  layout = new nux::VLayout(NUX_TRACKER_LOCATION);

  layout->AddView (preview_view, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);

  nux::GetWindowThread()->SetLayout (layout);
}

void TestRunner::InitWindowThread(nux::NThread* thread, void* InitData)
{
  TestRunner *self =  (TestRunner *) InitData;
  self->Init ();
}

void
ControlThread (nux::NThread* thread,
               void*         data)
{
  // sleep for 3 seconds
  nux::SleepForMilliseconds (3000);
  printf ("ControlThread successfully started\n");
}


int main(int argc, char **argv)
{
  nux::SystemThread* st = NULL;
  nux::WindowThread* wt = NULL;

  // no real tests right now, just make sure we don't get any criticals and such
  // waiting on nice perceptual diff support before we can build real tests
  // for views

  g_type_init ();
  
  gtk_init (&argc, &argv);

  nux::NuxInitialize(0);

  // The instances for the pseudo-singletons.
  unity::dash::Style dash_style;

  TestRunner *test_runner = new TestRunner ();
  wt = nux::CreateGUIThread(TEXT("Unity Places Tile Test"),
                            800 , 600,
                            0,
                            &TestRunner::InitWindowThread,
                            test_runner);

  st = nux::CreateSystemThread (NULL, ControlThread, wt);

  if (st)
    st->Start (NULL);

  wt->Run (NULL);
  delete st;
  delete wt;
  return 0;
}
