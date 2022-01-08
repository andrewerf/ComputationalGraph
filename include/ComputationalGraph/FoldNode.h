#ifndef COMPUTATIONALGRAPH_FOLDNODE_H
#define COMPUTATIONALGRAPH_FOLDNODE_H

#include "Node.hpp"


template <typename TOutput, typename TInput>
class FoldNode : public Node<TOutput, std::list<TInput>>
{
public:
    using TFunction = std::function<TOutput(TInput, TOutput)>;
    using TLInput = std::list<TInput>;
    using TNode = Node<TOutput, TLInput>;

    explicit FoldNode(size_t id_);
    FoldNode(FoldNode &&node) noexcept;
    FoldNode(size_t id_, const TFunction &func, TOutput init);

    template<class ...TNodes>
    FoldNode(size_t id_, const TFunction &func, TOutput init, TNodes& ...nodes);

    bool isReady() const override;

    void run() override;

    template <typename T>
    void add(T &&input);

private:
    TFunction foldFunction;
};

template<typename TOutput, typename TInput>
FoldNode<TOutput, TInput>::FoldNode(size_t id_):
    TNode(id_)
{}

template<typename TOutput, typename TInput>
FoldNode<TOutput, TInput>::FoldNode(FoldNode &&node) noexcept:
    TNode(std::move(node))
{}

template<typename TOutput, typename TInput>
FoldNode<TOutput, TInput>::FoldNode(size_t id_, const FoldNode::TFunction &func, TOutput init):
    foldFunction(func),
    TNode(id_)
{
    TNode::result = init;
}

template<typename TOutput, typename TInput>
template<class ...TNodes>
FoldNode<TOutput, TInput>::FoldNode(size_t id_, const TFunction &func, TOutput init, TNodes& ...nodes):
    FoldNode(id_, func, init)
{
    (connect(nodes, *this) , ...);
}

template<typename TOutput, typename TInput>
bool FoldNode<TOutput, TInput>::isReady() const
{
    return true;
}

template<typename TOutput, typename TInput>
void FoldNode<TOutput, TInput>::run()
{
    std::for_each(TNode::outputCallbacks.begin(), TNode::outputCallbacks.end(), [this](auto callback){callback(*TNode::result);});
}

template<typename TOutput, typename TInput>
template<typename T>
void FoldNode<TOutput, TInput>::add(T &&input)
{
    TNode::result = foldFunction(input, *TNode::result);
}


template<typename TOutput1, typename ...TInputs1, typename TOutput2, typename ...TInputs2>
void connect(Node<TOutput1, TInputs1...> &a, FoldNode<TOutput2, TInputs2...> &b)
{
    a.addCallback([&b](TOutput1 &output){
        b.add(output);
    }, b.getId());
}


#endif //COMPUTATIONALGRAPH_FOLDNODE_H
