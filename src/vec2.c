#include "vec2.h"

vec2_t vec2_add(vec2_t v1, vec2_t v2) {
  return (vec2_t){
    .x=v1.x + v2.x,
    .y=v1.y + v2.y,
  };
}

