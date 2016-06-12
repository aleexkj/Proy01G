#include "viewport.h"

#include "CMU462.h"

namespace CMU462 {

void ViewportImp::set_viewbox( float x, float y, float span ) {

  // Task 4 (part 2):
  // Set svg to normalized device coordinate transformation. Your input
  // arguments are defined as SVG canvans coordinates.
  this->x = x;
  this->y = y;
  this->span = span;

  Matrix3x3 B;
  float view = 1.0 / (span * 2.0);
  float wxmin = -(x-span);
  float wymin = -(y-span);
  /* scale & translate -> normalized*/
  B(0,0) = view; B(0,1) = 0.; B(0,2) = view * wxmin;
  B(1,0) = 0.; B(1,1) = view; B(1,2) = view * wymin;
  B(2,0) = 0.; B(2,1) = 0.; B(2,2) = 1.;

  svg_2_norm = B;
}

void ViewportImp::update_viewbox( float dx, float dy, float scale ) {

  this->x -= dx;
  this->y -= dy;
  this->span *= scale;
  set_viewbox( x, y, span );
}

} // namespace CMU462
