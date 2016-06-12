#include "colormanipulation.h"

#define maximum(A, B, C) max(A, max(B, C))
#define minimum(A, B, C) min(A, min(B, C))
#define truncate(C) C < 0.0? 0.0 : (C > 1.0? 1.0 : C)
#define blend(A, B) sqrt(0.5*pow(A, 2) + 0.5*pow(B,2))

using namespace std;

namespace CMU462 {
  Color bright_color(Color c, float bright_factor)
 {
  bright_factor = 1 - bright_factor;

  /* RGB to HSV */
  float cmax = maximum(c.r, c.g, c.b);
  float cmin = minimum(c.r, c.g, c.b);

  float incr = cmax - cmin;
  float hue = 0;

  if(incr == c.r)
    hue = int((c.g - c.b) / incr) % 6;
  if(incr == c.g)
    hue = ((c.b - c.r) / incr + 2);
  if(incr == c.b)
    hue = ((c.r - c.g) / incr + 4);

  float sat = cmax == 0? 0 : incr/cmax;
  float v = bright_factor; /* aumentar brillo */

  float cf = v*sat;
  float x = cf * (1 - fabs((int) hue % 2 - 1));
  float m = v - cf;

  Color color = Color();
  /* HSV to RGB */

  if(hue < 60)
  {
    color.r = cf;
    color.g = x;
    color.b = 0;
  }
  else if(hue >= 60 && hue < 120)
  {
    color.r = x;
    color.g = cf;
    color.b = 0;
  }
  else if(hue >= 120 && hue < 180)
  {
    color.r = 0;
    color.g = cf;
    color.b = x;
  }
  else if(hue >= 180 && hue < 240)
  {
    color.r = 0;
    color.g = x;
    color.b = cf;
  }
  else if(hue >= 240 && hue < 300)
  {
    color.r = x;
    color.g = 0;
    color.b = cf;
  }
  else if(hue >= 300)
  {
    color.r = cf;
    color.g = 0;
    color.b = x;
  }

  color.r +=m;
  color.g +=m;
  color.b +=m;
  color.a = c.a; /* not change alpha value */

  return color;
 }

 Color blend_colors(Color fg, Color bg) {
   Color c = Color();
   c.a = 1 - (1 - fg.a) * (1 - bg.a);
   if(c.a < 1.0e-6) return c; // full transparent
   /*c.r = fg.r * fg.a / c.a + bg.r * bg.a * (1 - fg.a) / c.a;
   c.g = fg.g * fg.a / c.a + bg.g * bg.a * (1 - fg.a) / c.a;
   c.b = fg.b * fg.a / c.a + bg.b * bg.a * (1 - fg.a) / c.a;*/
   c.r = (fg.r * fg.a) + (bg.r * (1.0 - fg.a));
   c.g = (fg.g * fg.a) + (bg.g * (1.0 - fg.a));
   c.b = (fg.b * fg.a) + (bg.b * (1.0 - fg.a)); // over blend
   return c;
}

Color avg(Color fg, Color bg) {
  Color c = Color();
   c.r = truncate(blend(fg.r, bg.r));
   c.g = truncate(blend(fg.g, bg.g));
   c.b = truncate(blend(fg.b, bg.b));
   c.a = truncate(0.5*fg.a + 0.5*bg.a);
   return c;
 }

 Color tint_color(Color fg, float tint_factor){
   Color color = Color();
   color.r = fg.r + (1.0 - fg.r) * tint_factor;
   color.g = fg.g + (1.0 - fg.g) * tint_factor;
   color.b = fg.b + (1.0 - fg.b) * tint_factor;
   color.a = fg.a;
   return color;
 }
}
