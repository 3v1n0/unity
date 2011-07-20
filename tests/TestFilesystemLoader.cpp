#include <UnityCore/FilesystemLenses.h>
#include <NuxCore/Logger.h>
#include <gio/gio.h>

using unity::dash::FilesystemLenses;

int
main(int argc, char** argv)
{
  g_type_init();
  g_thread_init(NULL);

  // Slightly higher as we're more likely to test things we know will fail
  nux::logging::configure_logging("<root>=error");

  // but you can still change it if your debugging ;)
  nux::logging::configure_logging(::getenv("UNITY_LOG_SEVERITY"));

  unity::dash::FilesystemLenses lenses;
  
  auto callback = [] (gpointer data) -> gboolean {
    FilesystemLenses* lenses = static_cast<FilesystemLenses*>(data);
    lenses->GetLenses()[0]->Search("Hello");
    lenses->GetLenses()[0]->GlobalSearch("Goodbye");
    return TRUE;
  };
  g_timeout_add_seconds(5, callback, &lenses);
  
  while (1)
    g_main_context_iteration(g_main_context_get_thread_default(), TRUE);
}
