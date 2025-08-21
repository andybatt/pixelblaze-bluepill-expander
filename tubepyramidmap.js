function (pixelCount) {
  var isSecondPb = false;
  
  //distance from one row to another is the same as a 60 degree rotation
  var yOffset = Math.sin(60 / 180 * Math.PI);
  var gap = yOffset * 2.5; //2.5 for a tight gap based on true center, increase for a gap
  var angleOffset = 30; //adjust the whole thing by this much

  //rotate a point (x, y), along a center (cx, cy), by an angle in degrees
  function rotate(cx, cy, x, y, angle) {
    var radians = (Math.PI / 180) * angle,
        cos = Math.cos(radians),
        sin = Math.sin(radians),
        nx = (cos * (x - cx)) + (sin * (y - cy)) + cx,
        ny = (cos * (y - cy)) - (sin * (x - cx)) + cy;
    return [nx, ny];
  }
  
  function tubePyramid(x, y, angle) {
    var tubeMap = [];
    for (var row = 4; row > 0; row--) {
      for (var col = 0; col < row; col++) {
        var px = col +  (4 - row)/2;
        var py = 4 - row * yOffset;
        for (var z = 0; z < 15; z++) {
          var [rx, ry] = rotate(1.5, yOffset * 2, px, py, angle)
          tubeMap.push([rx + x, ry + y, z])
        }
      }
    }
    return tubeMap
  }
  

  //assemble all of the tube pyramids
  var map = [];
  
  for (var i = 0; i < 6; i++) {
    var angle = 60 * i;
    var [x, y] = rotate(0, gap, 0, 0, angle + angleOffset)
    map = map.concat(tubePyramid(x,y,angle + angleOffset))  
  }
  
  if (isSecondPb) {
    var firstHalf = map.splice(0, 450) //cut out the first half from map
    map = map.concat(firstHalf) //add them back at the end
  }

  return map
}