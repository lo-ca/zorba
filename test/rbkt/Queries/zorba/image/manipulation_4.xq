(:~
 : Simple test module for the manipulation functions of the image library.
 : 
 : @author Daniel Thomas
 :)
import module namespace basic = 'http://www.zorba-xquery.com/modules/image/basic';
import module namespace file = 'http://www.zorba-xquery.com/modules/file';
import module namespace man = 'http://www.zorba-xquery.com/modules/image/manipulation';
import schema namespace image = 'http://www.zorba-xquery.com/modules/image/image';

declare variable $local:gif as xs:base64Binary := file:read("images/bird.gif");
declare variable $local:jpg as xs:base64Binary := file:read("images/bird.jpg");


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
 : @return true if the man:stereo function works.
 :)
declare function local:test-stereo() as xs:boolean {
    let $right-image := man:zoom($local:jpg, 0.9)
    let $stereod := man:stereo($local:jpg, $right-image)
    let $stereod-ref := file:read("images/manipulation/stereodBird.jpg")
    return($stereod eq $stereod-ref)
};

(:~
 : @return true if the man:transparent function works.
 :)
declare function local:test-transparent() as xs:boolean {
    let $transparented := man:transparent($local:gif, image:colorType("#000000"))
    let $transparented-ref := file:read("images/manipulation/transparentedBird.gif")
    return($transparented eq $transparented-ref)
};

(:~
 : @return true if the man:swirl function works.
 :)
declare function local:test-swirl() as xs:boolean {
    let $swirled := man:swirl($local:gif, -500)
    let $swirled-ref := file:read("images/manipulation/swirledBird.gif")
    return($swirled eq $swirled-ref)
};

(:~
 : @return true if the man:reduce-noise function works.
 :)
declare function local:test-reduce-noise() as xs:boolean {
    let $reduced := man:reduce-noise($local:gif, -0.4)
    let $reduced-ref := file:read("images/manipulation/reducedBird.gif")
    return($reduced eq $reduced-ref)
};


(:~
 : @return true if the man:contrast function works.
 :)
declare function local:test-contrast() as xs:boolean {
    let $contrasted := man:contrast($local:gif, 0.8)
    let $contrasted-ref := file:read("images/manipulation/contrastedBird.gif")
    return($contrasted eq $contrasted-ref)
};


declare sequential function local:main() as xs:string* {

  let $a := local:test-stereo()
  return
    if (fn:not($a)) then
      exit returning local:error(("Stereo of images failed."))
    else ();
    
    
  let $b := local:test-transparent()
  return
    if (fn:not($b)) then
      exit returning local:error(("Making a color of an image tranparent failed."))
    else ();  
    
     
  let $c := local:test-swirl()
  return
    if (fn:not($c)) then
      exit returning local:error(("Swirl of image failed."))
    else ();  
    
  let $d := local:test-reduce-noise()
  return
    if (fn:not($d)) then
      exit returning local:error(("Reduction of noise from image failed."))
    else ();  
   
  let $e := local:test-contrast()
  return
    if (fn:not($e)) then
      exit returning local:error(("Contrasting image failed."))
    else ();  
       
    
    
  (: If all went well ... make sure the world knows! :)  
  "SUCCESS";

    


};

local:main();


