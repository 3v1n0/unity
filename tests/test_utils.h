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

    guint32 timeout_id = g_timeout_add_seconds(10, timeout_cb, &timeout_reached);

    while (model.count != n_rows && !timeout_reached)
    {
      g_main_context_iteration(g_main_context_get_thread_default(), TRUE);
    }
    if (model.count == n_rows)
      g_source_remove(timeout_id);
  }
};

}

#endif
