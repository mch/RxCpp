// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

// testbench.cpp : Defines the entry point for the console application.
//

#include "cpprx/rx.hpp"
#include "cpplinq/linq.hpp"

#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <string>
#include <exception>
#include <regex>

using namespace std;

bool IsPrime(int x);

struct Count {
    Count() : subscriptions(0), nexts(0), completions(0), errors(0), disposals(0) {}
    std::atomic<int> subscriptions;
    std::atomic<int> nexts;
    std::atomic<int> completions;
    std::atomic<int> errors;
    std::atomic<int> disposals;
};
template <class T>
std::shared_ptr<rxcpp::Observable<T>> Record(
    const std::shared_ptr<rxcpp::Observable<T>>& source,
    Count* count
)
{
    return rxcpp::CreateObservable<T>(
      [=](std::shared_ptr<rxcpp::Observer<T>> observer) -> rxcpp::Disposable
      {
          ++count->subscriptions;
          rxcpp::ComposableDisposable cd;
          cd.Add(rxcpp::Disposable([=](){
              ++count->disposals;}));
          cd.Add(rxcpp::Subscribe(
              source,
              // on next
              [=](T element)
              {
                  ++count->nexts;
                  observer->OnNext(std::move(element));
              },
              // on completed
              [=]
              {
                  ++count->completions;
                  observer->OnCompleted();
              },
              // on error
              [=](const std::exception_ptr& error)
              {
                  ++count->errors;
                  observer->OnError(error);
              }));
          return cd;
      });
}
struct record {};
template<class T>
rxcpp::Binder<std::shared_ptr<rxcpp::Observable<T>>> rxcpp_chain(record&&, const std::shared_ptr<rxcpp::Observable<T>>& source, Count* count) {
    return rxcpp::from(Record(source, count));
}

vector<int> vector_range(int start, int end)
{
    vector<int> v;
    for (int i = start; i < end; ++i)
        v.push_back(i);
    return v;
}

void IxToRx(int n) {
    Count sourcecount, takencount, observedcount;

    std::cout << "IxToRx: first " << n << " primes squared" << endl;
    auto values = vector_range(2, n * 10);

    auto primes = cpplinq::from(values)
        .where(IsPrime)
        .select([](int x) { return std::make_pair(x,  x*x); });

    auto output = std::make_shared<rxcpp::EventLoopScheduler>();
    rxcpp::from(rxcpp::Iterate(primes))
        .template chain<record>(&sourcecount)
        .take(n)
        .template chain<record>(&takencount)
        .observe_on(output)
        .template chain<record>(&observedcount)
        .for_each(rxcpp::MakeTupleDispatch(
            [](int p, int s) {
                cout << p << " =square=> " << s << endl;
            }));

    cout << "location: subscriptions, nexts, completions, errors, disposals" << endl;
    cout << "sourcecount:   " << sourcecount.subscriptions << ", " << sourcecount.nexts << ", " << sourcecount.completions << ", " << sourcecount.errors << ", " << sourcecount.disposals << endl;
    cout << "takencount:    " << takencount.subscriptions << ", " << takencount.nexts << ", " << takencount.completions << ", " << takencount.errors << ", " << takencount.disposals << endl;
    cout << "observedcount: " << observedcount.subscriptions << ", " << observedcount.nexts << ", " << observedcount.completions << ", " << observedcount.errors << ", " << observedcount.disposals << endl;
}

void PrintPrimes(int n)
{
    std::cout << "Rx: first " << n << " primes squared" << endl;
    auto values = rxcpp::Range(2); // infinite (until overflow) stream of integers
    rxcpp::from(values)
        .where(IsPrime)
        .materialize()
        .dematerialize()
        .select([](int x) { return std::make_pair(x,  x*x); })
        .take(n)
        .for_each(rxcpp::MakeTupleDispatch(
            [](int p, int s) {
                cout << p << " =square=> " << s << endl;
            }));
}

