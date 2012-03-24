#include <list>
#include <algorithm>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <UnityShowdesktopHandler.h>

using namespace unity;
using ::testing::_;
using ::testing::Return;

compiz::WindowInputRemoverInterface::~WindowInputRemoverInterface () {}

class MockWindowInputRemover :
  public compiz::WindowInputRemoverInterface
{
  public:

    MockWindowInputRemover ()
    {
      ON_CALL (*this, saveInput ()).WillByDefault (Return (true));
      ON_CALL (*this, removeInput ()).WillByDefault (Return (true));
      ON_CALL (*this, restoreInput ()).WillByDefault (Return (true));
    }

    MOCK_METHOD0 (saveInput, bool ());
    MOCK_METHOD0 (removeInput, bool ());
    MOCK_METHOD0 (restoreInput, bool ());
};


class MockUnityShowdesktopHandlerWindow :
  public UnityShowdesktopHandlerWindowInterface
{
  public:

    MockUnityShowdesktopHandlerWindow ()
    {
      ON_CALL (*this, isOverrideRedirect ()).WillByDefault (Return (false));
      ON_CALL (*this, isManaged ()).WillByDefault (Return (true));
      ON_CALL (*this, isGrabbed ()).WillByDefault (Return (false));
      ON_CALL (*this, isDesktopOrDock ()).WillByDefault (Return (false));
      ON_CALL (*this, isSkipTaskbarOrPager ()).WillByDefault (Return (false));
      ON_CALL (*this, isHidden ()).WillByDefault (Return (false));
      ON_CALL (*this, isInShowdesktopMode ()).WillByDefault (Return (false));
      ON_CALL (*this, isShaded ()).WillByDefault (Return (false));
      ON_CALL (*this, isMinimized ()).WillByDefault (Return (false));

      ON_CALL (*this, doHandleAnimations (_)).WillByDefault (Return (UnityShowdesktopHandlerWindowInterface::PostPaintAction::Damage));
      ON_CALL (*this, getNoCoreInstanceMask ()).WillByDefault (Return (1));
      ON_CALL (*this, getInputRemover ()).WillByDefault (Return (new MockWindowInputRemover ()));
    }

    MOCK_METHOD0 (doEnableFocus, void ());
    MOCK_METHOD0 (doDisableFocus, void ());
    MOCK_METHOD0 (isOverrideRedirect, bool ());
    MOCK_METHOD0 (isManaged, bool ());
    MOCK_METHOD0 (isGrabbed, bool ());
    MOCK_METHOD0 (isDesktopOrDock, bool ());
    MOCK_METHOD0 (isSkipTaskbarOrPager, bool ());
    MOCK_METHOD0 (isHidden, bool ());
    MOCK_METHOD0 (isInShowdesktopMode, bool ());
    MOCK_METHOD0 (isShaded, bool ());
    MOCK_METHOD0 (isMinimized, bool ());
    MOCK_METHOD1 (doOverrideFrameRegion, void (CompRegion &));
    MOCK_METHOD0 (doHide, void ());
    MOCK_METHOD0 (doNotifyHidden, void ());
    MOCK_METHOD0 (doShow, void ());
    MOCK_METHOD0 (doNotifyShown, void ());
    MOCK_METHOD0 (doMoveFocusAway, void ());
    MOCK_METHOD1 (doHandleAnimations, UnityShowdesktopHandlerWindowInterface::PostPaintAction (unsigned int));
    MOCK_METHOD0 (doAddDamage, void ());
    MOCK_METHOD0 (getNoCoreInstanceMask, unsigned int ());
    MOCK_METHOD0 (getInputRemover, compiz::WindowInputRemoverInterface * ());
    MOCK_METHOD0 (doDeleteHandler, void ());
};

class UnityShowdesktopHandlerTest :
  public ::testing::Test
{
};

TEST_F(UnityShowdesktopHandlerTest, TestNoORWindowsSD)
{
  MockUnityShowdesktopHandlerWindow mMockWindow;

  EXPECT_CALL (mMockWindow, getInputRemover ());

  UnityShowdesktopHandler mMockHandler (static_cast <UnityShowdesktopHandlerWindowInterface *> (&mMockWindow));

  EXPECT_CALL (mMockWindow, isOverrideRedirect ()).WillOnce (Return (true));
  EXPECT_FALSE (UnityShowdesktopHandler::shouldHide (&mMockWindow));
}
