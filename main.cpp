#include <iostream>
#include <cmath>
#include <functional>
#include <ComputationalGraph/Graph.hpp>
#include <ComputationalGraph/FoldNode.hpp>
#include <chrono>

using namespace std::chrono_literals;

int main()
{
    ComputationalGraph graph(8);
    auto &input = graph.addInput<int>();
    graph.setInput(input.getId(), 10);

    auto &sqr = graph.addNode<double, int>([](int x){return x*x;}, input);

    auto &sqrt = graph.addNode<double, int>([](int x){return std::sqrt(x);}, input);

    auto &sum = graph.addNode<FoldNode, double, double>(false, [](double x, double y){return x+y;}, 0, sqr, sqrt);

    graph.run();

    std::cout << sum.getResult().value();

    return 0;
}
