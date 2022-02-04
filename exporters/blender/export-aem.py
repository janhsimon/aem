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
      meshes = [obj.original.to_mesh() for obj in bpy.context.scene.objects if obj.type == "MESH"]

      # Calculate the total number of vertices and triangles in the scene
      num_vertices = num_triangles = 0
      for mesh in meshes:
        num_vertices += len(mesh.vertices)

        mesh.calc_loop_triangles() # Triangulate each mesh first
        num_triangles += len(mesh.loop_triangles)

      # File header
      file.write(b"AEM")               # Magic number
      file.write(struct.pack("<B", 1)) # Version number
      file.write(struct.pack("<I", num_vertices))
      file.write(struct.pack("<I", num_triangles))
      file.write(struct.pack("<I", len(meshes)))
      file.write(struct.pack("<I", 0))

      # Vertex section
      for mesh in meshes:
        for vertex in mesh.vertices:
          # Position
          file.write(struct.pack("<f", vertex.co[0]))
          file.write(struct.pack("<f", vertex.co[1]))
          file.write(struct.pack("<f", vertex.co[2]))

          # Normal
          file.write(struct.pack("<f", vertex.normal[0]))
          file.write(struct.pack("<f", vertex.normal[1]))
          file.write(struct.pack("<f", vertex.normal[2]))

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
      for mesh in meshes:
        for loop in mesh.loop_triangles:
          file.write(struct.pack("<I", loop.vertices[0]))
          file.write(struct.pack("<I", loop.vertices[1]))
          file.write(struct.pack("<I", loop.vertices[2]))

      # Mesh section
      for mesh in meshes:
        file.write(struct.pack("<I", len(mesh.loop_triangles)))

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