void Scan()
{
    int test = 0;
    auto next = [&](int i) {
        cout << "next " << test << ":" << i << endl;
    };
    auto complete = [&]() {
        cout << "completed " << test << endl;
    };
    auto error = [&](std::exception_ptr e) {
        try {
            std::rethrow_exception(e);
        }
        catch(std::exception ex) {
            cout << ex.what() << endl;
        }
    };

    auto input = std::make_shared<rxcpp::ImmediateScheduler>();

    auto add = [](int x, int y){return x + y;};

    // Usage
    auto obs1 = rxcpp::from(rxcpp::Empty<int>(input)).scan(add);
    auto obs2 = rxcpp::from(rxcpp::Empty<int>(input)).scan(0, add);
     
    auto obs3 = rxcpp::from(rxcpp::Range(1, 3, 1, input)).scan(add);
    auto obs4 = rxcpp::from(rxcpp::Range(1, 3, 1, input)).scan(0, add);

    test = 1;
    obs1.subscribe(next, complete, error);
    // => completed 1
     
    test = 2;
    obs2.subscribe(next, complete, error);
    // => next 2:0 
    // => completed 2
     
    test = 3;
    obs3.subscribe(next, complete, error);
    // => next 3:1 
    // => next 3:3 
    // => next 3:6 
    // => completed 3
     
    test = 4;
    obs4.subscribe(next, complete, error);
    // => next 4:1 
    // => next 4:3 
    // => next 4:6 
    // => completed 4
}

void Concat(int n)
{
    auto input1 = std::make_shared<rxcpp::EventLoopScheduler>();
    auto input2 = std::make_shared<rxcpp::EventLoopScheduler>();
    auto output = std::make_shared<rxcpp::EventLoopScheduler>();

    auto values1 = rxcpp::Range(100); // infinite (until overflow) stream of integers
    auto s1 = rxcpp::from(values1)
        .subscribe_on(input1)
        .where(IsPrime)
        .select([](int prime) -> std::tuple<const char *, int> {this_thread::yield(); return std::make_tuple("1:", prime);})
        .take(n/2);

    auto values2 = rxcpp::Range(2); // infinite (until overflow) stream of integers
    auto s2 = rxcpp::from(values2)
        .subscribe_on(input2)
        .where(IsPrime)
        .select([](int prime) -> std::tuple<const char *, int> {this_thread::yield(); return std::make_tuple("2:", prime);})
        .take(n/2);

    rxcpp::from(s2)
        .concat(s1)
        .take(n)
        .observe_on(output)
        .for_each(rxcpp::MakeTupleDispatch(
            [](const char* s, int p) {
                cout << s << " =concat=> " << p << endl;
            }));
}

void Combine(int n)
{
    auto input1 = std::make_shared<rxcpp::EventLoopScheduler>();
    auto input2 = std::make_shared<rxcpp::EventLoopScheduler>();
    auto output = std::make_shared<rxcpp::EventLoopScheduler>();

    auto values1 = rxcpp::Range(100); // infinite (until overflow) stream of integers
    auto s1 = rxcpp::from(values1)
        .subscribe_on(input1)
        .where(IsPrime)
        .select([](int prime) -> int {this_thread::yield(); return prime;});

    auto values2 = rxcpp::Range(2); // infinite (until overflow) stream of integers
    rxcpp::from(values2)
        .subscribe_on(input2)
        .where(IsPrime)
        .select([](int prime) -> int {this_thread::yield(); return prime;})
        .combine_latest(s1)
        .take(n)
        .observe_on(output)
        .for_each(rxcpp::MakeTupleDispatch(
            [](int p2, int p1) {
                cout << p2 << " =combined=> " << p1 << endl;
            }));
}



template<class InputScheduler, class OutputScheduler>
void Zip(int n)
{
    auto input1 = std::make_shared<InputScheduler>();
    auto input2 = std::make_shared<InputScheduler>();
    auto output = std::make_shared<OutputScheduler>();

    Count s1count, s2count, zipcount, takecount, outputcount;

    auto values1 = rxcpp::Range(100); // infinite (until overflow) stream of integers
    auto s1 = rxcpp::from(values1)
        .subscribe_on(input1)
        .where(IsPrime)
        .template chain<record>(&s1count)
        .select([](int prime) -> int {this_thread::yield(); return prime;});

    auto values2 = rxcpp::Range(2); // infinite (until overflow) stream of integers
    rxcpp::from(values2)
        .subscribe_on(input2)
        .where(IsPrime)
        .template chain<record>(&s2count)
        .select([](int prime) -> int {this_thread::yield(); return prime;})
        .zip(s1)
        .template chain<record>(&zipcount)
        .take(n)
        .template chain<record>(&takecount)
        .observe_on(output)
        .template chain<record>(&outputcount)
        .for_each(rxcpp::MakeTupleDispatch(
            [](int p2, int p1) {
                cout << p2 << " =zipped=> " << p1 << endl;
            }));

    cout << "location: subscriptions, nexts, completions, errors, disposals" << endl;
    cout << "s1count:     " << s1count.subscriptions << ", " << s1count.nexts << ", " << s1count.completions << ", " << s1count.errors << ", " << s1count.disposals << endl;
    cout << "s2count:     " << s2count.subscriptions << ", " << s2count.nexts << ", " << s2count.completions << ", " << s2count.errors << ", " << s2count.disposals << endl;
    cout << "zipcount:    " << zipcount.subscriptions << ", " << zipcount.nexts << ", " << zipcount.completions << ", " << zipcount.errors << ", " << zipcount.disposals << endl;
    cout << "takecount:   " << takecount.subscriptions << ", " << takecount.nexts << ", " << takecount.completions << ", " << takecount.errors << ", " << takecount.disposals << endl;
    cout << "outputcount: " << outputcount.subscriptions << ", " << outputcount.nexts << ", " << outputcount.completions << ", " << outputcount.errors << ", " << outputcount.disposals << endl;
}

