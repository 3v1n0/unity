#ifndef _SERVICE_MODEL_H_
#define _SERVICE_MODEL_H_

#include <dee.h>
#include <UnityCore/GLibWrapper.h>

namespace unity
{
namespace service
{

class Model
{
public:
  Model();

private:
  glib::Object<DeeModel> model_;
  glib::Object<DeeModel> results_model_;
  glib::Object<DeeModel> categories_model_;
};

}
}

#endif /* _SERVICE_MODEL_H_ */
