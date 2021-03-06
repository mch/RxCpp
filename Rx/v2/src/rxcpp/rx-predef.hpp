// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_PREDEF_HPP)
#define RXCPP_RX_PREDEF_HPP

#include "rx-includes.hpp"

namespace rxcpp {

struct tag_action {};
template<class T>
class is_action
{
    struct not_void {};
    template<class C>
    static typename C::action_tag* check(int);
    template<class C>
    static not_void check(...);
public:
    static const bool value = std::is_convertible<decltype(check<typename std::decay<T>::type>(0)), tag_action*>::value;
};

struct tag_scheduler {};
template<class T>
class is_scheduler
{
    struct not_void {};
    template<class C>
    static typename C::scheduler_tag* check(int);
    template<class C>
    static not_void check(...);
public:
    static const bool value = std::is_convertible<decltype(check<typename std::decay<T>::type>(0)), tag_scheduler*>::value;
};

struct tag_schedulable {};
template<class T>
class is_schedulable
{
    struct not_void {};
    template<class C>
    static typename C::schedulable_tag* check(int);
    template<class C>
    static not_void check(...);
public:
    static const bool value = std::is_convertible<decltype(check<typename std::decay<T>::type>(0)), tag_schedulable*>::value;
};


template<class T>
class dynamic_observer;

template<class T, class I = dynamic_observer<T>>
class observer;

struct tag_observer {};
template<class T>
class is_observer
{
    template<class C>
    static typename C::observer_tag* check(int);
    template<class C>
    static void check(...);
public:
    static const bool value = std::is_convertible<decltype(check<typename std::decay<T>::type>(0)), tag_observer*>::value;
};

struct tag_dynamic_observer {};
template<class T>
class is_dynamic_observer
{
    struct not_void {};
    template<class C>
    static typename C::dynamic_observer_tag* check(int);
    template<class C>
    static not_void check(...);
public:
    static const bool value = std::is_convertible<decltype(check<typename std::decay<T>::type>(0)), tag_dynamic_observer*>::value;
};

struct tag_subscriber {};
template<class T>
class is_subscriber
{
    struct not_void {};
    template<class C>
    static typename C::subscriber_tag* check(int);
    template<class C>
    static not_void check(...);
public:
    static const bool value = std::is_convertible<decltype(check<typename std::decay<T>::type>(0)), tag_subscriber*>::value;
};

struct tag_dynamic_observable {};
template<class T>
class is_dynamic_observable
{
    struct not_void {};
    template<class C>
    static typename C::dynamic_observable_tag* check(int);
    template<class C>
    static not_void check(...);
public:
    static const bool value = std::is_convertible<decltype(check<typename std::decay<T>::type>(0)), tag_dynamic_observable*>::value;
};

template<class T>
class dynamic_observable;

template<
    class T = void,
    class SourceObservable = typename std::conditional<std::is_same<T, void>::value,
        void, dynamic_observable<T>>::type>
class observable;

template<class T, class Source>
observable<T> make_dynamic_observable(Source&&);

struct tag_observable {};
template<class T>
struct observable_base {
    typedef tag_observable observable_tag;
    typedef T value_type;
};
template<class T>
class is_observable
{
    template<class C>
    static typename C::observable_tag check(int);
    template<class C>
    static void check(...);
public:
    static const bool value = std::is_convertible<decltype(check<typename std::decay<T>::type>(0)), tag_observable>::value;
};

}

#endif
