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
#include "Nux/NuxTimerTickSource.h"
#include "Nux/VLayout.h"
#include "Nux/WindowThread.h"
#include "NuxGraphics/GraphicsEngine.h"
#include <Nux/Layout.h>
#include <NuxCore/AnimationController.h>
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


const unity::RawPixel WIDTH(1000);
const unity::RawPixel HEIGHT(600);

using namespace unity;
using namespace unity::dash;

static double scale = 1.0;

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
  virtual ~DummyView() {}
  
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
  container_->scale = scale;

  DummyView* dummyView = new DummyView(container_.GetPointer());
  layout_ = new nux::VLayout(NUX_TRACKER_LOCATION);
  layout_->AddView(dummyView, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
  nux::GetWindowThread()->SetLayout (layout_);

  NavRight();
}

void TestRunner::NavRight()
{
  std::stringstream title;
  title << "Up " << ++nav_iter;

  const char* subtitle = "2009";
  const char* description = "By tying thousands of balloons to his home, 78-year-old Carl sets out to fulfill his lifelong dream to see the wilds of South America. Russell, a wilderness explorer 70 years younger, inadvertently becomes a stowaway.";

  glib::Object<UnityProtocolPreview> proto_obj(UNITY_PROTOCOL_PREVIEW(unity_protocol_movie_preview_new()));

  unity_protocol_movie_preview_set_rating(UNITY_PROTOCOL_MOVIE_PREVIEW(proto_obj.RawPtr()), 0.5);
  unity_protocol_movie_preview_set_num_ratings(UNITY_PROTOCOL_MOVIE_PREVIEW(proto_obj.RawPtr()), 17);

  unity_protocol_preview_set_image_source_uri(proto_obj, "http://ia.media-imdb.com/images/M/MV5BMTMwODg0NDY1Nl5BMl5BanBnXkFtZTcwMjkwNTgyMg@@._V1._SY317_.jpg");
  unity_protocol_preview_set_title(proto_obj, title.str().c_str());
  unity_protocol_preview_set_subtitle(proto_obj, subtitle);
  unity_protocol_preview_set_description(proto_obj, description);
  unity_protocol_preview_add_action(proto_obj, "play", "Play", NULL, 0);
  unity_protocol_preview_add_action(proto_obj, "upgradHD", "Upgrade to HD", NULL, 0);
  unity_protocol_preview_add_info_hint(proto_obj, "director", "Director", NULL, g_variant_new("s", "Steve Martino, Mike Thurmeier"));
  unity_protocol_preview_add_info_hint(proto_obj, "cast",  "Cast", NULL, g_variant_new("s", "Ray Romano, Denis Leary and John Leguizamo"));
  unity_protocol_preview_add_info_hint(proto_obj, "genre",  "Genre", NULL, g_variant_new("s", "Animation"));

  glib::Variant v(dee_serializable_serialize(DEE_SERIALIZABLE(proto_obj.RawPtr())),
              glib::StealRef());

  dash::Preview::Ptr preview_model(dash::Preview::PreviewForVariant(v));
  container_->Preview(preview_model, previews::Navigation::RIGHT);
}

void TestRunner::NavLeft()
{
  std::stringstream title;
  title << "Ice Age, Continental Drift" << --nav_iter;

  const char* subtitle = "2012, 88 min";
  const char* description = "Manny, Diego, and Sid embark upon another adventure after their continent is set adrift. Using an iceberg as a ship, they encounter sea creatures and battle pirates as they explore a new world.";

  glib::Object<UnityProtocolPreview> proto_obj(UNITY_PROTOCOL_PREVIEW(unity_protocol_movie_preview_new()));

  unity_protocol_movie_preview_set_rating(UNITY_PROTOCOL_MOVIE_PREVIEW(proto_obj.RawPtr()), 0.5);
  unity_protocol_movie_preview_set_num_ratings(UNITY_PROTOCOL_MOVIE_PREVIEW(proto_obj.RawPtr()), 17);

  unity_protocol_preview_set_image_source_uri(proto_obj, "http://ia.media-imdb.com/images/M/MV5BMTM3NDM5MzY5Ml5BMl5BanBnXkFtZTcwNjExMDUwOA@@._V1._SY317_.jpg");
  unity_protocol_preview_set_title(proto_obj, title.str().c_str());
  unity_protocol_preview_set_subtitle(proto_obj, subtitle);
  unity_protocol_preview_set_description(proto_obj, description);
  unity_protocol_preview_add_action(proto_obj, "play", "Play", NULL, 0);
  unity_protocol_preview_add_action(proto_obj, "upgradHD", "Upgrade to HD", NULL, 0);
  unity_protocol_preview_add_info_hint(proto_obj, "director", "Director", NULL, g_variant_new("s", "Steve Martino, Mike Thurmeier"));
  unity_protocol_preview_add_info_hint(proto_obj, "cast",  "Cast", NULL, g_variant_new("s", "Ray Romano, Denis Leary and John Leguizamo"));
  unity_protocol_preview_add_info_hint(proto_obj, "genre",  "Genre", NULL, g_variant_new("s", "Animation"));

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
  unity::glib::Error err;

  GOptionEntry args_parsed[] =
  {
    { "scaling-factor", 's', 0, G_OPTION_ARG_DOUBLE, &scale, "The dash scaling factor", "F" },
    { NULL }
  };

  std::shared_ptr<GOptionContext> ctx(g_option_context_new("Unity Preview"), g_option_context_free);
  g_option_context_add_main_entries(ctx.get(), args_parsed, NULL);
  if (!g_option_context_parse(ctx.get(), &argc, &argv, &err))
    std::cerr << "Got error when parsing arguments: " << err << std::endl;

  TestRunner *test_runner = new TestRunner ();
  wt = nux::CreateGUIThread(TEXT("Unity Preview"),
                            WIDTH.CP(scale), HEIGHT.CP(scale),
                            0,
                            &TestRunner::InitWindowThread,
                            test_runner);

  nux::NuxTimerTickSource tick_source;
  nux::animation::AnimationController animation_controller(tick_source);

  wt->Run (NULL);
  delete wt;
  return 0;
}


