#include <list>
#include <algorithm>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <UnityShowdesktopHandler.h>

using namespace unity;
using ::testing::_;
using ::testing::Return;
using ::testing::InSequence;

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
public:

  ~UnityShowdesktopHandlerTest ()
  {
    UnityShowdesktopHandler::animating_windows.clear ();
  }
};

TEST_F(UnityShowdesktopHandlerTest, TestNoORWindowsSD)
{
  MockUnityShowdesktopHandlerWindow mMockWindow;

  EXPECT_CALL (mMockWindow, getInputRemover ());

  UnityShowdesktopHandler mMockHandler (static_cast <UnityShowdesktopHandlerWindowInterface *> (&mMockWindow));

  EXPECT_CALL (mMockWindow, isOverrideRedirect ()).WillOnce (Return (true));
  EXPECT_FALSE (UnityShowdesktopHandler::shouldHide (&mMockWindow));
}

TEST_F(UnityShowdesktopHandlerTest, TestNoUnmanagedWindowsSD)
{
  MockUnityShowdesktopHandlerWindow mMockWindow;

  EXPECT_CALL (mMockWindow, getInputRemover ());

  UnityShowdesktopHandler mMockHandler (static_cast <UnityShowdesktopHandlerWindowInterface *> (&mMockWindow));

  EXPECT_CALL (mMockWindow, isOverrideRedirect ());
  EXPECT_CALL (mMockWindow, isManaged ()).WillOnce (Return (false));
  EXPECT_FALSE (UnityShowdesktopHandler::shouldHide (&mMockWindow));
}

TEST_F(UnityShowdesktopHandlerTest, TestNoGrabbedWindowsSD)
{
  MockUnityShowdesktopHandlerWindow mMockWindow;

  EXPECT_CALL (mMockWindow, getInputRemover ());

  UnityShowdesktopHandler mMockHandler (static_cast <UnityShowdesktopHandlerWindowInterface *> (&mMockWindow));

  EXPECT_CALL (mMockWindow, isOverrideRedirect ());
  EXPECT_CALL (mMockWindow, isManaged ());
  EXPECT_CALL (mMockWindow, isGrabbed ()).WillOnce (Return (true));
  EXPECT_FALSE (UnityShowdesktopHandler::shouldHide (&mMockWindow));
}

TEST_F(UnityShowdesktopHandlerTest, TestNoDesktopOrDockWindowsSD)
{
  MockUnityShowdesktopHandlerWindow mMockWindow;

  EXPECT_CALL (mMockWindow, getInputRemover ());

  UnityShowdesktopHandler mMockHandler (static_cast <UnityShowdesktopHandlerWindowInterface *> (&mMockWindow));

  EXPECT_CALL (mMockWindow, isOverrideRedirect ());
  EXPECT_CALL (mMockWindow, isManaged ());
  EXPECT_CALL (mMockWindow, isGrabbed ());
  EXPECT_CALL (mMockWindow, isDesktopOrDock ()).WillOnce (Return (true));
  EXPECT_FALSE (UnityShowdesktopHandler::shouldHide (&mMockWindow));
}

TEST_F(UnityShowdesktopHandlerTest, TestNoSkipTaskbarOrPagerWindowsSD)
{
  MockUnityShowdesktopHandlerWindow mMockWindow;

  EXPECT_CALL (mMockWindow, getInputRemover ());

  UnityShowdesktopHandler mMockHandler (static_cast <UnityShowdesktopHandlerWindowInterface *> (&mMockWindow));

  EXPECT_CALL (mMockWindow, isOverrideRedirect ());
  EXPECT_CALL (mMockWindow, isManaged ());
  EXPECT_CALL (mMockWindow, isGrabbed ());
  EXPECT_CALL (mMockWindow, isDesktopOrDock ());
  EXPECT_CALL (mMockWindow, isSkipTaskbarOrPager ()).WillOnce (Return (true));
  EXPECT_FALSE (UnityShowdesktopHandler::shouldHide (&mMockWindow));
}

