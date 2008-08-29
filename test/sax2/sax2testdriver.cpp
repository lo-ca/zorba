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
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <cassert>
#include <cstring>

#include <zorba/util/file.h>

#include "sax2testdriverconfig.h" // SRC and BIN dir definitions

#include <zorba/zorba.h>
#include <zorba/default_content_handler.h>
#include <zorba/error_handler.h>
#include <zorba/exception.h>

#include <zorbautils/strutil.h>

#include <simplestore/simplestore.h>

using namespace zorba;

void
printFile(std::ostream& os, std::string aInFile)
{
  std::ifstream lInFileStream(aInFile.c_str());
  assert(lInFileStream);

  os << lInFileStream.rdbuf() << std::endl;
}

// print parts of a file
// starting at aStartPos with the length of aLen
void
printPart(std::ostream& os, std::string aInFile, 
          int aStartPos, int aLen)
{
  char* buffer = new char [aLen];
  try {
    std::ifstream lIn(aInFile.c_str());
    lIn.seekg(aStartPos);

    int lCharsRead = lIn.readsome (buffer, aLen);
    os.write (buffer, lCharsRead);
    os.flush();
    delete[] buffer;
  } catch (...)
  {
    delete[] buffer;
  }
  return;
}

Zorba_CompilerHints
getCompilerHints()
{
  Zorba_CompilerHints lHints;

  // ZORBA_OPTLEVEL=O0 | O1
  char* lOptLevel = getenv("ZORBA_OPTLEVEL");
  if ( lOptLevel != NULL && strcmp(lOptLevel, "O0") == 0 ) {
    lHints.opt_level = ZORBA_OPT_LEVEL_O0;
    std::cout << "testdriver is using optimization level O0" << std::endl;
  } else {
    lHints.opt_level = ZORBA_OPT_LEVEL_O1;
    std::cout << "testdriver is using optimization level O1" << std::endl;
  }
  return lHints; 
}

class TestContentHandler: public DefaultContentHandler
{
  protected:
    std::ostream & theOStream;
    bool preserveWhiteSpaces;

  public:

  TestContentHandler( std::ostream & aOStream, bool useWS = true )
    : theOStream( aOStream ), preserveWhiteSpaces( useWS ){}

  virtual ~TestContentHandler(){}
 
  void startDocument()
  {
    theOStream << "startDocument()" << std::endl;
  }

  void startElement(
        const   String & uri,
        const   String & localname,
        const   String & qname,
        const   SAX2_Attributes & attrs )
  {
    theOStream << "startElement()" << std::endl;
    theOStream << "LocalName: " << localname << std::endl;
    theOStream << "QName: " << qname << std::endl;
    for ( unsigned int i = 0; i < attrs.getLength(); i++ )
    {
      theOStream << "Attribute name: " << attrs.getQName( i ) << ", ";
      theOStream << "value: " << attrs.getValue( i ) << std::endl;
      theOStream << "URI: " << attrs.getURI( i ) << std::endl;
      theOStream << "Local name: " << attrs.getLocalName( i ) << std::endl;
      theOStream << "Type: " << attrs.getType( i ) << std::endl;
      assert ( attrs.getType ( i )  == attrs.getType ( attrs.getQName ( i ) ) );
      assert ( attrs.getType ( i )  == attrs.getType ( attrs.getURI( i ), attrs.getLocalName( i ) ) );
      assert ( attrs.getValue( i )  == attrs.getValue( attrs.getQName( i ) ) );
      assert ( attrs.getValue ( i ) == attrs.getValue( attrs.getURI( i ), attrs.getLocalName( i ) ) );
    }
  }

  void startPrefixMapping (const String & prefix,	const String & uri)
  {
    theOStream << "startPrefixMapping()" << std::endl;
    theOStream << "Prefix: " << prefix << std::endl;
    theOStream << "URI: " << uri << std::endl;
  }

  void endPrefixMapping( const String & prefix )
  {
    theOStream << "endPrefixMapping()" << std::endl;
    theOStream << "Prefix: " << prefix << std::endl;
  }

  void skippedEntity( const String & name )
  {
    theOStream << "Skipped entity: " << name << std::endl;
  }

  void comment( const String &  chars )
  {
    theOStream << "Comment: " << chars << std::endl;
  }

