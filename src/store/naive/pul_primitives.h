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
#ifndef ZORBA_SIMPLE_STORE_PUL_PRIMITIVES
#define ZORBA_SIMPLE_STORE_PUL_PRIMITIVES

#include <vector>

#include "store/naive/shared_types.h"
#include "store/naive/text_node_content.h"
#include "store/api/update_consts.h"
#include "store/api/collection.h"
#include "store/api/iterator.h"

#include "store/api/index.h" // for index spec obj
#include "store/api/ic.h" // for index spec obj


namespace zorba { namespace simplestore {


class PULImpl;
class CollectionPul;
class XmlNode;
class InternalNode;


/*******************************************************************************
  A class to store the type-related info of a node whose type is removed by the
  upd:removeType() primitive. Used to undo the effects of upd:removeType() in
  case of errors.
********************************************************************************/
class NodeTypeInfo
{
public:
  XmlNode           * theNode;

  store::Item_t       theTypeName;
  TextNodeContent     theTextContent;
  uint32_t            theFlags;
  uint32_t            theChildFlags;

  NodeTypeInfo() : theFlags(0), theChildFlags(0) { }

  NodeTypeInfo(const NodeTypeInfo& other);

  ~NodeTypeInfo();

  void transfer(NodeTypeInfo& other);

  NodeTypeInfo& operator=(const NodeTypeInfo&);
};


/*******************************************************************************

********************************************************************************/
typedef std::vector<NodeTypeInfo> TypeUndoList;


/*******************************************************************************
  Base class for all update primitives

  thePul           : The PUL where this primitive belongs to.

  theCollectionPul : XQUF and collection primitives are grouped into sub-puls
                     by the collection that gets affected by the primitive.
                     For such primitives, theCollectionPul points to the sub-pul
                     where the primitive belongs to.

  theTarget        : The target node of this primitive. All XQUF and some
                     collection primitives have a target node. 

  theIsApplied     : Set to true by the apply() method of each primitive to
                     inidicate that the primitive is at least partially applied,
                     and as a result, undo should be performed for this primitive
                     in case of errors.

  theRemoveType    : Set to true if upd:removeType() must be applied to the target
                     node.

  theTypeUndoList  : Stores the type-related info of the target node and its
                     ancestors, if upd:removeType has been applied to these nodes.
                     Used to undo the effects of upd:removeType() in case of
                     errors.
********************************************************************************/
class UpdatePrimitive
{
  friend class PULImpl;
  friend class CollectionPul;
  friend class XmlNode;
  friend class InternalNode;
  friend class PULPrimitiveFactory;

protected:
  PULImpl        * thePul;
  CollectionPul  * theCollectionPul;

  store::Item_t    theTarget;
  bool             theIsApplied;
  bool             theRemoveType;
  TypeUndoList     theTypeUndoList;

protected:
  UpdatePrimitive(PULImpl* pul, store::Item_t& target);

  UpdatePrimitive(CollectionPul* pul, store::Item_t& target);

  UpdatePrimitive(PULImpl* pul);

  UpdatePrimitive(CollectionPul* pul);

public:
  virtual ~UpdatePrimitive();

  void addNodeForValidation(zorba::store::Item* node);

  virtual store::UpdateConsts::UpdPrimKind getKind() const = 0;

  virtual void apply() = 0;
  virtual void undo() = 0;

  virtual void check() {}

  bool isApplied() const { return theIsApplied; }
};


/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
//  XQUF Primitives                                                            //
//                                                                             //
/////////////////////////////////////////////////////////////////////////////////


/*******************************************************************************

********************************************************************************/
class UpdDelete : public UpdatePrimitive
{
  friend class PULImpl;
  friend class CollectionPul;
  friend class XmlNode;
  friend class InternalNode;
  friend class PULPrimitiveFactory;

protected:
  InternalNode   * theParent;
  ulong            thePos;
  store::Item_t    theRightSibling;
  zstring          theLeftContent;

protected:
  UpdDelete(CollectionPul* pul, store::Item_t& target)
    :
    UpdatePrimitive(pul, target)
  {
  }

public:
  store::UpdateConsts::UpdPrimKind getKind() const
  {
    return store::UpdateConsts::UP_DELETE; 
  }

