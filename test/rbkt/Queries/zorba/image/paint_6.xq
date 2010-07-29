(:~
 : Simple test module for the paint functions of the image library.
 : 
 : @author Daniel Thomas
 :)
import module namespace basic = 'http://www.zorba-xquery.com/modules/image/basic';
import module namespace file = 'http://www.zorba-xquery.com/modules/file';
import module namespace paint = 'http://www.zorba-xquery.com/modules/image/paint';
import schema namespace image = 'http://www.zorba-xquery.com/modules/image/image';

declare variable $local:jpg as xs:base64Binary := basic:create(xs:unsignedInt(100), xs:unsignedInt(100), image:imageType("JPEG"));


(:~
 : Outputs a nice error message to the screen ...
 :
 : @param $messsage is the message to be displayed
 : @return The passed message but really very, very nicely formatted.
 :)
declare function local:error($messages as xs:string*) as xs:string* {
  "
************************************************************************
ERROR:
  Location:", file:path-to-full-path("."), "
  Cause:",
  $messages,
  "
************************************************************************
"
};


(:~
 : @return true if the man:draw-polygon function works.
 :)
declare function local:test-draw-polygon() as xs:boolean {
    let $draw := paint:draw-polygon($local:jpg, (10,40,50), (10, 80, 30), (), (), (), ())
    let $draw-ref := file:read("images/paint/polygon.jpg")
    return($draw eq $draw-ref)
};

(:~
 : @return true if the man:draw-polygon function works.
 :)
declare function local:test-draw-polygon-red() as xs:boolean {
    let $draw := paint:draw-polygon($local:jpg, (10,40,50), (10, 80, 30), image:colorType("#FF0000"), (), (), ())
    let $draw-ref := file:read("images/paint/polygonRed.jpg")
    return($draw eq $draw-ref)
};

(:~
 : @return true if the man:draw-polygon function works.
 :)
declare function local:test-draw-polygon-red-green() as xs:boolean {
    let $draw := paint:draw-polygon($local:jpg, (10,40,50), (10, 80, 30), image:colorType("#FF0000"),image:colorType("#00FF00"), (), ())
    let $draw-ref := file:read("images/paint/polygonRedGreen.jpg")
    return($draw eq $draw-ref)
};

(:~
 : @return true if the man:draw-polygon function works.
 :)
declare function local:test-draw-polygon-wide() as xs:boolean {
    let $draw := paint:draw-polygon($local:jpg, (10,40,50), (10, 80, 30), image:colorType("#FF0000"),image:colorType("#00FF00"), 3, ())
    let $draw-ref := file:read("images/paint/polygonWide.jpg")
    return($draw eq $draw-ref)
};


(:~
 : @return true if the man:draw-polygon function works.
 :)
declare function local:test-draw-polygon-anti-aliased() as xs:boolean {
    let $draw := paint:draw-polygon($local:jpg, (10,40,50), (10, 80, 30), image:colorType("#FF0000"),image:colorType("#00FF00"), 3,true())
    let $draw-ref := file:read("images/paint/polygonAntiAliased.jpg")
    return($draw eq $draw-ref)
};


declare sequential function local:main() as xs:string* {

  let $a := local:test-draw-polygon()
  return
    if (fn:not($a)) then
      exit returning local:error(("Drawing a polygon on an image failed."))
    else ();
  
  
  let $b := local:test-draw-polygon-red()
  return
    if (fn:not($b)) then
      exit returning local:error(("Drawing a red polygon on an image failed."))
    else ();
      
   
  let $c := local:test-draw-polygon-red-green()
  return
    if (fn:not($c)) then
      exit returning local:error(("Drawing a red polygon filled with green color on an image failed."))
    else ();
      
  
  let $d := local:test-draw-polygon-wide()
  return
    if (fn:not($d)) then
      exit returning local:error(("Drawing a polygon with wide strokes  on an image failed."))
    else ();
      
  let $e := local:test-draw-polygon-anti-aliased()
  return
    if (fn:not($e)) then
      exit returning local:error(("Drawing an anti-aliased polygon on an image failed."))
    else ();
              
      
  (: If all went well ... make sure the world knows! :)  
  "SUCCESS";

    


};

local:main();


