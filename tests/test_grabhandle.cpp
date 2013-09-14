#include <list>
#include <algorithm>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <unity-mt-grab-handle.h>
#include <unity-mt-grab-handle-layout.h>
#include <unity-mt-grab-handle-group.h>
#include <unity-mt-grab-handle-impl-factory.h>
#include <unity-mt-texture.h>
#include <memory>

unsigned int unity::MT::MaximizedHorzMask = (1 << 0);
unsigned int unity::MT::MaximizedVertMask = (1 << 1);
unsigned int unity::MT::MoveMask = (1 << 0);
unsigned int unity::MT::ResizeMask = (1 << 1);

using namespace unity::MT;
using ::testing::AtLeast;
using ::testing::_;

class MockLayoutGrabHandleImpl : public unity::MT::GrabHandle::Impl
{
public:
  MockLayoutGrabHandleImpl () : unity::MT::GrabHandle::Impl ()
  {
    EXPECT_CALL (*this, damage (_)).Times (AtLeast (3));
    EXPECT_CALL (*this, lockPosition (_, _, unity::MT::PositionSet | unity::MT::PositionLock)).Times (AtLeast (1));
  }

  MOCK_METHOD0 (show, void ());
  MOCK_METHOD0 (hide, void ());
  MOCK_CONST_METHOD3 (buttonPress, void (int, int, unsigned int));
  MOCK_METHOD3 (lockPosition, void (int, int, unsigned int));
  MOCK_METHOD1 (damage, void (const nux::Geometry &));
};

class MockLayoutGrabHandleImplFactory : public unity::MT::GrabHandle::ImplFactory
{
public:
  MockLayoutGrabHandleImplFactory () : ImplFactory () {};

  GrabHandle::Impl * create (const GrabHandle::Ptr &h);
};

class MockAnimationGrabHandleImpl : public unity::MT::GrabHandle::Impl
{
public:
  MockAnimationGrabHandleImpl () : unity::MT::GrabHandle::Impl ()
  {
    EXPECT_CALL (*this, damage (_)).Times (AtLeast (unity::MT::FADE_MSEC * 2));
    EXPECT_CALL (*this, show ()).Times (AtLeast (1));
    EXPECT_CALL (*this, hide ()).Times (AtLeast (1));
  }

  MOCK_METHOD0 (show, void ());
  MOCK_METHOD0 (hide, void ());
  MOCK_CONST_METHOD3 (buttonPress, void (int, int, unsigned int));
  MOCK_METHOD3 (lockPosition, void (int, int, unsigned int));
  MOCK_METHOD1 (damage, void (const nux::Geometry &));
};

class MockAnimationGrabHandleImplFactory : public unity::MT::GrabHandle::ImplFactory
{
public:
  MockAnimationGrabHandleImplFactory () : ImplFactory () {};

  GrabHandle::Impl * create (const GrabHandle::Ptr &h);
};

class MockShowHideGrabHandleImpl : public unity::MT::GrabHandle::Impl
{
public:
  MockShowHideGrabHandleImpl () : unity::MT::GrabHandle::Impl ()
  {
    EXPECT_CALL (*this, damage (_)).Times (AtLeast (1));
    EXPECT_CALL (*this, show ()).Times (AtLeast (2));
    EXPECT_CALL (*this, hide ()).Times (AtLeast (2));
  }

  MOCK_METHOD0 (show, void ());
  MOCK_METHOD0 (hide, void ());
  MOCK_CONST_METHOD3 (buttonPress, void (int, int, unsigned int));
  MOCK_METHOD3 (lockPosition, void (int, int, unsigned int));
  MOCK_METHOD1 (damage, void (const nux::Geometry &));
};

class MockShowHideGrabHandleImplFactory : public unity::MT::GrabHandle::ImplFactory
{
public:
  MockShowHideGrabHandleImplFactory () : ImplFactory () {};

  GrabHandle::Impl * create (const GrabHandle::Ptr &h);
};

class MockGrabHandleTexture : public unity::MT::Texture
{
public:
  typedef std::shared_ptr <MockGrabHandleTexture> Ptr;
  MockGrabHandleTexture () : unity::MT::Texture () {};
};

class MockGrabHandleTextureFactory : public unity::MT::Texture::Factory
{
public:
  MockGrabHandleTextureFactory () : Factory () {};

  unity::MT::Texture::Ptr create ();
};

class MockGrabHandleWindow : public unity::MT::GrabHandleWindow
{
public:
  MockGrabHandleWindow () : GrabHandleWindow () {};
  MOCK_METHOD4 (requestMovement, void (int, int, unsigned int, unsigned int));
  MOCK_METHOD1 (raiseGrabHandle, void (const std::shared_ptr <const unity::MT::GrabHandle> &));
};

