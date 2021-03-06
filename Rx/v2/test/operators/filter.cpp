
#define RXCPP_USE_OBSERVABLE_MEMBERS 1

#include "rxcpp/rx.hpp"
namespace rx=rxcpp;
namespace rxu=rxcpp::util;
namespace rxo=rxcpp::operators;
namespace rxs=rxcpp::sources;
namespace rxsc=rxcpp::schedulers;
namespace rxsub=rxcpp::subjects;
namespace rxn=rxcpp::notifications;

#include "rxcpp/rx-test.hpp"
namespace rxt=rxcpp::test;

#include "catch.hpp"

namespace {
bool IsPrime(int x)
{
    if (x < 2) return false;
    for (int i = 2; i <= x/2; ++i)
    {
        if (x % i == 0)
            return false;
    }
    return true;
}
}

SCENARIO("filter stops on completion", "[filter][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = rxsc::make_test();
        typedef rxsc::test::messages<int> m;
        typedef rxn::subscription life;
        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_error = m::on_error;
        auto on_completed = m::on_completed;
        auto subscribe = m::subscribe;

        long invoked = 0;

        record messages[] = {
            on_next(110, 1),
            on_next(180, 2),
            on_next(230, 3),
            on_next(270, 4),
            on_next(340, 5),
            on_next(380, 6),
            on_next(390, 7),
            on_next(450, 8),
            on_next(470, 9),
            on_next(560, 10),
            on_next(580, 11),
            on_completed(600),
            on_next(610, 12),
            on_error(620, std::runtime_error("error in unsubscribed stream")),
            on_completed(630)
        };
        auto xs = sc.make_hot_observable(messages);

        WHEN("filtered to ints that are primes"){
            auto res = sc.start<int>(
                [&xs, &invoked]() {
#if 0 && RXCPP_USE_OBSERVABLE_MEMBERS
                    return xs
                        .filter([&invoked](int x) {
                            invoked++;
                            return IsPrime(x);
                        })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
#else
                    return xs
                        >> rxo::filter([&invoked](int x) {
                            invoked++;
                            return IsPrime(x);
                        })
                        // demonstrates insertion of user definied operator
                        >> [](rx::observable<int> o)->rx::observable<int>{
                            return rxo::filter([](int){return true;})(o);
                        }
                        // forget type to workaround lambda deduction bug on msvc 2013
                        >> rxo::as_dynamic();
#endif
                }
            );
            THEN("the output only contains primes"){
                record items[] = {
                    on_next(230, 3),
                    on_next(340, 5),
                    on_next(390, 7),
                    on_next(580, 11),
                    on_completed(600)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                life items[] = {
                    subscribe(200, 600)
                };
                auto required = rxu::to_vector(items);
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("filter was called until completed"){
                REQUIRE(9 == invoked);
            }
        }
    }
}


SCENARIO("filter stops on disposal", "[where][filter][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = rxsc::make_test();
        typedef rxsc::test::messages<int> m;
        typedef rxn::subscription life;
        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_completed = m::on_completed;
        auto subscribe = m::subscribe;

        long invoked = 0;

        record messages[] = {
            on_next(110, 1),
            on_next(180, 2),
            on_next(230, 3),
            on_next(270, 4),
            on_next(340, 5),
            on_next(380, 6),
            on_next(390, 7),
            on_next(450, 8),
            on_next(470, 9),
            on_next(560, 10),
            on_next(580, 11),
            on_completed(600)
        };
        auto xs = sc.make_hot_observable(rxu::to_vector(messages));

        WHEN("filtered to ints that are primes"){

            auto res = sc.start<int>(
                [&xs, &invoked]() {
#if RXCPP_USE_OBSERVABLE_MEMBERS
                    return xs
                        .filter([&invoked](int x) {
                            invoked++;
                            return IsPrime(x);
                        })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
#else
                    return xs
                        >> rxo::filter([&invoked](int x) {
                            invoked++;
                            return IsPrime(x);
                        })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        >> rxo::as_dynamic();
#endif
                },
                400
            );

            THEN("the output only contains primes that arrived before disposal"){
                record items[] = {
                    on_next(230, 3),
                    on_next(340, 5),
                    on_next(390, 7)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                life items[] = {
                    subscribe(200, 400)
                };
                auto required = rxu::to_vector(items);
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("where was called until disposed"){
                REQUIRE(5 == invoked);
            }
        }
    }
}

SCENARIO("filter stops on error", "[where][filter][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = rxsc::make_test();
        typedef rxsc::test::messages<int> m;
        typedef rxn::subscription life;
        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_error = m::on_error;
        auto on_completed = m::on_completed;
        auto subscribe = m::subscribe;

        long invoked = 0;

        std::runtime_error ex("filter on_error from source");

        record messages[] = {
            on_next(110, 1),
            on_next(180, 2),
            on_next(230, 3),
            on_next(270, 4),
            on_next(340, 5),
            on_next(380, 6),
            on_next(390, 7),
            on_next(450, 8),
            on_next(470, 9),
            on_next(560, 10),
            on_next(580, 11),
            on_error(600, ex),
            on_next(610, 12),
            on_error(620, std::runtime_error("error in unsubscribed stream")),
            on_completed(630)
        };
        auto xs = sc.make_hot_observable(rxu::to_vector(messages));

        WHEN("filtered to ints that are primes"){

            auto res = sc.start<int>(
                [xs, &invoked]() {
#if RXCPP_USE_OBSERVABLE_MEMBERS
                    return xs
                        .filter([&invoked](int x) {
                            invoked++;
                            return IsPrime(x);
                        })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
#else
                    return xs
                        >> rxo::filter([&invoked](int x) {
                            invoked++;
                            return IsPrime(x);
                        })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        >> rxo::as_dynamic();
#endif
                }
            );

            THEN("the output only contains primes"){
                record items[] = {
                    on_next(230, 3),
                    on_next(340, 5),
                    on_next(390, 7),
                    on_next(580, 11),
                    on_error(600, ex),
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                life items[] = {
                    subscribe(200, 600)
                };
                auto required = rxu::to_vector(items);
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("where was called until error"){
                REQUIRE(9 == invoked);
            }
        }
    }
}

SCENARIO("filter stops on throw from predicate", "[where][filter][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = rxsc::make_test();
        typedef rxsc::test::messages<int> m;
        typedef rxn::subscription life;
        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_error = m::on_error;
        auto on_completed = m::on_completed;
        auto subscribe = m::subscribe;

        long invoked = 0;

        std::runtime_error ex("filter predicate error");

        record messages[] = {
            on_next(110, 1),
            on_next(180, 2),
            on_next(230, 3),
            on_next(270, 4),
            on_next(340, 5),
            on_next(380, 6),
            on_next(390, 7),
            on_next(450, 8),
            on_next(470, 9),
            on_next(560, 10),
            on_next(580, 11),
            on_completed(600),
            on_next(610, 12),
            on_error(620, std::runtime_error("error in unsubscribed stream")),
            on_completed(630)
        };
        auto xs = sc.make_hot_observable(rxu::to_vector(messages));

        WHEN("filtered to ints that are primes"){

            auto res = sc.start<int>(
                [ex, xs, &invoked]() {
#if RXCPP_USE_OBSERVABLE_MEMBERS
                    return xs
                        .filter([ex, &invoked](int x) {
                            invoked++;
                            if (x > 5) {
                                throw ex;
                            }
                            return IsPrime(x);
                        })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
#else
                    return xs
                        >> rxo::filter([ex, &invoked](int x) {
                            invoked++;
                            if (x > 5) {
                                throw ex;
                            }
                            return IsPrime(x);
                        })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        >> rxo::as_dynamic();
#endif
                }
            );

            THEN("the output only contains primes"){
                record items[] = {
                    on_next(230, 3),
                    on_next(340, 5),
                    on_error(380, ex)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                life items[] = {
                    subscribe(200, 380)
                };
                auto required = rxu::to_vector(items);
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("where was called until error"){
                REQUIRE(4 == invoked);
            }
        }
    }
}

SCENARIO("filter stops on dispose from predicate", "[where][filter][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = rxsc::make_test();
        typedef rxsc::test::messages<int> m;
        typedef rxn::subscription life;
        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_error = m::on_error;
        auto on_completed = m::on_completed;
        auto subscribe = m::subscribe;

        long invoked = 0;

        record messages[] = {
            on_next(110, 1),
            on_next(180, 2),
            on_next(230, 3),
            on_next(270, 4),
            on_next(340, 5),
            on_next(380, 6),
            on_next(390, 7),
            on_next(450, 8),
            on_next(470, 9),
            on_next(560, 10),
            on_next(580, 11),
            on_completed(600),
            on_next(610, 12),
            on_error(620, std::exception()),
            on_completed(630)
        };
        auto xs = sc.make_hot_observable(rxu::to_vector(messages));

        auto res = sc.make_subscriber<int>();

        rx::observable<int, rx::dynamic_observable<int>> ys;

        WHEN("filtered to ints that are primes"){

            sc.schedule_absolute(rxsc::test::created_time,
                [&invoked, &res, &ys, &xs](const rxsc::schedulable& scbl) {
#if RXCPP_USE_OBSERVABLE_MEMBERS
                    ys = xs
                        .filter([&invoked, &res](int x) {
                            invoked++;
                            if (x == 8)
                                res.unsubscribe();
                            return IsPrime(x);
                        });
#else
                    ys = xs
                        >> rxo::filter([&invoked, &res](int x) {
                            invoked++;
                            if (x == 8)
                                res.unsubscribe();
                            return IsPrime(x);
                        });
#endif
                });

            sc.schedule_absolute(rxsc::test::subscribed_time, [&ys, &res](const rxsc::schedulable& scbl) {
                ys.subscribe(res);
            });

            sc.schedule_absolute(rxsc::test::unsubscribed_time, [&res](const rxsc::schedulable& scbl) {
                res.unsubscribe();
            });

            sc.start();

            THEN("the output only contains primes"){
                record items[] = {
                    on_next(230, 3),
                    on_next(340, 5),
                    on_next(390, 7)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                life items[] = {
                    subscribe(200, 450)
                };
                auto required = rxu::to_vector(items);
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("where was called until disposed"){
                REQUIRE(6 == invoked);
            }
        }
    }
}
