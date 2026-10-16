#include "util.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct Vector
{
  uint32_t count, capacity; // In elements
  uint32_t element_size;    // In bytes
  void* data;
};

struct Vector* util_make_vector(uint32_t element_size, uint32_t initial_capacity)
{
  assert(element_size > 0);
  assert(initial_capacity > 0);

  struct Vector* v = malloc(sizeof(*v));
  assert(v);

  v->count = 0;
  v->capacity = initial_capacity;
  v->element_size = element_size;

  v->data = malloc(v->capacity * element_size);
  assert(v->data);

  return v;
}

void util_destroy_vector(struct Vector* vector)
{
  assert(vector);
  free(vector->data);
  free(vector);
}

void util_append_vector_element(struct Vector* vector, const void* new_element)
{
  assert(vector);
  assert(new_element);

  // Double capacity if necessary
  if (vector->count >= vector->capacity)
  {
    vector->capacity <<= 1;

    void* new_data = realloc(vector->data, vector->capacity * vector->element_size);
    assert(new_data);

    vector->data = new_data;
  }

  memcpy((uint8_t*)vector->data + vector->count * vector->element_size, new_element, vector->element_size);
  vector->count++;
}

void util_remove_vector_element(struct Vector* vector, uint32_t element_index)
{
  assert(vector);
  assert(element_index < vector->count);

  const uint32_t last_index = vector->count - 1;
  if (element_index < last_index)
  {
    // Override the removed with the last element
    memcpy((uint8_t*)vector->data + element_index * vector->element_size,
           (uint8_t*)vector->data + last_index * vector->element_size, vector->element_size);
  }

  vector->count--;
}

uint32_t util_get_vector_element_count(const struct Vector* vector)
{
  return vector->count;
}

void* util_get_vector_element(struct Vector* vector, uint32_t element_index)
{
  assert(vector);
  assert(element_index < vector->count);

  return (uint8_t*)vector->data + element_index * vector->element_size;
}

void* util_get_vector_raw_data(struct Vector* vector)
{
  assert(vector);
  return vector->data;
}