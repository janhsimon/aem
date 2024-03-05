#include "texture.h"

#include <util/util.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#include <assimp/defs.h>
#include <assimp/scene.h>
#include <assimp/texture.h>
#include <assimp/types.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

struct aiTexture* init_texture(const struct aiScene* scene,
                               const struct aiMaterial* material,
                               enum aiTextureType type,
                               unsigned int* total_texture_count)
{
  struct aiString texture_id;
  if (aiGetMaterialTexture(material, type, 0, &texture_id, NULL, NULL, NULL, NULL, NULL, NULL) != aiReturn_SUCCESS)
  {
    return NULL;
  }

  if (texture_id.length <= 1 || texture_id.data[0] != AI_EMBEDDED_TEXNAME_PREFIX[0])
  {
    return NULL;
  }

  // Skip the first character (*) and just read the ID number
  int id = 0;
  if (sscanf(&texture_id.data[1], "%d", &id) != 1)
  {
    return NULL;
  }

  ++(*total_texture_count);
  return scene->mTextures[id];
}

void save_texture(const struct aiTexture* texture, const char* path, const char* filename, int channel_count)
{
  const unsigned int size = ((texture->mHeight == 0) ? texture->mWidth : (texture->mWidth * texture->mHeight));
  int x = 0, y = 0, channels = 0;
  stbi_uc* data = stbi_load_from_memory((stbi_uc*)texture->pcData, size, &x, &y, &channels, channel_count);

  char filepath[STRING_SIZE * 2 + 2];
  sprintf(filepath, "%s/%s", path, filename);
  stbi_flip_vertically_on_write(1);
  stbi_write_png(filepath, x, y, channel_count, data, 0);

  printf("%dx%dx%d)\n", x, y, channel_count);

  stbi_image_free(data);
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