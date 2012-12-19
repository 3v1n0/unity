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
#ifndef _UNITY_ABSTRACT_INTERFACE_GENERATOR_H
#define _UNITY_ABSTRACT_INTERFACE_GENERATOR_H

#include <memory>
#include <functional>

namespace unity
{
namespace internal
{
template <class ConcreteCollection>
class ReverseWrapper
{
  private:

    ConcreteCollection const& collection_;

  public:

    ReverseWrapper(ConcreteCollection const& collection)
      : collection_(collection)
    {
    }

    decltype(collection_.rbegin ()) begin() const { return collection_.rbegin(); }
    decltype(collection_.rend ()) end () const { return collection_.rend(); }
};
}

/*
 * This can be passed to objects which want to visit some interfaces
 * they can use from a collection of concrete objects they do not need
 * to know about
 */
template <class AbstractInterface>
class AbstractInterfaceGenerator
{
  public:

    typedef std::shared_ptr <AbstractInterfaceGenerator<AbstractInterface> > Ptr;
    typedef std::function <void (AbstractInterface &)> InterfaceVisitor;
    virtual ~AbstractInterfaceGenerator () {}

    virtual void VisitEachInterface (InterfaceVisitor const& visit) = 0;
    virtual void VisitEachInterfaceReverse (InterfaceVisitor const& visit) = 0;
};

/*
 * This is a helper template to retrieve abstract interfaces from collections
 * containing concrete types. For example a CompWindowList or CompWindowVector
 * for which you might want to retreive a relevant FooInterface from each
 * element.
 */
template <class AbstractInterface, class ConcreteCollection>
class AbstractInterfaceCollectionGenerator :
  public AbstractInterfaceGenerator <AbstractInterface>
{
  public:

    typedef AbstractInterfaceGenerator<AbstractInterface> InterfaceGenerator;
    typedef typename InterfaceGenerator::InterfaceVisitor Visitor;
    typedef std::function <AbstractInterface & (typename ConcreteCollection::value_type const&) >
      ElementRetrievalFunc;

    AbstractInterfaceCollectionGenerator (ConcreteCollection const&   collection,
					  ElementRetrievalFunc const& retrieval)
      : collection_(collection)
      , retrieval_(retrieval)
    {
    }

    void VisitEachInterface(Visitor const& visit)
    {
      VisitEachInterfaceInternal(collection_, visit);
    }

    void VisitEachInterfaceReverse(Visitor const& visit)
    {
      VisitEachInterfaceInternal(internal::ReverseWrapper<ConcreteCollection> (collection_), visit);
    }

  private:

    template <class Container>
    void VisitEachInterfaceInternal(Container const& container,
                                    Visitor const&   visit)
    {
      for (typename ConcreteCollection::value_type const& item : container)
      {
	AbstractInterface& interface (retrieval_ (item));
        visit (interface);
      }
    }

    ConcreteCollection const& collection_;
    ElementRetrievalFunc      retrieval_;
};
}

#endif
