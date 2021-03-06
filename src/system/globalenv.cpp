/*
 * Copyright 2006-2016 zorba.io
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
#include "stdafx.h"

#include "common/common.h"

#ifndef ZORBA_NO_ICU
# include <unicode/uclean.h>
# include <unicode/utypes.h>
# include <unicode/udata.h>
#endif /* ZORBA_NO_ICU */

#ifdef ZORBA_WITH_BIG_INTEGER
# include "zorbatypes/m_apm.h"
#endif /* ZORBA_WITH_BIG_INTEGER */

#include "zorbautils/fatal.h"

#include "globalenv.h"

#include "types/root_typemanager.h"
#include "types/schema/schema.h"

#include "context/root_static_context.h"
#include "context/default_url_resolvers.h"
#include "context/dynamic_loader.h"

#include "functions/library.h"

#include "annotations/annotations.h"

#include "compiler/api/compiler_api.h"
#include "compiler/xqueryx/xqueryx_to_xquery.h"

#include "types/schema/schema.h"

#include "zorbatypes/m_apm.h"
#include "zorbautils/condition.h"

#include "store/api/collection.h"

#include "store/api/store.h"

#ifdef ZORBA_XQUERYX
#include <libxslt/xslt.h>
#include <libxml/parser.h>
#endif

#include "diagnostics/assert.h"


#include "types/schema/xercesIncludes.h"
#include "types/schema/StrX.h"


using namespace zorba;


GlobalEnvironment * GlobalEnvironment::m_globalEnv = 0;

/*******************************************************************************

********************************************************************************/
void GlobalEnvironment::init(store::Store* store)
{
  // initialize Xerces-C lib
#ifndef ZORBA_NO_XMLSCHEMA
  try
  {
    XERCES_CPP_NAMESPACE::XMLPlatformUtils::Initialize();
  }
  catch (const XERCES_CPP_NAMESPACE::XMLException& toCatch)
  {
    std::cerr << "Error during Xerces-C initialization! Message:\n"
              << StrX(toCatch.getMessage()) << std::endl;
    abort();
  }
#endif

  m_globalEnv = new GlobalEnvironment();

  m_globalEnv->init_icu();

  ZORBA_FATAL(store != NULL, "Must provide store during zorba initialization");

  m_globalEnv->theStore = store;

  m_globalEnv->theRootTypeManager = new RootTypeManager;
  RCHelper::addReference(m_globalEnv->theRootTypeManager);

  m_globalEnv->theRootStaticContext = new root_static_context();
  m_globalEnv->theRootStaticContext->init();

  m_globalEnv->theFunctionLib = new BuiltinFunctionLibrary();

  m_globalEnv->theFunctionLib->populate(m_globalEnv->theRootStaticContext);

  AnnotationInternal::createBuiltIn();

#ifdef ZORBA_XQUERYX
  //libxml2 and libxslt are needed
  xmlInitMemory();
  xmlInitParser();

  LIBXML_TEST_VERSION
 
  // TODO function not available on mac
  //xsltInit();
  m_globalEnv->xqueryx_convertor = new XQueryXConvertor;
#endif

  std::unique_ptr<XQueryCompilerSubsystem> lSubSystem = 
  XQueryCompilerSubsystem::create();

  m_globalEnv->m_compilerSubSys = lSubSystem.release();

  m_globalEnv->m_http_resolver = new internal::HTTPURLResolver();

  m_globalEnv->theDynamicLoader = 0;

  m_globalEnv->theHostCountry = locale::get_host_country();

  m_globalEnv->theHostLang = locale::get_host_lang();
}


/*******************************************************************************
  destroy all components that were initialized in init 
  note: destruction must be done in reverse initialization order
********************************************************************************/
void GlobalEnvironment::destroy()
{
  delete m_globalEnv->theDynamicLoader;

  delete m_globalEnv->m_http_resolver;

  serialization::ClassSerializer::getInstance()->destroyArchiverForHardcodedObjects();

  delete m_globalEnv->m_compilerSubSys;
  m_globalEnv->m_compilerSubSys = 0;

#ifdef ZORBA_XQUERYX
    //free libxml2 and libxslt

  xsltCleanupGlobals();
  xmlCleanupParser();
  delete m_globalEnv->xqueryx_convertor;
#endif

  delete m_globalEnv->theRootStaticContext;

  delete m_globalEnv->theFunctionLib;

  RCHelper::removeReference(m_globalEnv->theRootTypeManager);
  m_globalEnv->theRootTypeManager = 0;

  AnnotationInternal::destroyBuiltIn();

  m_globalEnv->theStore = NULL;

  // we shutdown icu
  // again it is important to mention this in the documentation
  // we might disable this call because it only
  // releases statically initialized memory and prevents
  // valgrind from reporting those problems at the end
  // see http://www.icu-project.org/apiref/icu4c/uclean_8h.html#93f27d0ddc7c196a1da864763f2d8920
  m_globalEnv->cleanup_icu();

  delete m_globalEnv;
	m_globalEnv = NULL;

  // terminate Xerces-C lib
#ifndef ZORBA_NO_XMLSCHEMA
  XERCES_CPP_NAMESPACE::XMLPlatformUtils::Terminate();
#endif
}


