import module namespace j = "http://www.jsoniq.org/functions";

variable $o := { "foo" : "bar", "blub" : "bar" };

j:rename($o, "foo", "blub")