  void apply();
  void undo();
};


/*******************************************************************************
  This class represents the following primitives:

  upd:insertInto, upd:insertIntoFirst, upd:insertIntoLast, upd:insertBefore, and
  upd:insertAfter.

  theTarget      : Inherited from UpdatePrimitive: The parent node under which
                   the new nodes will be inserted.

  theKind        : The kind of primitive represented by "this".

  theNewChildren : The list of new nodes to insert as children of theTarget.
                   The list does not contain any consecutive text nodes (see
                   UpdInsertChildren constructor).

  theSibling     : If theKind is InsertBefore or InsertAfter, theSibling is the
                   child node of theTarget next to which the new nodes will be
                   inserted. 

  theNumApplied  : The number of new children that were actually inserted during
                   the apply() method. It is needed by the undo() method to
                   handle the case where an error occured in the middle of the
                   apply() method, before all the new children were inserted.

  theMergedNode  : If either the 1st or the last node in theNewChildren is a
                   text node that needs to be merged with an existing text child
                   N of theTarget, then N is first saved in theMergedNode, then
                   removed as a child of theTarget, and finally its text content
                   is merged with the content of the 1st or last node in  
                   theNewChildren. If the apply must be undone, the undo()
                   method will restore N in its original position.
********************************************************************************/
class UpdInsertChildren : public UpdatePrimitive
{
  friend class PULImpl;
  friend class CollectionPul;
  friend class XmlNode;
  friend class InternalNode;
  friend class PULPrimitiveFactory;

protected:
  store::UpdateConsts::UpdPrimKind theKind;
  std::vector<store::Item_t>       theNewChildren;
  store::Item_t                    theSibling;

  long                             theNumApplied;
  store::Item_t                    theMergedNode;

  UpdInsertChildren(
        CollectionPul* pul,
        store::UpdateConsts::UpdPrimKind kind,
        store::Item_t& target,
        store::Item_t& sibling,
        std::vector<store::Item_t>& children);

public:
  store::UpdateConsts::UpdPrimKind getKind() const { return theKind; }

  void apply();
  void undo();
};


/*******************************************************************************
  theNewAtrs :     The list of new attrs to insert as attributes of theTarget.

  theNumApplied  : The number of new children that were actually inserted during
                   the apply() method. It is needed by the undo() method to
                   handle the case where an error occured in the middle of the
                   apply() method, before all the new children were inserted.

  theNewBindings :
********************************************************************************/
class UpdInsertAttributes : public UpdatePrimitive
{
  friend class PULImpl;
  friend class ElementNode;
  friend class PULPrimitiveFactory;

protected:
  std::vector<store::Item_t>  theNewAttrs;

  ulong                       theNumApplied;
  std::vector<store::Item*>   theNewBindings;

  UpdInsertAttributes(
        CollectionPul* pul,
        store::Item_t& target,
        std::vector<store::Item_t>&  attrs);

public:
  store::UpdateConsts::UpdPrimKind getKind() const
  {
    return store::UpdateConsts::UP_INSERT_ATTRIBUTES;
  }

  void apply();
  void undo();
  void check();
};


/*******************************************************************************

********************************************************************************/
class UpdReplaceAttribute : public UpdatePrimitive
{
  friend class PULImpl;
  friend class CollectionPul;
  friend class ElementNode;
  friend class PULPrimitiveFactory;

protected:
  store::Item_t               theAttr;
  std::vector<store::Item_t>  theNewAttrs;

  ulong                       theNumApplied;
  std::vector<store::Item*>   theNewBindings;
  ulong                       thePos;

  UpdReplaceAttribute(
        CollectionPul* pul,
        store::Item_t& target,
        store::Item_t& attr,
        std::vector<store::Item_t>& newAttrs);

public:
  store::UpdateConsts::UpdPrimKind getKind() const
  {
    return store::UpdateConsts::UP_REPLACE_ATTRIBUTE;
  }

  void apply();
  void undo();
  void check();
};


/*******************************************************************************

********************************************************************************/
class UpdReplaceChild : public UpdatePrimitive
{
  friend class PULImpl;
  friend class CollectionPul;
  friend class XmlNode;
  friend class InternalNode;
  friend class PULPrimitiveFactory;

protected:
  store::Item_t               theChild;
  std::vector<store::Item_t>  theNewChildren;

  ulong                       theNumApplied;
  store::Item_t               theLeftMergedNode;
  store::Item_t               theRightMergedNode;
  bool                        theIsTyped;

