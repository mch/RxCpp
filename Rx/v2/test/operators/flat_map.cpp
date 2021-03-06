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

SCENARIO("flat_map completes", "[flat_map][map][operators]"){
    GIVEN("two cold observables. one of ints. one of strings."){
        auto sc = rxsc::make_test();
        typedef rxsc::test::messages<int> m;
        typedef rxsc::test::messages<std::string> ms;
        typedef rxn::subscription life;

        typedef m::recorded_type irecord;
        auto ion_next = m::on_next;
        auto ion_completed = m::on_completed;
        auto isubscribe = m::subscribe;

        typedef ms::recorded_type srecord;
        auto son_next = ms::on_next;
        auto son_completed = ms::on_completed;
        auto ssubscribe = ms::subscribe;

        irecord int_messages[] = {
            ion_next(100, 4),
            ion_next(200, 2),
            ion_next(300, 3),
            ion_next(400, 1),
            ion_completed(500)
        };
        auto xs = sc.make_cold_observable(int_messages);

        srecord string_messages[] = {
            son_next(50, "foo"),
            son_next(100, "bar"),
            son_next(150, "baz"),
            son_next(200, "qux"),
            son_completed(250)
        };
        auto ys = sc.make_cold_observable(string_messages);

        WHEN("each int is mapped to the strings"){

            auto res = sc.start<std::string>(
                [&]() {
                    return xs
                        .flat_map(
                            [&](int){
                                return ys;},
                            [](int, std::string s){
                                return s;})
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains strings repeated for each int"){
                srecord items[] = {
                    son_next(350, "foo"),
                    son_next(400, "bar"),
                    son_next(450, "baz"),
                    son_next(450, "foo"),
                    son_next(500, "qux"),
                    son_next(500, "bar"),
                    son_next(550, "baz"),
                    son_next(550, "foo"),
                    son_next(600, "qux"),
                    son_next(600, "bar"),
                    son_next(650, "baz"),
                    son_next(650, "foo"),
                    son_next(700, "bar"),
                    son_next(700, "qux"),
                    son_next(750, "baz"),
                    son_next(800, "qux"),
                    son_completed(850)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ints"){
                life items[] = {
                    isubscribe(200, 700)
                };
                auto required = rxu::to_vector(items);
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there were four subscription and unsubscription to the strings"){
                life items[] = {
                    ssubscribe(300, 550),
                    ssubscribe(400, 650),
                    ssubscribe(500, 750),
                    ssubscribe(600, 850)
                };
                auto required = rxu::to_vector(items);
                auto actual = ys.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}


SCENARIO("flat_map source never ends", "[flat_map][map][operators]"){
    GIVEN("two cold observables. one of ints. one of strings."){
        auto sc = rxsc::make_test();
        typedef rxsc::test::messages<int> m;
        typedef rxsc::test::messages<std::string> ms;
        typedef rxn::subscription life;

        typedef m::recorded_type irecord;
        auto ion_next = m::on_next;
        auto isubscribe = m::subscribe;

        typedef ms::recorded_type srecord;
        auto son_next = ms::on_next;
        auto son_completed = ms::on_completed;
        auto ssubscribe = ms::subscribe;

        irecord int_messages[] = {
            ion_next(100, 4),
            ion_next(200, 2),
            ion_next(300, 3),
            ion_next(400, 1),
            ion_next(500, 5),
            ion_next(700, 0)
        };
        auto xs = sc.make_cold_observable(int_messages);

        srecord string_messages[] = {
            son_next(50, "foo"),
            son_next(100, "bar"),
            son_next(150, "baz"),
            son_next(200, "qux"),
            son_completed(250)
        };
        auto ys = sc.make_cold_observable(string_messages);

        WHEN("each int is mapped to the strings"){

            auto res = sc.start<std::string>(
                [&]() {
                    return xs
                        .flat_map([&](int){return ys;}, [](int, std::string s){return s;})
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains strings repeated for each int"){
                srecord items[] = {
                    son_next(350, "foo"),
                    son_next(400, "bar"),
                    son_next(450, "baz"),
                    son_next(450, "foo"),
                    son_next(500, "bar"),
                    son_next(500, "qux"),
                    son_next(550, "baz"),
                    son_next(550, "foo"),
                    son_next(600, "bar"),
                    son_next(600, "qux"),
                    son_next(650, "baz"),
                    son_next(650, "foo"),
                    son_next(700, "bar"),
                    son_next(700, "qux"),
                    son_next(750, "baz"),
                    son_next(750, "foo"),
                    son_next(800, "bar"),
                    son_next(800, "qux"),
                    son_next(850, "baz"),
                    son_next(900, "qux"),
                    son_next(950, "foo")
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ints"){
                life items[] = {
                    isubscribe(200, 1000)
                };
                auto required = rxu::to_vector(items);
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there were four subscription and unsubscription to the strings"){
                life items[] = {
                    ssubscribe(300, 550),
                    ssubscribe(400, 650),
                    ssubscribe(500, 750),
                    ssubscribe(600, 850),
                    ssubscribe(700, 950),
                    ssubscribe(900, 1000)
                };
                auto required = rxu::to_vector(items);
                auto actual = ys.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("flat_map inner error", "[flat_map][map][operators]"){
    GIVEN("two cold observables. one of ints. one of strings."){
        auto sc = rxsc::make_test();
        typedef rxsc::test::messages<int> m;
        typedef rxsc::test::messages<std::string> ms;
        typedef rxn::subscription life;

        typedef m::recorded_type irecord;
        auto ion_next = m::on_next;
        auto ion_completed = m::on_completed;
        auto isubscribe = m::subscribe;

        typedef ms::recorded_type srecord;
        auto son_next = ms::on_next;
        auto son_error = ms::on_error;
        auto ssubscribe = ms::subscribe;

        irecord int_messages[] = {
            ion_next(100, 4),
            ion_next(200, 2),
            ion_next(300, 3),
            ion_next(400, 1),
            ion_completed(500)
        };
        auto xs = sc.make_cold_observable(int_messages);

        std::runtime_error ex("filter on_error from inner source");

        srecord string_messages[] = {
            son_next(55, "foo"),
            son_next(104, "bar"),
            son_next(153, "baz"),
            son_next(202, "qux"),
            son_error(301, ex)
        };
        auto ys = sc.make_cold_observable(string_messages);

        WHEN("each int is mapped to the strings"){

            auto res = sc.start<std::string>(
                [&]() {
                    return xs
                        .flat_map([&](int){return ys;}, [](int, std::string s){return s;})
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains strings repeated for each int"){
                srecord items[] = {
                    son_next(355, "foo"),
                    son_next(404, "bar"),
                    son_next(453, "baz"),
                    son_next(455, "foo"),
                    son_next(502, "qux"),
                    son_next(504, "bar"),
                    son_next(553, "baz"),
                    son_next(555, "foo"),
                    son_error(601, ex)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ints"){
                life items[] = {
                    isubscribe(200, 601)
                };
                auto required = rxu::to_vector(items);
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there were four subscription and unsubscription to the strings"){
                life items[] = {
                    ssubscribe(300, 601),
                    ssubscribe(400, 601),
                    ssubscribe(500, 601),
                    ssubscribe(600, 601)
                };
                auto required = rxu::to_vector(items);
                auto actual = ys.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}