Texture::Ptr
MockGrabHandleTextureFactory::create ()
{
  Texture::Ptr pt(static_cast<unity::MT::Texture*>(new MockGrabHandleTexture()));
  return pt;
}

GrabHandle::Impl *
MockLayoutGrabHandleImplFactory::create (const GrabHandle::Ptr &h)
{
  return new MockLayoutGrabHandleImpl ();
}

GrabHandle::Impl *
MockShowHideGrabHandleImplFactory::create (const GrabHandle::Ptr &h)
{
  return new MockShowHideGrabHandleImpl ();
}

GrabHandle::Impl *
MockAnimationGrabHandleImplFactory::create (const GrabHandle::Ptr &h)
{
  return new MockAnimationGrabHandleImpl ();
}

namespace {

// The fixture for testing class UnityMTGrabHandleTest.
class UnityMTGrabHandleTest : public ::testing::Test {
protected:

  UnityMTGrabHandleTest() :
    handlesMask(0)
  {}

  unsigned int handlesMask;
  MockGrabHandleWindow window;
};

TEST_F(UnityMTGrabHandleTest, TestLayoutMasks) {
  unity::MT::GrabHandle::ImplFactory::SetDefault (new MockLayoutGrabHandleImplFactory ());
  unity::MT::Texture::Factory::SetDefault (new MockGrabHandleTextureFactory ());

  handlesMask = unity::MT::getLayoutForMask (MaximizedVertMask, MoveMask | ResizeMask);
  EXPECT_EQ (handlesMask, LeftHandle | RightHandle | MiddleHandle);

  handlesMask = unity::MT::getLayoutForMask (MaximizedHorzMask, MoveMask | ResizeMask);
  EXPECT_EQ (handlesMask, BottomHandle | TopHandle | MiddleHandle);

  handlesMask = unity::MT::getLayoutForMask (MaximizedHorzMask | MaximizedVertMask, MoveMask | ResizeMask);
  EXPECT_EQ (handlesMask, MiddleHandle);

  handlesMask = unity::MT::getLayoutForMask (MaximizedHorzMask | MaximizedVertMask, ResizeMask);
  EXPECT_EQ (handlesMask, 0);

  handlesMask = unity::MT::getLayoutForMask (MaximizedHorzMask, ResizeMask);
  EXPECT_EQ (handlesMask, BottomHandle | TopHandle);

  handlesMask = unity::MT::getLayoutForMask (MaximizedHorzMask, ResizeMask);
  EXPECT_EQ (handlesMask, BottomHandle | TopHandle);

  handlesMask = unity::MT::getLayoutForMask (MaximizedVertMask, ResizeMask);
  EXPECT_EQ (handlesMask, RightHandle | LeftHandle);

  handlesMask = unity::MT::getLayoutForMask (0, ResizeMask);
  EXPECT_EQ (handlesMask, TopLeftHandle | TopHandle | TopRightHandle |
                          LeftHandle | RightHandle |
                          BottomLeftHandle | BottomHandle | BottomRightHandle);

  handlesMask = unity::MT::getLayoutForMask (0, ResizeMask | MoveMask);
  EXPECT_EQ (handlesMask, TopLeftHandle | TopHandle | TopRightHandle |
                          LeftHandle | RightHandle | MiddleHandle |
                          BottomLeftHandle | BottomHandle | BottomRightHandle);

  handlesMask = unity::MT::getLayoutForMask (0, MoveMask);
  EXPECT_EQ (handlesMask, MiddleHandle);

  handlesMask = unity::MT::getLayoutForMask (0, 0);
  EXPECT_EQ (handlesMask, 0);
}

TEST_F(UnityMTGrabHandleTest, TestLayouts)
{
  unity::MT::GrabHandle::ImplFactory::SetDefault (new MockLayoutGrabHandleImplFactory ());
  unity::MT::Texture::Factory::SetDefault (new MockGrabHandleTextureFactory ());

  std::vector <TextureSize> textures;

  for (unsigned int i = 0; i < unity::MT::NUM_HANDLES; i++)
    textures.push_back (TextureSize (MockGrabHandleTextureFactory::Default ()->create (), nux::Geometry (0, 0, 100, 100)));

  GrabHandleGroup::Ptr group = GrabHandleGroup::create (&window, textures);

  group->relayout (nux::Geometry (250, 250, 1000, 1000), true);

  /* Offset by x = -50, y = -50
   * since that is size / 2 */
  struct expected_positions
  {
    float x;
    float y;
  } positions[9] =
  {
    { 200, 200 },
    { 700, 200 },
    { 1200, 200 },
    { 1200, 700 },
    { 1200, 1200 },
    { 700, 1200 }, 
    { 200, 1200 },
    { 200, 700 },
    { 700, 700 }
  };

  unsigned int count = 0;

  /* Check handle positions */
  group->forEachHandle ([&](const unity::MT::GrabHandle::Ptr &h)
                        {
                          EXPECT_EQ (h->x (), positions[count].x);
                          EXPECT_EQ (h->y (), positions[count].y);
                          count++;
                        });
}

TEST_F(UnityMTGrabHandleTest, TestShowHide)
{
  unity::MT::GrabHandle::ImplFactory::SetDefault (new MockShowHideGrabHandleImplFactory ());
  unity::MT::Texture::Factory::SetDefault (new MockGrabHandleTextureFactory ());

  std::vector <TextureSize> textures;

  for (unsigned int i = 0; i < unity::MT::NUM_HANDLES; i++)
    textures.push_back (TextureSize (MockGrabHandleTextureFactory::Default ()->create (), nux::Geometry (0, 0, 100, 100)));

  GrabHandleGroup::Ptr group = GrabHandleGroup::create (&window, textures);

  group->show (0);
  group->show (TopLeftHandle | TopHandle | TopRightHandle);
  group->show (LeftHandle);
  group->show (MiddleHandle);
  group->show (RightHandle);
  group->show (BottomLeftHandle | BottomHandle | BottomRightHandle);

  group->hide ();
  group->show ();
  group->hide ();
}

TEST_F(UnityMTGrabHandleTest, TestAnimations)
{
  int opacity = 0;
  unity::MT::GrabHandle::ImplFactory::SetDefault (new MockAnimationGrabHandleImplFactory ());
  unity::MT::Texture::Factory::SetDefault (new MockGrabHandleTextureFactory ());

  unity::MT::FADE_MSEC = 10;

  std::vector <TextureSize> textures;

  for (unsigned int i = 0; i < unity::MT::NUM_HANDLES; i++)
    textures.push_back (TextureSize (MockGrabHandleTextureFactory::Default ()->create (), nux::Geometry (0, 0, 100, 100)));

  GrabHandleGroup::Ptr group = GrabHandleGroup::create (&window, textures);

  group->show ();
  for (unsigned int i = 0; i < unity::MT::FADE_MSEC; i++)
  {
    group->animate (1);
    EXPECT_TRUE (group->needsAnimate ());
    EXPECT_TRUE (group->visible ());
    opacity += ((1 /
                static_cast <float> (unity::MT::FADE_MSEC)) *
                std::numeric_limits <unsigned short>::max ());
    opacity = std::min (opacity, static_cast <int> (std::numeric_limits <unsigned short>::max ()));
    EXPECT_EQ (group->opacity (), opacity);
    group->forEachHandle ([&](const unity::MT::GrabHandle::Ptr &h)
                          {
                            h->damage (nux::Geometry (h->x (),
                                                      h->y (),
                                                      h->width (),
                                                      h->height ()));
                          });
  }

  group->animate (1);
  group->forEachHandle ([&](const unity::MT::GrabHandle::Ptr &h)
                        {
                          h->damage (nux::Geometry (h->x (),
                                                    h->y (),
                                                    h->width (),
                                                    h->height ()));
                        });
  EXPECT_FALSE (group->needsAnimate ());
  EXPECT_EQ (group->opacity (), std::numeric_limits <unsigned short>::max ());

  opacity = group->opacity ();

  group->hide ();
  for (unsigned int i = 0; i < unity::MT::FADE_MSEC - 1; i++)
  {
    group->animate (1);
    EXPECT_TRUE (group->needsAnimate ());
    EXPECT_TRUE (group->visible ());
    opacity -= ((1 /
                static_cast <float> (unity::MT::FADE_MSEC)) *
                std::numeric_limits <unsigned short>::max ());
    opacity = std::max (opacity, 0);
    EXPECT_EQ (group->opacity (), opacity);
    group->forEachHandle ([&](const unity::MT::GrabHandle::Ptr &h)
                          {
                            h->damage (nux::Geometry (h->x (),
                                                      h->y (),
                                                      h->width (),
                                                      h->height ()));
                          });
  }

  group->animate (1);
  group->forEachHandle ([&](const unity::MT::GrabHandle::Ptr &h)
                        {
                          h->damage (nux::Geometry (h->x (),
                                                    h->y (),
                                                    h->width (),
                                                    h->height ()));
                        });
  EXPECT_FALSE (group->needsAnimate ());
  EXPECT_EQ (group->opacity (), 0);

}

}  // namespace
