#include <list>
#include <algorithm>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <unity-mt-grab-handle.h>
#include <unity-mt-grab-handle-layout.h>
#include <unity-mt-grab-handle-group.h>
#include <unity-mt-grab-handle-impl-factory.h>
#include <unity-mt-texture.h>
#include <boost/shared_ptr.hpp>

unsigned int unity::MT::MaximizedHorzMask = (1 << 0);
unsigned int unity::MT::MaximizedVertMask = (1 << 1);
unsigned int unity::MT::MoveMask = (1 << 0);
unsigned int unity::MT::ResizeMask = (1 << 1);

using namespace unity::MT;
using ::testing::AtLeast;
using ::testing::_;

class MockGrabHandleImpl : public unity::MT::GrabHandle::Impl
{
public:
  MockGrabHandleImpl () : unity::MT::GrabHandle::Impl ()
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

class MockGrabHandleImplFactory : public unity::MT::GrabHandle::ImplFactory
{
public:
  MockGrabHandleImplFactory () : ImplFactory () {};

  virtual GrabHandle::Impl * create (const GrabHandle::Ptr &h);
};

class MockGrabHandleTexture : public unity::MT::Texture
{
public:
  typedef boost::shared_ptr <MockGrabHandleTexture> Ptr;
  MockGrabHandleTexture () : unity::MT::Texture () {};
};

class MockGrabHandleTextureFactory : public unity::MT::Texture::Factory
{
public:
  MockGrabHandleTextureFactory () : Factory () {};

  virtual unity::MT::Texture::Ptr create ();
};

class MockGrabHandleWindow : public unity::MT::GrabHandleWindow
{
public:
  MockGrabHandleWindow () : GrabHandleWindow () {};
  MOCK_METHOD4 (requestMovement, void (int, int, unsigned int, unsigned int));
  MOCK_METHOD1 (raiseGrabHandle, void (const boost::shared_ptr <const unity::MT::GrabHandle> &));
};

Texture::Ptr
MockGrabHandleTextureFactory::create ()
{
  return boost::shared_static_cast <Texture> (MockGrabHandleTexture::Ptr (new MockGrabHandleTexture ()));
}

GrabHandle::Impl *
MockGrabHandleImplFactory::create (const GrabHandle::Ptr &h)
{
  return new MockGrabHandleImpl ();
}

namespace {

// The fixture for testing class UnityMTGrabHandleTest.
class UnityMTGrabHandleTest : public ::testing::Test {
protected:

  UnityMTGrabHandleTest() :
    handlesMask (0),
    window (new MockGrabHandleWindow)
  {
    unity::MT::GrabHandle::ImplFactory::SetDefault (new MockGrabHandleImplFactory ());
    unity::MT::Texture::Factory::SetDefault (new MockGrabHandleTextureFactory ());
  }

  virtual ~UnityMTGrabHandleTest() {
  }

  virtual void SetUp() {
  }

  virtual void TearDown() {
  }

  unsigned int handlesMask;
  MockGrabHandleWindow *window;
};

TEST_F(UnityMTGrabHandleTest, TestLayoutMasks) {
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
  std::vector <TextureSize> textures;

  for (unsigned int i = 0; i < unity::MT::NUM_HANDLES; i++)
    textures.push_back (TextureSize (MockGrabHandleTextureFactory::Default ()->create (), nux::Geometry (0, 0, 100, 100)));

  GrabHandleGroup::Ptr group = GrabHandleGroup::create (window, textures);

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

}  // namespace
