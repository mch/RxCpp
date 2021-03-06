// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_FLATMAP_HPP)
#define RXCPP_OPERATORS_RX_FLATMAP_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class Observable, class CollectionSelector, class ResultSelector>
struct flat_map_traits {
    typedef typename std::decay<Observable>::type source_type;
    typedef typename std::decay<CollectionSelector>::type collection_selector_type;
    typedef typename std::decay<ResultSelector>::type result_selector_type;

    typedef typename source_type::value_type source_value_type;

    struct tag_not_valid {};
    template<class CV, class CCS>
    static auto collection_check(int) -> decltype((*(CCS*)nullptr)(*(CV*)nullptr));
    template<class CV, class CCS>
    static tag_not_valid collection_check(...);

    static_assert(!std::is_same<decltype(collection_check<source_value_type, collection_selector_type>(0)), tag_not_valid>::value, "flat_map CollectionSelector must be a function with the signature observable(flat_map::source_value_type)");

    typedef decltype((*(collection_selector_type*)nullptr)((*(source_value_type*)nullptr))) collection_type;

#if _MSC_VER >= 1900
    static_assert(is_observable<collection_type>::value, "flat_map CollectionSelector must return an observable");
#endif

    typedef typename collection_type::value_type collection_value_type;

    template<class CV, class CCV, class CRS>
    static auto result_check(int) -> decltype((*(CRS*)nullptr)(*(CV*)nullptr, *(CCV*)nullptr));
    template<class CV, class CCV, class CRS>
    static tag_not_valid result_check(...);

    static_assert(!std::is_same<decltype(result_check<source_value_type, collection_value_type, result_selector_type>(0)), tag_not_valid>::value, "flat_map ResultSelector must be a function with the signature flat_map::value_type(flat_map::source_value_type, flat_map::collection_value_type)");

    typedef decltype((*(result_selector_type*)nullptr)(*(source_value_type*)nullptr, *(collection_value_type*)nullptr)) value_type;
};