  void startDTD( const String & name, const String & publicId, const String & systemId )
  {
    theOStream << "startDTD()" << std::endl;
    theOStream << "Name: " << name << std::endl;
    //std::cerr << "PublicId: " << publicId << std::endl;
    //std::cerr << "SystemId: " << systemId << std::endl;
  }

  void endDTD()
  {
    theOStream << "endDTD()" << std::endl;
  }

  void startCDATA()
  {
    theOStream << "startCDATA()" << std::endl;
  }

  void endCDATA()
  {
    theOStream << "endCDATA()" << std::endl;
  }
};

class TestErrorHandler : public zorba::ErrorHandler {
  public:
    void staticError(const zorba::StaticException& aStaticError)
    {
      registerError(aStaticError);
    }

    void dynamicError(const zorba::DynamicException& aDynamicError)
    {
      registerError(aDynamicError);
    }

    void typeError(const zorba::TypeException& aTypeError)
    {
      registerError(aTypeError);
    }

    void serializationError(const zorba::SerializationException& aSerializationError)
    {
      registerError(aSerializationError);
    }

    void systemError(const zorba::SystemException& aSystemError)
    {
      registerError(aSystemError);
    }

    bool errors()
    {
      return !m_errors.empty();
    }

    const std::vector<std::string>& getErrorList()
    {
      return m_errors;
    }
    const std::vector<zorba::String>& getErrorDescs()
    {
      return m_desc;
    }

  private:
    std::vector<std::string> m_errors;
    std::vector<zorba::String> m_desc;

    void registerError(const zorba::ZorbaException& e)
    {
      m_errors.push_back(ZorbaException::getErrorCodeAsString(e.getErrorCode()).c_str());
      m_desc.push_back(e.getDescription());
    }
};

// print all errors that were raised
void
printErrors(TestErrorHandler& errHandler)
{
  if (!errHandler.errors()) {
    return;
  }
  std::cerr << "Errors:" << std::endl;
  
  const std::vector<std::string>& errors = errHandler.getErrorList();
  const std::vector<zorba::String>& descs = errHandler.getErrorDescs();

  std::vector<std::string>::const_iterator codeIter = errors.begin();
  std::vector<zorba::String>::const_iterator descIter = descs.begin();

    for(; codeIter != errors.end(); ++codeIter, ++descIter) {
      assert (descIter != descs.end());
      std::cerr << *codeIter << ": " << *descIter << std::endl;
  }
  return;
}

void
trim(std::string& str) {

  std::string::size_type  notwhite = str.find_first_not_of(" \t\n");
  str.erase(0,notwhite);

  notwhite = str.find_last_not_of(" \t\n"); 
  str.erase(notwhite+1); 
}

// return false if the files are not equal
// aLine contains the line number in which the first difference occurs
// aCol contains the column number in which the first difference occurs
// aPos is the character number off the first difference in the file
// -1 is returned for aLine, aCol, and aPos if the files are equal
bool
isEqual(zorba::file aRefFile, zorba::file aResFile, int& aLine, int& aCol, int& aPos)
{
  std::ifstream li(aRefFile.get_path().c_str());
  std::ifstream ri(aResFile.get_path().c_str()); 
  
  std::string lLine, rLine;

  aLine = 1; aCol = 0; aPos = -1;
  while (! li.eof() )
  {
    if ( ri.eof() ) {
      std::getline(li, lLine);
      if (li.peek() == -1) // ignore end-of-line in the ref result
        return true;
      else 
        return false;
    }
    std::getline(li, lLine);
    std::getline(ri, rLine);
    ++aLine;
    if ( (aCol = lLine.compare(rLine)) != 0) {
      return false;
    }
  }

  return true;
}

void 
slurp_file (const char *fname, std::string &result) {
  std::ifstream qfile(fname, std::ios::binary | std::ios_base::in); assert (qfile);

  qfile.seekg (0, std::ios::end);
  size_t len = qfile.tellg ();
  qfile.seekg (0, std::ios::beg);
  char *str = new char [len];
  qfile.read (str, len);
  len = qfile.gcount();
  
  std::string sstr (str, len);
  result.swap (sstr);
  delete [] str;
}

