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
#include <UnityCore/SocialPreview.h>
#include <unity-protocol.h>

#include "unity-shared/FontSettings.h"
#include "unity-shared/UnitySettings.h"
#include "unity-shared/PreviewStyle.h"
#include "unity-shared/DashStyle.h"
#include "unity-shared/ThumbnailGenerator.h"

#include "Preview.h"
#include "PreviewContainer.h"


#define WIDTH 910 
#define HEIGHT 400

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
  container_->request_close.connect([&]() { exit(0); });

  DummyView* dummyView = new DummyView(container_.GetPointer());
  layout_ = new nux::VLayout(NUX_TRACKER_LOCATION);
  layout_->AddView(dummyView, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
  nux::GetWindowThread()->SetLayout (layout_);

  const char* title = "Candace Flynn";
  const char* subtitle = "@candacejeremyxo, 10 Aug 2012 05:01";
  const char* description = "Ugh. They made a chocolate factory!";

 // creates a generic preview object
  glib::Object<GIcon> iconHint1(g_icon_new_for_string("https://twimg0-a.akamaihd.net/profile_images/635808028/200px-JeremyHoldingCandace_reasonably_small.jpg", NULL));
  glib::Object<GIcon> iconHint2(g_icon_new_for_string("/usr/share/unity/6/lens-nav-home.svg", NULL));

  glib::Object<UnityProtocolPreview> proto_obj(UNITY_PROTOCOL_PREVIEW(unity_protocol_social_preview_new()));

  unity_protocol_social_preview_set_avatar(UNITY_PROTOCOL_SOCIAL_PREVIEW(proto_obj.RawPtr()), iconHint1);

  // DONT NEED THESE
  //unity_protocol_social_preview_set_sender(UNITY_PROTOCOL_SOCIAL_PREVIEW(proto_obj.RawPtr()), title);
  unity_protocol_social_preview_set_content(UNITY_PROTOCOL_SOCIAL_PREVIEW(proto_obj.RawPtr()), description);

  unity_protocol_preview_set_title(proto_obj, title);
  unity_protocol_preview_set_subtitle(proto_obj, subtitle);
  unity_protocol_preview_set_description(proto_obj, description);
  unity_protocol_preview_add_action(proto_obj, "view", "View", iconHint2, 0);
  unity_protocol_preview_add_action(proto_obj, "retweet", "Retweet", nullptr, 0);

  glib::Variant v(dee_serializable_serialize(DEE_SERIALIZABLE(proto_obj.RawPtr())),
              glib::StealRef());

  dash::Preview::Ptr preview_model(dash::Preview::PreviewForVariant(v));
  container_->Preview(preview_model, previews::Navigation::RIGHT);

}

void TestRunner::NavRight()
{
  const char* title = "Phineas Flynn";
  const char* subtitle = "@phineasflynn12, 10 Aug 2012 05:01";
  const char* description = "I know what we are going to do today!";

 // creates a generic preview object
  glib::Object<GIcon> iconHint1(g_icon_new_for_string("https://twimg0-a.akamaihd.net/profile_images/363231741/250px-Phineas2.jpg", NULL));
  glib::Object<GIcon> iconHint2(g_icon_new_for_string("/usr/share/unity/6/lens-nav-home.svg", NULL));

  glib::Object<UnityProtocolPreview> proto_obj(UNITY_PROTOCOL_PREVIEW(unity_protocol_social_preview_new()));

  unity_protocol_social_preview_set_avatar(UNITY_PROTOCOL_SOCIAL_PREVIEW(proto_obj.RawPtr()), iconHint1);

  unity_protocol_preview_set_title(proto_obj, title);
  unity_protocol_preview_set_subtitle(proto_obj, subtitle);
  unity_protocol_preview_set_description(proto_obj, description);
  unity_protocol_preview_add_action(proto_obj, "view", "View", iconHint2, 0);
  unity_protocol_preview_add_action(proto_obj, "retweet", "Retweet", nullptr, 0);
  unity_protocol_preview_add_info_hint(proto_obj, "likes",  "Favorites", nullptr, g_variant_new("i", 1210));
  unity_protocol_preview_add_info_hint(proto_obj, "retweets",  "Retweets", nullptr, g_variant_new("i", 21));

  glib::Variant v(dee_serializable_serialize(DEE_SERIALIZABLE(proto_obj.RawPtr())),
              glib::StealRef());

  dash::Preview::Ptr preview_model(dash::Preview::PreviewForVariant(v));
  container_->Preview(preview_model, previews::Navigation::RIGHT);
}

void TestRunner::NavLeft()
{
  const char* title = "Ferb Fletcher";
  const char* subtitle = "@ferbfletcher123, 10 Aug 2012 05:01";
  const char* description = "Profile pictures are what people want them to think they look like. Tagged pictures are what they really look like.";

 // creates a generic preview object
  glib::Object<GIcon> iconHint1(g_icon_new_for_string("https://twimg0-a.akamaihd.net/profile_images/307005344/phineas-and-ferb-300a071708_reasonably_small.jpg", NULL));
  glib::Object<GIcon> iconHint2(g_icon_new_for_string("/usr/share/unity/6/lens-nav-home.svg", NULL));

  glib::Object<UnityProtocolPreview> proto_obj(UNITY_PROTOCOL_PREVIEW(unity_protocol_social_preview_new()));

  unity_protocol_social_preview_set_avatar(UNITY_PROTOCOL_SOCIAL_PREVIEW(proto_obj.RawPtr()), iconHint1);

  unity_protocol_preview_set_title(proto_obj, title);
  unity_protocol_preview_set_subtitle(proto_obj, subtitle);
  unity_protocol_preview_set_description(proto_obj, description);
  unity_protocol_preview_add_action(proto_obj, "view", "View", iconHint2, 0);
  unity_protocol_preview_add_action(proto_obj, "retweet", "Retweet", nullptr, 0);
  unity_protocol_preview_add_info_hint(proto_obj, "likes",  "Favorites", nullptr, g_variant_new("i", 123));
  unity_protocol_preview_add_info_hint(proto_obj, "retweets",  "Retweets", nullptr, g_variant_new("i", 12));


  glib::Variant v(dee_serializable_serialize(DEE_SERIALIZABLE(proto_obj.RawPtr())),
              glib::StealRef());

  dash::Preview::Ptr preview_model(dash::Preview::PreviewForVariant(v));
  container_->Preview(preview_model, previews::Navigation::LEFT);
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


