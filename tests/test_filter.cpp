#include <gtest/gtest.h>
#include <glib-object.h>
#include <UnityCore/Filter.h>

using namespace std;
using namespace unity::dash;

namespace {

class FilterRecorder : public Filter
{
  FilterRecorder(DeeModel* model, DeeModelIter* iter)
  {
    model_ = model;
    iter_ = iter;
    Connect();
  }
};

TEST(TestFilter, TestConstruction)
{
  Filter filter;
  Filter::Ptr(new Filter());
}

TEST(TestFilter, TestConnect)
{
  ;
}

}
