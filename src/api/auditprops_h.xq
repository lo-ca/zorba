(:
 : Copyright 2013 The FLWOR Foundation.
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
import module namespace file = "http://expath.org/ns/file";

declare variable $auditspecfile as xs:string external;

declare function local:gen-group-spec($group as element()?) as xs:string*
{
  if ($group)
  then
    ($group/@name, local:gen-group-spec($group/parent::group))
  else ()
};


declare function local:group($group-spec as xs:string*) as xs:string
{
'extern const PropertyGroupImpl '
  || string-join($group-spec ! upper-case(.), "_")
  || ';
'
};

string-join(
  (
  let $spec := parse-xml(file:read-text($auditspecfile))
  for $first-prop in $spec//group/property[1]
  let $group-spec := reverse(local:gen-group-spec($first-prop/parent::group))
  return
      local:group($group-spec) ||
      string-join(
        for $prop in ($first-prop, $first-prop/following-sibling::property)
        return
          'extern const PropertyImpl '
            || string-join($group-spec ! upper-case(.), "_")
            || '_' || replace(upper-case($prop/@name), "-", "_")
            || ';'
      , "
") || '

'))

