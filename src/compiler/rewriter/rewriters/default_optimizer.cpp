#include "compiler/rewriter/framework/rule_driver.h"
#include "compiler/rewriter/rules/ruleset.h"
#include "compiler/rewriter/rewriters/default_optimizer.h"

namespace zorba {

#define ADD_DRIVER( d ) \
  m_childRewriters.push_back(rewriter_ptr_t (new d))
#define ADD_SINGLETON_DRIVER(rule) m_childRewriters.push_back(rewriter_ptr_t(new SingletonRuleMajorDriver<rule>))
#define ADD_SINGLETON_DRIVER_BASE(rule) \
  m_childRewriters.push_back(rewriter_ptr_t(new SingletonRuleMajorDriverBase (RuleMajorDriver::rule_ptr_t (new rule))))

#define ADD_RULE( rule ) \
  m_rules.push_back (rule_ptr_t (new rule))

  class FoldRules : public RuleMajorDriver {
  public:
    FoldRules() {
      ADD_RULE(FoldConst (false));
      ADD_RULE(PartialEval);      
      ADD_RULE(RefactorPredFLWOR);
      ADD_RULE(EliminateUnusedLetVars);
    }
  };

DefaultOptimizer::DefaultOptimizer()
{
  ADD_SINGLETON_DRIVER(InferVarTypes);
  ADD_SINGLETON_DRIVER(MarkNodesWithNodeIdProperties);
  ADD_SINGLETON_DRIVER(EliminateDocOrderSort);
  ADD_SINGLETON_DRIVER(EliminateTypeEnforcingOperations);
  ADD_SINGLETON_DRIVER(EliminateExtraneousPathSteps);
  ADD_SINGLETON_DRIVER(MarkFreeVars);
  ADD_SINGLETON_DRIVER(MarkExpensiveOps);
  ADD_SINGLETON_DRIVER(MarkUnfoldableOps);
  ADD_DRIVER(FoldRules);
}

DefaultOptimizer::~DefaultOptimizer() throw ()
{
}

}
