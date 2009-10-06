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
#include <typeinfo>
#include <iostream>

#include "types/root_typemanager.h"
#include "types/typeops.h"
#include "system/globalenv.h"

#include "context/static_context.h"

#include "compiler/normalizer/normalizer.h"
#include "compiler/expression/expr.h"
#include "compiler/api/compilercb.h"
#include "compiler/expression/expr_visitor.h"

#include "functions/signature.h"
#include "functions/function.h"

namespace zorba {

#define LOOKUP_FN( pfx, local, arity ) (sctx->lookup_fn (pfx, local, arity))
#define LOOKUP_OP1( local ) (m_cb->m_sctx->lookup_builtin_fn (":" local, 1))


static inline expr::expr_t wrap_in_bev(short context, static_context *sctx, expr::expr_t e)
{
  expr::expr_t fh(new fo_expr(context, e->get_loc(), LOOKUP_FN("fn", "boolean", 1)));
  fo_expr *fp = static_cast<fo_expr *>(fh.getp());
  fp->add(e);
  return fh;
}


static inline expr::expr_t wrap_in_atomization(
    short context,
    static_context *sctx,
    expr::expr_t e)
{
  expr::expr_t fh(new fo_expr(context, e->get_loc(), LOOKUP_FN("fn", "data", 1)));
  fo_expr* fp = static_cast<fo_expr *>(fh.getp());
  fp->add(e);
  return fh;
}


static inline expr::expr_t wrap_in_type_conversion(
    short context,
    expr::expr_t e,
    xqtref_t type)
{
  expr::expr_t ph = new promote_expr(context, e->get_loc(), e, type);
  // TODO : Need to add convert_simple_operand
  return ph;
}


static inline expr::expr_t wrap_in_typematch(
    short context, 
    expr::expr_t e,
    xqtref_t type)
{
  // treat_expr should be avoided for updating expressions too,
  // but in that case "type" will be item()* anyway
  return TypeOps::is_subtype(*GENV_TYPESYSTEM.ITEM_TYPE_STAR, *type)
    ? e
    : new treat_expr(context, e->get_loc(), e, type, XPTY0004);
}


static inline void checkNonUpdating(const expr* lExpr)
{
  if (lExpr != 0 && lExpr->is_updating())
    ZORBA_ERROR_LOC(XUST0001, lExpr->get_loc());
}


class normalizer : public expr_visitor 
{
  CompilerCB*     m_cb;

public:
  normalizer(CompilerCB* aCompilerCB) : m_cb(aCompilerCB) {}