/*******************************************************************************

********************************************************************************/
void GlobalEnvironment::destroyStatics()
{
  // release resources aquired by the mapm library
  // this will force zorba users to reinit mapm
  // if they shutdown zorba but want to use mapm beyond
  m_apm_free_all_mem();
}


/*******************************************************************************

********************************************************************************/
GlobalEnvironment::GlobalEnvironment()
  :
  theStore(0), 
  theRootTypeManager(NULL),
  theRootStaticContext(0),
  m_compilerSubSys(0)
{
}



/*******************************************************************************

********************************************************************************/
GlobalEnvironment::~GlobalEnvironment()
{
}


/*******************************************************************************

********************************************************************************/
void GlobalEnvironment::init_icu()
{
  // initialize the icu library
  // we do this here because we are sure that is used
  // from one thread only
  // see http://www.icu-project.org/userguide/design.html#Init_and_Termination
  // and http://www.icu-project.org/apiref/icu4c/uclean_8h.html
#ifndef ZORBA_NO_ICU
#  if defined U_STATIC_IMPLEMENTATION && (defined WIN32 || defined WINCE)
  {
    TCHAR    self_path[1024];
    GetModuleFileName(NULL, self_path, sizeof(self_path));
        //PathRemoveFileSpec(self_path);
    TCHAR  *filename;
    filename = _tcsrchr(self_path, _T('\\'));
    if(filename)
      filename[1] = 0;
    else
      self_path[0] = 0;
        //strcat(self_path, "\\");
        //_tcscat(self_path, _T(U_ICUDATA_NAME));//icudt39l.dat");
    _tcscat(self_path, _T("icudt") _T(U_ICU_VERSION_SHORT) _T(U_ICUDATA_TYPE_LETTER));//icudt39l.dat");
    _tcscat(self_path, _T(".dat"));
        //unsigned char *icu_data;
    HANDLE    hfile;
    hfile = CreateFile(self_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    ZORBA_ASSERT(hfile != INVALID_HANDLE_VALUE);
    DWORD   icusize;
    icusize = GetFileSize(hfile, NULL);
    icu_appdata = new unsigned char[icusize];
    DWORD   nr_read;
    ReadFile(hfile, icu_appdata, icusize, &nr_read, NULL);
    CloseHandle(hfile);
    UErrorCode    data_err = U_ZERO_ERROR;
    udata_setCommonData(icu_appdata, &data_err);
    ZORBA_ASSERT(data_err == U_ZERO_ERROR);
  
    // u_setDataDirectory(self_path);
  }
#  endif
  UErrorCode lICUInitStatus = U_ZERO_ERROR;
  u_init(&lICUInitStatus);
  ZORBA_ASSERT(lICUInitStatus == U_ZERO_ERROR);
#endif /* ZORBA_NO_ICU */
}


void GlobalEnvironment::cleanup_icu()
{
  // we shutdown icu
  // again it is important to mention this in the documentation
  // we might disable this call because it only
  // releases statically initialized memory and prevents
  // valgrind from reporting those problems at the end
  // see http://www.icu-project.org/apiref/icu4c/uclean_8h.html#93f27d0ddc7c196a1da864763f2d8920
#ifndef ZORBA_NO_ICU
  u_cleanup();
# if defined U_STATIC_IMPLEMENTATION && (defined WIN32 || defined WINCE)
  delete[] icu_appdata;
# endif
#endif /* ZORBA_NO_ICU */
}


RootTypeManager& GlobalEnvironment::getRootTypeManager() const
{
  return *theRootTypeManager;
}


static_context& GlobalEnvironment::getRootStaticContext() const
{
  return *theRootStaticContext;
}


bool GlobalEnvironment::isRootStaticContextInitialized() const
{
  return theRootStaticContext != NULL;
}


store::Store& GlobalEnvironment::getStore()
{
  return *theStore;
}


store::ItemFactory* GlobalEnvironment::getItemFactory()
{
  return theStore->getItemFactory();
}


store::IteratorFactory* GlobalEnvironment::getIteratorFactory()
{
  return theStore->getIteratorFactory();
}


XQueryCompilerSubsystem& GlobalEnvironment::getCompilerSubsystem()
{
  return *m_compilerSubSys;
}

DynamicLoader* GlobalEnvironment::getDynamicLoader() const
{
  if (!theDynamicLoader)
  {
    theDynamicLoader = new DynamicLoader();
  }
  return theDynamicLoader;
}

#ifdef ZORBA_XQUERYX
XQueryXConvertor    *GlobalEnvironment::getXQueryXConvertor()
{
  return xqueryx_convertor;
}
#endif
/* vim:set et sw=2 ts=2: */
