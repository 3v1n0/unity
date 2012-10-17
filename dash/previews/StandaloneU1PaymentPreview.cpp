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
#include <UnityCore/Preview.h>
#include <unity-protocol.h>

#include "unity-shared/FontSettings.h"
#include "unity-shared/UnitySettings.h"
#include "unity-shared/PreviewStyle.h"
#include "unity-shared/DashStyle.h"
#include "unity-shared/ThumbnailGenerator.h"

#include "Preview.h"
#include "PreviewContainer.h"


#define WIDTH 1100 
#define HEIGHT 600

using namespace unity;
using namespace unity::dash;

namespace
{
nux::logging::Logger logger("unity.dash.StandaloneMusicPreview");
}

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
    layout->SetPadding(10);
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

  previews::PreviewContainer::Ptr container_;
  nux::Layout *layout_;
  unsigned int nav_iter;
  previews::Navigation nav_direction_;
  std::string search_string_;
  bool first_;
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
  container_->request_close.connect([&]() { exit(0); });
  container_->DisableNavButton(previews::Navigation::BOTH);

  DummyView* dummyView = new DummyView(container_.GetPointer());
  layout_ = new nux::VLayout(NUX_TRACKER_LOCATION);
  layout_->AddView(dummyView, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
  nux::GetWindowThread()->SetLayout (layout_);

  // create the generic preview model that will be used
  GVariant *info_hints;
  GVariantBuilder *builder = g_variant_builder_new (G_VARIANT_TYPE ("a{sv}"));

  GVariant *title = g_variant_new_string("This Modern Glitch");
  GVariant *subtitle = g_variant_new_string("The Wombats");
  GVariant *header = g_variant_new_string("Hi mandel, you purchased in the past from Ubuntu One, would you like to use the same payment details? Please review your order:");
  GVariant *email_label = g_variant_new_string("Ubuntu One email:");
  GVariant *email = g_variant_new_string("manuel@canonical.com");
  GVariant *payment_label = g_variant_new_string("Payment method:");
  GVariant *payment_method = g_variant_new_string("***** *** **** 246");
  GVariant *password_label = g_variant_new_string("Ubuntu One password:");
  GVariant *password_hint = g_variant_new_string("Password");
  GVariant *purchase_hint = g_variant_new_string("Ubuntu One Best Offer");
  GVariant *purchase_prize = g_variant_new_string("10 eur");
  GVariant *purchase_type = g_variant_new_string("Digital CD");

  g_variant_builder_add (builder, "{sv}", "title", title);
  g_variant_builder_add (builder, "{sv}", "subtitle", subtitle);
  g_variant_builder_add (builder, "{sv}", "header", header);
  g_variant_builder_add (builder, "{sv}", "email_label", email_label);
  g_variant_builder_add (builder, "{sv}", "email", email);
  g_variant_builder_add (builder, "{sv}", "payment_label", payment_label);
  g_variant_builder_add (builder, "{sv}", "payment_method", payment_method);
  g_variant_builder_add (builder, "{sv}", "password_label", password_label);
  g_variant_builder_add (builder, "{sv}", "password_hint", password_hint);
  g_variant_builder_add (builder, "{sv}", "purchase_hint", purchase_hint);
  g_variant_builder_add (builder, "{sv}", "purchase_prize", purchase_prize);
  g_variant_builder_add (builder, "{sv}", "purchase_type", purchase_type);
  info_hints = g_variant_builder_end(builder); 

  glib::Object<UnityProtocolPreview> proto_obj(UNITY_PROTOCOL_PREVIEW(
			  unity_protocol_generic_preview_new()));

  // set the title of the genric preview so that the code generates a 
  // payment preview
  unity_protocol_preview_set_title(proto_obj, U1_PAYMENT_TITLE);
  // set the info
  unity_protocol_preview_add_info_hint(proto_obj, "album_purchase_preview", "", NULL,
		  info_hints);
  // set the diff actions
  unity_protocol_preview_add_action(proto_obj, "purchase_album", "Buy Now", NULL, 0);
  unity_protocol_preview_add_action(proto_obj, "cancel_purchase", "Cancel", NULL, 0);
  unity_protocol_preview_add_action(proto_obj, "forgot_password", "forgotten your Ubuntu One password?", NULL, 0);
  unity_protocol_preview_add_action(proto_obj, "change_payment_method", "change payment method", NULL, 0);

  glib::Variant v(dee_serializable_serialize(DEE_SERIALIZABLE(proto_obj.RawPtr())),
              glib::StealRef());

  dash::Preview::Ptr preview_model(dash::Preview::PreviewForVariant(v));
  container_->Preview(preview_model, previews::Navigation::LEFT);

  g_variant_unref(info_hints);
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


