#include "texture.h"

#include "config.h"

#include <aem/aem.h>

#include <util/util.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb/stb_image_resize2.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#include <assimp/defs.h>
#include <assimp/scene.h>
#include <assimp/texture.h>
#include <assimp/types.h>

#include <glad/gl.h>

#include <ktx/ktx.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

enum aiTextureType base_color_texture_type = aiTextureType_BASE_COLOR;

void scan_material_structure(const struct aiScene* scene, enum aiTextureType* base_color_texture_type)
{
  bool used_texture_types[AI_TEXTURE_TYPE_MAX];
  memset(used_texture_types, 0, sizeof(used_texture_types));

  for (unsigned int material_index = 0; material_index < scene->mNumMaterials; ++material_index)
  {
    const struct aiMaterial* material = scene->mMaterials[material_index];

    for (unsigned int texture_type = 0; texture_type < AI_TEXTURE_TYPE_MAX; ++texture_type)
    {
      struct aiString path;

      const bool hit =
        (aiGetMaterialTexture(material, texture_type, 0, &path, NULL, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS);
      if (hit)
      {
        used_texture_types[texture_type] = true;
      }
    }
  }

  /*printf("Used texture types:\n");
  for (unsigned int texture_type = 0; texture_type < AI_TEXTURE_TYPE_MAX; ++texture_type)
  {
    if (!used_texture_types[texture_type])
    {
      continue;
    }

    printf("\t%s\n", aiTextureTypeToString(texture_type));
  }*/

  // Determine the base color texture type
  {
    *base_color_texture_type = aiTextureType_BASE_COLOR;

    if (used_texture_types[aiTextureType_DIFFUSE] && used_texture_types[aiTextureType_BASE_COLOR])
    {
      // assert(false);
      return;
    }

    if (used_texture_types[aiTextureType_DIFFUSE])
    {
      *base_color_texture_type = aiTextureType_DIFFUSE;
    }
  }
}

struct aiTexture* init_texture(const struct aiScene* scene,
                               const struct aiMaterial* material,
                               enum aiTextureType type,
                               unsigned int* total_texture_count)
{
  struct aiString path;
  /*enum aiTextureMapping mapping;
  unsigned int uvindex;
  ai_real blend;
  enum aiTextureOp op;
  enum aiTextureMapMode mapmode[2];
  unsigned int flags;*/
  if (aiGetMaterialTexture(material, type, 0, &path, NULL, NULL, NULL, NULL, NULL, NULL) != aiReturn_SUCCESS)
  {
    return NULL;
  }

  aiGetMaterialString(material, _AI_MATKEY_TEXTURE_BASE, type, 0, &path);

  if (path.length <= 1)
  {
    return NULL;
  }

  if (path.data[0] == AI_EMBEDDED_TEXNAME_PREFIX[0])
  {
    // Skip the first character (*) and just read the ID number
    int id = 0;
    if (sscanf(&path.data[1], "%d", &id) != 1)
    {
      return NULL;
    }

    ++(*total_texture_count);
    return scene->mTextures[id];
  }
  else
  {
    // printf("External texture: \"%s\"\n", path.data);
    return NULL;
  }
}

static void* convert_to_2channel(int width, int height, unsigned char* input)
{
  unsigned char* output = malloc(width * height * 2);
  assert(output);
  for (int y = 0; y < height; ++y)
  {
    for (int x = 0; x < width; ++x)
    {
      const int index = y * width + x;
      output[index * 2 + 0] = input[index * 4 + 0];
      output[index * 2 + 1] = input[index * 4 + 1];
    }
  }

  return output;
}

static bool create_mip_level(uint32_t width,
                             uint32_t height,
                             void* data,
                             ktxTexture2** mip_level,
                             enum AEMTextureCompression compression)
{
  {
    ktxTextureCreateInfo textureCreateInfo;
    memset(&textureCreateInfo, 0, sizeof(ktxTextureCreateInfo));

    // 16: VK_FORMAT_R8G8_UNORM
    // 37: VK_FORMAT_R8G8B8A8_UNORM
    // 23: VK_FORMAT_R8G8B8_UNORM
    textureCreateInfo.vkFormat = (compression == AEMTextureCompression_BC5 ? 16 : 37);
    textureCreateInfo.baseWidth = width;
    textureCreateInfo.baseHeight = height;
    textureCreateInfo.baseDepth = 1;
    textureCreateInfo.numDimensions = 2;
    textureCreateInfo.numLevels = 1;
    textureCreateInfo.numLayers = 1;
    textureCreateInfo.numFaces = 1;

    const KTX_error_code result = ktxTexture2_Create(&textureCreateInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, mip_level);
    if (result != KTX_SUCCESS)
    {
      printf("KTX Error: %s\n", ktxErrorString(result));
      return false;
    }
  }

  KTX_error_code result = ktxTexture_SetImageFromMemory(ktxTexture(*mip_level), 0, 0, 0, data, (*mip_level)->dataSize);
  if (result != KTX_SUCCESS)
  {
    printf("KTX Error: %s\n", ktxErrorString(result));
    return false;
  }

  ktxBasisParams params = { 0 };
  memset(&params, 0, sizeof(params));
  params.structSize = sizeof(params);
  params.uastc = false; // Use ETC1S base
  params.compressionLevel = KTX_ETC1S_DEFAULT_COMPRESSION_LEVEL;
  params.normalMap = false;
  result = ktxTexture2_CompressBasisEx(*mip_level, &params);
  if (result != KTX_SUCCESS)
  {
    printf("KTX Error: %s\n", ktxErrorString(result));
    return false;
  }

  /*result = ktxTexture2_CompressBasis(*mip_level, 0);
  if (result != KTX_SUCCESS)
  {
    printf("KTX Error: %s\n", ktxErrorString(result));
    return false;
  }

  assert(ktxTexture2_NeedsTranscoding(*mip_level));
  if (ktxTexture2_NeedsTranscoding(*mip_level))*/
  {
    const KTX_error_code result =
      ktxTexture2_TranscodeBasis(*mip_level,
                                 compression == AEMTextureCompression_BC5 ? KTX_TTF_BC5_RG : KTX_TTF_BC7_RGBA, 0);
    if (result != KTX_SUCCESS)
    {
      printf("KTX Error: %s\n", ktxErrorString(result));
      return false;
    }
  }

  return true;
}

bool save_texture(const struct aiTexture* texture,
                  const char* path,
                  const char* filename,
                  enum TextureType texture_type,
                  enum AEMTextureCompression compression,
                  unsigned int* texture_counter,
                  char* full_filename)
{
  // Extend the filename with the type and extension
  if (texture_type == TextureType_BaseColorOpacity)
  {
    sprintf(full_filename, "%s_BCO.aet", filename); // Null-terminates string
  }
  else if (texture_type == TextureType_Normal)
  {
    sprintf(full_filename, "%s_N.aet", filename); // Null-terminates string
  }
  else if (texture_type == TextureType_OcclusionRoughnessMetalness)
  {
    sprintf(full_filename, "%s_ORM.aet", filename); // Null-terminates string
  }

#ifdef DUMP_TEXTURES
  printf("Texture #%u: \"%s\"\n\tType: Base Color/Opacity\n", *texture_counter, full_filename);
#endif

  ++(*texture_counter);

  stbi_set_flip_vertically_on_load(1);

  char filepath[AEM_STRING_SIZE * 2];
  sprintf(filepath, "%s/%s", path, full_filename); // TODO: Make this longer?

  FILE* output = fopen(filepath, "wb");
  if (!output)
  {
    printf("KTX Error: Failed to open output file \"%s\" for texture writing. Skipping export.\n\n", filepath);
    return false;
  }

  // Load the base texture
  int base_width = 0, base_height = 0;
  uint32_t mip_level_count = 0;
  stbi_uc* base_data = NULL;
  {
    const unsigned int size = ((texture->mHeight == 0) ? texture->mWidth : (texture->mWidth * texture->mHeight));
    base_data = stbi_load_from_memory((stbi_uc*)texture->pcData, size, &base_width, &base_height, NULL, 4);
    if (!base_data)
    {
      return false;
    }

    mip_level_count = floor(log2(max(base_width, base_height))) + 1;
  }

  fwrite(&base_width, sizeof(base_width), 1, output);
  fwrite(&base_height, sizeof(base_height), 1, output);
  fwrite(&mip_level_count, sizeof(mip_level_count), 1, output);

#ifdef DUMP_TEXTURES
  printf("\tBase resolution: %d x %d\n", base_width, base_height);

  if (compression == AEMTextureCompression_None)
  {
    printf("\tCompression: None\n");
  }
  else if (compression == AEMTextureCompression_BC5)
  {
    printf("\tCompression: BC5\n");
  }
  else
  {
    printf("\tCompression: BC7\n");
  }

  printf("\tMip level count: %u\n", mip_level_count);
#endif

  if (compression == AEMTextureCompression_None)
  {
    void** mip_levels = malloc(sizeof(void*) * mip_level_count);
    assert(mip_levels);

    int mip_width = base_width;
    int mip_height = base_height;
    for (uint32_t mip_index = 0; mip_index < mip_level_count; ++mip_index)
    {
      void* mip_data = base_data;
      if (mip_index > 0)
      {
        if (texture_type = TextureType_BaseColorOpacity)
        {
          mip_data = stbir_resize_uint8_srgb(base_data, base_width, base_height, 0, NULL, mip_width, mip_height, 0,
                                             STBIR_4CHANNEL);
        }
        else
        {
          mip_data = stbir_resize_uint8_linear(base_data, base_width, base_height, 0, NULL, mip_width, mip_height, 0,
                                               STBIR_4CHANNEL);
        }

        if (!mip_data)
        {
          return false;
        }
      }

#ifdef EXPORT_TEXTURE_MIP_LEVELS
      char out[AEM_STRING_SIZE * 2 + 10];
      sprintf(out, "%s/%s.#%u.png", path, filename, mip_index);
      stbi_write_png(out, mip_width, mip_height, 4, mip_data, 0);
#endif

      const uint64_t mip_size = mip_width * mip_height * 4;
      fwrite(&mip_size, sizeof(mip_size), 1, output);

#ifdef DUMP_TEXTURES
      printf("\t\tMip level #%u: %u x %u, %llu bytes\n", mip_index, mip_width, mip_height, mip_size);
#endif

      mip_levels[mip_index] = mip_data;

      if (mip_width > 1)
      {
        mip_width >>= 1;
      }

      if (mip_height > 1)
      {
        mip_height >>= 1;
      }
    }

    // Write the texture data of all mip levels
    mip_width = base_width;
    mip_height = base_height;
    for (uint32_t mip_index = 0; mip_index < mip_level_count; ++mip_index)
    {
      fwrite(mip_levels[mip_index], mip_width * mip_height * 4, 1, output);

      if (mip_index > 0)
      {
        free(mip_levels[mip_index]);
      }

      if (mip_width > 1)
      {
        mip_width >>= 1;
      }

      if (mip_height > 1)
      {
        mip_height >>= 1;
      }
    }

    free(mip_levels);
  }
  else
  {
    ktxTexture2** mip_levels = malloc(sizeof(ktxTexture2*) * mip_level_count);
    assert(mip_levels);

    // Create the mip levels and write the texture size for each
    {
      int mip_width = base_width;
      int mip_height = base_height;
      for (uint32_t mip_index = 0; mip_index < mip_level_count; ++mip_index)
      {
        void* mip_data = base_data;
        if (mip_index > 0)
        {
          if (texture_type == TextureType_BaseColorOpacity)
          {
            mip_data = stbir_resize_uint8_srgb(base_data, base_width, base_height, 0, NULL, mip_width, mip_height, 0,
                                               STBIR_4CHANNEL);
          }
          else
          {
            mip_data = stbir_resize_uint8_linear(base_data, base_width, base_height, 0, NULL, mip_width, mip_height, 0,
                                                 STBIR_4CHANNEL);
          }

          if (!mip_data)
          {
            return false;
          }
        }

        void* processed_data = mip_data;

        if (compression == AEMTextureCompression_BC5)
        {
          processed_data = convert_to_2channel(mip_width, mip_height, mip_data);
        }

        if (!create_mip_level(mip_width, mip_height, processed_data, &mip_levels[mip_index], compression))
        {
          return false;
        }

        if (compression == AEMTextureCompression_BC5)
        {
          free(processed_data);
        }

#ifdef EXPORT_TEXTURE_MIP_LEVELS
        char out[AEM_STRING_SIZE * 2 + 2];
        sprintf(out, "%s.#%u.png", filepath, mip_index);
        stbi_write_png(out, mip_width, mip_height, 4, mip_data, 0);
#endif

        const uint64_t mip_size = mip_levels[mip_index]->dataSize;
        fwrite(&mip_size, sizeof(mip_size), 1, output);

#ifdef DUMP_TEXTURES
        printf("\t\tMip level #%u: %u x %u, %llu bytes\n", mip_index, mip_width, mip_height, mip_size);
#endif

        if (mip_index > 0)
        {
          free(mip_data);
        }

        if (mip_width > 1)
        {
          mip_width >>= 1;
        }

        if (mip_height > 1)
        {
          mip_height >>= 1;
        }
      }
    }

    // Write the texture data of all mip levels
    {
      int mip_width = base_width;
      int mip_height = base_height;
      for (uint32_t mip_index = 0; mip_index < mip_level_count; ++mip_index)
      {
        const ktxTexture2* mip_level = mip_levels[mip_index];
        fwrite(mip_level->pData, mip_level->dataSize, 1, output);

        ktxTexture_Destroy(ktxTexture(mip_level));

        if (mip_width > 1)
        {
          mip_width >>= 1;
        }

        if (mip_height > 1)
        {
          mip_height >>= 1;
        }
      }
    }

    free(mip_levels);
  }

  stbi_image_free(base_data);
}

void print_material_properties(const struct aiScene* scene)
{
  for (unsigned int material_index = 0; material_index < scene->mNumMaterials; ++material_index)
  {
    const struct aiMaterial* material = scene->mMaterials[material_index];

    struct aiString material_name;
    aiGetMaterialString(material, AI_MATKEY_NAME, &material_name);
    printf("Material \"%s\"\n", material_name.data);

    for (unsigned int i = 0; i < material->mNumProperties; ++i)
    {
      struct aiMaterialProperty* prop = material->mProperties[i];
      printf("\tProp %u (%s) %s ", i, aiTextureTypeToString(prop->mSemantic), prop->mKey.data);

      if (prop->mType == aiPTI_Float)
      {
        ai_real* f = malloc(prop->mDataLength);
        unsigned int elem_count = prop->mDataLength / sizeof(ai_real);
        aiGetMaterialFloatArray(material, prop->mKey.data, prop->mSemantic, 0, f, &elem_count);
        printf("[float len %u]:", elem_count);
        for (unsigned int elem = 0; elem < elem_count; ++elem)
        {
          printf(" %f", f[elem]);
        }
        printf("\n");
        free(f);
      }
      else if (prop->mType == aiPTI_String)
      {
        struct aiString s;
        aiGetMaterialString(material, prop->mKey.data, prop->mSemantic, 0, &s);
        printf("[string len %u]: %s\n", s.length, s.data);
      }
      else if (prop->mType == aiPTI_Integer)
      {
        int* d = malloc(prop->mDataLength);
        assert(d);
        unsigned int elem_count = prop->mDataLength / sizeof(int);
        aiGetMaterialIntegerArray(material, prop->mKey.data, prop->mSemantic, 0, d, &elem_count);
        printf("[int len %u]:", elem_count);
        for (unsigned int elem = 0; elem < elem_count; ++elem)
        {
          printf(" %d", d[elem]);
        }
        printf("\n");
        free(d);
      }
      else if (prop->mType == aiPTI_Buffer)
      {
        printf("[buffer]\n");
      }
      else
      {
        assert(0);
        printf("[unsupported]\n");
      }
    }
  }
}