TEST_F(UnityShowdesktopHandlerTest, TestHiddenNotSDAndShadedWindowsNoSD)
{
  MockUnityShowdesktopHandlerWindow mMockWindow;

  EXPECT_CALL (mMockWindow, getInputRemover ());

  UnityShowdesktopHandler mMockHandler (static_cast <UnityShowdesktopHandlerWindowInterface *> (&mMockWindow));

  EXPECT_CALL (mMockWindow, isOverrideRedirect ());
  EXPECT_CALL (mMockWindow, isManaged ());
  EXPECT_CALL (mMockWindow, isGrabbed ());
  EXPECT_CALL (mMockWindow, isDesktopOrDock ());
  EXPECT_CALL (mMockWindow, isSkipTaskbarOrPager ());
  EXPECT_CALL (mMockWindow, isHidden ()).WillOnce (Return (true));
  EXPECT_CALL (mMockWindow, isInShowdesktopMode ()).WillOnce (Return (false));
  EXPECT_CALL (mMockWindow, isShaded ()).WillOnce (Return (true));
  EXPECT_FALSE (UnityShowdesktopHandler::shouldHide (&mMockWindow));
}

TEST_F(UnityShowdesktopHandlerTest, TestHiddenSDAndShadedWindowsNoSD)
{
  MockUnityShowdesktopHandlerWindow mMockWindow;

  EXPECT_CALL (mMockWindow, getInputRemover ());

  UnityShowdesktopHandler mMockHandler (static_cast <UnityShowdesktopHandlerWindowInterface *> (&mMockWindow));

  EXPECT_CALL (mMockWindow, isOverrideRedirect ());
  EXPECT_CALL (mMockWindow, isManaged ());
  EXPECT_CALL (mMockWindow, isGrabbed ());
  EXPECT_CALL (mMockWindow, isDesktopOrDock ());
  EXPECT_CALL (mMockWindow, isSkipTaskbarOrPager ());
  EXPECT_CALL (mMockWindow, isHidden ()).WillOnce (Return (true));
  EXPECT_CALL (mMockWindow, isInShowdesktopMode ()).WillOnce (Return (true));
  EXPECT_FALSE (UnityShowdesktopHandler::shouldHide (&mMockWindow));
}

TEST_F(UnityShowdesktopHandlerTest, TestHiddenNotSDAndNotShadedWindowsSD)
{
  MockUnityShowdesktopHandlerWindow mMockWindow;

  EXPECT_CALL (mMockWindow, getInputRemover ());

  UnityShowdesktopHandler mMockHandler (static_cast <UnityShowdesktopHandlerWindowInterface *> (&mMockWindow));

  EXPECT_CALL (mMockWindow, isOverrideRedirect ());
  EXPECT_CALL (mMockWindow, isManaged ());
  EXPECT_CALL (mMockWindow, isGrabbed ());
  EXPECT_CALL (mMockWindow, isDesktopOrDock ());
  EXPECT_CALL (mMockWindow, isSkipTaskbarOrPager ());
  EXPECT_CALL (mMockWindow, isHidden ()).WillOnce (Return (true));
  EXPECT_CALL (mMockWindow, isInShowdesktopMode ()).WillOnce (Return (false));
  EXPECT_CALL (mMockWindow, isShaded ()).WillOnce (Return (false));
  EXPECT_TRUE (UnityShowdesktopHandler::shouldHide (&mMockWindow));
}

class MockWindowInputRemoverTestFadeOut :
  public compiz::WindowInputRemoverInterface
{
  public:

    MockWindowInputRemoverTestFadeOut ()
    {
      ON_CALL (*this, saveInput ()).WillByDefault (Return (true));
      ON_CALL (*this, removeInput ()).WillByDefault (Return (true));
      ON_CALL (*this, restoreInput ()).WillByDefault (Return (true));

      EXPECT_CALL (*this, saveInput ()).WillOnce (Return (true));
      EXPECT_CALL (*this, removeInput ()).WillOnce (Return (true));
    }

    MOCK_METHOD0 (saveInput, bool ());
    MOCK_METHOD0 (removeInput, bool ());
    MOCK_METHOD0 (restoreInput, bool ());
};

