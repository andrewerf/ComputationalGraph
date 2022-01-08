#ifndef COMPUTATIONALGRAPH_CPP_COMPUTATIONALGRAPH_H
#define COMPUTATIONALGRAPH_CPP_COMPUTATIONALGRAPH_H

#include <memory>
#include <set>
#include "Node.hpp"
#include "ThreadsPool.hpp"


class ComputationalGraph
{
public:
    /**
     * @brief Creates ComputationalGraph with ThreadPool with specified count of threads.
     * @param threadsCount_ count of threads.
     */
    explicit ComputationalGraph(int threadsCount_);

    /**
     * @brief Adds node of type TNode to graph.
     * @tparam TNode type of node (e.g. FoldNode).
     * @tparam TOutput type of output of the node.
     * @tparam TInputs variadic type of input of the node.
     * @tparam TArgs variadic type of args of the TNode constructor.
     * @param args TNode constructor arguments.
     * @return reference to the added Node.
     */
    template <template<typename TOutput_, typename ...TInputs_> class TNode, typename TOutput, typename ...TInputs, class ...TArgs>
    TNode<TOutput, TInputs...>& addNode(TArgs&& ...args);

    /**
     * This overload just defaults the TNode to Node.
     */
    template <typename TOutput, typename ...TInputs, class ...TArgs>
    Node<TOutput, TInputs...>& addNode(TArgs&& ...args);

    /**
     * @brief Adds input Node.
     * Input node is just a Node with no inputs.
     * @tparam T type of the output of added node (actually just type of the input).
     * @return reference to the added Node.
     */
    template <typename T>
    Node<T>& addInput();

    /**
     * @brief Sets input to val.
     * @tparam T type of the input.
     * @param id input id.
     * @param val input value.
     */
    template <typename T>
    void setInput(size_t id, T &&val);

    /**
     * @brief Run all computations and wait until they are ended.
     */
    void run();

private:
    inline void onComplete(size_t completedId);

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
