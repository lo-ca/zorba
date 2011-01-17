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
#ifndef __WEB_H_INCLUDED__
#define __WEB_H_INCLUDED__

#include <iostream>

#include "zorbatypes/zstring.h"

namespace zorba
{
#ifdef ZORBA_WITH_TIDY
  int tidy(const char* input,
           zstring& result,
           zstring& diagnostics,
           const char* userOpt) throw();

  int tidy(const std::istream& stream,
           std::iostream& result,
           zstring& diagnostics,
           const char* userOpt);
#endif
}

#endif