  ~normalizer() { }

#define DEF_VISIT_METHODS( e )                  \
  bool begin_visit (e& node) { return true; }   \
  void end_visit (e& node) {}

DEF_VISIT_METHODS (expr)

DEF_VISIT_METHODS (debugger_expr)

DEF_VISIT_METHODS (wrapper_expr)

DEF_VISIT_METHODS (sequential_expr)

DEF_VISIT_METHODS (var_expr)

DEF_VISIT_METHODS (order_modifier)

DEF_VISIT_METHODS (eval_expr)

DEF_VISIT_METHODS (typeswitch_expr)


void wrap_flwor_wincond(short context, static_context* sctx, flwor_wincond* c) 
{
  if (c == NULL)
    return;

  c->set_cond(wrap_in_bev(context, sctx, c->get_cond()));
}


bool begin_visit(flwor_expr& node) 
{
  return true;
}


void end_visit(flwor_expr& node)
{
  for (unsigned i = 0; i < node.num_clauses(); i++) 
  {
    flwor_clause& c = *(node[i]);

    if (c.get_kind() == flwor_clause::where_clause) 
    {
      where_clause* wc = static_cast<where_clause *>(&c);
      wc->set_where(wrap_in_bev(m_cb->m_cur_sctx, m_cb->m_sctx.getp(), wc->get_where()));
    }
    else if (c.get_kind() == flwor_clause::order_clause)
    {
      orderby_clause* obc = static_cast<orderby_clause *>(&c);

      unsigned numColumns = obc->num_columns();

      for (unsigned i = 0; i < numColumns; ++i)
      { 
        obc->set_column_expr(i, wrap_in_atomization(m_cb->m_cur_sctx, m_cb->m_sctx.getp(), obc->get_column_expr(i)));
      }
    }
    else if (c.get_kind() == flwor_clause::for_clause)
    {
      for_clause* fc = static_cast<for_clause *> (&c);

      xqtref_t vartype = fc->get_var()->get_type();
      if (vartype != NULL) 
      {
        if (TypeOps::is_empty(*vartype))
          ZORBA_ERROR_LOC_PARAM(XPTY0004, fc->get_loc(), "empty-sequence()", "");

        xqtref_t promote_type = m_cb->m_sctx->get_typemanager()->
                                create_type(*vartype, TypeConstants::QUANT_STAR);

        expr_t e = fc->get_expr();
        fc->set_expr(wrap_in_typematch(m_cb->m_cur_sctx, e, promote_type));
      }
    }
    else if (c.get_kind() == flwor_clause::let_clause)
    {
      let_clause* lc = static_cast<let_clause *> (&c);

      xqtref_t vartype = lc->get_var()->get_type();
      if (vartype != NULL) 
      {
        expr_t e = lc->get_expr();
        lc->set_expr(wrap_in_typematch(m_cb->m_cur_sctx, e, vartype));
      }
    }
    else if (c.get_kind() == flwor_clause::window_clause)
    {
      window_clause* wc = static_cast<window_clause *> (&c);

      xqtref_t vartype = wc->get_var()->get_type();
      if (vartype != NULL) 
      {
        expr_t e = wc->get_expr();
        wc->set_expr(wrap_in_typematch(m_cb->m_cur_sctx, e, vartype));
      }

      wrap_flwor_wincond(m_cb->m_cur_sctx, m_cb->m_sctx, wc->get_win_start());
      wrap_flwor_wincond(m_cb->m_cur_sctx, m_cb->m_sctx, wc->get_win_stop());
    }
  }
}


void end_visit (case_clause&) 
{
}

bool begin_visit (case_clause& node)
{
  checkNonUpdating(&*node.get_var_h());
  return true;
}


void end_visit (promote_expr&) {}

bool begin_visit (promote_expr& node)
{
  checkNonUpdating(&*node.get_input());
  return true;
}


void end_visit (trycatch_expr&) {}

bool begin_visit (trycatch_expr& node)
{
  checkNonUpdating(&*node.get_try_expr());
  return true;
}


void end_visit (if_expr&)
{
}

bool begin_visit (if_expr& node)
{
  node.set_cond_expr(wrap_in_bev(m_cb->m_cur_sctx, m_cb->m_sctx, node.get_cond_expr()));
  return true;
}


void end_visit (fo_expr&) 
{
}

bool begin_visit (fo_expr& node)
{
  RootTypeManager& rtm = GENV_TYPESYSTEM;

  const signature& sign = node.get_signature();

  int n = node.size();

  const function* func = node.get_func();

  for(int i = 0; i < n; ++i) 
  {
    expr::expr_t param = node[i];

    if (!node.is_updating())
      checkNonUpdating(&*param);

    xqtref_t param_type;

    if (func->getKind() == FunctionConsts::FN_INDEX_PROBE_POINT)
    {
      if (i == 0)
        param_type = sign[i];
      else
        param_type = rtm.ANY_ATOMIC_TYPE_QUESTION;
    }
    else if (func->getKind() == FunctionConsts::FN_INDEX_PROBE_RANGE)
    {
      if (i == 0)
        param_type = sign[i];
      else if (i % 6 == 1 || i % 6 == 2)
        param_type = rtm.ANY_ATOMIC_TYPE_QUESTION;
      else
        param_type = rtm.BOOLEAN_TYPE_QUESTION;
    }
    else
    {
      param_type = sign[i];
    }

    xqtref_t param_prime_type = TypeOps::prime_type(*param_type);

    if (TypeOps::is_subtype(*param_prime_type, *rtm.ANY_ATOMIC_TYPE_ONE)) 
    {
      param = wrap_in_atomization(m_cb->m_cur_sctx, m_cb->m_sctx, param);
      param = wrap_in_type_conversion(m_cb->m_cur_sctx, param, param_type);
    }
    else
    {
      param = wrap_in_typematch(m_cb->m_cur_sctx, param, param_type);
    }

    node[i] = param;
  }

  return true;
}


void end_visit (instanceof_expr&) {}

bool begin_visit (instanceof_expr& node)
{
  checkNonUpdating(&*node.get_input());
  return true;
}

DEF_VISIT_METHODS (treat_expr)

void end_visit (castable_expr&) 
{
}

bool begin_visit (castable_expr& node)
{
  checkNonUpdating(&*node.get_input());
  return true;
}

void end_visit (cast_expr&) 
{
}

bool begin_visit (cast_expr& node)
{
  checkNonUpdating(&*node.get_input());

  expr_t arg = wrap_in_atomization(m_cb->m_cur_sctx, m_cb->m_sctx, node.get_input());

  node.set_input(arg);

  return true;
}

void end_visit (name_cast_expr&) 
{
}

bool begin_visit (name_cast_expr& node)
{
  checkNonUpdating(&*node.get_input());
  return true;
}

void end_visit (validate_expr&) 
{
}

bool begin_visit (validate_expr& node)
{
  checkNonUpdating(&*node.get_expr());
  return true;
}

void end_visit (extension_expr&) 
{
}

bool begin_visit (extension_expr& node)
{
  checkNonUpdating(&*node.get_expr());
  return true;
}

void end_visit (relpath_expr&) 
{
}

bool begin_visit (relpath_expr& node)
{
#if 0
  if (node.size() > 0) {
    expr_t ie = node[0];
    expr_t pe = new treat_expr(m_cb->m_cur_sctx, ie->get_loc(), ie, GENV_TYPESYSTEM.ANY_NODE_TYPE_STAR, XPTY0019);
    node[0] = pe;
  }
#endif
  return true;
}

DEF_VISIT_METHODS (axis_step_expr)
DEF_VISIT_METHODS (match_expr)
DEF_VISIT_METHODS (const_expr)
DEF_VISIT_METHODS (order_expr)

void end_visit (elem_expr&) {}
bool begin_visit (elem_expr& node)
{
  //node.setQNameExpr(wrap_in_atomization(m_sctx, node.getQNameExpr()));
  checkNonUpdating(&*node.getQNameExpr());
  checkNonUpdating(&*node.getAttrs());
  checkNonUpdating(&*node.getContent());
  return true;
}

void end_visit (doc_expr&) {}
bool begin_visit (doc_expr& node)
{
  checkNonUpdating(node.getContent());
  return true;
}


void end_visit (attr_expr&) {}
bool begin_visit (attr_expr& node)
{
  assert (node.getQNameExpr() != NULL);

  checkNonUpdating(&*node.getQNameExpr());
  checkNonUpdating(&*node.getValueExpr());

  // node.setQNameExpr(wrap_in_atomization(m_sctx, node.getQNameExpr()));
  
  if (node.getValueExpr() != NULL) {
    rchandle<expr> lExpr = node.getValueExpr();
    fo_expr* lFoExpr = 0;
    if ((lFoExpr = dynamic_cast<fo_expr*>(&*lExpr)))
    {
      function* lTestFunc = LOOKUP_OP1("enclosed-expr");
      if (lFoExpr->get_func() == lTestFunc)
      {
        (*lFoExpr)[0] = wrap_in_atomization(m_cb->m_cur_sctx, m_cb->m_sctx, (*lFoExpr)[0].getp());
        return true;
      }
    }
    node.setValueExpr(wrap_in_atomization(m_cb->m_cur_sctx, m_cb->m_sctx, node.getValueExpr()));
  }

  return true;
}

#undef LOOKUP_OP1

void end_visit (text_expr&) {}
bool begin_visit (text_expr& node)
{
  checkNonUpdating(&*node.get_text());
  return true;
}

void end_visit (pi_expr&) {}
bool begin_visit (pi_expr& node)
{
  checkNonUpdating(&*node.get_target_expr());
  return true;
}

DEF_VISIT_METHODS (insert_expr)
DEF_VISIT_METHODS (delete_expr)
DEF_VISIT_METHODS (rename_expr)
DEF_VISIT_METHODS (replace_expr)
DEF_VISIT_METHODS (transform_expr)
DEF_VISIT_METHODS (exit_expr)
DEF_VISIT_METHODS (while_expr)
DEF_VISIT_METHODS (flowctl_expr)
DEF_VISIT_METHODS (function_def_expr)

DEF_VISIT_METHODS (ft_select_expr)
DEF_VISIT_METHODS (ft_contains_expr)

};


void normalize_expr_tree(
    const char* norm_descr,
    CompilerCB* aCompilerCB,
    expr_t& root,
    const XQType* rType) 
{
  normalizer n (aCompilerCB);

  root->accept(n);

  if (rType != NULL)
  {
    if (TypeOps::is_builtin_simple(*rType)) 
    {
      root = wrap_in_atomization(aCompilerCB->m_cur_sctx, aCompilerCB->m_sctx, root);
      root = wrap_in_type_conversion(aCompilerCB->m_cur_sctx, root, rType);
    }
    else 
    {
      root = wrap_in_typematch (aCompilerCB->m_cur_sctx, root, rType);
    }
  }

  if (aCompilerCB->m_config.normalize_cb)
    aCompilerCB->m_config.normalize_cb (&*root, norm_descr);
}

}

/* vim:set ts=2 sw=2: */
