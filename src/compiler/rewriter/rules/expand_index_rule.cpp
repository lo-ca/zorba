/*
 * Copyright 2006-2008 The FLWOR Foundation.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "compiler/rewriter/rules/ruleset.h"
#include "compiler/expression/expr.h"
#include "compiler/rewriter/tools/expr_tools.h"
#include "indexing/value_index.h"

namespace zorba {

static store::Item_t get_uri(expr *e)
{
  // The following two checks are required if we run after
  // normalization. It does not hurt, otherwise -- so leave it
  // here.
  if (e->get_expr_kind() == promote_expr_kind) {
    e = static_cast<promote_expr *>(e)->get_input();
  }
  if (is_data(e)) {
    e = get_fo_arg(e, 0);
  }
  if (e->get_expr_kind() == const_expr_kind) {
    return static_cast<const_expr *>(e)->get_val();
  }
  return NULL;
}

RULE_REWRITE_PRE(ExpandBuildIndex)
{
  if (node->get_expr_kind() == fo_expr_kind) {
    fo_expr *fo = static_cast<fo_expr *>(&*node);
    if (fo->get_func() == LOOKUP_RESOLVED_FN(ZORBA_OPEXTENSIONS_NS, "build-index", 1)) {
      store::Item_t uri_item(get_uri((*fo)[0].getp()));
      if (uri_item == NULL) {
        ZORBA_ASSERT(false);
      }
      xqpStringStore_t uristore;
      uri_item->getStringValue(uristore);
      xqp_string uri(uristore);
      ValueIndex *vi = rCtx.getStaticContext()->lookup_index(uri);
      if (vi == NULL) {
        ZORBA_ASSERT(false);
      }
      std::vector<expr_t> se_args;
      expr_t open_index_arg(new const_expr(fo->get_loc(), uri_item));
      expr_t open_index(new fo_expr(fo->get_loc(), LOOKUP_OP1("index-session-opener"), open_index_arg));
      se_args.push_back(open_index);

      expr::substitution_t subst;

      flwor_expr::clause_list_t clauses;
      expr_t de = vi->getDomainExpression()->clone(subst);
      var_expr_t dot = vi->getDomainVariable();
      var_expr_t pos = vi->getDomainPositionVariable();
      var_expr_t newdot = new var_expr(dot->get_loc(), dot->get_kind(), dot->get_varname());
      var_expr_t newpos = new var_expr(pos->get_loc(), pos->get_kind(), pos->get_varname());
      subst[dot] = newdot;
      subst[pos] = newpos;
      flwor_expr::forletref_t fc = new forlet_clause(forlet_clause::for_clause, newdot, newpos, NULL, de);
      newdot->set_forlet_clause(fc);
      newpos->set_forlet_clause(fc);

      clauses.push_back(fc);

      std::vector<expr_t> index_insert_args;
      expr_t insert_index_uri(new const_expr(fo->get_loc(), uri_item));
      index_insert_args.push_back(insert_index_uri);

      expr_t insert_index_var(new wrapper_expr(fo->get_loc(), newdot));
      index_insert_args.push_back(insert_index_var);

      const std::vector<expr_t>& idx_fields(vi->getIndexFieldExpressions());
      int n = idx_fields.size();
      for(int i = 0; i < n; ++i) {
        index_insert_args.push_back(idx_fields[i]->clone(subst));
      }
      expr_t re(new fo_expr(fo->get_loc(), LOOKUP_OPN("index-builder"), index_insert_args));
      
      rchandle<flwor_expr> flwor = new flwor_expr(fo->get_loc(), clauses, re);

      se_args.push_back(flwor.getp());

      expr_t close_index_arg(new const_expr(fo->get_loc(), uri_item));
      expr_t close_index(new fo_expr(fo->get_loc(), LOOKUP_OP1("index-session-closer"), close_index_arg));
      se_args.push_back(close_index);
      expr_t se = new sequential_expr(fo->get_loc(), se_args);
      return se;
    }
  }
  return NULL;
}

RULE_REWRITE_POST(ExpandBuildIndex)
{
  return NULL;
}

}
/* vim:set ts=2 sw=2: */
