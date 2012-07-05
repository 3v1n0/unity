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
#include <gtk/gtk.h>

#include "Nux/Nux.h"
#include "Nux/VLayout.h"
#include "Nux/WindowThread.h"
#include "NuxGraphics/GraphicsEngine.h"
#include <NuxCore/Logger.h>
#include <UnityCore/Variant.h>
#include <UnityCore/Preview.h>
#include <UnityCore/ApplicationPreview.h>
#include <UnityCore/MoviePreview.h>
#include <UnityCore/MusicPreview.h>
#include <UnityCore/SeriesPreview.h>
#include <unity-protocol.h>
#include "PreviewFactory.h"

#include "unity-shared/FontSettings.h"
#include "unity-shared/UnitySettings.h"
#include "unity-shared/PreviewStyle.h"

#include "Preview.h"
#include "PreviewContainer.h"


#define WIDTH 1024
#define HEIGHT 768

using namespace unity;
using namespace unity::dash;

class TestRunner
{
public:
  TestRunner ();
  ~TestRunner ();

  static void InitWindowThread (nux::NThread* thread, void* InitData);
  void Init ();

  previews::PreviewContainer::Ptr container_;
  nux::Layout *layout_;
};

TestRunner::TestRunner ()
{
}

TestRunner::~TestRunner ()
{
}

void TestRunner::Init ()
{
  container_ = new previews::PreviewContainer(NUX_TRACKER_LOCATION);
  container_->SetMinMaxSize(WIDTH, HEIGHT);

  layout_ = new nux::VLayout(NUX_TRACKER_LOCATION);
  layout_->AddView(container_.GetPointer(), 1, nux::MINOR_POSITION_CENTER);
  layout_->SetMinMaxSize(WIDTH, HEIGHT);
  
  // creates a generic preview object
  glib::Object<GIcon> icon(g_icon_new_for_string("accessories", NULL));
  glib::Object<UnityProtocolPreview> proto_obj(UNITY_PROTOCOL_PREVIEW(unity_protocol_generic_preview_new()));
  unity_protocol_preview_set_title(proto_obj, "Title");
  unity_protocol_preview_set_subtitle(proto_obj, "Subtitle");
  unity_protocol_preview_set_description(proto_obj, "Description");
  unity_protocol_preview_set_thumbnail(proto_obj, icon);
  unity_protocol_preview_add_action(proto_obj, "action1", "Action #1", NULL, 0);
  unity_protocol_preview_add_action(proto_obj, "action2", "Action #2", NULL, 0);
  unity_protocol_preview_add_info_hint(proto_obj, "hint1", "Hint 1", NULL, g_variant_new("i", 34));
  unity_protocol_preview_add_info_hint(proto_obj, "hint2", "Hint 2", NULL, g_variant_new("s", "string hint"));

  glib::Variant v(dee_serializable_serialize(DEE_SERIALIZABLE(proto_obj.RawPtr())),
                  glib::StealRef());

  container_->preview(v);

  nux::GetWindowThread()->SetLayout (layout_);
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

  nux::NuxInitialize(0);
  nux::logging::configure_logging(::getenv("UNITY_LOG_SEVERITY"));
  // The instances for the pseudo-singletons.
  unity::dash::previews::Style panel_style;
  unity::dash::PreviewFactory preview_factory;
  unity::Settings settings;

  TestRunner *test_runner = new TestRunner ();
  wt = nux::CreateGUIThread(TEXT("Unity Preview"),
                            WIDTH, HEIGHT,
                            0,
                            &TestRunner::InitWindowThread,
                            test_runner);

  wt->Run (NULL);
  delete wt;
  return 0;
}