TEST_F(UnityShowdesktopHandlerTest, TestFadeOutHidesWindow)
{
  MockUnityShowdesktopHandlerWindow mMockWindow;

  EXPECT_CALL (mMockWindow, getInputRemover ()).WillOnce (Return (new MockWindowInputRemoverTestFadeOut ()));

  UnityShowdesktopHandler mMockHandler (static_cast <UnityShowdesktopHandlerWindowInterface *> (&mMockWindow));

  EXPECT_CALL (mMockWindow, isHidden ());
  EXPECT_CALL (mMockWindow, doHide ());
  EXPECT_CALL (mMockWindow, doNotifyHidden ());

  mMockHandler.fadeOut ();

  EXPECT_EQ (UnityShowdesktopHandler::animating_windows.size (), 1);
}

class MockWindowInputRemoverTestFadeOutAlready :
  public compiz::WindowInputRemoverInterface
{
  public:

    MockWindowInputRemoverTestFadeOutAlready ()
    {
      ON_CALL (*this, saveInput ()).WillByDefault (Return (true));
      ON_CALL (*this, removeInput ()).WillByDefault (Return (true));
      ON_CALL (*this, restoreInput ()).WillByDefault (Return (true));
    }

    MOCK_METHOD0 (saveInput, bool ());
    MOCK_METHOD0 (removeInput, bool ());
    MOCK_METHOD0 (restoreInput, bool ());
};

TEST_F(UnityShowdesktopHandlerTest, TestFadeOutOnHiddenDoesntHideWindow)
{
  MockUnityShowdesktopHandlerWindow mMockWindow;

  EXPECT_CALL (mMockWindow, getInputRemover ()).WillOnce (Return (new MockWindowInputRemoverTestFadeOutAlready ()));

  UnityShowdesktopHandler mMockHandler (static_cast <UnityShowdesktopHandlerWindowInterface *> (&mMockWindow));

  EXPECT_CALL (mMockWindow, isHidden ()).WillOnce (Return (true));

  mMockHandler.fadeOut ();

  EXPECT_EQ (UnityShowdesktopHandler::animating_windows.size (), 0);
}

TEST_F(UnityShowdesktopHandlerTest, TestFadeOutAlreadyFadedDoesntHideWindow)
{
  MockUnityShowdesktopHandlerWindow mMockWindow;

  EXPECT_CALL (mMockWindow, getInputRemover ()).WillOnce (Return (new MockWindowInputRemoverTestFadeOut ()));

  UnityShowdesktopHandler mMockHandler (static_cast <UnityShowdesktopHandlerWindowInterface *> (&mMockWindow));

  EXPECT_CALL (mMockWindow, isHidden ());
  EXPECT_CALL (mMockWindow, doHide ());
  EXPECT_CALL (mMockWindow, doNotifyHidden ());

  mMockHandler.fadeOut ();
  mMockHandler.fadeOut ();

  EXPECT_EQ (UnityShowdesktopHandler::animating_windows.size (), 1);
}

TEST_F(UnityShowdesktopHandlerTest, TestFadeInNonFadedDoesntShowWindow)
{
  MockUnityShowdesktopHandlerWindow mMockWindow;

  EXPECT_CALL (mMockWindow, getInputRemover ()).WillOnce (Return (new MockWindowInputRemoverTestFadeOutAlready ()));

  UnityShowdesktopHandler mMockHandler (static_cast <UnityShowdesktopHandlerWindowInterface *> (&mMockWindow));

  mMockHandler.fadeIn ();

  EXPECT_EQ (UnityShowdesktopHandler::animating_windows.size (), 0);
}

class MockWindowInputRemoverTestFadeOutFadeIn :
  public compiz::WindowInputRemoverInterface
{
  public:

    MockWindowInputRemoverTestFadeOutFadeIn ()
    {
      ON_CALL (*this, saveInput ()).WillByDefault (Return (true));
      ON_CALL (*this, removeInput ()).WillByDefault (Return (true));
      ON_CALL (*this, restoreInput ()).WillByDefault (Return (true));

      EXPECT_CALL (*this, saveInput ()).WillOnce (Return (true));
      EXPECT_CALL (*this, removeInput ()).WillOnce (Return (true));
      EXPECT_CALL (*this, restoreInput ()).WillOnce (Return (true));
    }

    MOCK_METHOD0 (saveInput, bool ());
    MOCK_METHOD0 (removeInput, bool ());
    MOCK_METHOD0 (restoreInput, bool ());
};

