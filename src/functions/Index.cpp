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
#include "functions/Index.h"
#include "functions/function_impl.h"

#include "runtime/indexing/value_index_impl.h"
#include "runtime/indexing/context_index_impl.h"


namespace zorba
{

/******************************************************************************
  create-internal-index($indexName as xs:QName) as ()

  This is an intenal function that is used by the hashjoins rule to create and
  populate a temp index. There are 3 reasons why we need a different function
  than the regular create-index, which is defined below:

  1. create-index assumes that the domain and key expressions do not reference
     any free variables. This is not true for the temp index created by the
     hashjoin rule.
  2. The argument to create-index can be any arbitrary expression that returns
     a QName. In the case of create-internal-index we know that the arg is a
     const expr holding a qname item.
  3. create-internal-index is a "simple" function, whereas create-index is an
     updating function.
********************************************************************************/
class FunctionCreateInternalIndex : public function 
{
public:
  FunctionCreateInternalIndex(const signature& sig) 
    :
    function(sig, FunctionConsts::OP_CREATE_INTERNAL_INDEX_1)
  {
  }

  bool requires_dyn_ctx() const { return true; }

  CODEGEN_DECL();
};


CODEGEN_DEF(FunctionCreateInternalIndex)
{
  // There is no single iterator that implements this function. Instead, a
  // whole sub-plan is generated to build the index. This is done in 
  // plan_visitor.cpp because the sun-plan must be generated by the same 
  // visitor as th rest of the query plan.
  ZORBA_ASSERT(false);
}


/******************************************************************************
  create-index($indexName as xs:QName) as pul()

  This is an updating function. During normal runtime (see CreateIndexIterator),
  it checks that index does not exist already (in the dynamic context) and 
  generates an update primitive. During applyUpdates(), it creates the index
  container in the store and populates it according to the index definition.
  Also creates in the dynamic context an entry that maps the index qname and
  an index object that is returned by the store to act as a handle to the index. 

  The specification for the index to create is taken from the static context,
  which stores a mapping from the index uri to ValueIndex obj (defined in
  indexing/value_index.h).

  Note: the population of the index is done by a runtime plan that implements
  the following flwor expr:

    for $$dot at $$pos in domainExpr
    return index-entry-builder($$dot, fieldExpr1, ..., fieldExprN);

  This plan is generated "on-the-fly" by the CreateIndexIterator. The generated
  plan is stored in the update primitive, and during applyUpdates(), it is given
  as an arg to the SimpleStore::createIndex() method. 
********************************************************************************/
class FunctionCreateIndex : public function 
{
public:
  FunctionCreateIndex(const signature& sig) 
    :
    function(sig, FunctionConsts::FN_CREATE_INDEX_1)
  { 
  }

  bool requires_dyn_ctx() const { return true; }

 expr_script_kind_t getUpdateType() const { return UPDATE_EXPR; }

  DEFAULT_UNARY_CODEGEN(CreateIndexIterator);
};


/******************************************************************************
  drop-index($indexName as xs:QName) as ()

  This is an updating function. During normal runtime (see DropIndexIterator),
  it checks that index exists (in the dynamic context) and generates an update
  primitive. During applyUpdates(), it destroys the index container in the store
  and removes the index entry from the dynamic context. 
********************************************************************************/
class FunctionDropIndex : public function 
{
public:
  FunctionDropIndex(const signature& sig)
    :
    function(sig, FunctionConsts::FN_DROP_INDEX_1)
  {
  }

  bool requires_dyn_ctx() const { return true; }

 expr_script_kind_t getUpdateType() const { return UPDATE_EXPR; }

  DEFAULT_UNARY_CODEGEN(DropIndexIterator);
};


/******************************************************************************
  refresh-index($indexName as xs:QName) as pul()

  This is an updating function. During normal runtime (see RefreshIndexIterator),
  it checks that index exists (in the dynamic context) and generates an update
  primitive. During applyUpdates(), it clears the index of its contents and then
  rebuilds the index the same way as the create-index() function.
********************************************************************************/
class FunctionRefreshIndex : public function 
{
public:
  FunctionRefreshIndex(const signature& sig) 
    :
    function(sig, FunctionConsts::FN_REFRESH_INDEX_1)
  {
  }

