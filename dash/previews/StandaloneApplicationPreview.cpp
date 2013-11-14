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
#include "Nux/WindowThread.h"
#include "NuxGraphics/GraphicsEngine.h"
#include <Nux/Layout.h>
#include <NuxCore/Logger.h>
#include <UnityCore/Variant.h>
#include <UnityCore/ApplicationPreview.h>
#include <unity-protocol.h>

#include "unity-shared/FontSettings.h"
#include "unity-shared/UnitySettings.h"
#include "unity-shared/PreviewStyle.h"
#include "unity-shared/DashStyle.h"
#include "unity-shared/ThumbnailGenerator.h"

#include "Preview.h"
#include "PreviewContainer.h"


#define WIDTH 972
#define HEIGHT 452

using namespace unity;
using namespace unity::dash;

class DummyView : public nux::View
{
public:
  DummyView(nux::View* view)
  : View(NUX_TRACKER_LOCATION)
  {
    SetAcceptKeyNavFocusOnMouseDown(false);
    SetAcceptKeyNavFocusOnMouseEnter(false);

    nux::ROPConfig rop;
    rop.Blend = true;
    rop.SrcBlend = GL_ONE;
    rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
    bg_layer_.reset(new nux::ColorLayer(nux::Color(81, 26, 48), true, rop));

    nux::Layout* layout = new nux::VLayout();
    layout->SetPadding(16);
    layout->AddView(view, 1, nux::MINOR_POSITION_CENTER);
    SetLayout(layout);
  }
  
  // Keyboard navigation
  bool AcceptKeyNavFocus()
  {
    return false;
  }

protected:
  virtual void Draw(nux::GraphicsEngine& gfx_engine, bool force_draw)
  {
    nux::Geometry const& base = GetGeometry();

    gfx_engine.PushClippingRectangle(base);
    nux::GetPainter().PaintBackground(gfx_engine, base);

    unsigned int alpha, src, dest = 0;
    gfx_engine.GetRenderStates().GetBlend(alpha, src, dest);
    gfx_engine.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    bg_layer_->SetGeometry(GetGeometry());
    nux::GetPainter().RenderSinglePaintLayer(gfx_engine, GetGeometry(), bg_layer_.get());

    gfx_engine.GetRenderStates().SetBlend(alpha, src, dest);

    gfx_engine.PopClippingRectangle();
  }

  virtual void DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw)
  {
    nux::Geometry const& base = GetGeometry();
    gfx_engine.PushClippingRectangle(base);

    if (!IsFullRedraw())
      nux::GetPainter().PushLayer(gfx_engine, GetGeometry(), bg_layer_.get());

    if (GetCompositionLayout())
      GetCompositionLayout()->ProcessDraw(gfx_engine, force_draw);

    if (!IsFullRedraw())
      nux::GetPainter().PopBackground();

    gfx_engine.PopClippingRectangle();
  }

   typedef std::unique_ptr<nux::AbstractPaintLayer> LayerPtr;
  LayerPtr bg_layer_;
};

class TestRunner
{
public:
  TestRunner ();
  ~TestRunner ();

  static void InitWindowThread (nux::NThread* thread, void* InitData);
  void Init ();
  void NavRight();
  void NavLeft();

  previews::PreviewContainer::Ptr container_;
  nux::Layout *layout_;
  int nav_iter;
  glib::Source::UniquePtr preview_wait_timer_;
};

TestRunner::TestRunner ()
{
  nav_iter = 0;
}

TestRunner::~TestRunner ()
{
}