template<class Observable, class CollectionSelector, class ResultSelector>
struct flat_map
    : public operator_base<typename flat_map_traits<Observable, CollectionSelector, ResultSelector>::value_type>
{
    typedef flat_map<Observable, CollectionSelector, ResultSelector> this_type;
    typedef flat_map_traits<Observable, CollectionSelector, ResultSelector> traits;

    typedef typename traits::source_type source_type;
    typedef typename traits::collection_selector_type collection_selector_type;
    typedef typename traits::result_selector_type result_selector_type;

    typedef typename traits::source_value_type source_value_type;
    typedef typename traits::collection_type collection_type;
    typedef typename traits::collection_value_type collection_value_type;

    struct values
    {
        values(source_type o, collection_selector_type s, result_selector_type rs)
            : source(std::move(o))
            , selectCollection(std::move(s))
            , selectResult(std::move(rs))
        {
        }
        source_type source;
        collection_selector_type selectCollection;
        result_selector_type selectResult;
    };
    values initial;

    flat_map(source_type o, collection_selector_type s, result_selector_type rs)
        : initial(std::move(o), std::move(s), std::move(rs))
    {
    }

    template<class Observer>
    void on_subscribe(Observer&& o) {
        static_assert(is_observer<Observer>::value, "subscribe must be passed an observer");

        typedef typename std::decay<Observer>::type output_type;

        struct state_type
            : public std::enable_shared_from_this<state_type>
            , public values
        {
            state_type(values i, output_type oarg)
                : values(std::move(i))
                , pendingCompletions(0)
                , out(std::move(oarg))
            {
            }
            // on_completed on the output must wait until all the
            // subscriptions have received on_completed
            std::atomic<int> pendingCompletions;
            // because multiple sources are subscribed to by flat_map,
            // calls to the output must be serialized by lock.
            // the on_error/on_complete and unsubscribe calls can
            // cause lock recursion.
            std::recursive_mutex lock;
            output_type out;
        };
        // take a copy of the values for each subscription
        auto state = std::shared_ptr<state_type>(new state_type(initial, std::forward<Observer>(o)));

        composite_subscription outercs;

        // when the out observer is unsubscribed all the
        // inner subscriptions are unsubscribed as well
        state->out.add(outercs);

        ++state->pendingCompletions;
        // this subscribe does not share the observer subscription
        // so that when it is unsubscribed the observer can be called
        // until the inner subscriptions have finished
        state->source.subscribe(
            state->out,
            outercs,
        // on_next
            [state](source_value_type st) {
                util::detail::maybe<collection_type> selectedCollection;
                try {
                    selectedCollection.reset(state->selectCollection(st));
                } catch(...) {
                    std::unique_lock<std::recursive_mutex> guard(state->lock);
                    state->out.on_error(std::current_exception());
                    return;
                }

                composite_subscription innercs;

                // when the out observer is unsubscribed all the
                // inner subscriptions are unsubscribed as well
                auto innercstoken = state->out.add(innercs);

                innercs.add(make_subscription([state, innercstoken](){
                    state->out.remove(innercstoken);
                }));

                ++state->pendingCompletions;
                // this subscribe does not share the source subscription
                // so that when it is unsubscribed the source will continue
                selectedCollection->subscribe(
                    state->out,
                    innercs,
                // on_next
                    [state, st](collection_value_type ct) {
                        util::detail::maybe<typename this_type::value_type> selectedResult;
                        try {
                            selectedResult.reset(state->selectResult(st, std::move(ct)));
                        } catch(...) {
                            std::unique_lock<std::recursive_mutex> guard(state->lock);
                            state->out.on_error(std::current_exception());
                            return;
                        }
                        std::unique_lock<std::recursive_mutex> guard(state->lock);
                        state->out.on_next(std::move(*selectedResult));
                    },
                // on_error
                    [state](std::exception_ptr e) {
                        std::unique_lock<std::recursive_mutex> guard(state->lock);
                        state->out.on_error(e);
                    },
                //on_completed
                    [state](){
                        if (--state->pendingCompletions == 0) {
                            std::unique_lock<std::recursive_mutex> guard(state->lock);
                            state->out.on_completed();
                        }
                    }
                );
            },
        // on_error
            [state](std::exception_ptr e) {
                std::unique_lock<std::recursive_mutex> guard(state->lock);
                state->out.on_error(e);
            },
        // on_completed
            [state]() {
                if (--state->pendingCompletions == 0) {
                    std::unique_lock<std::recursive_mutex> guard(state->lock);
                    state->out.on_completed();
                }
            }
        );
    }
};

template<class CollectionSelector, class ResultSelector>
class flat_map_factory
{
    typedef typename std::decay<CollectionSelector>::type collection_selector_type;
    typedef typename std::decay<ResultSelector>::type result_selector_type;

    collection_selector_type selectorCollection;
    result_selector_type selectorResult;
public:
    flat_map_factory(collection_selector_type s, result_selector_type rs)
        : selectorCollection(std::move(rs))
        , selectorResult(std::move(s))
    {
    }

    template<class Observable>
    auto operator()(Observable&& source)
        ->      observable<typename flat_map<Observable, CollectionSelector, ResultSelector>::value_type, flat_map<Observable, CollectionSelector, ResultSelector>> {
        return  observable<typename flat_map<Observable, CollectionSelector, ResultSelector>::value_type, flat_map<Observable, CollectionSelector, ResultSelector>>(
                                    flat_map<Observable, CollectionSelector, ResultSelector>(std::forward<Observable>(source), std::move(selectorCollection), std::move(selectorResult)));
    }
};

}

template<class CollectionSelector, class ResultSelector>
auto flat_map(CollectionSelector&& s, ResultSelector&& rs)
    ->      detail::flat_map_factory<CollectionSelector, ResultSelector> {
    return  detail::flat_map_factory<CollectionSelector, ResultSelector>(std::forward<CollectionSelector>(s), std::forward<ResultSelector>(rs));
}

}

}

#endif