  bool requires_dyn_ctx() const { return true; }

 expr_script_kind_t getUpdateType() const { return UPDATE_EXPR; }

  DEFAULT_UNARY_CODEGEN(RefreshIndexIterator);
};


/***************************************************************************//**
  This is a helper function for creating index entries (it constitutes the 
  return expr of the flwor expr that populates the index; see the create-index
  function above). Its first arg is a reference to the domain var, and its next
  N args evaluate and return the key expressions for each domain node. The 
  function simply evaluates each of its children in turn, and returns the
  resulting items one-by-one. This is similar to fn:concatenate, but contrary
  to concatenate, this function also returns NULL items.

  index-entry-builder($domainNode as node(),
                      $key1  as anyAtomic?,
                      ...,
                      $keyN  as anyAtomic?) as item()*
********************************************************************************/
class FunctionIndexEntryBuilder : public function 
{
public:
  FunctionIndexEntryBuilder(const signature& sig) : function(sig) { }

  bool requires_dyn_ctx() const { return true; }

  DEFAULT_NARY_CODEGEN(IndexEntryBuilderIterator);
};


/***************************************************************************//**
  probe-index-point($indexName as xs:QName,
                    $key1      as anyAtomic?,
                    ...,
                    $keyN      as anyAtomic?) as node()*
********************************************************************************/
class FunctionProbeIndexPoint : public function 
{
public:
  FunctionProbeIndexPoint(const signature& sig) 
    :
    function(sig, FunctionConsts::FN_INDEX_PROBE_POINT_N)
  {
  }

  bool requires_dyn_ctx() const { return true; }

  DEFAULT_NARY_CODEGEN(IndexPointProbeIterator);
};


/*******************************************************************************
  probe-index-range($indexName               as xs:QName,
                    $range1LowerBound         as anyAtomic?,
                    $range1UpperBound         as anyAtomic?,
                    $range1HaveLowerBound     as boolean?,
                    $range1HaveupperBound     as boolean?,
                    $range1LowerBoundIncluded as boolean?,
                    $range1upperBoundIncluded as boolean?,
                    ....,
                    $rangeNLowerBound         as anyAtomic?,
                    $rangeNUpperBound         as anyAtomic?,
                    $rangeNHaveLowerBound     as boolean?,
                    $rangeNHaveupperBound     as boolean?,
                    $rangeNLowerBoundIncluded as boolean?,
                    $rangeNupperBoundIncluded as boolean?) as node()*
********************************************************************************/
class FunctionProbeIndexRange : public function 
{
public:
  FunctionProbeIndexRange(const signature& sig) 
    :
    function(sig, FunctionConsts::FN_INDEX_PROBE_RANGE_N)
  {
  }

  bool requires_dyn_ctx() const { return true; }

  DEFAULT_NARY_CODEGEN(IndexRangeProbeIterator);
};

class FunctionIsDeclaredIndex : public function 
{
public:
  FunctionIsDeclaredIndex(const signature& sig) 
    :
    function(sig)
  {
  }

  bool requires_dyn_ctx() const { return false; }

  DEFAULT_NARY_CODEGEN(IsDeclaredIndexIterator);
};

class FunctionDeclaredIndexes : public function 
{
public:
  FunctionDeclaredIndexes(const signature& sig) 
    :
    function(sig)
  {
  }

  bool requires_dyn_ctx() const { return false; }

  DEFAULT_NARY_CODEGEN(DeclaredIndexesIterator);
};

class FunctionIsAvailableIndex : public function 
{
public:
  FunctionIsAvailableIndex(const signature& sig) 
    :
    function(sig)
  {
  }

  bool requires_dyn_ctx() const { return true; }