TEST_F(UnityShowdesktopHandlerTest, TestFadeOutHidesWindowFadeInShowsWindow)
{
  MockUnityShowdesktopHandlerWindow mMockWindow;

  EXPECT_CALL (mMockWindow, getInputRemover ()).WillOnce (Return (new MockWindowInputRemoverTestFadeOutFadeIn ()));

  UnityShowdesktopHandler mMockHandler (static_cast <UnityShowdesktopHandlerWindowInterface *> (&mMockWindow));

  EXPECT_CALL (mMockWindow, isHidden ());
  EXPECT_CALL (mMockWindow, doHide ());
  EXPECT_CALL (mMockWindow, doNotifyHidden ());

  mMockHandler.fadeOut ();

  EXPECT_CALL (mMockWindow, doShow ());
  EXPECT_CALL (mMockWindow, doNotifyShown ());

  mMockHandler.fadeIn ();

  EXPECT_EQ (UnityShowdesktopHandler::animating_windows.size (), 1);
}

TEST_F(UnityShowdesktopHandlerTest, TestAnimationPostPaintActions)
{
  MockUnityShowdesktopHandlerWindow mMockWindow;

  EXPECT_CALL (mMockWindow, getInputRemover ()).WillOnce (Return (new MockWindowInputRemoverTestFadeOutFadeIn ()));

  UnityShowdesktopHandler mMockHandler (static_cast <UnityShowdesktopHandlerWindowInterface *> (&mMockWindow));

  EXPECT_CALL (mMockWindow, isHidden ());
  EXPECT_CALL (mMockWindow, doHide ());
  EXPECT_CALL (mMockWindow, doNotifyHidden ());

  mMockHandler.fadeOut ();

  EXPECT_CALL (mMockWindow, doShow ());
  EXPECT_CALL (mMockWindow, doNotifyShown ());

  for (unsigned int i = 0; i < UnityShowdesktopHandler::fade_time; i++)
  {
    UnityShowdesktopHandlerWindowInterface::PostPaintAction action = mMockHandler.animate (1);

    if (i == 300)
      EXPECT_EQ (action, UnityShowdesktopHandlerWindowInterface::PostPaintAction::Wait);
    else
      EXPECT_EQ (action, UnityShowdesktopHandlerWindowInterface::PostPaintAction::Damage);
  }

  mMockHandler.fadeIn ();

  for (unsigned int i = 0; i < UnityShowdesktopHandler::fade_time; i++)
  {
    UnityShowdesktopHandlerWindowInterface::PostPaintAction action = mMockHandler.animate (1);

    if (i == 300)
      EXPECT_EQ (action, UnityShowdesktopHandlerWindowInterface::PostPaintAction::Remove);
    else
      EXPECT_EQ (action, UnityShowdesktopHandlerWindowInterface::PostPaintAction::Damage);
  }

  EXPECT_EQ (UnityShowdesktopHandler::animating_windows.size (), 1);
}

TEST_F(UnityShowdesktopHandlerTest, TestAnimationOpacity)
{
  MockUnityShowdesktopHandlerWindow mMockWindow;

  EXPECT_CALL (mMockWindow, getInputRemover ()).WillOnce (Return (new MockWindowInputRemoverTestFadeOutFadeIn ()));

  UnityShowdesktopHandler mMockHandler (static_cast <UnityShowdesktopHandlerWindowInterface *> (&mMockWindow));

  EXPECT_CALL (mMockWindow, isHidden ());
  EXPECT_CALL (mMockWindow, doHide ());
  EXPECT_CALL (mMockWindow, doNotifyHidden ());

  mMockHandler.fadeOut ();

  EXPECT_CALL (mMockWindow, doShow ());
  EXPECT_CALL (mMockWindow, doNotifyShown ());

  /* The funny expectations here are to account for rounding errors that would
   * otherwise make testing the code painful */

  for (unsigned int i = 0; i < UnityShowdesktopHandler::fade_time; i++)
  {
    unsigned short opacity = std::numeric_limits <unsigned short>::max ();
    mMockHandler.paintOpacity (opacity);

    mMockHandler.animate (1);

    if (i == 300)
      EXPECT_EQ (opacity, std::numeric_limits <unsigned short>::max ());
    else
    {
      float rem = opacity - std::numeric_limits <unsigned short>::max () * (1.0f - i / static_cast <float> (UnityShowdesktopHandler::fade_time));
      EXPECT_TRUE (rem <= 1.0f && rem >= -1.0f);
    }
  }

  mMockHandler.fadeIn ();

  for (unsigned int i = 0; i < UnityShowdesktopHandler::fade_time; i++)
  {
    unsigned short opacity = std::numeric_limits <unsigned short>::max ();
    mMockHandler.paintOpacity (opacity);

    mMockHandler.animate (1);

    if (i == 300)
      EXPECT_EQ (opacity, std::numeric_limits <unsigned short>::max ());
    else
    {
      float rem = opacity - std::numeric_limits <unsigned short>::max () * (i / static_cast <float> (UnityShowdesktopHandler::fade_time));
      EXPECT_TRUE (rem <= 1.0f && rem >= -1.0f);
    }
  }

  EXPECT_EQ (UnityShowdesktopHandler::animating_windows.size (), 1);
}

