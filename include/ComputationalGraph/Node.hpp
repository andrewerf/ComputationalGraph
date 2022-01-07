#ifndef COMPUTATIONALGRAPH_CPP_NODE_HPP
#define COMPUTATIONALGRAPH_CPP_NODE_HPP

#include <iostream>
#include <optional>
#include <list>
#include <array>
#include <numeric>
#include <functional>
#include <atomic>


class INode
{
public:
    virtual operator bool() const = 0;
    virtual void operator()() = 0;
    virtual void run() = 0;
    virtual size_t getId() const = 0;
    virtual std::list<size_t> getOutputs() const = 0;
};


template <typename TOutput, typename ...TInputs>
class Node : public INode
{
public:
    using TFunction = std::function<TOutput(TInputs...)>;
    using TCallback = std::function<void(TOutput &)>;

    explicit Node(size_t id_);
    Node(Node &&node) noexcept;
    Node(size_t id_, const TFunction &func);

    void setFunction(const TFunction &func);

    template<int inputNumber, typename TCurrentOutput, typename ...TCurrentInputs>
    void connectTo(Node<TCurrentOutput, TCurrentInputs...> &node);

    operator bool() const override;

    void operator()() override;

    void run() override;

    template<int inputNumber, typename T>
    void inputComputedCallback(T &&val);

    std::optional<TOutput> getResult() const;

    size_t getId() const override;

    std::list<size_t> getOutputs() const override;

private:
    std::optional<TOutput> result;
    std::tuple<TInputs...> inputs;
    std::list<size_t> outputs;
    std::array<bool, sizeof...(TInputs)> inputsSet;
    TFunction function;
    std::list<TCallback> outputCallbacks;

    size_t id;
};



template<typename TOutput, typename ...TInputs>
Node<TOutput, TInputs...>::Node(size_t id_):
    id(id_)
{
    std::for_each(inputsSet.begin(), inputsSet.end(), [](bool &v){v = false;});
}

template<typename TOutput, typename... TInputs>
Node<TOutput, TInputs...>::Node(Node &&node) noexcept:
    result(std::move(node.result)),
    inputs(std::move(node.inputs)),
    inputsSet(std::move(node.inputsSet)),
    function(std::move(node.function)),
    outputCallbacks(std::move(node.outputCallbacks)),
    id(node.id)
{}

template<typename TOutput, typename ...TInputs>
Node<TOutput, TInputs...>::Node(size_t id_, const std::function<TOutput(TInputs...)> &func):
    Node(id_)
{
    function = func;
}

template<typename TOutput, typename... TInputs>
void Node<TOutput, TInputs...>::setFunction(const Node::TFunction &func)
{
    function = func;
}

template<typename TOutput, typename... TInputs>
template<int inputNumber, typename TCurrentOutput, typename ...TCurrentInputs>
void Node<TOutput, TInputs...>::connectTo(Node<TCurrentOutput, TCurrentInputs...> &node)
{
    outputCallbacks.push_back([&node](TOutput &output){
        node.template inputComputedCallback<inputNumber>(std::forward<TOutput>(output));
    });
    outputs.push_back(node.getId());
}

template<typename TOutput, typename ...TInputs>
Node<TOutput, TInputs...>::operator bool() const
{
    return std::accumulate(inputsSet.begin(), inputsSet.end(), true, std::logical_and());
}

template<typename TOutput, typename... TInputs>
void Node<TOutput, TInputs...>::run()
{
    if(*this)
    {
        result = std::apply(function, inputs);
        std::for_each(outputCallbacks.begin(), outputCallbacks.end(), [this](auto callback){callback(*result);});
    }
    else
        throw std::runtime_error("Some inputs are not initialized");
}

template<typename TOutput, typename... TInputs>
void Node<TOutput, TInputs...>::operator()()
{
    run();
}

template<typename TOutput, typename... TInputs>
template<int inputNumber, typename T>
void Node<TOutput, TInputs...>::inputComputedCallback(T &&val)
{
    std::get<inputNumber>(inputs) = std::forward<T>(val);
    inputsSet[inputNumber] = true;
}

template<typename TOutput, typename... TInputs>
std::optional<TOutput> Node<TOutput, TInputs...>::getResult() const
{
    return result;
}

template<typename TOutput, typename... TInputs>
size_t Node<TOutput, TInputs...>::getId() const
{
    return id;
}

template<typename TOutput, typename... TInputs>
std::list<size_t> Node<TOutput, TInputs...>::getOutputs() const
{
    return outputs;
}



#endif //COMPUTATIONALGRAPH_CPP_NODE_HPP
