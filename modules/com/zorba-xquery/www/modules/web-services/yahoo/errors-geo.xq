(:
 : Copyright 2006-2009 The FLWOR Foundation.
 :
 : Licensed under the Apache License, Version 2.0 (the "License");
 : you may not use this file except in compliance with the License.
 : You may obtain a copy of the License at
 :
 : http://www.apache.org/licenses/LICENSE-2.0
 :
 : Unless required by applicable law or agreed to in writing, software
 : distributed under the License is distributed on an "AS IS" BASIS,
 : WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 : See the License for the specific language governing permissions and
 : limitations under the License.
 :)

(:~
 : Module that defines the errors raised in Yahoo! Search module.
 :
 : @author Stephanie Russell
 : @version 1.0
 :)
module namespace  err = "http://www.zorba-xquery.com/modules/web-services/yahoo/errors-geo" ;

(:~
 : Errors namespace URI.
 :)
declare variable $err:errNS as xs:string := "http://www.zorba-xquery.com/modules/web-services/yahoo/errors-geo";

(:~
 : xs:QName with namespace URI="http://www.zorba-xquery.com/modules/web-services/yahoo/errors-geo" and local name 'err:YS002'. Location not found.
:)
declare variable $err:YS002 as xs:QName := fn:QName($err:errNS, "err:YS002");