TEST_F(UnityShowdesktopHandlerTest, TestAnimationPaintMasks)
{
  MockUnityShowdesktopHandlerWindow mMockWindow;

  EXPECT_CALL (mMockWindow, getInputRemover ()).WillOnce (Return (new MockWindowInputRemoverTestFadeOutFadeIn ()));

  UnityShowdesktopHandler mMockHandler (static_cast <UnityShowdesktopHandlerWindowInterface *> (&mMockWindow));

  EXPECT_CALL (mMockWindow, isHidden ());
  EXPECT_CALL (mMockWindow, doHide ());
  EXPECT_CALL (mMockWindow, doNotifyHidden ());

  mMockHandler.fadeOut ();

  EXPECT_CALL (mMockWindow, doShow ());
  EXPECT_CALL (mMockWindow, doNotifyShown ());
  EXPECT_CALL (mMockWindow, getNoCoreInstanceMask ());

  mMockHandler.animate (UnityShowdesktopHandler::fade_time);

  EXPECT_EQ (mMockHandler.getPaintMask (), 1);

  mMockHandler.fadeIn ();

  mMockHandler.animate (UnityShowdesktopHandler::fade_time);

  EXPECT_EQ (mMockHandler.getPaintMask (), 0);

  EXPECT_EQ (UnityShowdesktopHandler::animating_windows.size (), 1);
}

class MockWindowInputRemoverTestFadeOutFadeInWithShapeEvent :
  public compiz::WindowInputRemoverInterface
{
  public:

    MockWindowInputRemoverTestFadeOutFadeInWithShapeEvent ()
    {
      ON_CALL (*this, saveInput ()).WillByDefault (Return (true));
      ON_CALL (*this, removeInput ()).WillByDefault (Return (true));
      ON_CALL (*this, restoreInput ()).WillByDefault (Return (true));

      InSequence s;

      EXPECT_CALL (*this, saveInput ()).WillOnce (Return (true));
      EXPECT_CALL (*this, removeInput ()).WillOnce (Return (true));
      EXPECT_CALL (*this, saveInput ()).WillOnce (Return (true));
      EXPECT_CALL (*this, removeInput ()).WillOnce (Return (true));
      EXPECT_CALL (*this, restoreInput ()).WillOnce (Return (true));
    }

    MOCK_METHOD0 (saveInput, bool ());
    MOCK_METHOD0 (removeInput, bool ());
    MOCK_METHOD0 (restoreInput, bool ());
};

TEST_F(UnityShowdesktopHandlerTest, TestShapeEvent)
{
  MockUnityShowdesktopHandlerWindow mMockWindow;

  EXPECT_CALL (mMockWindow, getInputRemover ()).WillOnce (Return (new MockWindowInputRemoverTestFadeOutFadeInWithShapeEvent ()));

  UnityShowdesktopHandler mMockHandler (static_cast <UnityShowdesktopHandlerWindowInterface *> (&mMockWindow));

  EXPECT_CALL (mMockWindow, isHidden ());
  EXPECT_CALL (mMockWindow, doHide ());
  EXPECT_CALL (mMockWindow, doNotifyHidden ());

  mMockHandler.fadeOut ();

  EXPECT_CALL (mMockWindow, doShow ());
  EXPECT_CALL (mMockWindow, doNotifyShown ());

  mMockHandler.handleShapeEvent ();

  mMockHandler.fadeIn ();

  EXPECT_EQ (UnityShowdesktopHandler::animating_windows.size (), 1);
}
