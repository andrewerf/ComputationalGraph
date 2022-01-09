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
    /**
     * @brief Checks whether all inputs are already computed or not
     * @return true if all inputs are computer, false otherwise.
     */
    virtual bool isReady() const = 0;

    /**
     * @brief Runs this node.
     * @throws std::runtime_error if isReady() is false.
     */
    virtual void run() = 0;

    /**
     * @brief Returns id of the node
     * @return the id.
     */
    virtual size_t getId() const = 0;

    /**
     * @brief Returns ids of output nodes.
     * @return ids of the output nodes.
     */
    virtual std::list<size_t> getOutputs() const = 0;
};


template <typename TOutput, typename ...TInputs>
class Node : public INode
{
public:
    /**
     * @brief The computation type itself.
     */
    using TFunction = std::function<TOutput(const TInputs& ...)>;

    /**
     * @brief Callback function type.
     */
    using TCallback = std::function<void(const TOutput &)>;

    /**
     * @brief Creates empty node.
     * @param id_ Node's id.
     */
    explicit Node(size_t id_);

    /**
     * @brief Move constructor.
     * @param node
     */
    Node(Node &&node) noexcept;

    /**
     * @brief Creates node with specified computation.
     * @param id_ Node's id.
     * @param func the computation func :: TInputs... -> TOutput.
     */
    Node(size_t id_, const TFunction &func);

    /**
     * @brief Create node with specified computetion and inputs.
     * @tparam TNodes variadic template for sequence of input Nodes.
     * @param id_ Node's id.
     * @param func the computation func :: TInputs... -> TOutput.
     * @param nodes sequence of input Nodes. i-th element of sequence corresponds to i-th input of created Node.
     */
    template<class ...TNodes>
    Node(size_t id_, const TFunction &func, TNodes& ...nodes);

    /**
     * @brief Replaces computation.
     * @param func new computation.
     */
    void setFunction(const TFunction &func);

    /**
     * @brief Connects all nodes to inputs.
     * @tparam TNodes variadic template for sequence of input Nodes.
     * @param nodes sequence of input Nodes.
     */
    template<class ...TNodes>
        requires (sizeof...(TNodes) == sizeof...(TInputs))
    void connectAll(TNodes& ...nodes);

    bool isReady() const override;

    void run() override;

    /**
     * @brief Callback for computer input.
     * @tparam inputNumber the number of computed input.
     * @param val value of input.
     */
    template<int inputNumber, typename T>
    void inputComputedCallback(T &&val);

    virtual std::optional<TOutput> getResult() const;

    size_t getId() const override;

    std::list<size_t> getOutputs() const override;

    /**
     * @brief Adds callback which is called when node is computed.
     * @param callback the callback
     */
    void addCallback(const TCallback &callback);

    /**
     * @brief Adds callback which is called when node is computed.
     * @param callback the callback
     * @param id_ id of node which is associated with output of the current node.
     */
    void addCallback(const TCallback &callback, size_t id_);

protected:
    std::optional<TOutput> result;
    std::tuple<TInputs*...> inputs;
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
Node<TOutput, TInputs...>::Node(size_t id_, const TFunction &func):
    Node(id_)
{
    function = func;
}

template <typename TOutput, typename ...TInputs>
template<class ...TNodes>
Node<TOutput, TInputs...>::Node(size_t id_, const TFunction &func, TNodes& ...nodes):
    Node(id_, func)
{
    connectAll(nodes...);
}

template<typename TOutput, typename... TInputs>
void Node<TOutput, TInputs...>::setFunction(const Node::TFunction &func)
{
    function = func;
}

template<typename TOutput, typename... TInputs>
template<class ...TNodes>
    requires (sizeof...(TNodes) == sizeof...(TInputs))
void Node<TOutput, TInputs...>::connectAll(TNodes& ...nodes)
{
    [this, &nodes...] <int ...inputsNumbers> (std::integer_sequence<int, inputsNumbers...> is)
    {
        (connect<inputsNumbers>(nodes, *this) , ...);
    }(std::make_integer_sequence<int, sizeof...(TNodes)>());
}

template<typename TOutput, typename ...TInputs>
bool Node<TOutput, TInputs...>::isReady() const
{
    return std::accumulate(inputsSet.begin(), inputsSet.end(), true, std::logical_and());
}

template<typename TOutput, typename... TInputs>
void Node<TOutput, TInputs...>::run()
{
    if(isReady())
    {
        result = std::apply([this](TInputs* ...inputs1){return function(*inputs1...);}, inputs);
        std::for_each(outputCallbacks.begin(), outputCallbacks.end(), [this](auto callback){callback(*result);});
    }
    else
        throw std::runtime_error("Some inputs are not initialized");
}

template<typename TOutput, typename... TInputs>
template<int inputNumber, typename T>
void Node<TOutput, TInputs...>::inputComputedCallback(T &&val)
{
    std::get<inputNumber>(inputs) = const_cast<std::remove_const_t<std::remove_reference_t<T>>*>(&val);
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

template<typename TOutput, typename... TInputs>
void Node<TOutput, TInputs...>::addCallback(const TCallback &callback)
{
    outputCallbacks.push_back(callback);
}

template<typename TOutput, typename... TInputs>
void Node<TOutput, TInputs...>::addCallback(const TCallback &callback, size_t id_)
{
    addCallback(callback);
    outputs.push_back(id_);
}


template<int inputNumber, typename TOutput1, typename ...TInputs1, typename TOutput2, typename ...TInputs2>
void connect(Node<TOutput1, TInputs1...> &a, Node<TOutput2, TInputs2...> &b)
{
    a.addCallback([&b](const TOutput1 &output){
        b.template inputComputedCallback<inputNumber>(output);
    }, b.getId());
}



#endif //COMPUTATIONALGRAPH_CPP_NODE_HPP