  UpdReplaceChild(
        CollectionPul* pul,
        store::Item_t& target,
        store::Item_t& child,
        std::vector<store::Item_t>& newChildren);

public:
  store::UpdateConsts::UpdPrimKind getKind() const
  {
    return store::UpdateConsts::UP_REPLACE_CHILD; 
  }

  void apply();
  void undo();
};


/*******************************************************************************
  Replace all the children of a target element node with a new child that is
  a text node.
********************************************************************************/
class UpdReplaceElemContent : public UpdatePrimitive
{
  friend class PULImpl;
  friend class CollectionPul;
  friend class ElementNode;
  friend class PULPrimitiveFactory;

protected:
  store::Item_t         theNewChild;

  std::vector<XmlNode*> theOldChildren;
  bool                  theIsTyped;

  UpdReplaceElemContent(
        CollectionPul* pul,
        store::Item_t& target,
        store::Item_t& newChild)
    :
    UpdatePrimitive(pul, target),
    theIsTyped(false)
  {
    theNewChild.transfer(newChild);
  }

public:
  store::UpdateConsts::UpdPrimKind getKind() const
  {
    return store::UpdateConsts::UP_REPLACE_CONTENT; 
  }

  void apply();
  void undo();
};


/*******************************************************************************
  theNewBinding :  Whether the update resulted in a new ns bindings added
                   among the ns bindings of the target. If true, this new
                   binding must be removed during undo.
********************************************************************************/
class UpdRenameElem : public UpdatePrimitive
{
  friend class PULImpl;
  friend class ElementNode;
  friend class PULPrimitiveFactory;

protected:
  store::Item_t   theNewName;

  store::Item_t   theOldName;
  bool            theNewBinding;
  bool            theRestoreParentType;

  UpdRenameElem(
        CollectionPul* pul,
        store::Item_t& target,
        store::Item_t& newName) 
    :
    UpdatePrimitive(pul, target),
    theNewBinding(false),
    theRestoreParentType(false)
  {
    theNewName.transfer(newName);
  }

public:
  store::UpdateConsts::UpdPrimKind getKind() const
  { 
    return store::UpdateConsts::UP_RENAME_ELEM; 
  }

  void apply();
  void undo();
};


/*******************************************************************************
  A target node cannot have more than 1 UpdReplaceAttrValue primitives
********************************************************************************/
class UpdReplaceAttrValue : public UpdatePrimitive
{
  friend class PULImpl;
  friend class AttributeNode;
  friend class PULPrimitiveFactory;

protected:
  zstring         theNewValue;

  store::Item_t   theOldValue;

  UpdReplaceAttrValue(
        CollectionPul* pul,
        store::Item_t& target,
        zstring& newValue)
    :
    UpdatePrimitive(pul, target)
  {
    theNewValue.take(newValue);
  }

public:
  store::UpdateConsts::UpdPrimKind getKind() const
  {
    return store::UpdateConsts::UP_REPLACE_ATTR_VALUE;
  }

  void apply();
  void undo();
};


/*******************************************************************************
  theOldValue   : Renaming an attribute also removes its type and removing the
                  type implies that the current typed value V of the attribute
                  must be replaced with a new value V' that is equal to the
                  string value of V as an instance of untyped atomic. So, we must
                  save V here so that we can perform undo.
  theNewBinding : Whether the update resulted in a new ns bindings added among
                  the ns bindings of the parent of the target. If true, this
                  new binding must be removed during undo.
********************************************************************************/
class UpdRenameAttr : public UpdatePrimitive
{
  friend class PULImpl;
  friend class AttributeNode;
  friend class PULPrimitiveFactory;

protected:
  store::Item_t   theNewName;

  store::Item_t   theOldValue;
  store::Item_t   theOldName;
  bool            theNewBinding;

  UpdRenameAttr(
        CollectionPul* pul,
        store::Item_t& target,
        store::Item_t& newName)
    :
    UpdatePrimitive(pul, target),
    theNewBinding(false)
  {
    theNewName.transfer(newName);
  }

public:
  store::UpdateConsts::UpdPrimKind getKind() const
  {
    return store::UpdateConsts::UP_RENAME_ATTR; 
  }

