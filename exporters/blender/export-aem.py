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
            position = obj.matrix_world @ vertex.co
            if (triangle.use_smooth == True):
              normal = vertex.normal # Use vertex normal when smooth shading
            else:
              normal = triangle.normal # Use face normal when flat shading
            key = (position.freeze(), normal.copy().freeze())
            if key in vertices:
              # Reuse an existing vertex
              indices.append(vertices[key])
            else:
              # Add a new vertex
              vertices[key] = counter
              indices.append(vertices[key])
              counter = counter + 1

      # File header
      file.write(b"AEM")               # Magic number
      file.write(struct.pack("<B", 1)) # Version number
      file.write(struct.pack("<I", len(vertices)))
      file.write(struct.pack("<I", len(indices) // 3))
      file.write(struct.pack("<I", len(meshes)))
      file.write(struct.pack("<I", 0))

      # Vertex section
      for position, normal in vertices:
        # Position
        file.write(struct.pack("<f", position.x))
        file.write(struct.pack("<f", position.z))
        file.write(struct.pack("<f", position.y))

        # Normal
        file.write(struct.pack("<f", normal.x))
        file.write(struct.pack("<f", normal.z))
        file.write(struct.pack("<f", normal.y))

        # Tangent
        file.write(struct.pack("<f", 0))
        file.write(struct.pack("<f", 0))
        file.write(struct.pack("<f", 0))

        # UV
        file.write(struct.pack("<f", 0))
        file.write(struct.pack("<f", 0))

        # Bone indices
        file.write(struct.pack("<I", 0))
        file.write(struct.pack("<I", 0))
        file.write(struct.pack("<I", 0))
        file.write(struct.pack("<I", 0))

        # Bone weights
        file.write(struct.pack("<f", 0))
        file.write(struct.pack("<f", 0))
        file.write(struct.pack("<f", 0))
        file.write(struct.pack("<f", 0))

      # Triangle section
      for index in indices:
        file.write(struct.pack("<I", index))

      # Mesh section
      for obj, mesh in meshes:
        file.write(struct.pack("<I", len(mesh.loop_triangles)))

    self.report({'INFO'}, "AEM export successful.")

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