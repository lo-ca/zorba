import module namespace file = "http://expath.org/ns/file";
import module namespace string = "http://zorba.io/modules/string";

let $x := file:read-text(concat("", "D:/ZORBA/update3.0/update3.0_string/test/rbkt/Queries/zorba/file/sample_files/sample.txt"))
return (string:is-seekable($x), string:is-seekable("test"))