void Merge(int n)
{
    auto input1 = std::make_shared<rxcpp::EventLoopScheduler>();
    auto input2 = std::make_shared<rxcpp::EventLoopScheduler>();
    auto output = std::make_shared<rxcpp::EventLoopScheduler>();

    cout << "merge==> <source>: <prime>" << endl;

    auto values1 = rxcpp::Range(100); // infinite (until overflow) stream of integers
    auto s1 = rxcpp::from(values1)
        .subscribe_on(input1)
        .where(IsPrime)
        .select([](int prime1) -> std::tuple<const char *, int> {this_thread::yield(); return std::make_tuple("1: ", prime1);});

    auto values2 = rxcpp::Range(2); // infinite (until overflow) stream of integers
    rxcpp::from(values2)
        .subscribe_on(input2)
        .where(IsPrime)
        .select([](int prime2) -> std::tuple<const char *, int> {this_thread::yield(); return std::make_tuple("2: ", prime2);})
        .merge(s1)
        .take(n)
        .observe_on(output)
        .for_each(rxcpp::MakeTupleDispatch(
            [](const char* s, int p) {
                cout << s << p << endl;
            }));
}

void RefCount(int n)
{
    auto loop = std::make_shared<rxcpp::EventLoopScheduler>();

    auto values1 = rxcpp::from(rxcpp::Range(1))
        .where(IsPrime)
        .select([](int p) -> int {cout << endl << "producing: " << p << "-> "; return p;})
        .publish()
        .ref_count(); // infinite (until overflow) stream of prime integers

    auto v1 = rxcpp::from(values1)
        .select([](int prime1) -> std::tuple<const char *, int> {return std::make_tuple("1: ", prime1);});

    auto v2 = rxcpp::from(values1)
        .select([](int prime1) -> std::tuple<const char *, int> {return std::make_tuple("2: ", prime1);});

    cout << "Merge 2 subscriptions to published primes:";
    rxcpp::from(v1)
        .merge(v2)
        .subscribe_on(loop)
        .take(n)
        .for_each(rxcpp::MakeTupleDispatch(
            [](const char* s, int p) {
                cout << s << p << ", ";
            }));

    auto values2 = rxcpp::from(rxcpp::Range(100))
        .where(IsPrime)
        .select([](int p) -> int {cout << endl << "producing: " << p << "-> "; return p;})
        .publish(1000)
        .ref_count(); // infinite (until overflow) stream of prime integers

    cout << endl << "Subscription 1 - published primes:" << endl;
    rxcpp::from(values2)
        .subscribe_on(loop)
        .take(n/2)
        .for_each(
            [](int p) {
                cout << p << ", ";
            });

    cout << endl << "Subscription 2 - published primes:" << endl;
    rxcpp::from(values2)
        .subscribe_on(loop)
        .take(n/2)
        .for_each(
            [](int p) {
                cout << p << ", ";
            });

    auto values3 = rxcpp::from(rxcpp::Range(200))
        .where(IsPrime)
        .select([](int p) -> int {cout << endl << "producing: " << p << "-> "; return p;})
        .take(n/2)
        .publish_last()
        .ref_count(); // last of n/2 prime integers

    cout << endl << "Subscription 1 - last published prime:";
    rxcpp::from(values3)
        .subscribe_on(loop)
        .for_each(
            [](int p) {
                cout << p << ", ";
            });

    cout << endl << "Subscription 2 - last published prime:" << endl;
    rxcpp::from(values3)
        .subscribe_on(loop)
        .for_each(
            [](int p) {
                cout << p << ", ";
            });
    cout << endl;
}