void TestRunner::Init ()
{
  container_ = new previews::PreviewContainer(NUX_TRACKER_LOCATION);
  container_->navigate_right.connect(sigc::mem_fun(this, &TestRunner::NavRight));
  container_->navigate_left.connect(sigc::mem_fun(this, &TestRunner::NavLeft));
  container_->request_close.connect([this]() { exit(0); });

  DummyView* dummyView = new DummyView(container_.GetPointer());
  layout_ = new nux::VLayout(NUX_TRACKER_LOCATION);
  layout_->AddView(dummyView, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
  nux::GetWindowThread()->SetLayout (layout_);


  std::stringstream app_name;
  app_name << "Skype";

  const char* subtitle = "Version 3.2, Size 32 MB";
  std::stringstream description;
  for (int i = 0; i < 700; i++)
    description << "Application description " << i << std::endl;

 // creates a generic preview object
  glib::Object<GIcon> iconHint1(g_icon_new_for_string("/usr/share/unity/5/lens-nav-music.svg", NULL));
  glib::Object<GIcon> iconHint2(g_icon_new_for_string("/usr/share/unity/5/lens-nav-home.svg", NULL));
  glib::Object<GIcon> iconHint3(g_icon_new_for_string("/usr/share/unity/5/lens-nav-people.svg", NULL));

  GHashTable* action_hints1(g_hash_table_new(g_direct_hash, g_direct_equal));
  g_hash_table_insert (action_hints1, g_strdup ("extra-text"), g_variant_new_string("£30.99"));

  glib::Object<UnityProtocolPreview> proto_obj(UNITY_PROTOCOL_PREVIEW(unity_protocol_application_preview_new()));

  unity_protocol_application_preview_set_app_icon(UNITY_PROTOCOL_APPLICATION_PREVIEW(proto_obj.RawPtr()), g_icon_new_for_string("/home/nick/SkypeIcon.png", NULL));
  unity_protocol_application_preview_set_license(UNITY_PROTOCOL_APPLICATION_PREVIEW(proto_obj.RawPtr()), "Proprietary");
  unity_protocol_application_preview_set_copyright(UNITY_PROTOCOL_APPLICATION_PREVIEW(proto_obj.RawPtr()), "(c) Skype 2012");
  unity_protocol_application_preview_set_last_update(UNITY_PROTOCOL_APPLICATION_PREVIEW(proto_obj.RawPtr()), "11th Apr 2012");
  unity_protocol_application_preview_set_rating(UNITY_PROTOCOL_APPLICATION_PREVIEW(proto_obj.RawPtr()), 0.5);
  unity_protocol_application_preview_set_num_ratings(UNITY_PROTOCOL_APPLICATION_PREVIEW(proto_obj.RawPtr()), 17);

  unity_protocol_preview_set_image_source_uri(proto_obj, "file:///home/nick/Skype.png");
  unity_protocol_preview_set_title(proto_obj, app_name.str().c_str());
  unity_protocol_preview_set_subtitle(proto_obj, subtitle);
  unity_protocol_preview_set_description(proto_obj, description.str().c_str());
  unity_protocol_preview_add_action(proto_obj, "uninstall", "Uninstall", iconHint1, 0);
  unity_protocol_preview_add_action_with_hints(proto_obj, "launch", "Download", iconHint2, 0, action_hints1);
  unity_protocol_preview_add_info_hint(proto_obj, "time", "Total time", iconHint1, g_variant_new("s", "16 h 34miin 45sec"));
  unity_protocol_preview_add_info_hint(proto_obj, "energy",  "Energy", iconHint2, g_variant_new("s", "58.07 mWh"));
  unity_protocol_preview_add_info_hint(proto_obj, "load",  "CPU Load", iconHint3, g_variant_new("i", 12));

  glib::Variant v(dee_serializable_serialize(DEE_SERIALIZABLE(proto_obj.RawPtr())),
              glib::StealRef());

  dash::Preview::Ptr preview_model(dash::Preview::PreviewForVariant(v));
  container_->Preview(preview_model, previews::Navigation::RIGHT);

  g_hash_table_unref(action_hints1);
}

void TestRunner::NavRight()
{
  std::stringstream app_name;
  app_name << "Title " << ++nav_iter;

  const char* subtitle = "Version 3.2, Size 32 MB";
  const char* description = "Skype is a proprietary voice-over-Internet Protocol service and software application originally created by Niklas Zennström and Janus Friis in 2003, and owned by Microsoft since 2011. \
The service allows users to communicate with peers by voice, video, and instant messaging over the Internet. Phone calls may be placed to recipients on the traditional telephone networks. Calls to other users within the Skype service are free of charge, while calls to landline telephones and mobile phones are charged via a debit-based user account system.";

 // creates a generic preview object
  glib::Object<GIcon> iconHint1(g_icon_new_for_string("/usr/share/unity/5/lens-nav-music.svg", NULL));
  glib::Object<GIcon> iconHint2(g_icon_new_for_string("/usr/share/unity/5/lens-nav-home.svg", NULL));
  glib::Object<GIcon> iconHint3(g_icon_new_for_string("/usr/share/unity/5/lens-nav-people.svg", NULL));

  GHashTable* action_hints1(g_hash_table_new(g_direct_hash, g_direct_equal));
  g_hash_table_insert (action_hints1, g_strdup ("extra-text"), g_variant_new_string("£30.99"));

  glib::Object<UnityProtocolPreview> proto_obj(UNITY_PROTOCOL_PREVIEW(unity_protocol_application_preview_new()));

  unity_protocol_application_preview_set_app_icon(UNITY_PROTOCOL_APPLICATION_PREVIEW(proto_obj.RawPtr()), g_icon_new_for_string("/home/nick/SkypeIcon.png", NULL));
  unity_protocol_application_preview_set_license(UNITY_PROTOCOL_APPLICATION_PREVIEW(proto_obj.RawPtr()), "Proprietary");
  unity_protocol_application_preview_set_copyright(UNITY_PROTOCOL_APPLICATION_PREVIEW(proto_obj.RawPtr()), "(c) Skype 2012");
  unity_protocol_application_preview_set_last_update(UNITY_PROTOCOL_APPLICATION_PREVIEW(proto_obj.RawPtr()), "11th Apr 2012");
  unity_protocol_application_preview_set_rating(UNITY_PROTOCOL_APPLICATION_PREVIEW(proto_obj.RawPtr()), 0.25);
  unity_protocol_application_preview_set_num_ratings(UNITY_PROTOCOL_APPLICATION_PREVIEW(proto_obj.RawPtr()), 5);

  unity_protocol_preview_set_image_source_uri(proto_obj, "file:///home/nick/Skype.png");
  unity_protocol_preview_set_title(proto_obj, app_name.str().c_str());
  unity_protocol_preview_set_subtitle(proto_obj, subtitle);
  unity_protocol_preview_set_description(proto_obj, description);
  unity_protocol_preview_add_action(proto_obj, "uninstall", "Uninstall", iconHint1, 0);
  unity_protocol_preview_add_action_with_hints(proto_obj, "launch", "Download", iconHint2, 0, action_hints1);
  unity_protocol_preview_add_info_hint(proto_obj, "time", "Total time", iconHint1, g_variant_new("s", "16 h 34miin 45sec"));
  unity_protocol_preview_add_info_hint(proto_obj, "energy",  "Energy", iconHint2, g_variant_new("s", "58.07 mWh"));
  unity_protocol_preview_add_info_hint(proto_obj, "load",  "CPU Load", iconHint3, g_variant_new("d", 12.1));

  glib::Variant v(dee_serializable_serialize(DEE_SERIALIZABLE(proto_obj.RawPtr())),
              glib::StealRef());

  dash::Preview::Ptr preview_model(dash::Preview::PreviewForVariant(v));
  container_->Preview(preview_model, previews::Navigation::RIGHT);

  g_hash_table_unref(action_hints1);
}

void TestRunner::NavLeft()
{
  preview_wait_timer_.reset(new glib::Timeout(2000, [this] () {


    std::stringstream app_name;
    app_name << "Title " << --nav_iter;

    const char* subtitle = "Version 3.2, Size 32 MB";
    const char* description = "Skype is a proprietary voice-over-Internet Protocol service and software application originally created by Niklas Zennström and Janus Friis in 2003, and owned by Microsoft since 2011. \
  The service allows users to communicate with peers by voice, video, and instant messaging over the Internet. Phone calls may be placed to recipients on the traditional telephone networks. Calls to other users within the Skype service are free of charge, while calls to landline telephones and mobile phones are charged via a debit-based user account system.";

   // creates a generic preview object
    glib::Object<GIcon> iconHint1(g_icon_new_for_string("/usr/share/unity/5/lens-nav-music.svg", NULL));
    glib::Object<GIcon> iconHint2(g_icon_new_for_string("/usr/share/unity/5/lens-nav-home.svg", NULL));
    glib::Object<GIcon> iconHint3(g_icon_new_for_string("/usr/share/unity/5/lens-nav-people.svg", NULL));

    GHashTable* action_hints1(g_hash_table_new(g_direct_hash, g_direct_equal));
  g_hash_table_insert (action_hints1, g_strdup ("extra-text"), g_variant_new_string("£30.99"));

  glib::Object<UnityProtocolPreview> proto_obj(UNITY_PROTOCOL_PREVIEW(unity_protocol_application_preview_new()));


    unity_protocol_application_preview_set_app_icon(UNITY_PROTOCOL_APPLICATION_PREVIEW(proto_obj.RawPtr()), g_icon_new_for_string("/home/nick/SkypeIcon.png", NULL));
    unity_protocol_application_preview_set_license(UNITY_PROTOCOL_APPLICATION_PREVIEW(proto_obj.RawPtr()), "Proprietary");
    unity_protocol_application_preview_set_copyright(UNITY_PROTOCOL_APPLICATION_PREVIEW(proto_obj.RawPtr()), "(c) Skype 2012");
    unity_protocol_application_preview_set_last_update(UNITY_PROTOCOL_APPLICATION_PREVIEW(proto_obj.RawPtr()), "11th Apr 2012");
    unity_protocol_application_preview_set_rating(UNITY_PROTOCOL_APPLICATION_PREVIEW(proto_obj.RawPtr()), 0.8);
    unity_protocol_application_preview_set_num_ratings(UNITY_PROTOCOL_APPLICATION_PREVIEW(proto_obj.RawPtr()), 1223);

    unity_protocol_preview_set_image_source_uri(proto_obj, "file:///home/nick/Skype.png");
    unity_protocol_preview_set_title(proto_obj, app_name.str().c_str());
    unity_protocol_preview_set_subtitle(proto_obj, subtitle);
    unity_protocol_preview_set_description(proto_obj, description);
    unity_protocol_preview_add_action(proto_obj, "uninstall", "Uninstall", iconHint1, 0);
    unity_protocol_preview_add_action_with_hints(proto_obj, "launch", "Download", iconHint2, 0, action_hints1);
    unity_protocol_preview_add_info_hint(proto_obj, "time", "Total time", iconHint1, g_variant_new("s", "16 h 34miin 45sec"));
    unity_protocol_preview_add_info_hint(proto_obj, "energy",  "Energy", iconHint2, g_variant_new("s", "58.07 mWh"));
    unity_protocol_preview_add_info_hint(proto_obj, "load",  "CPU Load", iconHint3, g_variant_new("i", 22));

    glib::Variant v(dee_serializable_serialize(DEE_SERIALIZABLE(proto_obj.RawPtr())),
                glib::StealRef());

    dash::Preview::Ptr preview_model(dash::Preview::PreviewForVariant(v));
    container_->Preview(preview_model, previews::Navigation::LEFT);

    g_hash_table_unref(action_hints1);

    return false;
  }));
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
  unity::Settings settings;
  unity::dash::previews::Style panel_style;
  unity::dash::Style dash_style;
  unity::ThumbnailGenerator thumbnail_generator;

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


