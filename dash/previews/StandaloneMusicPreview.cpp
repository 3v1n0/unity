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
#include "LensDBusTestRunner.h"


#define WIDTH 972
#define HEIGHT 452

using namespace unity;
using namespace unity::dash;

DECLARE_LOGGER(logger, "unity.dash.preview.standalone");

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

class TestRunner :  public previews::ScopeDBusTestRunner 
{
public:
  TestRunner(std::string const& search_string);
  ~TestRunner ();

  static void InitWindowThread (nux::NThread* thread, void* InitData);
  void Init ();
  void NavRight();
  void NavLeft();

  previews::PreviewContainer::Ptr container_;
  nux::Layout *layout_;
  unsigned int nav_iter;
  previews::Navigation nav_direction_;
  std::string search_string_;
  bool first_;
};

TestRunner::TestRunner (std::string const& search_string)
: ScopeDBusTestRunner("com.canonical.Unity.Scope.Music","/com/canonical/unity/scope/music", "com.canonical.Unity.Scope")
, search_string_(search_string)
, first_(true)
{
  nav_iter = 0;
  nav_direction_ = previews::Navigation::RIGHT;

  connected.connect([this](bool connected) {
    if (connected)
    {
      Search(search_string_);
    } 
  });

  results_->result_added.connect([this](Result const& result)
  {
    previews::Navigation navDisabled =  previews::Navigation::BOTH;
    if (nav_iter < results_->count.Get() - 1)
      navDisabled = previews::Navigation( static_cast<unsigned>(navDisabled) & ~static_cast<unsigned>(previews::Navigation::RIGHT));
    if (results_->count.Get() > 0 && nav_iter > 0)
      navDisabled = previews::Navigation( static_cast<unsigned>(navDisabled) & ~static_cast<unsigned>(previews::Navigation::LEFT));

    if (first_)
    {
      first_ = false;
      Preview(result.uri);
    }

    container_->DisableNavButton(navDisabled);
  });
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

  container_->DisableNavButton(previews::Navigation::BOTH);
  
  preview_ready.connect([this](std::string const& uri, dash::Preview::Ptr preview_model)
  {
    container_->Preview(preview_model, nav_direction_);
  });
}

void TestRunner::NavRight()
{
  nav_direction_ = previews::Navigation::RIGHT;
  Result result = results_->RowAtIndex(++nav_iter);
  LOG_DEBUG(logger) << "Preview: " << result.uri.Get();
  Preview(result.uri);

  previews::Navigation navDisabled =  previews::Navigation::BOTH;
  if (nav_iter < results_->count.Get() - 1)
    navDisabled = previews::Navigation( static_cast<unsigned>(navDisabled) & ~static_cast<unsigned>(previews::Navigation::RIGHT));
  if (results_->count.Get() > 0 && nav_iter > 0)
    navDisabled = previews::Navigation( static_cast<unsigned>(navDisabled) & ~static_cast<unsigned>(previews::Navigation::LEFT));

  container_->DisableNavButton(navDisabled);
}

void TestRunner::NavLeft()
{
  nav_direction_ = previews::Navigation::LEFT;
  Result result = results_->RowAtIndex(--nav_iter);
  LOG_DEBUG(logger) << "Preview: " << result.uri.Get();
  Preview(result.uri);

  previews::Navigation navDisabled =  previews::Navigation::BOTH;
  if (nav_iter < results_->count.Get() - 1)
    navDisabled = previews::Navigation( static_cast<unsigned>(navDisabled) & ~static_cast<unsigned>(previews::Navigation::RIGHT));
  if (results_->count.Get() > 0 && nav_iter > 0)
    navDisabled = previews::Navigation( static_cast<unsigned>(navDisabled) & ~static_cast<unsigned>(previews::Navigation::LEFT));

  container_->DisableNavButton(navDisabled);
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
    printf("Usage: music_previews SEARCH_STRING\n");
    return 1;
  }

  nux::NuxInitialize(0);
  nux::logging::configure_logging(::getenv("UNITY_LOG_SEVERITY"));
  // The instances for the pseudo-singletons.
  unity::Settings settings;
  unity::dash::previews::Style panel_style;
  unity::dash::Style dash_style;
  unity::ThumbnailGenerator thumbnail_generator;

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


