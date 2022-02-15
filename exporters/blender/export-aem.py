bl_info = {
  "name": "AEM format",
  "category": "Import-Export",
  "author": "janhsimon",
  "version": (0, 1),
  "blender": (2, 93, 4),
  "location": "File > Export",
  "description": "Export AEM files from Blender"
}

import bpy
import mathutils
import struct

from bpy_extras.io_utils import ExportHelper

class Exporter(bpy.types.Operator, ExportHelper):
  """Export scene as AEM file"""
  bl_idname  = "export_aem.exporter"
  bl_label   = "Export AEM"
  bl_options = {"PRESET"}

  filename_ext = ".aem"

  def draw(self, context):
    layout = self.layout
    box = layout.box()
    col = box.column(align = True)
    col.label(text="AEM Version 1")

  def execute(self, context):
    with open(self.filepath, "wb") as file:
      # Grab a list of all meshes in the scene
      meshes = [(obj, obj.original.to_mesh()) for obj in bpy.context.scene.objects if obj.type == "MESH"]

      # Construct a list of bones and store the armature transform
      bones = []
      armature_transform = mathutils.Matrix()
      armature_found = False
      for obj, mesh in meshes:
        for modifier in obj.modifiers:
          if modifier.type == "ARMATURE":
            armature_found = True
            armature_transform = obj.matrix_local.inverted() # Transforms from armature to mesh space
            for bone in obj.parent.data.bones:
              bones.append((obj, bone))
        if armature_found == True:
          break

      # Create custom vertex and index data
      # This takes vertex/face normals into account for smooth/fat shading respectively
      counter = 0
      vertices = {}
      indices = []
      for obj, mesh in meshes:
        mesh.calc_loop_triangles() # Each mesh needs to be triangulated first
        for triangle in mesh.loop_triangles:
          for i in range(3):
            index = triangle.vertices[i]
            vertex = mesh.vertices[index]

            # Convert vertex position from mesh to model space
            position = obj.matrix_world @ vertex.co
            
            # Store the smooth or flat shaded normal
            if (triangle.use_smooth == True):
              normal = vertex.normal # Use vertex normal when smooth shading
            else:
              normal = triangle.normal # Use face normal when flat shading

            # Create a list of bone weights and ensure it has four elements
            bone_weight_list = [(group.group, group.weight) for group in vertex.groups]
            while len(bone_weight_list) < 4:
              bone_weight_list.append((-1, 0.0))
            if len(bone_weight_list) > 4:
              bone_weight_list = bone_weight_list[:4]

            # Find the corresponding bone indices for the bone weights
            bone_index_list = []
            for group, weight in bone_weight_list:
              for j, (obj, bone) in enumerate(bones):
                if obj.vertex_groups[group].name == bone.name:
                  bone_index_list.append(j)
                  break

            # Isolate and normalize the bone weights
            bone_weights = mathutils.Vector((0.0, 0.0, 0.0, 0.0))
            for j, (group, weight) in enumerate(bone_weight_list):
              bone_weights[j] = weight
            for weight in bone_weights:
              weight /= bone_weights.length

            # Isolate bone indices
            bone_index_0 = bone_index_list[0]
            bone_index_1 = bone_index_list[1]
            bone_index_2 = bone_index_list[2]
            bone_index_3 = bone_index_list[3]

            key = (position.freeze(), normal.copy().freeze(), (bone_index_0, bone_index_1, bone_index_2, bone_index_3), bone_weights.freeze())
            if key in vertices:
              # Reuse an existing vertex
              indices.append(vertices[key])
            else:
              # Add a new vertex
              vertices[key] = counter
              indices.append(vertices[key])
              counter += 1

      # File header
      file.write(b"AEM")               # Magic number
      file.write(struct.pack("<B", 1)) # Version number
      file.write(struct.pack("<I", len(vertices)))
      file.write(struct.pack("<I", len(indices) // 3))
      file.write(struct.pack("<I", len(meshes)))
      file.write(struct.pack("<I", len(bones)))

      # Vertex section
      for position, normal, (bone_index_0, bone_index_1, bone_index_2, bone_index_3), bone_weight in vertices:
        # Position
        file.write(struct.pack("<f", position.x))
        file.write(struct.pack("<f", position.y))
        file.write(struct.pack("<f", position.z))

        # Normal
        file.write(struct.pack("<f", normal.x))
        file.write(struct.pack("<f", normal.y))
        file.write(struct.pack("<f", normal.z))

        # Tangent
        file.write(struct.pack("<f", 0))
        file.write(struct.pack("<f", 0))
        file.write(struct.pack("<f", 0))

        # UV
        file.write(struct.pack("<f", 0))
        file.write(struct.pack("<f", 0))

        # Bone indices
        file.write(struct.pack("<I", bone_index_0))
        file.write(struct.pack("<I", bone_index_1))
        file.write(struct.pack("<I", bone_index_2))
        file.write(struct.pack("<I", bone_index_3))

        # Bone weights
        file.write(struct.pack("<f", bone_weight[0]))
        file.write(struct.pack("<f", bone_weight[1]))
        file.write(struct.pack("<f", bone_weight[2]))
        file.write(struct.pack("<f", bone_weight[3]))

      # Triangle section
      for index in indices:
        file.write(struct.pack("<I", index))

      # Mesh section
      for obj, mesh in meshes:
        file.write(struct.pack("<I", len(mesh.loop_triangles)))

      # Bone section
      for obj, bone in bones:
        # Transform from bone in bind pose to armature space, then to mesh and then to model space
        inverse_bind_pose_matrix = obj.matrix_world @ armature_transform @ bone.matrix_local
        inverse_bind_pose_matrix.invert()
        for x in range(4):
          for y in range(4):
            file.write(struct.pack("<f", inverse_bind_pose_matrix[y][x])) # Column-major

    self.report({"INFO"}, "AEM export successful.")

    return {"FINISHED"}


def create_menu(self, context):
  self.layout.operator(Exporter.bl_idname, text="AEM (.aem)")

def register():
  bpy.utils.register_class(Exporter)
  bpy.types.TOPBAR_MT_file_export.append(create_menu)

def unregister():
  bpy.types.TOPBAR_MT_file_export.remove(create_menu)
  bpy.utils.unregister_class(Exporter)


if (__name__ == "__main__"):
  register()