  void apply();
  void undo();
  void check();
};


/*******************************************************************************

********************************************************************************/
class UpdReplaceTextValue : public UpdatePrimitive
{
  friend class PULImpl;
  friend class TextNode;
  friend class PULPrimitiveFactory;

protected:
  zstring            theNewContent;

  store::Item_t      theOldNode;
  ulong              theOldPos;
  TextNodeContent    theOldContent;
  bool               theIsTyped;

  UpdReplaceTextValue(
        CollectionPul* pul,
        store::Item_t& target,
        zstring& newValue) 
    :
    UpdatePrimitive(pul, target),
    theIsTyped(false)
  {
    theNewContent.take(newValue);
  }

public:
  ~UpdReplaceTextValue();

  store::UpdateConsts::UpdPrimKind getKind() const
  {
    return store::UpdateConsts::UP_REPLACE_TEXT_VALUE;
  }

  void apply();
  void undo();
};


/*******************************************************************************

********************************************************************************/
class UpdReplacePiValue : public UpdatePrimitive
{
  friend class PULImpl;
  friend class PiNode;
  friend class PULPrimitiveFactory;

protected:
  zstring   theNewValue;
  zstring   theOldValue;

  UpdReplacePiValue(
        CollectionPul* pul,
        store::Item_t& target,
        zstring& newValue)
    :
    UpdatePrimitive(pul, target)
  {
    theNewValue.take(newValue);
  }

public:
  store::UpdateConsts::UpdPrimKind getKind() const
  {
    return store::UpdateConsts::UP_REPLACE_PI_VALUE;
  }

  void apply();
  void undo();
};


/*******************************************************************************

********************************************************************************/
class UpdRenamePi : public UpdatePrimitive
{
  friend class PULImpl;
  friend class PiNode;
  friend class PULPrimitiveFactory;

protected:
  zstring   theNewName;
  zstring   theOldName;

  UpdRenamePi(
        CollectionPul* pul,
        store::Item_t& target,
        zstring& newName) 
    :
    UpdatePrimitive(pul, target)
  {
    theNewName.take(newName);
  }

public:
  store::UpdateConsts::UpdPrimKind getKind() const
  {
    return store::UpdateConsts::UP_RENAME_PI; 
  }

  void apply();
  void undo();
};


/*******************************************************************************

********************************************************************************/
class UpdReplaceCommentValue : public UpdatePrimitive
{
  friend class PULImpl;
  friend class CommentNode;
  friend class PULPrimitiveFactory;

protected:
  zstring   theNewValue;
  zstring   theOldValue;

  UpdReplaceCommentValue(
        CollectionPul* pul,
        store::Item_t& target,
        zstring& newValue)
    :
    UpdatePrimitive(pul, target)
  {
    theNewValue.take(newValue);
  }

public:
  store::UpdateConsts::UpdPrimKind getKind() const
  {
    return store::UpdateConsts::UP_REPLACE_COMMENT_VALUE;
  }

  void apply();
  void undo();
};


/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
//  Revalidation Primitives                                                    //
//                                                                             //
/////////////////////////////////////////////////////////////////////////////////


/*******************************************************************************
  theHaveValue      : True if the target element node has a typed value. The only
                      case when an element node does not have a typed value is
                      when the type of the node is complex with complex content,
                      but with no mixed content allowed.
  theHaveEmptyValue : True if the target element node has a complex type with
                      empty content.
  theHaveTypedValue : True if the target element node has a typed value whose
                      type is not untypedAtomic. Note: This can happen only if
                      the type of the target element node has simple content.
  theHaveListValue  : True if theHaveTypedValue is true, and the type of the
                      typed value is a list type. 
********************************************************************************/
class UpdSetElementType : public UpdatePrimitive
{
  friend class PULImpl;
  friend class PULPrimitiveFactory;

protected:
  store::Item_t            theTypeName;
  store::Item_t            theTypedValue;
  bool                     theHaveValue;
  bool                     theHaveEmptyValue;
  bool                     theHaveTypedValue;
  bool                     theHaveListValue;
  bool                     theIsInSubstitutionGroup;

