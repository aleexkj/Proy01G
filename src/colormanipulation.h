#ifndef CMU462_COLORMANIPULATION_H
#define CMU462_COLORMANIPULATION_H

#include <cmath>
#include "color.h"

namespace CMU462 { // CMU462

  Color bright_color(Color color, float bright_factor);
  Color blend_colors(Color c1, Color c2);
  Color tint_color(Color color, float tint_factor);
}

#endif
