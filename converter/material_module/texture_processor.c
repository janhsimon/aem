#include "material_module.h"

#include "render_texture.h"
#include "texture_transform.h"

#include "config.h"

#include <util/util.h>

#include <cgltf/cgltf.h>

#include <glad/gl.h>

#include <glfw/glfw3.h>

#include <cglm/mat3.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

#include <assert.h>

static GLint texture_type_uniform_location, color_uniform_location, alpha_mode_uniform_location,
  alpha_mask_threshold_uniform_location, pbr_workflow_uniform_location, texture_bound_uniform_location;

static GLuint load_opengl_texture(const RenderTexture* render_texture,
                                  const cgltf_image* image,
                                  uint32_t* max_width,
                                  uint32_t* max_height,
                                  const char* path)
{
  // Load the embedded base image from the input file
  stbi_uc* base_data;
  int base_width, base_height;
  if (image->buffer_view)
  {
    // Internal image loading
    const cgltf_buffer_view* buffer_view = image->buffer_view;
    const stbi_uc* memory = (const stbi_uc*)cgltf_buffer_view_data(buffer_view);
    base_data = stbi_load_from_memory(memory, buffer_view->size, &base_width, &base_height, NULL, STBI_rgb_alpha);
    assert(base_data);
  }
  else
  {
    // External image loading
    assert(image->uri);
    char filepath[256];
    sprintf(filepath, "%s/%s", path, image->uri);
    base_data = stbi_load(filepath, &base_width, &base_height, NULL, STBI_rgb_alpha);
    assert(base_data);
  }

  // Treat the RGB triplet as sRGB for base color textures
  // OpenGL then automatically converts the values to linear when sampling in the fragment shader
  const GLint internal_format = (render_texture->type == RenderTextureType_BaseColor ? GL_SRGB8_ALPHA8 : GL_RGBA8);

  // Create an OpenGL texture from the GLB texture
  GLuint tex;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexImage2D(GL_TEXTURE_2D, 0, internal_format, base_width, base_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, base_data);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLint)render_texture->wrap_mode[0]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLint)render_texture->wrap_mode[1]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  stbi_image_free(base_data);

  *max_width = (base_width > *max_width) ? base_width : *max_width;
  *max_height = (base_height > *max_height) ? base_height : *max_height;

  return tex;
}