  UpdSetElementType(
        PULImpl*       pul,
        store::Item_t& target,
        store::Item_t& typeName,
        store::Item_t& typedValue,
        bool           haveValue,
        bool           haveEmptyValue,
        bool           haveTypedValue,
        bool           haveListValue,
        bool           isInSubstitutionGroup)
    :
    UpdatePrimitive(pul, target),
    theHaveValue(haveValue),
    theHaveEmptyValue(haveEmptyValue),
    theHaveTypedValue(haveTypedValue),
    theHaveListValue(haveListValue),
    theIsInSubstitutionGroup(isInSubstitutionGroup)
  {
    theTypeName.transfer(typeName);
    theTypedValue.transfer(typedValue);
  }

public:
  store::UpdateConsts::UpdPrimKind getKind() const
  {
    return store::UpdateConsts::UP_SET_ELEMENT_TYPE; 
  }

  void apply();
  void undo() {}
};


/*******************************************************************************

********************************************************************************/
class UpdSetAttributeType : public UpdatePrimitive
{
  friend class PULImpl;
  friend class PULPrimitiveFactory;

protected:
  store::Item_t            theTypeName;
  store::Item_t            theTypedValue;
  bool                     theHaveListValue;

  UpdSetAttributeType(
        PULImpl*       pul,
        store::Item_t& target,
        store::Item_t& typeName,
        store::Item_t& typedValue,
        bool           haveListValue)
    :
    UpdatePrimitive(pul, target),
    theHaveListValue(haveListValue)
  {
    theTypeName.transfer(typeName);
    theTypedValue.transfer(typedValue);
  }

public:
  store::UpdateConsts::UpdPrimKind getKind() const
  {
    return store::UpdateConsts::UP_SET_ATTRIBUTE_TYPE; 
  }

  void apply();
  void undo() {}
};


/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
//  fn:put primitive                                                           //
//                                                                             //
/////////////////////////////////////////////////////////////////////////////////


/*******************************************************************************

********************************************************************************/
class UpdPut : public UpdatePrimitive
{
  friend class PULImpl;
  friend class PULPrimitiveFactory;

protected:
  store::Item_t theTargetUri;

  store::Item_t theOldDocument;

  UpdPut(PULImpl* pul, store::Item_t& target, store::Item_t& uri)
    :
    UpdatePrimitive(pul, target)
  {
    theTargetUri.transfer(uri);
  }

public:
  store::UpdateConsts::UpdPrimKind getKind() const
  {
    return store::UpdateConsts::UP_PUT;
  }

  void apply();
  void undo();
};


/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
//  Collection Primitives                                                      //
//                                                                             //
/////////////////////////////////////////////////////////////////////////////////


/*******************************************************************************
  Base class for other collection primitives
********************************************************************************/ 
class UpdCollection : public UpdatePrimitive
{
  friend class PULPrimitiveFactory;

protected:
  store::Item_t               theName;
  std::vector<store::Item_t>  theNodes;

  UpdCollection(
        CollectionPul* pul,
        store::Item_t& name)
    :
    UpdatePrimitive(pul),
    theName(name)
  {
  }

  UpdCollection(
        CollectionPul* pul,
        store::Item_t& name,
        std::vector<store::Item_t>& nodes);

  UpdCollection(
        CollectionPul* pul,
        store::Item_t& target,
        store::Item_t& name,
        std::vector<store::Item_t>& nodes);

public:
  const store::Item* getName() const { return theName.getp(); }

  ulong numNodes() const { return (ulong)theNodes.size(); }

  store::Item* getNode(ulong i) const { return theNodes[i].getp(); }
};


/*******************************************************************************

********************************************************************************/
class UpdCreateCollection : public UpdCollection
{
  friend class PULPrimitiveFactory;

protected:
  UpdCreateCollection(
        CollectionPul* pul,
        store::Item_t& name)
    :
    UpdCollection(pul, name)
  {
  }

public:
  store::UpdateConsts::UpdPrimKind getKind() const
  {
    return store::UpdateConsts::UP_CREATE_COLLECTION;
  }

  void apply();
  void undo();
};


/*******************************************************************************

********************************************************************************/
class UpdDeleteCollection : public UpdCollection
{
  friend class PULPrimitiveFactory;

protected:
  store::Collection_t theCollection; // only used for undo

  UpdDeleteCollection(
        CollectionPul* pul,
        store::Item_t& name)
    :
    UpdCollection(pul, name)
  {
  }

public:
  store::UpdateConsts::UpdPrimKind getKind() const
  {
    return store::UpdateConsts::UP_DELETE_COLLECTION;
  }

