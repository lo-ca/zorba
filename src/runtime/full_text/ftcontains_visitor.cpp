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

#include <limits>

#include "compiler/expression/ft_expr.h"
#include "compiler/expression/ftnode.h"
#include "runtime/full_text/apply.h"
#include "runtime/full_text/ftcontains_visitor.h"
#include "runtime/full_text/stl_helpers.h"
#include "zorbautils/container_util.h"

using namespace std;

namespace zorba {

#ifdef WIN32
// Windows annoyingly defines these as macros.
#undef max
#undef min
#endif

///////////////////////////////////////////////////////////////////////////////

ftcontains_visitor::ftcontains_visitor( FTTokenIterator &search_context,
                                        PlanState &state ) :
  search_context_( search_context ),
  plan_state_( state )
{
  // do nothing
}

ftcontains_visitor::~ftcontains_visitor() {
  while ( !eval_stack_.empty() )
    delete pop();
}

bool ftcontains_visitor::ftcontains() const {
  if ( eval_stack_.empty() )
    return false;
  ft_all_matches const &am = *top();
  if ( am.empty() )
    return false;
  //
  // See spec section 4.3.
  //
  FOR_EACH( ft_all_matches, m, am )
    if ( m->excludes.empty() )
      return true;
  return false;
}

expr_visitor* ftcontains_visitor::get_expr_visitor() {
  return NULL;
}

inline void ftcontains_visitor::push( ft_all_matches *m ) {
  eval_stack_.push( m );
}

inline ft_all_matches* ftcontains_visitor::top() const {
  return eval_stack_.top();
}

inline ft_all_matches* ftcontains_visitor::pop() {
  ft_all_matches *const m = top();
  eval_stack_.pop();
  return m;
}

inline void pop_helper( int line ) {
  cout << "POP @ line " << line << endl;
}

#if 0

#define PUSH(X)   cout << "PUSH @ line " << __LINE__ << endl; push(X)
#define POP(X)    ( pop_helper(__LINE__), pop(X) )

#else

#define PUSH(X)   push(X)
#define POP(X)    pop(X)

#endif

///////////////////////////////////////////////////////////////////////////////

#if 0

static int indent;

#define BEGIN_VISIT(LABEL) \
  cout << string( (indent++ * 2), ' ' ) << #LABEL << endl

#define END_VISIT(RETURN) \
  --indent

#else

#define BEGIN_VISIT(LABEL)  /* nothing */
#define END_VISIT()         /* nothing */

#endif /* NDEBUG */

#define V ftcontains_visitor

#undef DEF_FTNODE_VISITOR_BEGIN_VISIT
#define DEF_FTNODE_VISITOR_BEGIN_VISIT(V,C)     \
  ft_visit_result::type V::begin_visit( C& ) {  \
    BEGIN_VISIT(C);                             \
    return ft_visit_result::proceed;            \
  }

#undef DEF_FTNODE_VISITOR_END_VISIT
#define DEF_FTNODE_VISITOR_END_VISIT(V,C)   \
  void V::end_visit( C& ) {                 \
    END_VISIT();                            \
  }

///////////////////////////////////////////////////////////////////////////////

ft_visit_result::type V::begin_visit( ftand &a ) {
  BEGIN_VISIT( ftand );
  PUSH( NULL ); // sentinel
  return ft_visit_result::proceed;
}

void V::end_visit( ftand &a ) {
  while ( true ) {
    ft_all_matches *const ami = POP(), *const amj = POP();
    if ( !amj ) {
      PUSH( ami );
      break;
    }
    ft_all_matches *const result = new ft_all_matches;
    apply_ftand( *ami, *amj, *result );
    PUSH( result );
    delete ami;
    delete amj;
  }
  END_VISIT();
}

DEF_FTNODE_VISITOR_VISIT_MEM_FNS( V, ftextension_selection )

ft_visit_result::type V::begin_visit( ftmild_not &mn ) {
  BEGIN_VISIT( ftmild_not );
  PUSH( NULL ); // sentinel
  return ft_visit_result::proceed;
}

void V::end_visit( ftmild_not &mn ) {
  while ( true ) {
    ft_all_matches *const ami = POP(), *const amj = POP();
    if ( !amj ) {
      PUSH( ami );
      break;
    }
    ft_all_matches *const result = new ft_all_matches;
    apply_ftmild_not( *ami, *amj, *result );
    PUSH( result );
    delete ami;
    delete amj;
  }
  END_VISIT();
}

ft_visit_result::type V::begin_visit( ftor &o ) {
  BEGIN_VISIT( ftor );
  PUSH( NULL ); // sentinel
  return ft_visit_result::proceed;
}

void V::end_visit( ftor &o ) {
  while ( true ) {
    ft_all_matches *const ami = POP(), *const amj = POP();
    if ( !amj ) {
      PUSH( ami );
      break;
    }
    ft_all_matches *const result = new ft_all_matches;
    apply_ftor( *ami, *amj, *result );
    PUSH( result );
    delete ami;
    delete amj;
  }
  END_VISIT();
}

DEF_FTNODE_VISITOR_VISIT_MEM_FNS( V, ftprimary_with_options )
DEF_FTNODE_VISITOR_VISIT_MEM_FNS( V, ftrange )
DEF_FTNODE_VISITOR_VISIT_MEM_FNS( V, ftselection )

DEF_FTNODE_VISITOR_BEGIN_VISIT( V, ftunary_not )
void V::end_visit( ftunary_not& ) {
  apply_ftunary_not( *top() );
  END_VISIT();
}

DEF_FTNODE_VISITOR_BEGIN_VISIT( V, ftwords )
void V::end_visit( ftwords &w ) {
  store::Item_t item;
  PlanIterator::consumeNext( item, w.get_plan_iter(), plan_state_ );
  FTTokenIterator query_tokens( item->getQueryTokens() );
  FTToken::int_t query_pos = 0;         // TODO: what should this really be?
  ft_all_matches *const result = new ft_all_matches;
  apply_ftwords(
    search_context_, query_tokens, query_pos, w.get_mode(), *result
  );
  PUSH( result );
  END_VISIT();
}

DEF_FTNODE_VISITOR_VISIT_MEM_FNS( V, ftwords_times )

////////// FTPosFilters ///////////////////////////////////////////////////////

DEF_FTNODE_VISITOR_BEGIN_VISIT( V, ftcontent_filter )
void V::end_visit( ftcontent_filter &f ) {
  apply_ftcontent( *top(), f.get_mode() );
  END_VISIT();
}

DEF_FTNODE_VISITOR_BEGIN_VISIT( V, ftdistance_filter )
void V::end_visit( ftdistance_filter &f ) {
  ftrange const *const range = f.get_range();

  store::Item_t item1;
  PlanIterator::consumeNext( item1, range->get_plan_iter1(), plan_state_ );

  int at_least = 0;
  int at_most = numeric_limits<int>::max();

  switch ( range->get_mode() ) {
    case ft_range_mode::at_least:
      at_least = item1->getIntValue();
      break;
    case ft_range_mode::at_most:
      at_most = item1->getIntValue();
      break;
    case ft_range_mode::exactly:
      at_least = at_most = item1->getIntValue();
      break;
    case ft_range_mode::from_to:
      at_least = item1->getIntValue();
      store::Item_t item2;
      PlanIterator::consumeNext( item2, range->get_plan_iter2(), plan_state_ );
      at_most = item2->getIntValue();
      break;
  }

  ft_all_matches *const am = POP();
  ft_all_matches *const result = new ft_all_matches;
  apply_ftdistance( *am, at_least, at_most, f.get_unit(), *result );
  PUSH( result );
  delete am;
  END_VISIT();
}

DEF_FTNODE_VISITOR_BEGIN_VISIT( V, ftorder_filter )
void V::end_visit( ftorder_filter &f ) {
  apply_ftorder( *top() );
  END_VISIT();
}

DEF_FTNODE_VISITOR_BEGIN_VISIT( V, ftscope_filter )
void V::end_visit( ftscope_filter &f ) {
  ft_all_matches *const am = POP();
  ft_all_matches *const result = new ft_all_matches;
  apply_ftscope( *am, f.get_scope(), f.get_unit(), *result );
  PUSH( result );
  delete am;
  END_VISIT();
}

DEF_FTNODE_VISITOR_BEGIN_VISIT( V, ftwindow_filter )
void V::end_visit( ftwindow_filter &f ) {
  store::Item_t item;
  PlanIterator::consumeNext( item, f.get_plan_iter(), plan_state_ );
  ft_all_matches *const am = POP();
  ft_all_matches *const result = new ft_all_matches;
  apply_ftwindow( *am, item->getIntValue(), f.get_unit(), *result );
  PUSH( result );
  delete am;
  END_VISIT();
}

////////// FTMatchOptions /////////////////////////////////////////////////////

DEF_FTNODE_VISITOR_VISIT_MEM_FNS( V, ftcase_option )
DEF_FTNODE_VISITOR_VISIT_MEM_FNS( V, ftdiacritics_option )
DEF_FTNODE_VISITOR_VISIT_MEM_FNS( V, ftextension_option )
DEF_FTNODE_VISITOR_VISIT_MEM_FNS( V, ftlanguage_option )
DEF_FTNODE_VISITOR_VISIT_MEM_FNS( V, ftmatch_options )
DEF_FTNODE_VISITOR_VISIT_MEM_FNS( V, ftstem_option )
DEF_FTNODE_VISITOR_VISIT_MEM_FNS( V, ftstop_word_option )
DEF_FTNODE_VISITOR_VISIT_MEM_FNS( V, ftstop_words )
DEF_FTNODE_VISITOR_VISIT_MEM_FNS( V, ftthesaurus_id )
DEF_FTNODE_VISITOR_VISIT_MEM_FNS( V, ftthesaurus_option )
DEF_FTNODE_VISITOR_VISIT_MEM_FNS( V, ftwild_card_option )

#undef V

} // namespace zorba
/* vim:set et sw=2 ts=2: */
