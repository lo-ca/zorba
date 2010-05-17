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
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "system/properties.h"

#include "context/static_context_consts.h"

#include "compiler/expression/expr.h"
#include "compiler/expression/fo_expr.h"
#include "compiler/expression/ft_expr.h"
#include "compiler/expression/ftnode.h"
#include "compiler/expression/path_expr.h"
#include "compiler/expression/var_expr.h"
#include "compiler/expression/flwor_expr.h"
#include "compiler/expression/function_item_expr.h"
#include "compiler/parser/parse_constants.h"

#include "functions/function.h"
#include "functions/udf.h"


using namespace std;

namespace zorba 
{
  

static int printdepth0 = -2;

#define DENT      string(printdepth0, ' ')
#define UNDENT    printdepth0 -= 2;
#define INDENT    (printdepth0 += 2, DENT)

#define BEGIN_EXPR( descr) os << INDENT << #descr << expr_addr (this) << " [\n"

#define CLOSE_EXPR os << DENT << "]\n"; UNDENT; return os;

#define PUT_SUB( descr, sub ) do { if ((sub) != NULL) { os << DENT << (descr) << "\n"; (sub)->put (os); } } while (0)


static inline xqp_string qname_to_string(store::Item_t qname) 
{
  xqp_string result;
  xqp_string pfx = qname->getPrefix (), ns = qname->getNamespace();
  if (! ns.empty ())
    result += pfx + "[=" + qname->getNamespace()->str() + "]:";
  result += qname->getLocalName()->str();
  return result;
}


static inline ostream& put_qname(store::Item_t qname, ostream &os) 
{
  xqp_string pfx = qname->getPrefix (), ns = qname->getNamespace();
  if (! ns.empty ())
    os << pfx << "[=" << qname->getNamespace()->str() << "]:";
  os << qname->getLocalName()->str();
  return os;
}


static inline string expr_addr(const void* e) 
{
  if (Properties::instance()->noTreeIds ())
  {
    return "";
  }
  else 
  {
    ostringstream os;
    os << " (" << e << ")";
    return os.str ();
  }
}


std::ostream& debugger_expr::put(std::ostream& os) const
{
  BEGIN_EXPR (debugger_expr);
  theExpr->put(os);
  CLOSE_EXPR;
}

std::ostream& wrapper_expr::put(std::ostream& os) const
{
#ifdef VERBOSE
  get_expr()->put(os);
  return os;
#else
  if (get_expr()->get_expr_kind() == var_expr_kind)
  {
    const var_expr* varExpr = static_cast<const var_expr*>(get_expr());

    os << INDENT << "var_ref [ " << expr_addr(this) << " ";
    put_qname(varExpr->get_name(), os);
    os << expr_addr(varExpr) << " ]" << std::endl;
    UNDENT;
    return os;
  }
  else
  {
    BEGIN_EXPR(wrapper_expr);
    get_expr()->put(os);
    CLOSE_EXPR;
  }
#endif
}

ostream& sequential_expr::put( ostream& os) const
{
  BEGIN_EXPR (sequential_expr);
  for (checked_vector<expr_t>::const_iterator i = this->theArgs.begin ();
       i != theArgs.end (); i++)
    (*i)->put (os);
  CLOSE_EXPR;
}

ostream& var_expr::put(ostream& os) const
{
  os << INDENT << "var kind=" << decode_var_kind(get_kind()) << expr_addr(this);
  if (theName != NULL)
  {
    os << " name=";
    put_qname(get_name(), os);
  }

#if VERBOSE
  if (theDeclaredType != NULL) 
  {
    os << " type=" << theDeclaredType->toString ();
  }
#endif

  os << endl;
  UNDENT;
  return os;
}


ostream& for_clause::put(ostream& os) const
{
#if VERBOSE
  BEGIN_EXPR(FOR);

  theVarExpr->put(os);
  PUT_SUB ("AT", thePosVarExpr);
  PUT_SUB ("SCORE", theScoreVarExpr);

  PUT_SUB("IN", theDomainExpr);

#else
  os << INDENT << "FOR" << expr_addr(this) << " ";

  put_qname(theVarExpr->get_name(), os);
  os << expr_addr(theVarExpr.getp());
  if (thePosVarExpr != NULL)
  {
    os << " AT ";
    put_qname(thePosVarExpr->get_name(), os);
    os << expr_addr(thePosVarExpr.getp());
  }
  os << std::endl << DENT << "[\n";

  theDomainExpr->put(os);
#endif

  CLOSE_EXPR;
}


ostream& let_clause::put(ostream& os) const
{
#if VERBOSE
  BEGIN_EXPR(LET);

  theVarExpr->put(os);
  PUT_SUB ("SCORE", theScoreVarExpr);

  PUT_SUB(":=", theDomainExpr);

#else
  os << INDENT << "LET" << expr_addr(this) << " ";
  put_qname(theVarExpr->get_name(), os);
  os << expr_addr(theVarExpr.getp()) << std::endl << DENT << "[\n";

  theDomainExpr->put(os);
#endif

  CLOSE_EXPR;
}

ostream& window_clause::put(ostream& os) const
{
  BEGIN_EXPR (WINDOW);

  theVarExpr->put(os);

  PUT_SUB("IN", theDomainExpr);

  PUT_SUB ("START", theWinStartCond.getp());
  PUT_SUB ("STOP", theWinStopCond.getp());

  CLOSE_EXPR;
}


ostream& flwor_wincond::vars::put( ostream& os) const
{
  BEGIN_EXPR (flwor_wincond::vars);
  PUT_SUB ("AT", posvar);
  PUT_SUB ("CURR", curr);
  PUT_SUB ("NEXT", next);
  PUT_SUB ("PREV", prev);
  CLOSE_EXPR;
}

ostream& flwor_wincond::put( ostream& os) const
{
  BEGIN_EXPR (flwor_wincond);
  PUT_SUB ("IN-VARS", &get_in_vars());
  PUT_SUB ("OUT-VARS", &get_out_vars());
  PUT_SUB ("WHEN", theCondExpr);
  CLOSE_EXPR;
}


ostream& group_clause::put( ostream& os ) const 
{
  BEGIN_EXPR (group_clause);

  os << DENT << "GROUP BY EXPRS";

  for (unsigned i = 0; i < theGroupVars.size(); i++) 
  {
    PUT_SUB("", theGroupVars[i].first);
    os << DENT << "-->" << theGroupVars[i].second.getp() << std::endl;
  }
  os << endl;

  os << DENT << "NON GROUP BY VARS ";

  for (unsigned i = 0; i < theNonGroupVars.size(); i++) 
  {
    PUT_SUB("", theNonGroupVars[i].first);
    os << DENT << "-->" << theNonGroupVars[i].second.getp() << std::endl;
  }
  os << endl;

  CLOSE_EXPR;
}


ostream& orderby_clause::put( ostream &os ) const 
{
  BEGIN_EXPR(orderby_clause);

  //os << DENT << "ORDER BY ";

  unsigned numColumns = num_columns();

  for (unsigned i = 0; i < numColumns; i++) 
  {
    theOrderingExprs[i]->put(os);
  }
#if 0
  os << endl;

  os << DENT << "VAR REBINDS ";
  for (unsigned i = 0; i < theRebindList.size (); i++) 
  {
    os << "$";
    put_qname(theRebindList[i].first->get_varname(), os);
    os << " (" << theRebindList[i].first.getp() << " -> " 
       << theRebindList[i].second.getp() << ") ";
  }
  os << endl;
#endif
  CLOSE_EXPR;
}


ostream& flwor_expr::put( ostream& os) const
{
  BEGIN_EXPR (flwor_expr);

  for (unsigned i = 0; i < num_clauses(); i++) 
  {
    const flwor_clause& c = *((*this)[i]);

    if (c.get_kind() == flwor_clause::where_clause)
    {
      PUT_SUB ("WHERE", static_cast<const where_clause *>(&c)->get_expr());
    }
    else if (c.get_kind() == flwor_clause::count_clause) 
    {
      os << DENT << "COUNT $"; 
      put_qname(static_cast<const count_clause *>(&c)->get_var()->get_name(), os);
      os << endl;
    }
    else if (c.get_kind() == flwor_clause::for_clause) 
    {
      static_cast<const for_clause *>(&c)->put(os);
    }
    else if (c.get_kind() == flwor_clause::let_clause) 
    {
      static_cast<const let_clause *>(&c)->put(os);
    }
    else if (c.get_kind() == flwor_clause::window_clause) 
    {
      static_cast<const window_clause *>(&c)->put(os);
    }
    else if (c.get_kind() == flwor_clause::group_clause) 
    {
      static_cast<const group_clause *>(&c)->put(os);
    }
    else if (c.get_kind() == flwor_clause::order_clause) 
    {
      static_cast<const orderby_clause *>(&c)->put(os);
    }
  }

  os << DENT << "RETURN\n";
  if (theReturnExpr == NULL) 
  {
    os << "NULL";
  }
  else 
  {
    theReturnExpr->put(os);
  }
  CLOSE_EXPR;
}

ostream& promote_expr::put(ostream& os) const
{
  os << INDENT << "promote_expr " << theTargetType->toString()
     << expr_addr (this) << " [\n";
  theInputExpr->put(os);
  CLOSE_EXPR;
}

ostream& trycatch_expr::put( ostream& os) const
{
  BEGIN_EXPR (trycatch_expr);

  theTryExpr->put(os);

  ulong numClauses = theCatchClauses.size();

  for (ulong i = 0; i < numClauses; ++i)
  {
    catch_clause_t cc = theCatchClauses[i];
    os << DENT << "CATCH ";
    os << "\n";
    theCatchExprs[i]->put(os);
  }
  os << DENT << "]\n";
  return os;
}

ostream& eval_expr::put( ostream& os) const
{
  BEGIN_EXPR (eval_expr);
  theExpr->put (os);
  CLOSE_EXPR;
}


ostream& if_expr::put( ostream& os) const
{
  BEGIN_EXPR(if_expr);
  theCondExpr->put(os);
  PUT_SUB("THEN", theThenExpr);
  PUT_SUB("ELSE", theElseExpr);
  CLOSE_EXPR;
}

ostream& fo_expr::put( ostream& os) const
{
  const store::Item* qname = theFunction->getName();

  os << INDENT << qname->getStringValue() << "/" << num_args()
     << expr_addr(this) << " [\n";

  ulong numArgs = theArgs.size();

  for (ulong i = 0; i < numArgs; ++i)
  {
    theArgs[i]->put(os);
  }

  CLOSE_EXPR;
}


ostream& ftcontains_expr::put( ostream &os ) const {
  BEGIN_EXPR( ftcontains_expr );
  INDENT;
  PUT_SUB( "RANGE", range_ );
  os << DENT; ftselection_->put( os );
  PUT_SUB( "IGNORE", ftignore_ );
  UNDENT;
  CLOSE_EXPR;
}


std::ostream& function_item_expr::put(std::ostream& os) const
{
  os << INDENT << "funtion_item_expr " << expr_addr(this);

  if (theQName != NULL)
  {
    os << " " << theQName->getStringValue() << "/" << theArity;
    UNDENT;
    os << std::endl;
    return os;
  }
  else
  {
    os << " inline udf (" << theFunction.getp() << ") [\n";
    reinterpret_cast<const user_function*>(theFunction.getp())->getBody()->put(os);
    CLOSE_EXPR;
  }
}


ostream& dynamic_function_invocation_expr::put(ostream& os) const
{
  os << INDENT << "dynamic_function_invocation_expr " << expr_addr(this)
     << " [\n";

  theExpr->put(os);

  for (ulong i = 0; i < theArgs.size(); ++i)
    theArgs[i]->put(os);

  CLOSE_EXPR;
}


ostream& instanceof_expr::put( ostream& os) const
{
  os << INDENT << "instanceof_expr " << theTargetType->toString()
     << expr_addr (this) << " [\n";
  theInputExpr->put(os);
  CLOSE_EXPR;
}

ostream& treat_expr::put( ostream& os) const
{
  os << INDENT << "treat_expr " << theTargetType->toString()
     << expr_addr (this) << " [\n";
  theInputExpr->put(os);
  CLOSE_EXPR;
}

ostream& castable_expr::put( ostream& os) const
{
  os << INDENT << "castable_expr " << theTargetType->toString()
     << expr_addr (this) << " [\n";
  theInputExpr->put(os);
  CLOSE_EXPR;
}

ostream& cast_expr::put( ostream& os) const
{
  os << INDENT << "cast_expr " << theTargetType->toString()
     << expr_addr (this) << " [\n";
  theInputExpr->put(os);
  CLOSE_EXPR;
}

ostream& name_cast_expr::put( ostream& os) const
{
  BEGIN_EXPR (name_cast_expr);
  theInputExpr->put(os);
  CLOSE_EXPR;
}


ostream& validate_expr::put(ostream& os) const
{
  BEGIN_EXPR(validate_expr);

  switch (theMode) 
  {
  case ParseConstants::val_strict: os << "strict\n"; break;
  case ParseConstants::val_lax: os << "lax\n"; break;
  case ParseConstants::val_typename: os << "typename\n"; break;
  default: os << "??\n";
  }
  theExpr->put(os) << endl;
  CLOSE_EXPR;
}


ostream& extension_expr::put( ostream& os) const
{
  BEGIN_EXPR (extension_expr);

  os << INDENT;
  os << "?"; put_qname (thePragmas[0]->theQName, os);
  os << " " << thePragmas[0]->theContent << endl;
  UNDENT;

  theExpr->put(os) << endl;
  CLOSE_EXPR;
}

ostream& relpath_expr::put( ostream& os) const
{
  BEGIN_EXPR (relpath_expr);
  
  for (std::vector<expr_t>::const_iterator it = begin(); it != end(); ++it)
  {
    expr_t expr = *it;
    if (it == begin ())
    {
      expr->put (os);
    }
    else
    {
      os << INDENT << "REL STEP " ;
      expr->put(os);
      os << std::endl;
      UNDENT;
    }
  }
  CLOSE_EXPR;
}

ostream& axis_step_expr::put(ostream& os) const
{
  switch (theAxis)
  {
  case axis_kind_self:                os << "self::"; break;
  case axis_kind_child:               os << "child::"; break;
  case axis_kind_parent:              os << "parent::"; break;
  case axis_kind_descendant:          os << "descendant::"; break;
  case axis_kind_descendant_or_self:  os << "descendant-or-self::"; break;
  case axis_kind_ancestor:            os << "ancestor::"; break;
  case axis_kind_ancestor_or_self:    os << "ancestor-or-self::"; break;
  case axis_kind_following_sibling:   os << "following-sibling::"; break;
  case axis_kind_following:           os << "following::"; break;
  case axis_kind_preceding_sibling:   os << "preceding-sibling::"; break;
  case axis_kind_preceding:           os << "preceding::"; break;
  case axis_kind_attribute:           os << "attribute::"; break;
  default: os << "??";
  }

  if (theNodeTest != NULL) 
  {
    theNodeTest->put(os);
  }
  else
  {
    os << endl;
  }

  return os;
}


ostream& match_expr::put(ostream& os) const
{
  os << "match_expr [";
  switch (theTestKind)
  {
  case match_no_test:   os << "no_test("; break;
  case match_name_test: os << "name_test("; break;
  case match_doc_test:
  {
    os << "doc_test(";
    switch (theDocTestKind)
    {
    case match_no_test:   os << "no_test("; break;
    case match_elem_test: os << "element("; break;
    case match_attr_test: os << "attribute("; break;
    default: os << "(??";
    }
    break;
  }
  case match_elem_test:     os << "element("; break;
  case match_attr_test:     os << "attribute("; break;
  case match_xs_elem_test:  os << "schema-element("; break;
  case match_xs_attr_test:  os << "schema-element("; break;
  case match_pi_test:       os << "pi("; break;
  case match_comment_test:  os << "comment("; break;
  case match_text_test:     os << "text("; break;
  case match_anykind_test:  os << "node("; break;
  default: os << "(??";
  }

  switch (theWildKind)
  {
    case match_no_wild:
      if (theQName != NULL)
        put_qname (theQName, os);
      break;
    case match_all_wild:
      os << "*";
      break;
    case match_prefix_wild:
      os << "*:" << theWildName;
      break;
    case match_name_wild:
      os << theWildName << ":*";
      break;
    default:
      os << "??";
  }

  if (theTypeName != NULL)
  {
    put_qname (theTypeName, os) << endl;
  }

  os << ")]";
  return os;
}


ostream& const_expr::put(ostream& os) const
{
  os << INDENT << "const_expr" << expr_addr(this) << " ";
  if (theValue->isFunction())
  {
    os << "functrion item [ " << theValue->show().c_str() << " ]\n";
  }
  else
  {
    os << get_val()->getType()->getStringValue()->c_str()
       << " [ " << theValue->getStringValue() << " ]\n";
  }
  UNDENT;
  return os;
}


ostream& order_expr::put(ostream& os) const
{
  os << INDENT << "order_expr" << expr_addr (this) << "\n";
  os << DENT << "[ ";

  switch (theType) 
  {
  case ordered: os << "ordered\n"; break;
  case unordered: os << "unordered\n"; break;
  default: os << "??\n";
  }
  theExpr->put(os) << endl;
  CLOSE_EXPR;
}


ostream& elem_expr::put(ostream& os) const
{
  BEGIN_EXPR(elem_expr);

  if (theQNameExpr != NULL)
    theQNameExpr->put(os);
  if (theAttrs != NULL)
    theAttrs->put(os);
  if (theContent != NULL)
    theContent->put(os);
  CLOSE_EXPR;
}

ostream& doc_expr::put( ostream& os) const
{
  BEGIN_EXPR (doc_expr);

  theContent->put(os);
  CLOSE_EXPR;
}

ostream& attr_expr::put( ostream& os) const
{
  BEGIN_EXPR (attr_expr);

  theQNameExpr->put (os);
  PUT_SUB ("=", theValueExpr);
  CLOSE_EXPR;
}

ostream& text_expr::put(ostream& os) const
{
  BEGIN_EXPR(text_expr);

  theContentExpr->put(os);

  CLOSE_EXPR;
}

ostream& pi_expr::put( ostream& os) const
{
  BEGIN_EXPR (pi_expr);

  PUT_SUB ("TARGET", theTargetExpr);

  PUT_SUB ("CONTENT", theContentExpr);

  CLOSE_EXPR;
}


ostream& insert_expr::put( ostream& os) const
{
  BEGIN_EXPR (insert_expr);
  theSourceExpr->put(os);
  PUT_SUB (",", theTargetExpr);
  CLOSE_EXPR;
}

ostream& delete_expr::put( ostream& os) const
{
  BEGIN_EXPR (delete_expr);
  theTargetExpr->put(os);
  CLOSE_EXPR;
}

ostream& replace_expr::put( ostream& os) const
{
  BEGIN_EXPR (replace_expr);
  theTargetExpr->put(os);
  PUT_SUB (",", theReplaceExpr);
  CLOSE_EXPR;
}

ostream& rename_expr::put( ostream& os) const
{
  BEGIN_EXPR (rename_expr);
  theTargetExpr->put(os);
  PUT_SUB (",", theNameExpr);
  CLOSE_EXPR;
}

ostream& copy_clause::put( ostream& os) const
{
  BEGIN_EXPR (copy);
  theVar->put(os);
  theExpr->put(os);
  CLOSE_EXPR;
}

ostream& transform_expr::put( ostream& os) const
{
  BEGIN_EXPR (transform_expr);
  for (vector<rchandle<copy_clause> >::const_iterator it = theCopyClauses.begin();
       it != theCopyClauses.end(); ++it)
  {
    rchandle<copy_clause> e = *it;
    e->put(os);
  }
  theModifyExpr->put(os);
  PUT_SUB (",", theReturnExpr);
  CLOSE_EXPR;
}

ostream& exit_expr::put( ostream& os) const
{
  BEGIN_EXPR(exit_expr);
  theExpr->put(os);
  CLOSE_EXPR;
}

ostream& flowctl_expr::put( ostream& os) const
{
  BEGIN_EXPR(flowctl_expr);
  CLOSE_EXPR;
}

ostream& while_expr::put( ostream& os) const
{
  BEGIN_EXPR(while_expr);
  theBody->put(os);
  CLOSE_EXPR;
}

}  // namespace zorba
/* vim:set et sw=2 ts=2: */