  void apply();
  void undo();
};


/*******************************************************************************

********************************************************************************/
class UpdInsertIntoCollection : public UpdCollection
{
  friend class PULPrimitiveFactory;

protected:
  ulong theNumApplied;

  UpdInsertIntoCollection(
        CollectionPul* pul,
        store::Item_t& name, 
        std::vector<store::Item_t>& nodes)
      :
    UpdCollection(pul, name, nodes),
    theNumApplied(0)
  {
  }

public:
  store::UpdateConsts::UpdPrimKind getKind() const
  {
    return store::UpdateConsts::UP_INSERT_INTO_COLLECTION;
  }

  void apply();
  void undo();
};


/*******************************************************************************

********************************************************************************/
class UpdInsertFirstIntoCollection : public  UpdCollection
{
  friend class PULPrimitiveFactory;

protected:
  ulong theNumApplied;

  UpdInsertFirstIntoCollection(
      CollectionPul* pul,
      store::Item_t& name,
      std::vector<store::Item_t>& nodes)
    :
    UpdCollection(pul, name, nodes),
    theNumApplied(0)
  {
  }

public:
  store::UpdateConsts::UpdPrimKind getKind() const
  { 
    return store::UpdateConsts::UP_INSERT_FIRST_INTO_COLLECTION;
  }

  void apply();
  void undo();
};


/*******************************************************************************

********************************************************************************/
class UpdInsertLastIntoCollection : public  UpdCollection
{
  friend class PULPrimitiveFactory;

protected:
  ulong theNumApplied;

  UpdInsertLastIntoCollection(
        CollectionPul* pul,
        store::Item_t& name,
        std::vector<store::Item_t>& nodes)
    :
    UpdCollection(pul, name, nodes),
    theNumApplied(0)
  {
  }

public:
  store::UpdateConsts::UpdPrimKind getKind() const
  { 
    return store::UpdateConsts::UP_INSERT_LAST_INTO_COLLECTION;
  }

  void apply();
  void undo();
};


/*******************************************************************************

********************************************************************************/
class UpdInsertBeforeIntoCollection : public  UpdCollection
{
  friend class PULPrimitiveFactory;

protected:
  store::Item_t theFirstNode;
  ulong         theFirstPos;

  UpdInsertBeforeIntoCollection(
        CollectionPul* pul,
        store::Item_t& name,
        store::Item_t& target,
        std::vector<store::Item_t>& nodes)
    :
    UpdCollection(pul, target, name, nodes)
  {
  }

public:
  store::UpdateConsts::UpdPrimKind getKind() const
  { 
    return store::UpdateConsts::UP_INSERT_BEFORE_INTO_COLLECTION;
  }

  void apply();
  void undo();
};


/*******************************************************************************

********************************************************************************/
class UpdInsertAfterIntoCollection : public  UpdCollection
{
  friend class PULPrimitiveFactory;

protected:
  store::Item_t theFirstNode;
  ulong         theFirstPos;

  UpdInsertAfterIntoCollection(
        CollectionPul* pul,
        store::Item_t& name,
        store::Item_t& target,
        std::vector<store::Item_t>& nodes)
    :
    UpdCollection(pul, target, name, nodes)
  {
  }

public:
  store::UpdateConsts::UpdPrimKind getKind() const
  { 
    return store::UpdateConsts::UP_INSERT_AFTER_INTO_COLLECTION;
  }

  void apply();
  void undo();
};


/*******************************************************************************

********************************************************************************/
class UpdDeleteNodesFromCollection: public  UpdCollection
{
  friend class PULPrimitiveFactory;

protected:
  bool               theIsLast;

  ulong              theNumApplied;
  std::vector<bool>  theFound;
  std::vector<ulong> thePositions;

  UpdDeleteNodesFromCollection(
        CollectionPul* pul,
        store::Item_t& name,
        std::vector<store::Item_t>& nodes,
        bool isLast)
    :
    UpdCollection(pul, name, nodes),
    theIsLast(isLast),
    theNumApplied(0)
  {
  }

public:
  store::UpdateConsts::UpdPrimKind getKind() const
  { 
    return store::UpdateConsts::UP_REMOVE_FROM_COLLECTION;
  }

