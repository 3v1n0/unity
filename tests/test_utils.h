#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <glib.h>
#include <functional>
#include <gtest/gtest.h>

#include "GLibWrapper.h"
#include "config.h"

namespace
{

using namespace unity;

class Utils
{
public:
  static void WaitUntilMSec(bool& success, unsigned max_wait = 500, std::string const& error_msg = "")
  {
    WaitUntilMSec([&success] {return success;}, true, max_wait, error_msg);
  }

  static void WaitUntil(bool& success, unsigned max_wait = 1, std::string const& error_msg = "")
  {
    WaitUntilMSec(success, max_wait * 1000, error_msg);
  }

  static void WaitUntilMSec(std::function<bool()> const& check_function, bool expected_result = true, unsigned max_wait = 500, std::string const& error_msg = "")
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

    EXPECT_EQ(result, expected_result) << (error_msg.empty() ? "" : ("Error: " + error_msg));
  }

  static void WaitUntil(std::function<bool()> const& check_function, bool result = true, unsigned max_wait = 1, std::string const& error_msg = "")
  {
    WaitUntilMSec(check_function, result, max_wait * 1000, error_msg);
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
    ScheduleTimeout(&timeout_reached, timeout_duration);

    while (!timeout_reached)
      g_main_context_iteration(nullptr, TRUE);
  }

  static void init_gsettings_test_environment()
  {
    // set the data directory so gsettings can find the schema
    g_setenv("GSETTINGS_SCHEMA_DIR", BUILDDIR"/settings", true);
    g_setenv("GSETTINGS_BACKEND", "memory", true);
  }

  static void reset_gsettings_test_environment()
  {
    g_unsetenv("GSETTINGS_SCHEMA_DIR");
    g_unsetenv("GSETTINGS_BACKEND");
  }

  static void WaitPendingEvents(unsigned max_wait_ms = 5000)
  {
    gint64 start_time = g_get_monotonic_time();

    while (g_main_context_pending(nullptr) &&
           (g_get_monotonic_time() - start_time) / 1000 < max_wait_ms)
    {
      g_main_context_iteration(nullptr, TRUE);
    }
  }

private:
  static gboolean TimeoutCallback(gpointer data)
  {
    *(bool*)data = true;
    return FALSE;
  };
};

}

#endif