void process_textures(const char* path, RenderTexture* render_textures, uint32_t render_texture_count)
{
  // Start renderer
  GLFWwindow* window = NULL;
  {
    int result = glfwInit();
    assert(result);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    window = glfwCreateWindow(1, 1, "", NULL, NULL);
    assert(window);

    glfwMakeContextCurrent(window);
    result = gladLoadGL();
    assert(result);
  }

  // Create and start using shader program
  GLuint shader_program;
  {
    GLuint vertex_shader, fragment_shader;
    {
      const bool result = load_shader("shaders/texture.vert.glsl", GL_VERTEX_SHADER, &vertex_shader);
      assert(result);
    }

    {
      const bool result = load_shader("shaders/texture.frag.glsl", GL_FRAGMENT_SHADER, &fragment_shader);
      assert(result);
    }

    {
      const bool result = generate_shader_program(vertex_shader, fragment_shader, &shader_program);
      assert(result);
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    glUseProgram(shader_program);

    // Retrieve uniform locations
    texture_type_uniform_location = get_uniform_location(shader_program, "texture_type");
    color_uniform_location = get_uniform_location(shader_program, "color");
    alpha_mode_uniform_location = get_uniform_location(shader_program, "alpha_mode");
    alpha_mask_threshold_uniform_location = get_uniform_location(shader_program, "alpha_mask_threshold");
    pbr_workflow_uniform_location = get_uniform_location(shader_program, "pbr_workflow");
    texture_bound_uniform_location = get_uniform_location(shader_program, "texture_bound");

    // Set constant uniforms
    {
      const GLint samplers[] = { 0, 1, 2 };
      const GLint textures_uniform_location = get_uniform_location(shader_program, "textures");
      glUniform1iv(textures_uniform_location, 3, samplers);
    }
  }

  // Create a dummy VAO and an FBO
  GLuint vao, fbo;
  {
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  }

  for (cgltf_size texture_index = 0; texture_index < render_texture_count; ++texture_index)
  {
    RenderTexture* render_texture = &render_textures[texture_index];

    render_texture->width = render_texture->height = 1; // Avoid zero size in case no image or images exist

    // Create source textures for each image, note their maximum dimensions and keep track of which textures exist
    GLuint source_textures[3];
    GLint source_texture_exists[3] = { false, false, false };
    if (render_texture->type == RenderTextureType_BaseColor)
    {
      const cgltf_image* image = render_texture->base_color.image;
      if (image)
      {
        source_textures[0] =
          load_opengl_texture(render_texture, image, &render_texture->width, &render_texture->height, path);
        source_texture_exists[0] = true;
      }
      else
      {
        render_texture->width = render_texture->height = 1;
      }
    }
    else if (render_texture->type == RenderTextureType_Normal)
    {
      const cgltf_image* image = render_texture->normal.image;
      if (image)
      {
        source_textures[0] =
          load_opengl_texture(render_texture, image, &render_texture->width, &render_texture->height, path);
        source_texture_exists[0] = true;
      }
      else
      {
        render_texture->width = render_texture->height = 1;
      }
    }
    else if (render_texture->type == RenderTextureType_PBR)
    {
      const cgltf_image* metallic_roughness_image = render_texture->pbr.metallic_roughness_image;
      if (metallic_roughness_image)
      {
        source_textures[0] = load_opengl_texture(render_texture, metallic_roughness_image, &render_texture->width,
                                                 &render_texture->height, path);
        source_texture_exists[0] = true;
      }

      const cgltf_image* occlusion_image = render_texture->pbr.occlusion_image;
      if (occlusion_image)
      {
        source_textures[1] =
          load_opengl_texture(render_texture, occlusion_image, &render_texture->width, &render_texture->height, path);
        source_texture_exists[1] = true;
      }

      const cgltf_image* emissive_image = render_texture->pbr.emissive_image;
      if (emissive_image)
      {
        source_textures[2] =
          load_opengl_texture(render_texture, emissive_image, &render_texture->width, &render_texture->height, path);
        source_texture_exists[2] = true;
      }
    }

    // Determine the number of mip levels required
    const cgltf_size mip_level_count =
      1 + (cgltf_size)floor(log2f((float)max(render_texture->width, render_texture->height)));

    // Create target texture with mip levels and count the size of the image data
    GLuint target_texture;
    {
      glGenTextures(1, &target_texture);
      glBindTexture(GL_TEXTURE_2D, target_texture);

      for (cgltf_size level_index = 0; level_index < mip_level_count; ++level_index)
      {
        const uint32_t level_width = max(1, render_texture->width >> level_index);
        const uint32_t level_height = max(1, render_texture->height >> level_index);
        glTexImage2D(GL_TEXTURE_2D, level_index, GL_RGBA8, level_width, level_height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     NULL);

        render_texture->data_size += level_width * level_height * 4;
      }

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    }

    render_texture->data = malloc(render_texture->data_size);

    // Bind the source textures for each image
    for (cgltf_size texture_index = 0; texture_index < 3; ++texture_index)
    {
      if (source_texture_exists[texture_index])
      {
        glActiveTexture(GL_TEXTURE0 + texture_index);
        glBindTexture(GL_TEXTURE_2D, source_textures[texture_index]);
      }
    }

    // Set shader uniforms
    {
      glUniform1i(texture_type_uniform_location, (GLint)render_texture->type);

      if (render_texture->type == RenderTextureType_BaseColor)
      {
        glUniform4fv(color_uniform_location, 1, render_texture->base_color.color);
        glUniform1i(alpha_mode_uniform_location, (GLint)render_texture->base_color.alpha_mode);
        glUniform1f(alpha_mask_threshold_uniform_location, render_texture->base_color.alpha_mask_threshold);
      }
      else if (render_texture->type == RenderTextureType_PBR)
      {
        glUniform4fv(color_uniform_location, 1, render_texture->pbr.factors);
        glUniform1i(pbr_workflow_uniform_location, (GLint)render_texture->pbr.workflow);
      }

      glUniform1iv(texture_bound_uniform_location, 3, source_texture_exists);
    }

    // Render the base texture (mip level 0)
    {
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target_texture, 0);
      assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

      glViewport(0, 0, render_texture->width, render_texture->height);

      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    // Bind the target texture and generate a full mip chain
    glBindTexture(GL_TEXTURE_2D, target_texture);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Read pixels for each mip level
    {
      cgltf_size offset = 0;
      for (cgltf_size level_index = 0; level_index < mip_level_count; ++level_index)
      {
        const uint32_t level_width = max(1, render_texture->width >> level_index);
        const uint32_t level_height = max(1, render_texture->height >> level_index);

        uint8_t* level_data = &render_texture->data[offset];

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target_texture, level_index);
        assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

        glReadPixels(0, 0, level_width, level_height, GL_RGBA, GL_UNSIGNED_BYTE, level_data);

#ifdef DUMP_TEXTURES
        if (DUMP_TEXTURE_COUNT == 0 || texture_index < DUMP_TEXTURE_COUNT)
        {
          if (DUMP_TEXTURE_LEVEL_COUNT == 0 || level_index < DUMP_TEXTURE_LEVEL_COUNT)
          {
            char buffer[256];
            if (render_texture->type == RenderTextureType_BaseColor)
            {
              sprintf(buffer, "%s\\texture%llu_level%llu_basecolor.png", path, texture_index, level_index);
            }
            else if (render_texture->type == RenderTextureType_Normal)
            {
              sprintf(buffer, "%s\\texture%llu_level%llu_normal.png", path, texture_index, level_index);
            }
            else if (render_texture->type == RenderTextureType_PBR)
            {
              sprintf(buffer, "%s\\texture%llu_level%llu_pbr.png", path, texture_index, level_index);
            }
            else
            {
              assert(false);
            }

            stbi_write_png(buffer, level_width, level_height, 4, level_data, 0);
          }
        }
#endif

        offset += level_width * level_height * 4;
      }
    }

    // Cleanup the resources used by this render texture
    {
      glDeleteTextures(1, &target_texture);

      glDeleteTextures(1, &source_textures[0]);

      if (render_texture->type == RenderTextureType_PBR)
      {
        glDeleteTextures(2, &source_textures[1]);
      }
    }
  }

  // Cleanup the remaining resources
  {
    glDeleteFramebuffers(1, &fbo);
    glDeleteVertexArrays(1, &vao);

    glfwDestroyWindow(window);
    glfwTerminate();
  }
}