#ifndef _SERVICE_GDBUS_WRAPPER_H_
#define _SERVICE_GDBUS_WRAPPER_H_

#include <UnityCore/GLibDBusServer.h>

namespace unity
{
namespace service
{

class GDBus
{
public:
  GDBus();

private:
  glib::DBusServer server_;
  int ro_property_;
  int rw_property_;
  int wo_property_;
};

}
}

#endif /* _SERVICE_GDBUS_WRAPPER_H_ */
