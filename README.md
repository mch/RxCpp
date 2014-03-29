# Reactive Extensions:

The Reactive Extensions for Native (RxC) is a library for composing asynchronous and event-based programs using observable sequences and LINQ-style query operators in both C and C++. It is a header only library that can be used in your own programs. 

# Getting Started

RxCpp uses [Catch](https://github.com/philsquared/Catch), a header only library, for unit tests. It is included as a sub-module, so do this to get started: 
```
git submodule init
git submodule update
```

Alternativly, visit the [Catch builds page](http://builds.catch-lib.net/) and download the latest master build.

I use [SCons](http://scons.org/), which requires [Python 2.7](http://python.org/) to build the tests. 

```
cd projects/scons
scons
./rxcpp_test
./rxcppv2_test
./testbench1
```

# Interactive Extensions
* Ix: The Interactive Extensions (Ix) is a .NET library which extends LINQ to Objects to provide many of the operators available in Rx but targeted for IEnumerable<T>.
* IxJS: An implementation of LINQ to Objects and the Interactive Extensions (Ix) in JavaScript.
* Ix++: An implantation of LINQ for Native Developers in C++

# Applications:
* Tx: a set of code samples showing how to use LINQ to events, such as real-time standing queries and queries on past history from trace and log files, which targets ETW, Windows Event Logs and SQL Server Extended Events.
* LINQ2Charts: an example for Rx bindings.  Similar to existing APIs like LINQ to XML, it allows developers to use LINQ to create/change/update charts in an easy way and avoid having to deal with XML or other underneath data structures. We would love to see more Rx bindings like this one.

# Contributing Code

Before submitting a feature or substantial code contribution please  discuss it with the team and ensure it follows the product roadmap. Note that all code submissions will be rigorously reviewed and tested by the Rx Team, and only those that meet an extremely high bar for both quality and design/roadmap appropriateness will be merged into the source.

You will need to submit a  Contributor License Agreement form before submitting your pull request. This needs to only be done once for any Microsoft OSS project. Download the Contributor License Agreement (CLA). Please fill in, sign, scan and email it to msopentech-cla@microsoft.com.
