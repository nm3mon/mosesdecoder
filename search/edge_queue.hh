#ifndef SEARCH_EDGE_QUEUE__
#define SEARCH_EDGE_QUEUE__

#include "search/edge.hh"
#include "search/edge_generator.hh"
#include "search/note.hh"

#include <boost/pool/pool.hpp>
#include <boost/pool/object_pool.hpp>

#include <queue>

namespace search {

template <class Model> class Context;

class EdgeQueue {
  public:
    EdgeQueue() {}

    PartialEdge AllocateEdge(Arity arity) {
      return partial_edge_pool_.Allocate(arity);
    }

    void AddEdge(PartialEdge edge, Note note) {
      generate_.push(edge_pool_.construct(edge, note));
    }

    bool Empty() const { return generate_.empty(); }

    /* Generate hypotheses and send them to output.  Normally, output is a
     * VertexGenerator, but the decoder may want to route edges to different
     * vertices i.e. if they have different LHS non-terminal labels.  
     */
    template <class Model, class Output> void Search(Context<Model> &context, Output &output) {
      int to_pop = context.PopLimit();
      while (to_pop > 0 && !generate_.empty()) {
        EdgeGenerator *top = generate_.top();
        generate_.pop();
        PartialEdge ret(top->Pop(context, partial_edge_pool_));
        if (ret.Valid()) {
          output.NewHypothesis(ret, top->GetNote());
          --to_pop;
          if (top->TopScore() != -kScoreInf) {
            generate_.push(top);
          }
        } else {
          generate_.push(top);
        }
      }
      output.FinishedSearch();
    }

  private:
    boost::object_pool<EdgeGenerator> edge_pool_;

    struct LessByTopScore : public std::binary_function<const EdgeGenerator *, const EdgeGenerator *, bool> {
      bool operator()(const EdgeGenerator *first, const EdgeGenerator *second) const {
        return first->TopScore() < second->TopScore();
      }
    };

    typedef std::priority_queue<EdgeGenerator*, std::vector<EdgeGenerator*>, LessByTopScore> Generate;
    Generate generate_;

    PartialEdgePool partial_edge_pool_;
};

} // namespace search
#endif // SEARCH_EDGE_QUEUE__