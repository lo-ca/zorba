/* -*- mode: c++; indent-tabs-mode: nil -*-
 *
 *  $Id: xml_handler_test.cpp,v 1.1 2006/10/09 07:07:59 Paul Pedersen Exp $
 *
 *	Copyright 2006-2007 FLWOR Foundation.
 *
 *	Author: Paul Pedersen
 *
 */

#include "../util/file.h"
#include "../store/xml_ostream.h"
#include "xml_handler.h"
#include "xml_scanner.h"
#include "xml_term.h"

using namespace std;
using namespace xqp;

int main(int argc, char* argv[]) 
{
  try {
    if (argc<2) {
      cerr << "Expecting one argument: PATH\n";
      return -1;
    }
    string path(argv[1],0,strlen(argv[1]));
		file f(path);
    if (!f.exists()) {
      cout << "file '" << path << "' does not exist\n";
      return -1;
    }
		cout << "read file..\n";
    unsigned sz = f.get_size();
    size_t n = (sz > (1<<24) ? (1<<24) : (size_t)(sz));
    char* ibuf = new char[n+1];
		f.readfile(ibuf,n);

		cout << "allocating scan/handlers..\n";
    xml_scanner xscanner;
		uint64_t uri = 0;
		vector<xml_term> xterm_v;
		xml_ostream xos;
    xml_handler* xhandler = new xml_handler(uri, xterm_v, xos);

		cout << "scanning..\n";
    xscanner.scan(ibuf, n, dynamic_cast<scan_handler*>(xhandler));

		cout << "cleaning up..\n";
    delete xhandler;
		delete[] ibuf;

  } catch (xqp_exception& e1) {
    cout << "Application exception: " << e1.what() << '\t' << e1.get_msg() << endl;
  } catch (exception& e2) {
    cout << "System exception: " << e2.what() << endl;
  } catch (...) {
    cout << "Unhandled exception\n";
  }
	
}

