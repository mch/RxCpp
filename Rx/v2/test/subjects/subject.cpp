
#define RXCPP_SUBJECT_TEST_ASYNC 1

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


SCENARIO("subject test", "[hide][subject][subjects][perf]"){
    GIVEN("a subject"){
        WHEN("multicasting a million ints"){
            using namespace std::chrono;
            typedef steady_clock clock;

            const int onnextcalls = 100000000;

            {
                std::mutex m;
                int c = 0;
                int n = 1;
                auto start = clock::now();
                for (int i = 0; i < onnextcalls; i++) {
                    std::unique_lock<std::mutex> guard(m);
                    ++c;
                }
                auto finish = clock::now();
                auto msElapsed = duration_cast<milliseconds>(finish.time_since_epoch()) -
                       duration_cast<milliseconds>(start.time_since_epoch());
                std::cout << "loop mutex          : " << n << " subscribed, " << c << " on_next calls, " << msElapsed.count() << "ms elapsed " << std::endl;
            }

            {
                int c = 0;
                int n = 1;
#if 0
                auto onnext = [&c](int){++c;};
                auto onerror = [](std::exception_ptr){abort();};

                auto oex = rxu::detail::arg_resolver_set<rx::detail::tag_observer_set<int>::type>()(onnext, onerror);
#endif
                auto o = rx::make_observer<int>(
                    [&c](int){++c;},
                    [](std::exception_ptr){abort();});
                auto start = clock::now();
                for (int i = 0; i < onnextcalls; i++) {
                    o.on_next(i);
                }
                o.on_completed();
                auto finish = clock::now();
                auto msElapsed = duration_cast<milliseconds>(finish.time_since_epoch()) -
                       duration_cast<milliseconds>(start.time_since_epoch());
                std::cout << "loop -> observer    : " << n << " subscribed, " << c << " on_next calls, " << msElapsed.count() << "ms elapsed " << std::endl;
            }

            {
                int c = 0;
                int n = 1;
                auto o = rx::make_observer<int>(
                    [&c](int){++c;},
                    [](std::exception_ptr){abort();});
                auto start = clock::now();
                for (int i = 0; i < onnextcalls; i++) {
                    o.on_next(i);
                }
                o.on_completed();
                auto finish = clock::now();
                auto msElapsed = duration_cast<milliseconds>(finish.time_since_epoch()) -
                       duration_cast<milliseconds>(start.time_since_epoch());
                std::cout << "loop -> subscriber  : " << n << " subscribed, " << c << " on_next calls, " << msElapsed.count() << "ms elapsed " << std::endl;
            }

            {
                int c = 0;
                int n = 1;
                auto start = clock::now();
#if 0
                auto rso = rxu::detail::arg_resolver_set<rx::detail::tag_subscriber_set<int>::type>()(
                    [&c](int){
                        ++c;
                    },
                    [](std::exception_ptr){abort();});
                static_assert(std::tuple_size<decltype(rso)>::value == 7, "object must resolve 7 args");
                auto rsf = rxu::detail::resolve_arg_set(
                    rx::detail::tag_subscriber_set<int>::type(),
                    [&c](int){
                        ++c;
                    },
                    [](std::exception_ptr){abort();});
                static_assert(std::tuple_size<decltype(rsf)>::value == 7, "func must resolve 7 args");
                auto ss = rx::detail::make_subscriber_resolved<int>(rsf);
#endif
                rxs::range<int>(0, onnextcalls).subscribe(
                    [&c](int){
                        ++c;
                    },
                    [](std::exception_ptr){abort();});
                auto finish = clock::now();
                auto msElapsed = duration_cast<milliseconds>(finish.time_since_epoch()) -
                       duration_cast<milliseconds>(start.time_since_epoch());
                std::cout << "range -> subscriber : " << n << " subscribed, " << c << " on_next calls, " << msElapsed.count() << "ms elapsed " << std::endl;
            }

            for (int n = 0; n < 10; n++)
            {
                auto p = std::make_shared<int>(0);
                auto c = std::make_shared<int>(0);
                rxsub::subject<int> sub;

#if RXCPP_SUBJECT_TEST_ASYNC
                std::vector<std::future<int>> f(n);
#endif

                auto o = sub.get_subscriber();

                o.add(rx::make_subscription([c, n, onnextcalls](){
                    auto expected = n * onnextcalls;
                    REQUIRE(*c == expected);
                }));

                for (int i = 0; i < n; i++) {
#if RXCPP_SUBJECT_TEST_ASYNC
                    f[i] = std::async([sub, o]() {
                        auto source = sub.get_observable();
                        while(o.is_subscribed()) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                            rx::composite_subscription cs;
                            source.subscribe(cs, [cs](int){
                                cs.unsubscribe();
                            },
                            [](std::exception_ptr){abort();});
                        }
                        return 0;
                    });
#endif
                    sub.get_observable().subscribe(
                        [c, p](int){
                            ++(*c);
                        },
                        [](std::exception_ptr){abort();});
                }

                auto start = clock::now();
                for (int i = 0; i < onnextcalls; i++) {
#if RXCPP_DEBUG_SUBJECT_RACE
                    if (*p != *c) abort();
                    (*p) += n;
#endif
                    o.on_next(i);
                }
                o.on_completed();
                auto finish = clock::now();
                auto msElapsed = duration_cast<milliseconds>(finish.time_since_epoch()) -
                       duration_cast<milliseconds>(start.time_since_epoch());
                std::cout << "loop -> subject     : " << n << " subscribed, " << (*c) << " on_next calls, " << msElapsed.count() << "ms elapsed " << std::endl;
            }

            for (int n = 0; n < 10; n++)
            {
                auto p = std::make_shared<int>(0);
                auto c = std::make_shared<int>(0);
                rxsub::subject<int> sub;

#if RXCPP_SUBJECT_TEST_ASYNC
                std::vector<std::future<int>> f(n);
#endif

                auto o = sub.get_subscriber();

                o.add(rx::make_subscription([c, n, onnextcalls](){
                    auto expected = n * onnextcalls;
                    REQUIRE(*c == expected);
                }));

                for (int i = 0; i < n; i++) {
#if RXCPP_SUBJECT_TEST_ASYNC
                    f[i] = std::async([sub, o]() {
                        while(o.is_subscribed()) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                            rx::composite_subscription cs;
                            sub.get_observable().subscribe(cs, [cs](int){
                                cs.unsubscribe();
                            },
                            [](std::exception_ptr){abort();});
                        }
                        return 0;
                    });
#endif
                    sub.get_observable()
                        .subscribe(
                            [c, p](int){
                               ++(*c);
                            },
                            [](std::exception_ptr){abort();}
                        );
                }

                auto start = clock::now();
                rxs::range<int>(0, onnextcalls)
#if RXCPP_DEBUG_SUBJECT_RACE
                    .filter([c, p, n](int){
                        if (*p != *c) abort();
                        (*p) += n;
                        return true;
                    })
#endif
                    .subscribe(o);
                auto finish = clock::now();
                auto msElapsed = duration_cast<milliseconds>(finish.time_since_epoch()) -
                       duration_cast<milliseconds>(start.time_since_epoch());
                std::cout << "range -> subject    : " << n << " subscribed, " << (*c) << " on_next calls, " << msElapsed.count() << "ms elapsed " << std::endl;
            }
        }
    }
}



