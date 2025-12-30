#include "tracer_renderer.h"

#include <aem/model.h>

#include <cglm/vec3.h>

#include <glad/gl.h>

#include <string.h>

static struct AEMModel* tracer_model = NULL;
static uint32_t tracer_index_count = 0;

static GLuint vao, vertex_buffer, index_buffer;
static GLuint instance_starts, instance_ends;

bool load_tracer_renderer()
{
  // Load tracer model
  if (aem_load_model("models/tracer.aem", &tracer_model) != AEMModelResult_Success)
  {
    return false;
  }

  tracer_index_count = aem_get_model_index_count(tracer_model);

  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  glGenBuffers(1, &vertex_buffer);
  glGenBuffers(1, &index_buffer);
  glGenBuffers(1, &instance_starts);
  glGenBuffers(1, &instance_ends);

  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, aem_get_model_vertex_count(tracer_model) * AEM_VERTEX_SIZE,
               aem_get_model_vertex_buffer(tracer_model), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, aem_get_model_index_count(tracer_model) * AEM_INDEX_SIZE,
               aem_get_model_index_buffer(tracer_model), GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, AEM_VERTEX_SIZE, (void*)(sizeof(float) * 0));

  glBindBuffer(GL_ARRAY_BUFFER, instance_starts);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)(sizeof(float) * 0));
  glVertexAttribDivisor(1, 1);

  glBindBuffer(GL_ARRAY_BUFFER, instance_ends);
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)(sizeof(float) * 0));
  glVertexAttribDivisor(2, 1);

  aem_finish_loading_model(tracer_model);

  return true;
}

void free_tracer_renderer()
{
  glDeleteBuffers(1, &instance_starts);
  glDeleteBuffers(1, &instance_ends);
  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vertex_buffer);
  glDeleteBuffers(1, &index_buffer);

  aem_free_model(tracer_model);
}

void start_tracer_rendering()
{
  glBindVertexArray(vao);
}

void render_tracers(vec3* starts, vec3* ends, uint32_t tracer_count)
{
  if (tracer_count <= 0)
  {
    return;
  }

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glBindBuffer(GL_ARRAY_BUFFER, instance_starts);
  glBufferData(GL_ARRAY_BUFFER, sizeof(starts[0]) * tracer_count, starts, GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, instance_ends);
  glBufferData(GL_ARRAY_BUFFER, sizeof(ends[0]) * tracer_count, ends, GL_DYNAMIC_DRAW);

  glDrawElementsInstanced(GL_TRIANGLES, tracer_index_count, GL_UNSIGNED_INT, 0, tracer_count);
}