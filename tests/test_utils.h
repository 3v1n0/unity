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
  typedef std::function<gchar*()> ErrorStringFunc;
  static gchar* DefaultErrorString() { return nullptr; }

  static void WaitUntilMSec(bool& success, unsigned max_wait = 500, ErrorStringFunc const& error_func = &Utils::DefaultErrorString)
  {
    WaitUntilMSec([&success] {return success;}, true, max_wait);
  }

  static void WaitUntil(bool& success, unsigned max_wait = 1, ErrorStringFunc const& error_func = &Utils::DefaultErrorString)
  {
    WaitUntilMSec(success, max_wait * 1000, error_func);
  }

  static void WaitUntilMSec(std::function<bool()> const& check_function, bool result = true, unsigned max_wait = 500, ErrorStringFunc const& error_func = &Utils::DefaultErrorString)
  {
    ASSERT_NE(check_function, nullptr);

    bool timeout_reached = false;
    guint32 timeout_id = ScheduleTimeout(&timeout_reached, max_wait);

    bool check_function_result = false;
    while ((check_function_result=check_function())!=result && !timeout_reached)
      g_main_context_iteration(g_main_context_get_thread_default(), TRUE);

    if (!timeout_reached)
      g_source_remove(timeout_id);

    glib::String error(error_func());

    if (error)
    {
      EXPECT_EQ(check_function_result, result) << "Error: " << error;
    }
    else
    {
      EXPECT_EQ(check_function_result, result);
    }
  }

  static void WaitUntil(std::function<bool()> const& check_function, bool result = true, unsigned max_wait = 1, ErrorStringFunc const& error_func = &Utils::DefaultErrorString)
  {
    WaitUntilMSec(check_function, result, max_wait * 1000, error_func);
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
      g_main_context_iteration(g_main_context_get_thread_default(), TRUE);
  }

  static void init_gsettings_test_environment()
  {
    // set the data directory so gsettings can find the schema
    g_setenv("GSETTINGS_SCHEMA_DIR", BUILDDIR"/settings", true);
    g_setenv("GSETTINGS_BACKEND", "memory", true);
  }

  static void reset_gsettings_test_environment()
  {
    g_setenv("GSETTINGS_SCHEMA_DIR", "", true);
    g_setenv("GSETTINGS_BACKEND", "", true);
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
