import module namespace geo = "http://www.zorba-xquery.com/modules/geo";
declare namespace gml="http://www.opengis.net/gml";

geo:contains(<gml:Curve><gml:segments>
                <gml:LineStringSegment interpolation="linear"><gml:posList> 1 1 2 2</gml:posList></gml:LineStringSegment>
                <gml:LineStringSegment><gml:posList> 1 1 2 2 4 4</gml:posList></gml:LineStringSegment>
              </gml:segments></gml:Curve>,
           <gml:Point><gml:pos> 1 1</gml:pos></gml:Point>
              )