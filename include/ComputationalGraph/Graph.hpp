#ifndef COMPUTATIONALGRAPH_CPP_COMPUTATIONALGRAPH_H
#define COMPUTATIONALGRAPH_CPP_COMPUTATIONALGRAPH_H

#include <memory>
#include <set>
#include "Node.hpp"
#include "ThreadsPool.hpp"


class ComputationalGraph
{
public:
    explicit ComputationalGraph(int threadsCount_);

    template <template<typename TOutput_, typename ...TInputs_> class TNode, typename TOutput, typename ...TInputs, class ...TArgs>
    TNode<TOutput, TInputs...>& addNode(TArgs&& ...args);

    template <typename TOutput, typename ...TInputs, class ...TArgs>
    Node<TOutput, TInputs...>& addNode(TArgs&& ...args);

    template <typename TOutput, typename ...TInputs>
    Node<TOutput, TInputs...>& addNode();

    template <typename T>
    Node<T>& addInput();

    template <typename T>
    void setInput(size_t id, T &&val);

    void run();

    inline void onComplete(size_t completedId);

private:
    std::vector<std::unique_ptr<INode>> graph;
    std::vector<bool> isScheduled;
    std::set<size_t> inputsIds;
    ThreadsPool<> threadsPool;

    std::atomic<size_t> completedCount;
    std::condition_variable allCompleted;
    std::mutex completedMutex;
    std::mutex scheduledMutex;
};



ComputationalGraph::ComputationalGraph(int threadsCount_):
    threadsPool(threadsCount_)
{}

template <template<typename TOutput_, typename ...TInputs_> class TNode, typename TOutput, typename ...TInputs, class ...TArgs>
TNode<TOutput, TInputs...>& ComputationalGraph::addNode(TArgs&& ...args)
{
    std::unique_ptr<TNode<TOutput, TInputs...>> node{new TNode<TOutput, TInputs...>(graph.size(), std::forward<TArgs>(args)...)};
    TNode<TOutput, TInputs...> &ret_ref = *node;
    graph.emplace_back(std::move(node));
    return ret_ref;
}

template <typename TOutput, typename ...TInputs, class ...TArgs>
Node<TOutput, TInputs...>& ComputationalGraph::addNode(TArgs&& ...args)
{
    return addNode<Node, TOutput, TInputs...>(std::forward<TArgs>(args)...);
}

template <typename TOutput, typename ...TInputs>
Node<TOutput, TInputs...>& ComputationalGraph::addNode()
{
    std::unique_ptr<Node<TOutput, TInputs...>> node{new Node<TOutput, TInputs...>(graph.size())};
    Node<TOutput, TInputs...> &ret_ref = *node;
    graph.emplace_back(std::move(node));
    return ret_ref;
}

template <typename T>
Node<T>& ComputationalGraph::addInput()
{
    inputsIds.insert(graph.size());
    return addNode<T>();
}

template <typename T>
void ComputationalGraph::setInput(size_t id, T &&val)
{
    auto inputNode = dynamic_cast<Node<std::remove_reference_t<T>>*>(graph[id].get());
    if(inputNode == nullptr)
        throw std::runtime_error("Bad input node");

    inputNode->setFunction([v = std::forward<T>(val)]{return v;});
}

void ComputationalGraph::run()
{
    std::unique_lock completedLock(completedMutex);
    isScheduled = std::vector<bool>(graph.size(), false);

    for(size_t i : inputsIds)
    {
        graph[i]->run();
        isScheduled[i] = true;
    }

    completedCount = 0;

    for(size_t i : inputsIds)
        onComplete(i);

    allCompleted.wait(completedLock, [this]{
        return completedCount == graph.size();
    });
}

void ComputationalGraph::onComplete(size_t completedId)
{
    for(size_t childId : graph[completedId]->getOutputs())
    {
        if(graph[childId]->isReady() && !isScheduled[childId])
        {
            std::lock_guard scheduledLock(scheduledMutex);
            if(!isScheduled[childId])
                threadsPool.submit([childId, this]{graph[childId]->run(); onComplete(childId);});
            isScheduled[childId] = true;
        }
    }

    if(++completedCount == graph.size())
        allCompleted.notify_one();
}

#endif //COMPUTATIONALGRAPH_CPP_COMPUTATIONALGRAPH_H
