#ifndef COMPUTATIONALGRAPH_FOLDNODE_HPP
#define COMPUTATIONALGRAPH_FOLDNODE_HPP

#include "Node.hpp"


template <typename TOutput, typename TInput>
class FoldNode : public Node<TOutput, std::vector<TInput>>
{
public:
    using TFunction = std::function<TOutput(TOutput, TInput)>;
    using TLInput = std::vector<TInput>;
    using TNode = Node<TOutput, TLInput>;

    explicit FoldNode(size_t id_, bool computeWithInput);
    FoldNode(FoldNode &&node) noexcept;
    FoldNode(size_t id_, bool computeWithInput, const TFunction &func, TOutput init);

    template<class ...TNodes>
    FoldNode(size_t id_, bool computeWithInput, const TFunction &func, TOutput init, TNodes& ...nodes);

    bool isReady() const override;

    void run() override;

    template<typename ...TInputs1>
    void connectFrom(Node<TInput, TInputs1...> &node);

    template<typename ...TInputs1>
    void connectFrom(Node<std::vector<TInput>, TInputs1...> &node);

    virtual void add(const TInput &input);

    virtual std::optional<TOutput> getResult() const;

protected:
    TFunction foldFunction;
    bool computeInInputNode;
    std::atomic<int> inputsReadyCount;
    int inputsDeclaredCount;
    std::atomic<TOutput> currentValue;

    std::mutex addMutex;
};

template<typename TOutput, typename TInput>
FoldNode<TOutput, TInput>::FoldNode(size_t id_, bool computeWithInput):
    TNode(id_),
    computeInInputNode(computeWithInput),
    inputsReadyCount(0),
    inputsDeclaredCount(0)
{
    if(!computeInInputNode)
        std::get<0>(TNode::inputs) = new std::vector<TInput>;
}

template<typename TOutput, typename TInput>
FoldNode<TOutput, TInput>::FoldNode(FoldNode &&node) noexcept:
    TNode(std::move(node)),
    computeInInputNode(node.computeInInputNode),
    inputsReadyCount(node.inputsReadyCount),
    inputsDeclaredCount(node.inputsDeclaredCount),
    currentValue(std::move(node.currentValue))
{}

template<typename TOutput, typename TInput>
FoldNode<TOutput, TInput>::FoldNode(size_t id_, bool computeWithInput, const FoldNode::TFunction &func, TOutput init):
    FoldNode(id_, computeWithInput)
{
    foldFunction = func;
    TNode::inputsSet[0] = true;
    if(computeWithInput)
        currentValue = init;
    else
        TNode::setFunction([init, func](const TLInput &inputs){
            return std::accumulate(inputs.begin(), inputs.end(), init, func);
        });
}

template<typename TOutput, typename TInput>
template<class ...TNodes>
FoldNode<TOutput, TInput>::FoldNode(size_t id_, bool computeWithInput, const TFunction &func, TOutput init, TNodes& ...nodes):
    FoldNode(id_, computeWithInput, func, init)
{
    (connect(nodes, *this) , ...);
}

template<typename TOutput, typename TInput>
bool FoldNode<TOutput, TInput>::isReady() const
{
    return inputsDeclaredCount == inputsReadyCount;
}

template<typename TOutput, typename TInput>
void FoldNode<TOutput, TInput>::run()
{
    if(!computeInInputNode)
        TNode::run();
    else
    {
        TNode::result = currentValue;
        std::for_each(TNode::outputCallbacks.begin(), TNode::outputCallbacks.end(), [this](auto callback){callback(*TNode::result);});
    }
}

template<typename TOutput, typename TInput>
void FoldNode<TOutput, TInput>::add(const TInput &input)
{
    if(computeInInputNode)
    {
        TOutput copy = currentValue;
        while(!currentValue.compare_exchange_weak(copy, foldFunction(copy, input)))
            copy = currentValue;
    }
    else
    {
        std::lock_guard lock(addMutex);
        std::get<0>(TNode::inputs)->emplace_back(input);
    }
}


template<typename TOutput, typename TInput>
template<typename... TInputs1>
void FoldNode<TOutput, TInput>::connectFrom(Node<TInput, TInputs1...> &node)
{
    ++inputsDeclaredCount;
    node.addCallback([this](const TInput &output){
        add(output);
        ++inputsReadyCount;
    }, TNode::getId());
}

template<typename TOutput, typename TInput>
std::optional<TOutput> FoldNode<TOutput, TInput>::getResult() const
{
    if(computeInInputNode)
        return currentValue;
    else
        return TNode::result;
}

template<typename TOutput, typename TInput>
template<typename... TInputs1>
void FoldNode<TOutput, TInput>::connectFrom(Node<std::vector<TInput>, TInputs1...> &node)
{
    ++inputsDeclaredCount;
    node.addCallback([this](const std::vector<TInput> &output){
        if(computeInInputNode)
            for(const auto &v : output)
                add(v);
        else
        {
            std::lock_guard lock(addMutex);
            for(const auto &v : output)
                std::get<0>(TNode::inputs)->push_back(v);
        }
        ++inputsReadyCount;
    }, TNode::getId());
}


template<typename TOutput1, typename ...TInputs1, typename TOutput2>
void connect(Node<TOutput1, TInputs1...> &a, FoldNode<TOutput2, TOutput1> &b)
{
    b.connectFrom(a);
}

template<typename TOutput1, typename ...TInputs1, typename TOutput2>
void connect(Node<std::vector<TOutput1>, TInputs1...> &a, FoldNode<TOutput2, TOutput1> &b)
{
    b.connectFrom(a);
}


#endif //COMPUTATIONALGRAPH_FOLDNODE_HPP
