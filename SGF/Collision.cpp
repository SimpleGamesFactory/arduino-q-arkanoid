#include "Collision.h"

bool circleRectHit(int cx, int cy, int r, int x0, int y0, int x1, int y1) {
  int nx = cx;
  if (nx < x0) nx = x0;
  else if (nx > x1) nx = x1;

  int ny = cy;
  if (ny < y0) ny = y0;
  else if (ny > y1) ny = y1;

  int dx = cx - nx;
  int dy = cy - ny;
  return dx * dx + dy * dy <= r * r;
}
