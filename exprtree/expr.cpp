/* -*- mode: c++; indent-tabs-mode: nil -*-
 *
 *  $Id: exprnodes.cpp.cpp,v 1.1 2006/10/09 07:07:59 Paul Pedersen Exp $
 *
 *	Copyright 2006-2007 FLWOR Foundation.
 *
 *  Author: Paul Pedersen
 *
 */

#include "expr.h"
#include "../parser/parse_constants.h"
#include "../util/Assert.h"
#include "../util/xqp_exception.h"

#include <iostream>
#include <string>
#include <vector>

using namespace std;
namespace xqp {
  
  

/////////////////////////////////////////////////////////////////////////
//                                                                     //
//  XQuery 1.0 productions                                             //
//  [http://www.w3.org/TR/xquery/]                                     //
//                                                                     //
/////////////////////////////////////////////////////////////////////////


// [31] [http://www.w3.org/TR/xquery/#prod-xquery-Expr]

expr_list::expr_list(
	yy::location const& loc)
:
	expr(loc)
{
}

expr_list::~expr_list()
{
}

ostream& expr_list::put(ostream& os) const
{
	os << "expr_list[\n";
	vector<rchandle<expr> >::const_iterator it = begin();
	vector<rchandle<expr> >::const_iterator en = end();
	for (; it!=en; ++it) {
		rchandle<expr> e_h = *it;
		Assert<null_pointer>(e_h!=NULL);
		e_h->put(os) << endl;
	}
	return os << "]\n";
}



// [33a]

std::ostream& var_expr::put(std::ostream& os) const
{
	return os << "var_expr[]\n";
}



// [33] [http://www.w3.org/TR/xquery/#prod-xquery-FLWORExpr]

flwor_expr::flwor_expr(
	yy::location const& loc)
:
	expr(loc)
{
}

flwor_expr::~flwor_expr()
{
}

ostream& flwor_expr::put(ostream& os) const
{
	os << "flwor_expr[\n";

	vector<vartriple_t>::const_iterator var_it = var_begin();
	vector<vartriple_t>::const_iterator var_en = var_end();

	for (; var_it!=var_en; ++var_it) {
		vartriple_t vt = *var_it;
		varref_t v[] = { vt.first, vt.second, vt.third };
		for (unsigned i=0; i<3; ++i) {
			varref_t vf_h = v[i];
			switch (vf_h->kind) {
			case var_expr::for_var: os << "FOR\n"; break;
			case var_expr::let_var: os << "LET\n"; break;
			case var_expr::pos_var: os << "AT\n"; break;
			case var_expr::score_var: os << "SCORE\n"; break;
			default: os << "huh??\n";
			}
			Assert<null_pointer>(vf_h->varname_h!=NULL);
			vf_h->varname_h->put(os);
			if (vf_h->type_h!=NULL) {
				os << "AS " << vf_h->type_h->describe() << endl;
			}
			switch (vf_h->kind) {
			case var_expr::for_var: {
				os << "IN\n";
				vf_h->valexpr_h->put(os) << endl;
				break;
			}
			case var_expr::let_var: {
				os << ":=\n";
				vf_h->valexpr_h->put(os) << endl;
				break;
			}
			default: os << "huh??\n";
			}
		}
	}

	if (where_h!=NULL) where_h->put(os);

	vector<orderspec_t>::const_iterator ord_it = orderspec_begin();
	vector<orderspec_t>::const_iterator ord_en = orderspec_end();

	for (; ord_it!=ord_en; ++ord_it) {
		orderspec_t spec = *ord_it;
		exprref_t e_h = spec.first;
		Assert<null_pointer>(e_h!=NULL);
		orderref_t ord_h = spec.second;
		Assert<null_pointer>(ord_h!=NULL);
		os << "ORDERBY\n";
		os << e_h->put(os) << endl;
		switch (ord_h->dir) {
		case dir_ascending: os << "ASCENDING\n"; break;
		case dir_descending: os << "DESCENDING\n"; break;
		default: os << "??\n";
		}
		switch (ord_h->empty_mode) {
		case context::empty_greatest: os << "EMPTY GREATEST\n"; break;
		case context::empty_least: os << "EMPTY LEAST\n"; break;
		default: os << "??\n";
		}
		os << ord_h->collation << endl;
	}

	Assert<null_pointer>(retval_h!=NULL);
	os << "RETURN\n";
	retval_h->put(os);
	return os << "]\n";

}



// [42] [http://www.w3.org/TR/xquery/#prod-xquery-QuantifiedExpr]

quantified_expr::quantified_expr(
	yy::location const& loc,
	enum quantification_mode_t _qmode)
:
	expr(loc),
	qmode(_qmode)
{
}

quantified_expr::~quantified_expr()
{
}

ostream& quantified_expr::put(ostream& os) const
{
	os << "quantified_expr[\n";
	switch (qmode) {
	case quant_some: os << "SOME\n"; break;
	case quant_every: os << "EVERY\n"; break;
	default: os << "??\n";
	}

	vector<varref_t>::const_iterator it = var_v.begin();
	vector<varref_t>::const_iterator en = var_v.end();

	for (; it!=en; ++it) {
		varref_t var_h = *it;
		Assert<null_pointer>(var_h!=NULL);
		Assert<null_pointer>(var_h->varname_h!=NULL);
		var_h->varname_h->put(os) << " in ";
		Assert<null_pointer>(var_h->valexpr_h!=NULL);
		var_h->valexpr_h->put(os) << endl;
	}

	os << " satisfies\n";
	Assert<null_pointer>(sat_expr_h!=NULL);
	sat_expr_h->put(os);
	return os << "\n]\n";
}



// [43] [http://www.w3.org/TR/xquery/#prod-xquery-TypeswitchExpr]

typeswitch_expr::typeswitch_expr(
	yy::location const& loc)
:
	expr(loc)
{
}

typeswitch_expr::~typeswitch_expr()
{
}

ostream& typeswitch_expr::put(ostream& os) const
{
	os << "typeswitch_expr[\n";
	Assert<null_pointer>(switch_expr_h!=NULL);
	switch_expr_h->put(os);
	vector<case_clause>::const_iterator it = case_clause_hv.begin();
	vector<case_clause>::const_iterator en = case_clause_hv.end();
	for (; it!=en; ++it) {
		case_clause cc = *it;
		os << "case ";
		if (cc.var_h!=NULL) cc.var_h->put(os) << " as ";
		os << cc.seqtype.describe() << " return ";
		Assert<null_pointer>(cc.case_expr_h!=NULL);
		cc.case_expr_h->put(os) << endl;
	}
	return os << "\n]\n";
}



// [45] [http://www.w3.org/TR/xquery/#prod-xquery-IfExpr]

if_expr::if_expr(
	yy::location const& loc,
	rchandle<expr> _cond_expr_h,
	rchandle<expr> _then_expr_h,
	rchandle<expr> _else_expr_h)
:
	expr(loc),
	cond_expr_h(_cond_expr_h),
	then_expr_h(_then_expr_h),
	else_expr_h(_else_expr_h)
{
}

if_expr::if_expr(
	yy::location const& loc)
:
	expr(loc)
{
}

if_expr::~if_expr()
{
}

ostream& if_expr::put(ostream& os) const
{
	os << "if_expr[\n";
	Assert<null_pointer>(cond_expr_h!=NULL);
	cond_expr_h->put(os);
	Assert<null_pointer>(then_expr_h!=NULL);
	then_expr_h->put(os);
	Assert<null_pointer>(else_expr_h!=NULL);
	else_expr_h->put(os);
	return os << "\n]\n";
}














////////////////////////////////
//	first-order expressions
////////////////////////////////

// [46] [http://www.w3.org/TR/xquery/#prod-xquery-OrExpr]

fo_expr::fo_expr(
	yy::location const& loc)
:
	expr(loc)
{
}

fo_expr::~fo_expr()
{
}

ostream& fo_expr::put(ostream& os) const
{
	os << "fo_expr[\n";
	Assert<null_pointer>(func!=NULL);
	func->sig.fname.put(os) << endl;

	vector<rchandle<expr> >::const_iterator it = begin();
	vector<rchandle<expr> >::const_iterator en = end();
	for (; it!=en; ++it) {
		rchandle<expr> e_h = *it;
		Assert<null_pointer>(e_h!=NULL);
		e_h->put(os) << endl;
	}
	return os << "]\n";
}


// first-order expressions:
//
// [47] [http://www.w3.org/TR/xquery/#prod-xquery-AndExpr]
// [48] [http://www.w3.org/TR/xquery/#prod-xquery-ComparisonExpr]
// [49] [http://www.w3.org/TR/xquery/#prod-xquery-RangeExpr]
// [50] [http://www.w3.org/TR/xquery/#prod-xquery-AdditiveExpr]
// [51] [http://www.w3.org/TR/xquery/#prod-xquery-MultiplicativeExpr]
// [52] [http://www.w3.org/TR/xquery/#prod-xquery-UnionExpr]
// [53] [http://www.w3.org/TR/xquery/#prod-xquery-IntersectExceptExpr]



// [48a] [http://www.w3.org/TR/xquery-full-text/#prod-xquery-FTContainsExpr]

ft_contains_expr::ft_contains_expr(
	yy::location const& loc,
	rchandle<expr> _range_h,
	rchandle<expr> _ft_select_h,
	rchandle<expr> _ft_ignore_h)
:
	expr(loc),
	range_h(_range_h),
	ft_select_h(_ft_select_h),
	ft_ignore_h(_ft_ignore_h)
{
}

ft_contains_expr::~ft_contains_expr()
{
}

ostream& ft_contains_expr::put(ostream& os) const
{
	os << "ft_contains_expr[\n";
	Assert<null_pointer>(range_h!=NULL);
	range_h->put(os) << endl;
	Assert<null_pointer>(ft_select_h!=NULL);
	os << "ft_contains\n";
	ft_select_h->put(os) << endl;
	if (ft_ignore_h!=NULL) ft_ignore_h->put(os);
	return os << "\n]\n";
}




// [54] [http://www.w3.org/TR/xquery/#prod-xquery-InstanceofExpr]

instanceof_expr::instanceof_expr(
	yy::location const& loc,
	rchandle<expr> _expr_h,
	sequence_type const& _seqtype)
:
	expr(loc),
	expr_h(_expr_h),
	seqtype(_seqtype)
{
}

instanceof_expr::~instanceof_expr()
{
}

ostream& instanceof_expr::put(ostream& os) const
{
	os << "instanceof_expr[\n";
	Assert<null_pointer>(expr_h!=NULL);
	expr_h->put(os) << endl;
	os << "instance of\n";
	os << seqtype.describe();
	return os << "\n]\n";
}



// [55] [http://www.w3.org/TR/xquery/#prod-xquery-TreatExpr]

treat_expr::treat_expr(
	yy::location const& loc,
	rchandle<expr> _expr_h,
	sequence_type const& _seqtype)
:
	expr(loc),
	expr_h(_expr_h),
	seqtype(_seqtype)
{
}

treat_expr::~treat_expr()
{
}

ostream& treat_expr::put(ostream& os) const
{
	os << "treat_expr[\n";
	Assert<null_pointer>(expr_h!=NULL);
	expr_h->put(os) << endl;
	os << "treat as\n";
	os << seqtype.describe();
	return os << "\n]\n";
}



// [56] [http://www.w3.org/TR/xquery/#prod-xquery-CastableExpr]

castable_expr::castable_expr(
	yy::location const& loc,
	rchandle<expr> _expr_h,
	single_type_t _stype)
:
	expr(loc),
	expr_h(_expr_h),
	stype(_stype)
{
}

castable_expr::~castable_expr()
{
}

ostream& castable_expr::put(ostream& os) const
{
	os << "castable_expr[\n";
	Assert<null_pointer>(expr_h!=NULL);
	expr_h->put(os) << endl;
	os << "castable as\n";
	os << get_atomic_type().describe();
	if (is_optional()) os << "?";
	return os << "\n]\n";
}



// [57] [http://www.w3.org/TR/xquery/#prod-xquery-CastExpr]

cast_expr::cast_expr(
	yy::location const& loc,
	rchandle<expr> _expr_h,
	single_type_t _stype)
:
	expr(loc),
	expr_h(_expr_h),
	stype(_stype)
{
}

cast_expr::~cast_expr()
{
}

ostream& cast_expr::put(ostream& os) const
{
	os << "cast_expr[\n";
	Assert<null_pointer>(expr_h!=NULL);
	expr_h->put(os) << endl;
	os << "cast as\n";
	os << get_atomic_type().describe();
	if (is_optional()) os << "?";
	return os << "\n]\n";
}



// [58] [http://www.w3.org/TR/xquery/#prod-xquery-UnaryExpr]

unary_expr::unary_expr(
	yy::location const& loc,
	bool _neg_b,
	rchandle<expr> _expr_h)
:
	expr(loc),
	neg_b(_neg_b),
	expr_h(_expr_h)
{
}

unary_expr::~unary_expr()
{
}

ostream& unary_expr::put(ostream& os) const
{
	os << "unary_expr[";
	os << (neg_b ? "MINUS\n" : "\n");
	Assert<null_pointer>(expr_h!=NULL);
	expr_h->put(os) << endl;
	return os << "]\n";
}



// [63] [http://www.w3.org/TR/xquery/#prod-xquery-ValidateExpr]

validate_expr::validate_expr(
	yy::location const& loc,
	enum validation_mode_t _valmode,
	rchandle<expr> _expr_h)
:
	expr(loc),
	valmode(_valmode),
	expr_h(_expr_h)
{
}

validate_expr::~validate_expr()
{
}

ostream& validate_expr::put(ostream& os) const
{
	os << "validate_expr[";
	switch (valmode) {
	case val_strict: os << "strict\n"; break;
	case val_lax: os << "lax\n"; break;
	default: os << "??\n";
	}
	Assert<null_pointer>(expr_h!=NULL);
	expr_h->put(os) << endl;
	return os << "]\n";
}



// [65] [http://www.w3.org/TR/xquery/#prod-xquery-ExtensionExpr]

extension_expr::extension_expr(
	yy::location const& loc)
:
	expr(loc)
{
}

extension_expr::~extension_expr()
{
}

ostream& extension_expr::put(ostream& os) const
{
	os << "extension_expr[\n";
	vector<rchandle<pragma> >::const_iterator it = begin();
	vector<rchandle<pragma> >::const_iterator en = end();
	for (; it!=en; ++it) {
		rchandle<pragma> p_h = *it;
		Assert<null_pointer>(p_h!=NULL);
		Assert<null_pointer>(p_h->name_h!=NULL);
		p_h->name_h->put(os) << endl;
		os << p_h->content << endl;
	}
	Assert<null_pointer>(expr_h!=NULL);
	expr_h->put(os) << endl;
	return os << "]\n";
}



// [69] [http://www.w3.org/TR/xquery/#prod-xquery-RelativePathExpr]

relpath_expr::relpath_expr(
	yy::location const& loc)
:
	expr(loc)
{
}

relpath_expr::~relpath_expr()
{
}

ostream& relpath_expr::put(ostream& os) const
{
	os << "relpath_expr[\n";
	vector<rchandle<expr> >::const_iterator it = begin();
	vector<rchandle<expr> >::const_iterator en = end();
	for (; it!=en; ++it) {
		rchandle<expr> se_h = *it;
		Assert<null_pointer>(se_h!=NULL);
		se_h->put(os) << endl;
	}
	return os << "]\n";
}



// [70] [http://www.w3.org/TR/xquery/#prod-xquery-StepExpr]

step_expr::step_expr(
	yy::location const& loc)
:
	expr(loc)
{
}

step_expr::~step_expr()
{
}

ostream& step_expr::put(ostream& os) const
{
	return os << "step_expr[]\n";
}



// [71] [http://www.w3.org/TR/xquery/#prod-xquery-AxisStep]

axis_step_expr::axis_step_expr(
	yy::location const& loc)
:
	expr(loc)
{
}

axis_step_expr::~axis_step_expr()
{
}

ostream& axis_step_expr::put(ostream& os) const
{
	os << "axis_step_expr[";
	switch (axis) {
	case self: os << "self\n"; break;
	case child: os << "child\n"; break;
	case parent: os << "parent\n"; break;
	case descendant: os << "descendant\n"; break;
	case descendant_or_self: os << "descendant-or-self\n"; break;
	case ancestor: os << "ancestor\n"; break;
	case ancestor_or_self: os << "ancestor-or-self\n"; break;
	case following_sibling: os << "following-sibling\n"; break;
	case following: os << "following\n"; break;
	case preceding_sibling: os << "preceding-sibling\n"; break;
	case preceding: os << "preceding\n"; break;
	case attribute: os << "attribute\n"; break;
	default: os << "??\n";
	}

	switch (test) {
	case no_test: os << "no_test\n"; break;
	case name_test: os << "name_test\n"; break;
	case doc_test: {
		os << "doc_test\n";
		switch (docnode_test) {
		case no_test: os << "no_test\n"; break;
		case elem_test: os << "elem_test\n"; break;
		case attr_test: os << "attr_test\n"; break;
		default: os << "??\n";
		}
		break;
	}
	case elem_test: os << "elem_test\n"; break;
	case attr_test: os << "attr_test\n"; break;
	case xs_elem_test: os << "xs_elem_test\n"; break;
	case xs_attr_test: os << "xs_attr_test\n"; break;
	case pi_test: os << "pi_test\n"; break;
	case comment_test: os << "comment_test\n"; break;
	case text_test: os << "text_test\n"; break;
	case anykind_test: os << "anykind_test\n"; break;
	default: os << "??\n";
	}

	Assert<null_pointer>(name_h!=NULL);
	switch (wild) {
	case no_wild: name_h->put(os) << endl; break;
	case all_wild: os << "*\n"; break;
	case prefix_wild: os << "*:"; name_h->put(os) << endl; break;
	case name_wild: name_h->put(os) << ":*\n"; break;
	default: os << "??\n";
	}

	if (typename_h!=NULL) {
		typename_h->put(os) << endl;
	}

	vector<rchandle<expr> >::const_iterator it = pred_hv.begin();
	vector<rchandle<expr> >::const_iterator en = pred_hv.end();
	for (; it!=en; ++it) {
		rchandle<expr> e_h = *it;
		Assert<null_pointer>(e_h!=NULL);
		e_h->put(os) << endl;
	}
	return os << "]\n";
}



// [84] [http://www.w3.org/TR/xquery/#prod-xquery-PrimaryExpr]

primary_expr::primary_expr(
	yy::location const& loc)
:
	expr(loc)
{
}

primary_expr::~primary_expr()
{
}

ostream& primary_expr::put(ostream& os) const
{
	return os << "primary_expr[]\n";
}



// [85] [http://www.w3.org/TR/xquery/#prod-xquery-PrimaryExpr]

literal_expr::literal_expr(
	yy::location const& loc,
	uint32_t _sref)
:
	expr(loc),
	type(lit_string),
	sref(_sref)
{
}

literal_expr::literal_expr(
	yy::location const& loc,
	int i)
:
	expr(loc),
	type(lit_integer),
	ival(i)
{
}

literal_expr::literal_expr(
	yy::location const& loc,
	decimal d)
:
	expr(loc),
	type(lit_decimal),
	decval(d)
{
}

literal_expr::literal_expr(
	yy::location const& loc,
	double d)
:
	expr(loc),
	type(lit_double),
	dval(d)
{
}

literal_expr::~literal_expr()
{
}

ostream& literal_expr::put(ostream& os) const
{
	os << "literal_expr[\n";
	switch (type) {
	case lit_string: os << "string[" << sref << "]\n";
	case lit_integer: os << "integer[" << ival << "]\n";
	case lit_decimal: os << "decimal[" << decval << "]\n";
	case lit_double: os << "double[" << dval << "]\n";
	default: os << "??\n";
	}
	return os << "]\n";
}




// [91] [http://www.w3.org/TR/xquery/#prod-xquery-OrderedExpr]

order_expr::order_expr(
	yy::location const& loc,
	order_type_t _type,
	rchandle<expr> _expr_h)
:
	expr(loc),
	type(_type),
	expr_h(_expr_h)
{
}

order_expr::~order_expr()
{
}

ostream& order_expr::put(ostream& os) const
{
	os << "order_expr[";
	switch (type) {
	case ordered: os << "ordered\n"; break;
	case unordered: os << "unordered\n"; break;
	default: os << "??\n";
	}
	Assert<null_pointer>(expr_h!=NULL);
	expr_h->put(os) << endl;
	return os << "]\n";
}



// [93] [http://www.w3.org/TR/xquery/#prod-xquery-FunctionCall]

funcall_expr::funcall_expr(
	yy::location const& loc,
	rchandle<QName> _fname_h)
:
	expr(loc),
	fname_h(_fname_h)
{
}

funcall_expr::~funcall_expr()
{
}

ostream& funcall_expr::put(ostream& os) const
{
	os << "funcall_expr[";
	Assert<null_pointer>(fname_h!=NULL);
	fname_h->put(os) << endl;
	vector<rchandle<expr> >::const_iterator it = arg_hv.begin();
	vector<rchandle<expr> >::const_iterator en = arg_hv.end();
	for (; it!=en; ++it) {
		rchandle<expr> e_h = *it;
		Assert<null_pointer>(e_h!=NULL);
		e_h->put(os) << endl;
	}
	return os << "]\n";
}



// [109] [http://www.w3.org/TR/xquery/#prod-xquery-ComputedConstructor]

cons_expr::cons_expr(
	yy::location const& loc)
:
	expr(loc)
{
}

cons_expr::~cons_expr()
{
}

ostream& cons_expr::put(ostream& os) const
{
	return os << "cons_expr[]\n";
}



// [110] [http://www.w3.org/TR/xquery/#prod-xquery-CompDocConstructor]

doc_expr::doc_expr(
	yy::location const& loc,
	rchandle<expr> _docuri_h)
:
	expr(loc),
	docuri_h(_docuri_h)
{
}

doc_expr::~doc_expr()
{
}

ostream& doc_expr::put(ostream& os) const
{
	os << "doc_expr[\n";
	Assert<null_pointer>(docuri_h!=NULL);
	docuri_h->put(os) << endl;
	return os << "]\n";
}



// [111] [http://www.w3.org/TR/xquery/#prod-xquery-CompElemConstructor]

elem_expr::elem_expr(
	yy::location const& loc,
	rchandle<QName> _qname_h,
	rchandle<expr> _content_expr_h)
:
	expr(loc),
	qname_h(_qname_h),
	qname_expr_h(NULL),
	content_expr_h(_content_expr_h)
{
}

elem_expr::elem_expr(
	yy::location const& loc,
	rchandle<expr> _qname_expr_h,
	rchandle<expr> _content_expr_h)
:
	expr(loc),
	qname_h(NULL),
	qname_expr_h(_qname_expr_h),
	content_expr_h(_content_expr_h)
{
}

elem_expr::~elem_expr()
{
}

ostream& elem_expr::put(ostream& os) const
{
	os << "elem_expr[\n";
	Assert<bad_arg>(qname_h!=NULL || qname_expr_h!=NULL);
	if (qname_h!=NULL) {
		qname_h->put(os) << endl;
	}
	else {
		qname_expr_h->put(os) << endl;
	}
	Assert<null_pointer>(content_expr_h!=NULL);
	content_expr_h->put(os) << endl;

	nsb_v;
	vector<nsbinding>::const_iterator it = begin();
	vector<nsbinding>::const_iterator en = end();
	for (; it!=en; ++it) {
		nsbinding nsb = *it;
		string ncname = nsb.first;
		string nsuri = nsb.second;
		os << "xmlns:" << ncname << "=\"" << nsuri << "\"\n";
	}
	return os << "]\n";
}



// [113] [http://www.w3.org/TR/xquery/#prod-xquery-CompAttrConstructor]

attr_expr::attr_expr(
	yy::location const& loc,
	rchandle<QName> _qname_h,
	rchandle<expr> _val_expr_h)
:
	expr(loc),
	qname_h(_qname_h),
	qname_expr_h(NULL),
	val_expr_h(_val_expr_h)
{
}

attr_expr::attr_expr(
	yy::location const&,
	rchandle<expr> _qname_expr_h,
	rchandle<expr> _val_expr_h)
:
	expr(loc),
	qname_h(NULL),
	qname_expr_h(_qname_expr_h),
	val_expr_h(_val_expr_h)
{
}

attr_expr::~attr_expr()
{
}

ostream& attr_expr::put(ostream& os) const
{
	os << "attr_expr[\n";
	Assert<bad_arg>(qname_h!=NULL || qname_expr_h!=NULL);
	if (qname_h!=NULL) {
		qname_h->put(os) << endl;
	}
	else {
		qname_expr_h->put(os) << endl;
	}
	Assert<null_pointer>(val_expr_h!=NULL);
	val_expr_h->put(os) << endl;
	return os << "]\n";
}



// [114] [http://www.w3.org/TR/xquery/#prod-xquery-CompTextConstructor]

text_expr::text_expr(
	yy::location const& loc,
	rchandle<expr> _text_expr_h)
:
	expr(loc),
	text_expr_h(_text_expr_h)
{
}

text_expr::~text_expr()
{
}

ostream& text_expr::put(ostream& os) const
{
	os << "text_expr[\n";
	Assert<null_pointer>(text_expr_h!=NULL);
	text_expr_h->put(os) << endl;
	return os << "]\n";
}



// [115] [http://www.w3.org/TR/xquery/#prod-xquery-CompCommentConstructor]

comment_expr::comment_expr(
	yy::location const& loc,
	rchandle<expr> _comment_expr_h)
:
	expr(loc),
	comment_expr_h(_comment_expr_h)
{
}

comment_expr::~comment_expr()
{
}

ostream& comment_expr::put(ostream& os) const
{
	os << "comment_expr[\n";
	Assert<null_pointer>(comment_expr_h!=NULL);
	comment_expr_h->put(os) << endl;
	return os << "]\n";
}



// [114] [http://www.w3.org/TR/xquery/#prod-xquery-CompPIConstructor]

pi_expr::pi_expr(
	yy::location const& loc,
	std::string _target,
	rchandle<expr> _content_expr_h)
:
	expr(loc),
	target(_target),
	target_expr_h(NULL),
	content_expr_h(_content_expr_h)
{
}

pi_expr::pi_expr(
	yy::location const& loc,
	rchandle<expr> _target_expr_h,
	rchandle<expr> _content_expr_h)
:
	expr(loc),
	target(""),
	target_expr_h(_target_expr_h),
	content_expr_h(_content_expr_h)
{
}

pi_expr::~pi_expr()
{
}

ostream& pi_expr::put(ostream& os) const
{
	os << "pi_expr[\n";
	Assert<bad_arg>(target.length()>0 || target_expr_h!=NULL);
	if (target.length()>0) {
		os << target << endl;
	}
	else {
		target_expr_h->put(os) << endl;
	}
	Assert<null_pointer>(content_expr_h!=NULL);
	content_expr_h->put(os) << endl;
	return os << "]\n";
}

















} /* namespace xqp */

