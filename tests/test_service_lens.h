#ifndef _SERVICE_LENS_H_
#define _SERVICE_LENS_H_

#include <unity.h>
#include <UnityCore/GLibWrapper.h>

namespace unity
{
namespace service
{

class Lens
{
public:
  Lens();
  ~Lens();

private:
  void AddCategories();
  void AddFilters();

  glib::Object<UnityLens> lens_;
  glib::Object<UnityScope> scope_;
};

}
}

#endif /* _SERVICE_LENS_H_ */
