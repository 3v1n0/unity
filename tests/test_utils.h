#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <glib.h>
#include <functional>
#include <gtest/gtest.h>

namespace
{

class Utils
{
public:
  static void WaitUntilMSec(bool& success, unsigned int max_wait = 500)
  {
    WaitUntilMSec([&success] {return success;}, true, max_wait);
  }

  static void WaitUntil(bool& success, unsigned max_wait = 1)
  {
    WaitUntilMSec(success, max_wait * 1000);
  }

  static void WaitUntilMSec(std::function<bool()> const& check_function, bool expected_result = true, unsigned max_wait = 500)
  {
    ASSERT_NE(check_function, nullptr);

    bool timeout_reached = false;
    guint32 timeout_id = ScheduleTimeout(&timeout_reached, max_wait);
    bool result;

    while (!timeout_reached)
    {
      result = check_function();
      if (result == expected_result)
        break;

      g_main_context_iteration(NULL, TRUE);
    }

    if (result == expected_result)
      g_source_remove(timeout_id);

    EXPECT_EQ(result, expected_result);
  }

  static void WaitUntil(std::function<bool()> const& check_function, bool result = true, unsigned max_wait = 10)
  {
    WaitUntilMSec(check_function, result, max_wait * 1000);
  }

  static guint32 ScheduleTimeout(bool* timeout_reached, unsigned timeout_duration = 10)
  {
    return g_timeout_add(timeout_duration, TimeoutCallback, timeout_reached);
  }

  static void WaitForTimeout(unsigned timeout_duration = 1)
  {
    WaitForTimeoutMSec(timeout_duration * 1000);
  }

  static void WaitForTimeoutMSec(unsigned timeout_duration = 500)
  {
    bool timeout_reached = false;
    guint32 timeout_id = ScheduleTimeout(&timeout_reached, timeout_duration);

    while (!timeout_reached)
      g_main_context_iteration(NULL, TRUE);

    g_source_remove(timeout_id);
  }

private:
  static gboolean TimeoutCallback(gpointer data)
  {
    *static_cast<bool*>(data) = true;
    return FALSE;
  };
};

}

#endif
