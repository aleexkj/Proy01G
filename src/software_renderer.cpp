#include "software_renderer.h"

#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>
#include <stdlib.h>
#include <time.h>

#include "triangulation.h"
#include "colormanipulation.h"

#define fpart(X) X < 0? 1 - (X - floor(X)) : X - floor(X)
#define rfpart(X) 1.0 - fpart(X)
#define maximum(A, B, C) max(A, max(B, C))
#define minimum(A, B, C) min(A, min(B, C))
#define random_float(A, B) ((B - A) * ((float) rand() / (float) RAND_MAX)) + A
#define blend(A, B) sqrt(0.5*pow(A, 2) + 0.5*pow(B,2))
#define ablend(A, B) 0.5*A + 0.5*B

using namespace std;

namespace CMU462 {


// Implements SoftwareRenderer //

void SoftwareRendererImp::draw_svg( SVG& svg ) {

  // transformations
  tmatrix = std::stack<Matrix3x3>();
  tmatrix.push(canvas_to_screen);

  // draw all elements
  for ( size_t i = 0; i < svg.elements.size(); ++i ) {
    draw_element(svg.elements[i]);
  }

  transformation = tmatrix.top();

  // draw canvas outline
  Vector2D a = transform(Vector2D(    0    ,     0    )); a.x--; a.y--;
  Vector2D b = transform(Vector2D(svg.width,     0    )); b.x++; b.y--;
  Vector2D c = transform(Vector2D(    0    ,svg.height)); c.x--; c.y++;
  Vector2D d = transform(Vector2D(svg.width,svg.height)); d.x++; d.y++;

  rasterize_line(a.x, a.y, b.x, b.y, Color::Black);
  rasterize_line(a.x, a.y, c.x, c.y, Color::Black);
  rasterize_line(d.x, d.y, b.x, b.y, Color::Black);
  rasterize_line(d.x, d.y, c.x, c.y, Color::Black);

  // resolve and send to render target
  resolve();

}

void SoftwareRendererImp::set_sample_rate( size_t sample_rate ) {

  this->sample_rate = sample_rate;
  this->sample_w = target_w * sample_rate;
  this->sample_h = target_h * sample_rate;
  sample_buffer.resize(4 * sample_w * sample_h);

}

void SoftwareRendererImp::set_render_target( unsigned char* render_target,
                                             size_t width, size_t height ) {

  this->render_target = render_target;
  this->target_w = width;
  this->target_h = height;

  this->sample_w = width * sample_rate;
  this->sample_h = height * sample_rate;
  sample_buffer.resize(4 * sample_w * sample_h);
}

void SoftwareRendererImp::draw_element( SVGElement* element ) {

  tmatrix.push(tmatrix.top() * element->transform);
  transformation = tmatrix.top();

  switch(element->type) {
    case POINT:
      draw_point(static_cast<Point&>(*element));
      break;
    case LINE:
      draw_line(static_cast<Line&>(*element));
      break;
    case POLYLINE:
      draw_polyline(static_cast<Polyline&>(*element));
      break;
    case RECT: /* need to change the rasterize method */
      draw_rect(static_cast<Rect&>(*element));
      break;
    case POLYGON:
      draw_polygon(static_cast<Polygon&>(*element));
      break;
    case ELLIPSE:
      draw_ellipse(static_cast<Ellipse&>(*element));
      break;
    case IMAGE:
      draw_image(static_cast<Image&>(*element));
      break;
    case GROUP:
      draw_group(static_cast<Group&>(*element));
      break;
    default:
      break;
  }

  tmatrix.pop();
}

// Primitive Drawing //

void SoftwareRendererImp::draw_point( Point& point ) {

  Vector2D p = transform(point.position);
  rasterize_point( p.x, p.y, point.style.fillColor );

}

void SoftwareRendererImp::draw_line( Line& line ) {

  Vector2D p0 = transform(line.from);
  Vector2D p1 = transform(line.to);
  rasterize_line( p0.x, p0.y, p1.x, p1.y, line.style.strokeColor );

}

void SoftwareRendererImp::draw_polyline( Polyline& polyline ) {

  Color c = polyline.style.strokeColor;

  if( c.a != 0 ) {
    int nPoints = polyline.points.size();
    for( int i = 0; i < nPoints - 1; i++ ) {
      Vector2D p0 = transform(polyline.points[(i+0) % nPoints]);
      Vector2D p1 = transform(polyline.points[(i+1) % nPoints]);
      rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    }
  }
}

void SoftwareRendererImp::draw_rect( Rect& rect ) {

  Color c;

  // draw as two triangles
  float x = rect.position.x;
  float y = rect.position.y;
  float w = rect.dimension.x;
  float h = rect.dimension.y;

  Vector2D p0 = transform(Vector2D(   x   ,   y   ));
  Vector2D p1 = transform(Vector2D( x + w ,   y   ));
  Vector2D p2 = transform(Vector2D(   x   , y + h ));
  Vector2D p3 = transform(Vector2D( x + w , y + h ));

  // draw fill
  c = rect.style.fillColor;
  if (c.a != 0 ) {
    rasterize_triangle( p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c );
    rasterize_triangle( p2.x, p2.y, p1.x, p1.y, p3.x, p3.y, c );
  }

  // draw outline
  c = rect.style.strokeColor;
  if( c.a != 0 ) {
    rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    rasterize_line( p1.x, p1.y, p3.x, p3.y, c );
    rasterize_line( p3.x, p3.y, p2.x, p2.y, c );
    rasterize_line( p2.x, p2.y, p0.x, p0.y, c );
  }

}

void SoftwareRendererImp::draw_polygon( Polygon& polygon ) {

  Color c;

  // draw fill
  c = polygon.style.fillColor;
  if( c.a != 0 ) {

    // triangulate
    vector<Vector2D> triangles;
    triangulate( polygon, triangles );

    // draw as triangles
    for (size_t i = 0; i < triangles.size(); i += 3) {
      Vector2D p0 = transform(triangles[i + 0]);
      Vector2D p1 = transform(triangles[i + 1]);
      Vector2D p2 = transform(triangles[i + 2]);
      rasterize_triangle( p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c );
    }
  }

  // draw outline
  c = polygon.style.strokeColor;
  if( c.a != 0 ) {
    int nPoints = polygon.points.size();
    for( int i = 0; i < nPoints; i++ ) {
      Vector2D p0 = transform(polygon.points[(i+0) % nPoints]);
      Vector2D p1 = transform(polygon.points[(i+1) % nPoints]);
      rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    }
  }
}

void SoftwareRendererImp::draw_ellipse( Ellipse& ellipse ) {

  // Extra credit

}

void SoftwareRendererImp::draw_image( Image& image ) {

  Vector2D p0 = transform(image.position);
  Vector2D p1 = transform(image.position + image.dimension);

  rasterize_image( p0.x, p0.y, p1.x, p1.y, image.tex );
}

void SoftwareRendererImp::draw_group( Group& group ) {

  for ( size_t i = 0; i < group.elements.size(); ++i ) {
    draw_element(group.elements[i]);
  }

}

// Rasterization //

// The input arguments in the rasterization functions
// below are all defined in screen space coordinates

void SoftwareRendererImp::rasterize_point( float x, float y, Color color ) {

  // fill in the nearest pixel
  int sx = (int) floor(x);
  int sy = (int) floor(y);

  // check bounds
  if ( sx < 0 || sx >= target_w ) return;
  if ( sy < 0 || sy >= target_h ) return;

  fill_pixel(sx, sy, color);
}

void SoftwareRendererImp::raster_line(float x0, float y0,
  float x1, float y1, bool coords, Color color)
{ /* Must blend w/bg for aliasing */
  float dy = y1 - y0;
  float dx = x1 - x0;
  float gradient = dy / dx;
  /* Iniciales */
  float intery = y0;

  for(float x = x0; x <= x1; x++)
  {
    if(coords)
    {
      rasterize_point(x, (intery), bright_color(color, rfpart(intery)));
      rasterize_point(x, (intery) + 1, bright_color(color, fpart(intery)));
    }
    else
    {
      rasterize_point((intery), x, bright_color(color, rfpart(intery)));
      rasterize_point((intery) + 1, x, bright_color(color, fpart(intery)));
    }

    intery += gradient;
  }
}

void SoftwareRendererImp::rasterize_line( float x0, float y0,
                                          float x1, float y1,
                                          Color color)
{
  bool slope_x = true; /* El orden de las coordenadas */

  /* Detectar crecimiento.. se mueve sobre Y o X? */
  bool steep = fabs(y1 - y0) > fabs(x1 - x0);
  /* Si steep >= 1 ent intercambiamos las coords Y por X (rotamos para quedar en 1er octante) */
  if(steep)
  {
    x0 = x0 + y0;
    y0 = x0 - y0;
    x0 -= y0;

    x1 = x1 + y1;
    y1 = x1 - y1;
    x1 -= y1;

    slope_x = false; /* Manejar la transformación */
  }
  /* Actualizar inicios y finales */
  if(x0 > x1)
  {
    x0 = x0 + x1;
    x1 = x0 - x1;
    x0 -= x1;

    y0 = y0 + y1;
    y1 = y0 - y1;
    y0 -= y1;
  }

  /* Trazar la línea */
  raster_line(x0, y0, x1, y1, slope_x, color);

}

void SoftwareRendererImp::rasterize_triangle( float x0, float y0,
                                              float x1, float y1,
                                              float x2, float y2,
                                              Color color ) {
    int width, height;
    int xmin, ymin;
    int x, y;
    float center, size;

    /* min, max for bounding box*/
    width = ceil(maximum(x0, x1, x2));
    height = ceil(maximum(y0, y1, y2));
    xmin = floor(minimum(x0, x1, x2));
    ymin = floor(minimum(y0, y1, y2));
    x = xmin;
    /* supersampling size */
    size = 1.0 / sample_rate;
    center = size / 2.0;

    while(x <= width)
    {
      y = ymin;
      while(y <= height)
      {
        /* test the samples */
        for (int ix = 0; ix < sample_rate; ix++) {
          for (int iy = 0; iy < sample_rate; iy++) {
            float sx = x + center + ix * size;
            float sy = y + center + iy * size;
            if(inside(x0, y0, x1, y1, x2, y2, sx, sy) || inside(x0, y0, x2, y2, x1, y1, sx, sy))
              fill_sample(x, y, ix, iy, color);
          }
        }

        y += 1;
      }
      x += 1;
    }

}

void SoftwareRendererImp::rasterize_image( float x0, float y0,
                                           float x1, float y1,
                                           Texture& tex ) {
  // Task ?:
  // Implement image rasterization

}

// resolve samples to render target
void SoftwareRendererImp::resolve( void ) {

  for(int x = 0; x < target_w; x++)
  {
    for(int y = 0; y < target_h; y++)
    {
      int index = 4 * (x + y * target_w); /* coords en downsample */
      int real_index = 4 * (y * sample_rate * sample_w + x * sample_rate * sample_rate); /* coords en supers */
      bool fill = false; /* whether i blend or not */
      int r, g, b, a;

      for(int ix = 0; ix < sample_rate; ix++)
      {
        for(int iy = 0; iy < sample_rate; iy++)
        {
          int sindex = 4 * (ix + iy * sample_rate);

          if(!fill){
            fill = true;
            r = sample_buffer[real_index + sindex];
            g = sample_buffer[real_index + sindex + 1];
            b = sample_buffer[real_index + sindex + 2];
            a = sample_buffer[real_index + sindex + 3];
          }else {
            r = blend(sample_buffer[real_index + sindex    ], r);
            g = blend(sample_buffer[real_index + sindex + 1], g);
            b = blend(sample_buffer[real_index + sindex + 2], b);
            a = ablend(sample_buffer[real_index + sindex + 3],a); /*  avg of coloured samples */
          }
        }
      }

      if(render_target[index ] == 255 ||
          render_target[index + 1] == 255 ||
          render_target[index + 2] == 255) {
        render_target[index    ] = (uint8_t) r;
        render_target[index + 1] = (uint8_t) g;
        render_target[index + 2] = (uint8_t) b;
        render_target[index + 3] = (uint8_t) a;
      } else if(r != 255 || g != 255 || b != 255){
        /* drawing over something */
        render_target[index    ] = (uint8_t) blend(render_target[index ], r);
        render_target[index + 1] = (uint8_t) blend(render_target[index + 1], g);
        render_target[index + 2] = (uint8_t) blend(render_target[index + 2], b);
        render_target[index + 3] = (uint8_t) blend(render_target[index + 3], a);
      }
    }
  }
  return;
}

void SoftwareRendererImp::fill_pixel(int sx, int sy, Color color){
  int index = 4 * (sx + sy * target_w);
  render_target[index    ] = (uint8_t) (color.r * 255);
  render_target[index + 1] = (uint8_t) (color.g * 255);
  render_target[index + 2] = (uint8_t) (color.b * 255);
  render_target[index + 3] = (uint8_t) (color.a * 255);
}

void SoftwareRendererImp::fill_sample(int x, int y, int ix, int iy, Color fg){
  /* index del subpixel, cada pixel tiene rate*rate subpixeles */
  int index = 4 * (y * sample_rate * sample_w + x * sample_rate * sample_rate + (ix + iy * sample_rate));
  /* color que se encuentra en el buffer */
  Color bg = Color();
  bg.r = sample_buffer[index    ] / 255.0;
  bg.g = sample_buffer[index + 1] / 255.0;
  bg.b = sample_buffer[index + 2] / 255.0;
  bg.a = sample_buffer[index + 3] / 255.0;
  /* mezclar con el que se va a pintar */
  Color color = blend_colors(fg, bg);

  sample_buffer[index    ] = (uint8_t) (color.r * 255);
  sample_buffer[index + 1] = (uint8_t) (color.g * 255);
  sample_buffer[index + 2] = (uint8_t) (color.b * 255);
  sample_buffer[index + 3] = (uint8_t) (color.a * 255);
}

} // namespace CMU462