  void apply();
  void undo();
};


/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
//  Index Primitives                                                           //
//                                                                             //
/////////////////////////////////////////////////////////////////////////////////


/*******************************************************************************

********************************************************************************/
class UpdCreateIndex : public  UpdatePrimitive
{
  friend class PULImpl;
  friend class PULPrimitiveFactory;

protected:
  store::Item_t              theQName;
  store::IndexSpecification  theSpec;
  store::Iterator_t          theSourceIter;

  store::Index_t             theIndex;

  UpdCreateIndex(
      PULImpl* pul,
      const store::Item_t& qname,
      const store::IndexSpecification& spec,
      store::Iterator* sourceIter);

public:
  store::UpdateConsts::UpdPrimKind getKind() const
  { 
    return store::UpdateConsts::UP_CREATE_INDEX;
  }

  const store::Item* getName() const { return theQName.getp(); }

  void apply();
  void undo();
};


/*******************************************************************************

********************************************************************************/
class UpdDeleteIndex : public  UpdatePrimitive
{
  friend class PULImpl;
  friend class PULPrimitiveFactory;

protected:
  const store::Item_t  theQName;

  store::Index_t       theIndex;

  UpdDeleteIndex(PULImpl* pul, const store::Item_t& qname);

public:
  store::UpdateConsts::UpdPrimKind getKind() const
  { 
    return store::UpdateConsts::UP_DROP_INDEX;
  }

  void apply();
  void undo();
};


/*******************************************************************************

********************************************************************************/
class UpdRefreshIndex : public  UpdatePrimitive
{
  friend class PULImpl;
  friend class PULPrimitiveFactory;

protected:
  const store::Item_t  theQName;
  store::Iterator_t    theSourceIter;

  store::Index_t       theIndex;

  UpdRefreshIndex(
        PULImpl* pul,
        const store::Item_t& qname,
        store::Iterator* sourceIter);

public:
  ~UpdRefreshIndex();

  store::UpdateConsts::UpdPrimKind getKind() const
  { 
    return store::UpdateConsts::UP_REBUILD_INDEX;
  }

  void apply();
  void undo();
};


/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
//  Integrity Constraints Primitives                                           //
//                                                                             //
/////////////////////////////////////////////////////////////////////////////////


/*******************************************************************************

********************************************************************************/
class UpdActivateIC : public  UpdatePrimitive
{
  friend class PULImpl;
  friend class PULPrimitiveFactory;

protected:
  const store::Item_t  theQName;
  const store::Item_t  theCollectionName;

  UpdActivateIC(
        PULImpl* pul,
        const store::Item_t& aQName,
        const store::Item_t& aCollectionName);

public:
  ~UpdActivateIC();

  store::UpdateConsts::UpdPrimKind getKind() const
  { 
    return store::UpdateConsts::UP_ACTIVATE_IC;
  }

  void apply();
  void undo();
};


/*******************************************************************************

********************************************************************************/
class UpdActivateForeignKeyIC : public  UpdatePrimitive
{
  friend class PULImpl;
  friend class PULPrimitiveFactory;

protected:
  const store::Item_t  theQName;
  const store::Item_t  theFromCollectionName;
  const store::Item_t  theToCollectionName;

  UpdActivateForeignKeyIC(
        PULImpl* pul,
        const store::Item_t& qQName,
        const store::Item_t& aFromCollectionName,
        const store::Item_t& aToCollectionName);

public:
  ~UpdActivateForeignKeyIC();

  store::UpdateConsts::UpdPrimKind getKind() const
  { 
    return store::UpdateConsts::UP_ACTIVATE_FOREIGN_KEY_IC;
  }

  void apply();
  void undo();
};


/*******************************************************************************

********************************************************************************/
class UpdDeActivateIC : public  UpdatePrimitive
{
  friend class PULImpl;
  friend class PULPrimitiveFactory;

protected:
  const store::Item_t  theQName;
  store::Item_t        theFromCollectionName;
  store::Item_t        theToCollectionName;
  store::IC::ICKind    theICKind;

  UpdDeActivateIC(
        PULImpl* pul,
        const store::Item_t& qname);

public:
  ~UpdDeActivateIC();

  store::UpdateConsts::UpdPrimKind getKind() const
  { 
    return store::UpdateConsts::UP_DEACTIVATE_IC;
  }

  void apply();
  void undo();
};
}
}
#endif

/*
 * Local variables:
 * mode: c++
 * End:
 */
