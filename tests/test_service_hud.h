#ifndef _SERVICE_HUD_H_
#define _SERVICE_HUD_H_

#include <UnityCore/GLibDBusServer.h>
#include <UnityCore/GLibSource.h>

namespace unity
{
namespace service
{

class Hud
{
public:
  Hud();

private:
  void EmitSignal();
  GVariant* OnMethodCall(std::string const& method, GVariant *parameters);

  glib::DBusServer server_;
  glib::Source::UniquePtr timeout_;
};

}
}


#endif /* _SERVICE_HUD_H_ */
