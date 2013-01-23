#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <UnityCore/Model.h>

namespace
{

using unity::dash::Model;

class Utils
{
public:
  template <typename Adaptor>
  static void WaitForModelSynchronize(Model<Adaptor>& model, unsigned int n_rows)
  {
    bool timeout_reached = false;

    auto timeout_cb = [](gpointer data) -> gboolean
    {
      *(bool*)data = true;
      return FALSE;
    };

    guint32 timeout_id = g_timeout_add(10000, timeout_cb, &timeout_reached);

    while (model.count != n_rows && !timeout_reached)
    {
      g_main_context_iteration(g_main_context_get_thread_default(), TRUE);
    }
    if (model.count == n_rows)
      g_source_remove(timeout_id);
  }

  static void WaitUntilMSec(bool& success, unsigned int max_wait = 500)
  {
    bool timeout_reached = false;
    guint32 timeout_id = ScheduleTimeout(&timeout_reached, max_wait);

    while (!success && !timeout_reached)
      g_main_context_iteration(g_main_context_get_thread_default(), TRUE);

    if (success)
      g_source_remove(timeout_id);

    EXPECT_TRUE(success);
  }

  static void WaitUntil(bool& success, unsigned max_wait = 1)
  {
    WaitUntilMSec(success, max_wait * 1000);
  }

  static void WaitUntilMSec(std::function<bool()> const& check_function, bool result = true, unsigned max_wait = 500)
  {
    bool timeout_reached = false;
    guint32 timeout_id = ScheduleTimeout(&timeout_reached, max_wait);

    while (check_function() != result && !timeout_reached)
      g_main_context_iteration(g_main_context_get_thread_default(), TRUE);

    if (check_function() == result)
      g_source_remove(timeout_id);

    EXPECT_EQ(check_function(), result);
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
      g_main_context_iteration(g_main_context_get_thread_default(), TRUE);

    g_source_remove(timeout_id);
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
