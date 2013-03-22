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
};

}
}

#endif /* _SERVICE_GDBUS_WRAPPER_H_ */
