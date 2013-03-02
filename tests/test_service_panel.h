#ifndef _SERVICE_PANEL_H_
#define _SERVICE_PANEL_H_

#include <UnityCore/GLibDBusServer.h>

namespace unity
{
namespace service
{

class Panel
{
public:
  Panel();

private:
  GVariant* OnMethodCall(std::string const& method, GVariant *parameters);

  unsigned sync_return_mode_;
  bool trigger_resync1_sent_;

  glib::DBusServer server_;
};

}
}

#endif /* _SERVICE_PANEL_H_ */
