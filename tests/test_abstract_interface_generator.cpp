/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Sam Spilsbury <sam.spilsbury@canonical.com>
 *
 */
#include <vector>
#include <list>
#include <memory>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <AbstractInterfaceGenerator.h>

using ::testing::InSequence;

namespace
{
class AbstractObject
{
  public:

    virtual void check() const = 0;
};

class MockObject :
  public AbstractObject
{
  public:

    MOCK_CONST_METHOD0(check, void());
};

class ConcreteOwningObject
{
  public:

    ConcreteOwningObject(AbstractObject const& abstract)
      : mOwnedObject(abstract)
    {
    }

    AbstractObject const& owned() const { return mOwnedObject; }

  private:

    AbstractObject const& mOwnedObject;
};

typedef std::vector <ConcreteOwningObject> ConcreteVector;
typedef std::list   <ConcreteOwningObject> ConcreteList;
}

template <typename Collection>
class AbstractObjectGeneratorTest :
  public ::testing::Test
{
  public:

    template <typename Collectable>
    void AddToCollection(const Collectable &collectable)
    {
      collection.push_back(collectable);
    }

    typedef unity::AbstractInterfaceCollectionGenerator<AbstractObject const&, Collection> CollectionGeneratorType;
    typedef unity::AbstractInterfaceGenerator<AbstractObject const&> GeneratorType;
    typedef std::shared_ptr <GeneratorType> GeneratorPtr;
    typedef typename CollectionGeneratorType::ElementRetrievalFunc RetreivalFunc;

    GeneratorPtr MakeGenerator(RetreivalFunc const& func)
    {
      return std::make_shared <CollectionGeneratorType>(collection, func);
    }

  private:
    Collection collection;
};

typedef ::testing::Types<ConcreteVector, ConcreteList> GeneratorTypes;
TYPED_TEST_CASE(AbstractObjectGeneratorTest, GeneratorTypes);

TYPED_TEST(AbstractObjectGeneratorTest, TestConstruction)
{
  typedef AbstractObjectGeneratorTest<TypeParam> TParam;
  typedef typename TParam::GeneratorPtr GenPtr;

  GenPtr generator(TParam::MakeGenerator([](ConcreteOwningObject const& owner) -> AbstractObject const&
                                           {
                                             return owner.owned();
                                           }));
}

TYPED_TEST(AbstractObjectGeneratorTest, TestAddToCollectionAndConstruct)
{
  typedef AbstractObjectGeneratorTest<TypeParam> TParam;
  typedef typename TParam::GeneratorPtr GenPtr;

  TParam::AddToCollection(ConcreteOwningObject(MockObject()));
  GenPtr generator(TParam::MakeGenerator([](ConcreteOwningObject const& owner) -> AbstractObject const&
                                            {
                                              return owner.owned();
                                            }));
}

TYPED_TEST(AbstractObjectGeneratorTest, TestAddToCollectionAndVisitEach)
{
  typedef AbstractObjectGeneratorTest<TypeParam> TParam;
  typedef typename TParam::GeneratorPtr GenPtr;

  MockObject     mockOne, mockTwo, mockThree;
  ConcreteOwningObject concreteOne(mockOne), concreteTwo(mockTwo), concreteThree(mockThree);

  TParam::AddToCollection(concreteOne);
  TParam::AddToCollection(concreteTwo);
  TParam::AddToCollection(concreteThree);
  GenPtr generator(TParam::MakeGenerator([](ConcreteOwningObject const& owner) -> AbstractObject const&
                                            {
                                              return owner.owned();
                                            }));

  InSequence s;

  EXPECT_CALL(mockOne, check()).Times(1);
  EXPECT_CALL(mockTwo, check()).Times(1);
  EXPECT_CALL(mockThree, check()).Times(1);

  generator->VisitEachInterface([&](AbstractObject const& abstract)
                                {
                                  abstract.check();
                                });
}

TYPED_TEST(AbstractObjectGeneratorTest, TestAddToCollectionAndVisitEachReverse)
{
  typedef AbstractObjectGeneratorTest<TypeParam> TParam;
  typedef typename TParam::GeneratorPtr GenPtr;

  MockObject     mockOne, mockTwo, mockThree;
  ConcreteOwningObject concreteOne(mockOne), concreteTwo(mockTwo), concreteThree(mockThree);

  TParam::AddToCollection(concreteOne);
  TParam::AddToCollection(concreteTwo);
  TParam::AddToCollection(concreteThree);
  GenPtr generator(TParam::MakeGenerator([](ConcreteOwningObject const& owner) -> AbstractObject const&
                                            {
                                              return owner.owned();
                                            }));

  InSequence s;

  EXPECT_CALL(mockThree, check()).Times(1);
  EXPECT_CALL(mockTwo, check()).Times(1);
  EXPECT_CALL(mockOne, check()).Times(1);


  generator->VisitEachInterfaceReverse([&](AbstractObject const& abstract)
                                       {
                                         abstract.check();
                                       });
}