SCENARIO("subject - infinite source", "[subject][subjects]"){
    GIVEN("a subject and an infinite source"){

        auto sc = rxsc::make_test();
        typedef rxsc::test::messages<int> m;
        typedef rxn::subscription life;
        typedef m::recorded_type record;
        auto on_next = m::on_next;

        record messages[] = {
            on_next(70, 1),
            on_next(110, 2),
            on_next(220, 3),
            on_next(270, 4),
            on_next(340, 5),
            on_next(410, 6),
            on_next(520, 7),
            on_next(630, 8),
            on_next(710, 9),
            on_next(870, 10),
            on_next(940, 11),
            on_next(1020, 12)
        };
        auto xs = sc.make_hot_observable(messages);

        rxsub::subject<int> s;

        auto results1 = sc.make_subscriber<int>();

        auto results2 = sc.make_subscriber<int>();

        auto results3 = sc.make_subscriber<int>();

        WHEN("multicasting an infinite source"){

            auto o = s.get_subscriber();

            sc.schedule_absolute(100, [&s, &o](const rxsc::schedulable& scbl){
                s = rxsub::subject<int>(); o = s.get_subscriber();});
            sc.schedule_absolute(200, [&xs, &o](const rxsc::schedulable& scbl){
                xs.subscribe(o);});
            sc.schedule_absolute(1000, [&o](const rxsc::schedulable& scbl){
                o.unsubscribe();});

            sc.schedule_absolute(300, [&s, &results1](const rxsc::schedulable& scbl){
                s.get_observable().subscribe(results1);});
            sc.schedule_absolute(400, [&s, &results2](const rxsc::schedulable& scbl){
                s.get_observable().subscribe(results2);});
            sc.schedule_absolute(900, [&s, &results3](const rxsc::schedulable& scbl){
                s.get_observable().subscribe(results3);});

            sc.schedule_absolute(600, [&results1](const rxsc::schedulable& scbl){
                results1.unsubscribe();});
            sc.schedule_absolute(700, [&results2](const rxsc::schedulable& scbl){
                results2.unsubscribe();});
            sc.schedule_absolute(800, [&results1](const rxsc::schedulable& scbl){
                results1.unsubscribe();});
            sc.schedule_absolute(950, [&results3](const rxsc::schedulable& scbl){
                results3.unsubscribe();});

            sc.start();

            THEN("result1 contains expected messages"){
                record items[] = {
                    on_next(340, 5),
                    on_next(410, 6),
                    on_next(520, 7)
                };
                auto required = rxu::to_vector(items);
                auto actual = results1.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("result2 contains expected messages"){
                record items[] = {
                    on_next(410, 6),
                    on_next(520, 7),
                    on_next(630, 8)
                };
                auto required = rxu::to_vector(items);
                auto actual = results2.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("result3 contains expected messages"){
                record items[] = {
                    on_next(940, 11)
                };
                auto required = rxu::to_vector(items);
                auto actual = results3.get_observer().messages();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("subject - finite source", "[subject][subjects]"){
    GIVEN("a subject and an finite source"){

        auto sc = rxsc::make_test();
        typedef rxsc::test::messages<int> m;
        typedef rxn::subscription life;
        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_error = m::on_error;
        auto on_completed = m::on_completed;

        record messages[] = {
            on_next(70, 1),
            on_next(110, 2),
            on_next(220, 3),
            on_next(270, 4),
            on_next(340, 5),
            on_next(410, 6),
            on_next(520, 7),
            on_completed(630),
            on_next(640, 9),
            on_completed(650),
            on_error(660, std::runtime_error("error on unsubscribed stream"))
        };
        auto xs = sc.make_hot_observable(messages);

        rxsub::subject<int> s;

        auto results1 = sc.make_subscriber<int>();

        auto results2 = sc.make_subscriber<int>();

        auto results3 = sc.make_subscriber<int>();

        WHEN("multicasting an infinite source"){

            auto o = s.get_subscriber();

            sc.schedule_absolute(100, [&s, &o](const rxsc::schedulable& scbl){
                s = rxsub::subject<int>(); o = s.get_subscriber();});
            sc.schedule_absolute(200, [&xs, &o](const rxsc::schedulable& scbl){
                xs.subscribe(o);});
            sc.schedule_absolute(1000, [&o](const rxsc::schedulable& scbl){
                o.unsubscribe();});

            sc.schedule_absolute(300, [&s, &results1](const rxsc::schedulable& scbl){
                s.get_observable().subscribe(results1);});
            sc.schedule_absolute(400, [&s, &results2](const rxsc::schedulable& scbl){
                s.get_observable().subscribe(results2);});
            sc.schedule_absolute(900, [&s, &results3](const rxsc::schedulable& scbl){
                s.get_observable().subscribe(results3);});

            sc.schedule_absolute(600, [&results1](const rxsc::schedulable& scbl){
                results1.unsubscribe();});
            sc.schedule_absolute(700, [&results2](const rxsc::schedulable& scbl){
                results2.unsubscribe();});
            sc.schedule_absolute(800, [&results1](const rxsc::schedulable& scbl){
                results1.unsubscribe();});
            sc.schedule_absolute(950, [&results3](const rxsc::schedulable& scbl){
                results3.unsubscribe();});

            sc.start();

            THEN("result1 contains expected messages"){
                record items[] = {
                    on_next(340, 5),
                    on_next(410, 6),
                    on_next(520, 7)
                };
                auto required = rxu::to_vector(items);
                auto actual = results1.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("result2 contains expected messages"){
                record items[] = {
                    on_next(410, 6),
                    on_next(520, 7),
                    on_completed(630)
                };
                auto required = rxu::to_vector(items);
                auto actual = results2.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("result3 contains expected messages"){
                record items[] = {
                    on_completed(900)
                };
                auto required = rxu::to_vector(items);
                auto actual = results3.get_observer().messages();
                REQUIRE(required == actual);
            }

        }
    }
}


SCENARIO("subject - on_error in source", "[subject][subjects]"){
    GIVEN("a subject and a source with an error"){

        auto sc = rxsc::make_test();
        typedef rxsc::test::messages<int> m;
        typedef rxn::subscription life;
        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_error = m::on_error;
        auto on_completed = m::on_completed;

        std::runtime_error ex("subject on_error in stream");

        record messages[] = {
            on_next(70, 1),
            on_next(110, 2),
            on_next(220, 3),
            on_next(270, 4),
            on_next(340, 5),
            on_next(410, 6),
            on_next(520, 7),
            on_error(630, ex),
            on_next(640, 9),
            on_completed(650),
            on_error(660, std::runtime_error("error on unsubscribed stream"))
        };
        auto xs = sc.make_hot_observable(messages);

        rxsub::subject<int> s;

        auto results1 = sc.make_subscriber<int>();

        auto results2 = sc.make_subscriber<int>();

        auto results3 = sc.make_subscriber<int>();

        WHEN("multicasting an infinite source"){

            auto o = s.get_subscriber();

            sc.schedule_absolute(100, [&s, &o](const rxsc::schedulable& scbl){
                s = rxsub::subject<int>(); o = s.get_subscriber();});
            sc.schedule_absolute(200, [&xs, &o](const rxsc::schedulable& scbl){
                xs.subscribe(o);});
            sc.schedule_absolute(1000, [&o](const rxsc::schedulable& scbl){
                o.unsubscribe();});

            sc.schedule_absolute(300, [&s, &results1](const rxsc::schedulable& scbl){
                s.get_observable().subscribe(results1);});
            sc.schedule_absolute(400, [&s, &results2](const rxsc::schedulable& scbl){
                s.get_observable().subscribe(results2);});
            sc.schedule_absolute(900, [&s, &results3](const rxsc::schedulable& scbl){
                s.get_observable().subscribe(results3);});

            sc.schedule_absolute(600, [&results1](const rxsc::schedulable& scbl){
                results1.unsubscribe();});
            sc.schedule_absolute(700, [&results2](const rxsc::schedulable& scbl){
                results2.unsubscribe();});
            sc.schedule_absolute(800, [&results1](const rxsc::schedulable& scbl){
                results1.unsubscribe();});
            sc.schedule_absolute(950, [&results3](const rxsc::schedulable& scbl){
                results3.unsubscribe();});

            sc.start();

            THEN("result1 contains expected messages"){
                record items[] = {
                    on_next(340, 5),
                    on_next(410, 6),
                    on_next(520, 7)
                };
                auto required = rxu::to_vector(items);
                auto actual = results1.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("result2 contains expected messages"){
                record items[] = {
                    on_next(410, 6),
                    on_next(520, 7),
                    on_error(630, ex)
                };
                auto required = rxu::to_vector(items);
                auto actual = results2.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("result3 contains expected messages"){
                record items[] = {
                    on_error(900, ex)
                };
                auto required = rxu::to_vector(items);
                auto actual = results3.get_observer().messages();
                REQUIRE(required == actual);
            }

        }
    }
}