int
#ifdef _WIN32_WCE
_tmain(int argc, _TCHAR* argv[])
#else
main(int argc, char** argv)
#endif
{
  int flags = zorba::file::CONVERT_SLASHES | zorba::file::RESOLVE;

  // do initial stuff
  if ( argc != 2 ) {
    std::cerr << "\nusage:   testdriver [testfile]" << std::endl;
    return 1;
  }
  std::string lQueryFileString  = zorba::SAX2_SRC_DIR +"/Queries/" + argv[1];
  zorba::file lQueryFile (lQueryFileString, zorba::file::CONVERT_SLASHES | zorba::file::RESOLVE);

  std::string lQueryWithoutSuffix = std::string(argv[1]).substr( 0, std::string(argv[1]).size()-3 );
  std::cout << "test " << lQueryWithoutSuffix << std::endl;
  zorba::file lResultFile (zorba::SAX2_BINARY_DIR +"/QueryResults/" 
                           + lQueryWithoutSuffix + ".res", flags);
  zorba::file lErrorFile  (zorba::SAX2_BINARY_DIR +"/" 
                           + lQueryWithoutSuffix + ".err", flags);
  zorba::file lRefFile    (zorba::SAX2_SRC_DIR +"/ExpQueryResults/" 
                           + lQueryWithoutSuffix +".xml.res", flags);
  
  // does the query file exists
  if ( (! lQueryFile.exists ()) || lQueryFile.is_directory () ) {
    std::cerr << "\n query file " << lQueryFile.get_path() 
              << " does not exist or is not a file" << std::endl;
    return 2;
  }

  // delete previous files if they exists
  if ( lResultFile.exists () ) { lResultFile.remove (); }
  if ( lErrorFile.exists () )  { lErrorFile.remove ();  }

  // create the result directory
  zorba::file lBucket = lResultFile.branch_path();
  if ( ! lBucket.exists () )
    lBucket.deep_mkdir ();

  // we must either have a reference file or an expected error code
  if ( (! lRefFile.exists ()) || lRefFile.is_directory ())
  {
    std::cerr << "No reference result and no expected errors." << std::endl;
    return 3;
  }

  // print the query
  std::cout << "Query:" << std::endl;
  printFile(std::cout, lQueryFile.get_path());
  std::cout << std::endl;

  zorba::Zorba *engine = zorba::Zorba::getInstance(zorba::simplestore::SimpleStoreManager::getStore());

  TestErrorHandler errHandler;
  std::stringstream lResult;
  TestContentHandler contentHandler( lResult );

  // create and compile the query
  std::string lQueryString;
  slurp_file(lQueryFile.get_path().c_str(), lQueryString);
  zorba::XQuery_t lQuery = engine->compileQuery(lQueryString.c_str(), getCompilerHints(), &errHandler);
  
  if (errHandler.errors())
  {
    std::cerr << "Error compiling query" << std::endl;
  }

  {
    // serialize xml
    std::ofstream lResFileStream(lResultFile.get_path().c_str());
    assert (lResFileStream.good());

    lQuery->registerSAXHandler(&contentHandler);
    lQuery->executeSAX();
    lResFileStream << lResult.str();

    if (errHandler.errors())
    {
        std::cerr << "Error executing query" << std::endl;
        printErrors(errHandler); 
        return 6;
    }
  }
  std::cout << "Result:" << std::endl;
  printFile(std::cout, lResultFile.get_path());
  std::cout << "=== end of result ===" << std::endl;
  std::cout.flush();

  // last, we have to diff the result
  int lLine, lCol, lPos; // where do the files differ
  bool lRes = isEqual(lRefFile, lResultFile, lLine, lCol, lPos);
  if ( !lRes )  // results differ
  {
    std::cerr << std::endl << "Result does not match expected result:" << std::endl;
    printFile(std::cerr, lRefFile.get_path());
    std::cerr << "=== end of expected result ===" << std::endl;

    std::cerr << "See line " << lLine << ", col " << lCol << " of expected result. " << std::endl;
    std::cerr << "Actual: <";
    printPart(std::cerr, lResultFile.get_path(), lPos, 15);
    std::cerr << ">" << std::endl;
    std::cerr << "Expected: <";
    printPart(std::cerr, lRefFile.get_path(), lPos, 15);
    std::cerr << ">" << std::endl;

    return 8;
  }


  return 0;
}