void PrintIntervals(int n) {
    using namespace std::chrono;
    typedef steady_clock clock;
    struct Tick {
        Tick(size_t c, clock::time_point at) : cursor(c), at(at) {}
        size_t cursor;
        clock::time_point at;
    };
    auto source = std::make_shared<rxcpp::EventLoopScheduler>();
    auto subject = rxcpp::CreateSubject<Tick>();

    cout << n << " Intervals of .5 second: " << endl;
    rxcpp::from(subject)
        .zip(rxcpp::from(subject).skip(1))
        .select(rxcpp::MakeTupleDispatch(
            [=](Tick a, Tick b){
                return duration_cast<milliseconds>(b.at.time_since_epoch()) - 
                       duration_cast<milliseconds>(a.at.time_since_epoch());}))
        .to_vector()
        .subscribe(
        // on next
            [=](std::vector<milliseconds> d)
            {
                cout << endl;
                auto l = std::max_element(d.begin(), d.end());
                auto s = std::min_element(d.begin(), d.end());
                cout << "range: " << s->count() << "ms-" << l->count() << "ms" << endl;
            });

    rxcpp::from(rxcpp::Interval(std::chrono::milliseconds(500), source))
        .select([](size_t interval){return Tick(interval, clock::now());})
        .take(n)
        .for_each(
        // on next
            [=](Tick t)
            {
                cout << "." << flush;
                subject->OnNext(std::move(t));
            });
    subject->OnCompleted();
}

std::shared_ptr<rxcpp::Observable<string>> Data(
    string filename,
    rxcpp::Scheduler::shared scheduler = std::make_shared<rxcpp::CurrentThreadScheduler>()
);
string extract_value(const string& input, const string& key);

void run()
{
    using namespace cpplinq;

    struct item {
        string args;
        int    concurrency;
        double time;

        item(const item& other) : args(other.args), concurrency(other.concurrency), time(other.time) {
        }
        item(item&& other) : args(std::move(other.args)), concurrency(std::move(other.concurrency)), time(std::move(other.time)) {
        }
        item(const string& input) {
            args =              extract_value(input, "args");
            concurrency = atoi( extract_value(input, "concurrency").c_str() );
            time =        atof( extract_value(input, "time").c_str() );
        }
        item& operator=(item other){
            using std::swap;
            swap(args, other.args);
            swap(concurrency, other.concurrency);
            swap(time, other.time);
            return *this;
        }
    };

    auto input = std::make_shared<rxcpp::EventLoopScheduler>();
    auto output = std::make_shared<rxcpp::EventLoopScheduler>();

    auto dataLines = Data("data.txt");

    rxcpp::from(dataLines)
        .subscribe_on(input)
        // parse input into items
        .select([](const string& line) { 
            return item(line);}
        )
        // group items by args field
        .group_by([](const item& i) {
            return i.args;}
        )
        // flatten concurrencies in the same args
        .select_many(
            [](const std::shared_ptr<rxcpp::GroupedObservable<std::string, item>> & argsGroup){
                return rxcpp::from(argsGroup)
                    // group items by concurrency field
                    .group_by([](const item& i){
                        return i.concurrency;}
                    )
                    // flatten times in the same concurrency
                    .select_many(
                        [](const std::shared_ptr<rxcpp::GroupedObservable<int, item>> & concurrencyGroup){
                            return rxcpp::from(concurrencyGroup)
                                .select([](const item& i){
                                    return i.time;})
                                .to_vector();}, 
                        [](const std::shared_ptr<rxcpp::GroupedObservable<int, item>> & concurrencyGroup,
                            const std::vector<double> & times){
                                return std::make_tuple(concurrencyGroup->Key(), times);}
                    )
                    .to_vector();}, 
            [](const std::shared_ptr<rxcpp::GroupedObservable<std::string, item>> & argsGroup,
                const std::vector<std::tuple<int, std::vector<double>>> & ouputGroup){
                    return std::make_tuple(argsGroup->Key(), ouputGroup);}
        )
        .observe_on(output)
        .for_each(rxcpp::MakeTupleDispatch(
            [](const std::string& args, const std::vector<std::tuple<int, std::vector<double>>>& concurrencyGroup){
            cout<<"arguments: "<< args << endl;
            cout << "concurrency, mean, |, raw_data," << endl;
            for(auto& concurrencyItem : concurrencyGroup) {
                rxcpp::MakeTupleDispatch(
                    [](int concurrency, const std::vector<double>& rawtimes){
                    cout << concurrency << ", ";
                            
                    auto n = from(rawtimes).count();
                    auto sum = std::accumulate(rawtimes.begin(), rawtimes.end(), 0.0);

                    cout << (sum / n) << ", |";

                    for (auto timeIter = rawtimes.begin(), end = rawtimes.end();
                        timeIter != end;
                        ++timeIter)
                    {
                        cout << ", " << *timeIter;
                    }
                    cout << endl;})(concurrencyItem);}})
        );
}

