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
#include "common/common.h"

#include "node_test.h"

namespace zorba
{

SERIALIZABLE_CLASS_VERSIONS(NodeNameTest)
END_SERIALIZABLE_CLASS_VERSIONS(NodeNameTest)



NodeNameTest::NodeNameTest(
    const xqpStringStore_t& uri,
    const xqpStringStore_t& local)
  :
  m_uri(uri),
  m_local(local)
{
}


NodeNameTest::NodeNameTest(const store::Item_t& qname)
  :
  m_uri(qname->getNamespace()),
  m_local(qname->getLocalName())
{
}


bool NodeNameTest::operator==(const NodeNameTest& other) const
{
  if (m_uri == other.m_uri ||
      (m_uri != NULL && other.m_uri != NULL &&  other.m_uri->byteEqual(m_uri)))
  {
    if (m_local == other.m_local ||
        (m_local != NULL && other.m_local != NULL &&  other.m_local->byteEqual(m_local)))
      return true;
  }

  return false;
}


bool NodeNameTest::is_subname_of(const NodeNameTest& other) const
{
  return ((other.m_uri == NULL || other.m_uri->byteEqual(m_uri))
          &&
          (other.m_local == NULL || other.m_local->byteEqual(m_local)));
}


bool NodeNameTest::matches(const store::Item* qname) const
{
  return ((m_uri == NULL || m_uri->byteEqual(qname->getNamespace()))
          &&
          (m_local == NULL || m_local->byteEqual(qname->getLocalName())));
}


bool NodeNameTest::matches(
    const xqpStringStore* lname,
    const xqpStringStore* ns) const
{
  return ((m_uri == NULL || m_uri->byteEqual(ns))
          &&
          (m_local == NULL || m_local->byteEqual(lname)));
}

}

/* vim:set ts=2 sw=2: */