  DEFAULT_NARY_CODEGEN(IsAvailableIndexIterator);
};

class FunctionAvailableIndexes : public function 
{
public:
  FunctionAvailableIndexes(const signature& sig) 
    :
    function(sig)
  {
  }

  bool requires_dyn_ctx() const { return false; }

  DEFAULT_NARY_CODEGEN(AvailableIndexesIterator);
};

void populateContext_Index(static_context* sctx)
{
  DECL(sctx, FunctionCreateInternalIndex,
       (createQName(ZORBA_DDL_FN_NS, "fn-zorba-ddl", "create-internal-index"),
        GENV_TYPESYSTEM.QNAME_TYPE_ONE,
        GENV_TYPESYSTEM.EMPTY_TYPE));

  DECL(sctx, FunctionCreateIndex,
       (createQName(ZORBA_DDL_FN_NS, "fn-zorba-ddl", "create-index"),
        GENV_TYPESYSTEM.QNAME_TYPE_ONE,
        GENV_TYPESYSTEM.EMPTY_TYPE));

  DECL(sctx, FunctionRefreshIndex,
       (createQName(ZORBA_DDL_FN_NS, "fn-zorba-ddl", "refresh-index"),
        GENV_TYPESYSTEM.QNAME_TYPE_ONE,
        GENV_TYPESYSTEM.EMPTY_TYPE));

  DECL(sctx, FunctionDropIndex,
       (createQName(ZORBA_DDL_FN_NS, "fn-zorba-ddl", "drop-index"),
        GENV_TYPESYSTEM.QNAME_TYPE_ONE,
        GENV_TYPESYSTEM.EMPTY_TYPE));

  DECL(sctx, FunctionIndexEntryBuilder,
       (createQName(ZORBA_DDL_FN_NS, "fn-zorba-ddl", "index-entry-builder"),
        GENV_TYPESYSTEM.ANY_NODE_TYPE_ONE,
        true,
        GENV_TYPESYSTEM.ITEM_TYPE_STAR));

  DECL(sctx, FunctionProbeIndexPoint,
       (createQName(ZORBA_DYNAMICCONTEXT_FN_NS,
                    "fn-zorba-dynamiccontext", "probe-index-point"),
        GENV_TYPESYSTEM.QNAME_TYPE_ONE,
        true,
        GENV_TYPESYSTEM.ANY_NODE_TYPE_STAR));

  DECL(sctx, FunctionProbeIndexRange,
       (createQName(ZORBA_DYNAMICCONTEXT_FN_NS,
                    "fn-zorba-dynamiccontext", "probe-index-range"),
        GENV_TYPESYSTEM.QNAME_TYPE_ONE,
        true,
        GENV_TYPESYSTEM.ANY_NODE_TYPE_STAR));

  DECL(sctx, FunctionIsDeclaredIndex,
       (createQName(ZORBA_STATICCONTEXT_FN_NS,
                    "fn-zorba-staticcontext", "is-declared-index"),
        GENV_TYPESYSTEM.QNAME_TYPE_ONE,
        GENV_TYPESYSTEM.BOOLEAN_TYPE_ONE));

  DECL(sctx, FunctionDeclaredIndexes,
       (createQName(ZORBA_STATICCONTEXT_FN_NS,
                    "fn-zorba-staticcontext", "declared-indexes"),
        GENV_TYPESYSTEM.QNAME_TYPE_STAR));

  DECL(sctx, FunctionIsAvailableIndex,
       (createQName(ZORBA_DYNAMICCONTEXT_FN_NS,
                    "fn-zorba-dynamiccontext", "is-available-index"),
        GENV_TYPESYSTEM.QNAME_TYPE_ONE,
        GENV_TYPESYSTEM.BOOLEAN_TYPE_ONE));

  DECL(sctx, FunctionAvailableIndexes,
       (createQName(ZORBA_DYNAMICCONTEXT_FN_NS,
                    "fn-zorba-dynamiccontext", "available-indexes"),
        GENV_TYPESYSTEM.QNAME_TYPE_STAR));
}


}
/* vim:set ts=2 sw=2: */