template<class Scheduler>
void innerScheduler() {
    auto outer = std::make_shared<Scheduler>();
    rxcpp::Scheduler::shared inner;
    std::mutex lock;
    std::condition_variable wake;
    outer->Schedule([&](rxcpp::Scheduler::shared s) -> rxcpp::Disposable {
        std::lock_guard<std::mutex> guard(lock);
        inner = s; wake.notify_one();
        return rxcpp::Disposable::Empty();});
    {
        std::unique_lock<std::mutex> guard(lock);
        wake.wait(guard, [&]{return !!inner;});
    }
    inner->Schedule([&](rxcpp::Scheduler::shared s) -> rxcpp::Disposable {
        std::lock_guard<std::mutex> guard(lock);
        inner = nullptr; wake.notify_one();
        return rxcpp::Disposable::Empty();});
    {
        std::unique_lock<std::mutex> guard(lock);
        wake.wait(guard, [&]{return !inner;});
    }
    cout << "innerScheduler test succeeded" << endl;
}

int main(int argc, char* argv[])
{
    try {
        RefCount(20);
        PrintIntervals(10);
        IxToRx(20);
        PrintPrimes(20);
        cout << "Zip Immediate" << endl;
        Zip<rxcpp::ImmediateScheduler, rxcpp::ImmediateScheduler>(20);
        cout << "Zip Current" << endl;
        Zip<rxcpp::CurrentThreadScheduler, rxcpp::CurrentThreadScheduler>(20);
        cout << "Zip EventLoop" << endl;
        Zip<rxcpp::EventLoopScheduler, rxcpp::EventLoopScheduler>(20);
        Combine(20);
        Merge(20);
        Concat(20);

        innerScheduler<rxcpp::ImmediateScheduler>();
        innerScheduler<rxcpp::CurrentThreadScheduler>();
        innerScheduler<rxcpp::EventLoopScheduler>();

        Scan();

        run();
    } catch (exception& e) {
        cerr << "exception: " << e.what() << endl;
    }
}

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

regex key_value_pair("'([^\']*)'\\s*[:,]\\s*(\\d+(?:\\.\\d+)?|'[^']*')");

string extract_value(const string& input, const string& key)
{
    const std::sregex_iterator end;
    for (std::sregex_iterator i(input.cbegin(), input.cend(), key_value_pair);
        i != end;
        ++i)
    {
        if ((*i)[1] == key)
        {
            return (*i)[2];
        }
    }
    throw std::range_error("search key not found");
}

std::shared_ptr<rxcpp::Observable<string>> Data(
    string filename,
    rxcpp::Scheduler::shared scheduler
)
{
    return rxcpp::CreateObservable<string>(
        [=](std::shared_ptr<rxcpp::Observer<string>> observer) 
        -> rxcpp::Disposable
        {
            struct State 
            {
                State(string filename) 
                    : cancel(false), data(filename) {
                        if (data.fail()) {
                            throw logic_error("could not find file");
                        }
                    }
                bool cancel;
                ifstream data;
            };
            auto state = std::make_shared<State>(std::move(filename));

            rxcpp::ComposableDisposable cd;

            cd.Add(rxcpp::Disposable([=]{
                state->cancel = true;
            }));

            cd.Add(scheduler->Schedule(
                rxcpp::fix0([=](rxcpp::Scheduler::shared s, std::function<rxcpp::Disposable(rxcpp::Scheduler::shared)> self) -> rxcpp::Disposable
                {
                    if (state->cancel)
                        return rxcpp::Disposable::Empty();

                    string line;
                    if (!!getline(state->data, line))
                    {
                        observer->OnNext(std::move(line));
                        return s->Schedule(std::move(self));
                    }
                    else
                    {
                        observer->OnCompleted();
                    }
                    return rxcpp::Disposable::Empty();           
                })
            ));

            return cd;
        }
    );
}

