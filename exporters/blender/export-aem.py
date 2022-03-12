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

def generate_keyframe_list(armatures):
  keyframes = []

  for armature in armatures:
    anim = armature.animation_data
    if anim is None or anim.action is None: return
    for fcurve in anim.action.fcurves:
      for keyframe in fcurve.keyframe_points:
        time = keyframe.co.x
        if time not in keyframes:
          keyframes.append(time)

  keyframes.sort() # Sort chronologically

  # Ensure that the animation starts at time 0
  offset = keyframes[0]
  keyframes = [keyframe - offset for keyframe in keyframes]

  return keyframes

def find_pose_bone(armature, bone):
  for pose_bone in armature.pose.bones:
    if pose_bone.name == bone.name:
      return pose_bone
  return None

def find_bone_parent_index(bone, bones):
  if not bone.parent:
    return -1

  for parent_index, (parent_armature, parent_bone, parent_pose_bone, unused) in enumerate(bones):
    if parent_bone.name == bone.parent.name: #TODO: This could fail for multiple armatures
      return parent_index

  return -1

def generate_bone_list(armatures):
  bones = []

  # Build list without parent index
  for armature in armatures:
    for bone in armature.data.bones:
      pose_bone = find_pose_bone(armature, bone)
      bones.append((armature, bone, pose_bone, -1))

  # Fill parent index in
  bones = [(armature, bone, pose_bone, find_bone_parent_index(bone, bones)) for (armature, bone, pose_bone, unused) in bones]

  return bones

def write_matrix(file, matrix):
  for x in range(4):
    for y in range(4):
      file.write(struct.pack("<f", matrix[y][x])) # Column-major

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
      # Generate lists of keyframes and bones
      armatures = [armature for armature in bpy.context.scene.objects if armature.type ==  "ARMATURE"]
      keyframes = generate_keyframe_list(armatures)
      bones = generate_bone_list(armatures)

      # Create custom vertex and index data
      # This takes vertex/face normals into account for smooth/flat shading respectively
      counter = 0
      vertices = {}
      indices = []
      meshes = [(obj, obj.original.to_mesh()) for obj in bpy.context.scene.objects if obj.type == "MESH"]
      for obj, mesh in meshes:
        mesh.calc_loop_triangles() # Each mesh needs to be triangulated first
        for triangle in mesh.loop_triangles:
          for index in triangle.vertices:
            vertex = mesh.vertices[index]

            # Convert vertex position from mesh to model space
            position = obj.matrix_world @ vertex.co
            
            # Store the smooth or flat shaded normal
            if (triangle.use_smooth == True):
              normal = obj.matrix_world @ vertex.normal # Use vertex normal when smooth shading
            else:
              normal = obj.matrix_world @ triangle.normal # Use face normal when flat shading

            # Create a list of bone weights and ensure it has four elements
            bone_weight_list = [(group.group, group.weight) for group in vertex.groups]
            while len(bone_weight_list) < 4:
              bone_weight_list.append((-1, 0.0))
            if len(bone_weight_list) > 4:
              bone_weight_list = bone_weight_list[:4]

            # Find the corresponding bone indices for the bone weights
            bone_index_list = []
            for group, weight in bone_weight_list:
              if group < 0: continue
              for j, (armature, bone, pose_bone, parent_index) in enumerate(bones):
                if obj.vertex_groups[group].name == bone.name: #TODO: This could fail for multiple armatures
                  bone_index_list.append(j)
                  break

            # Isolate and normalize the bone weights
            bone_weights = mathutils.Vector((0.0, 0.0, 0.0, 0.0))
            for j, (group, weight) in enumerate(bone_weight_list):
              bone_weights[j] = weight
            for weight in bone_weights:
              if bone_weights.length > 0.0: weight /= bone_weights.length

            # Isolate bone indices
            bone_index_0 = bone_index_1 = bone_index_2 = bone_index_3 = 0
            if len(bone_index_list) > 0: bone_index_0 = bone_index_list[0]
            if len(bone_index_list) > 1: bone_index_1 = bone_index_list[1]
            if len(bone_index_list) > 2: bone_index_2 = bone_index_list[2]
            if len(bone_index_list) > 3: bone_index_3 = bone_index_list[3]

            key = (position.freeze(), normal.freeze(), (bone_index_0, bone_index_1, bone_index_2, bone_index_3), bone_weights.freeze())
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
      file.write(struct.pack("<I", len(keyframes)))

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
      for armature, bone, pose_bone, parent_index in bones:
        # Inverse bind pose matrix
        # Transform from bone to armature space, then from armature to model space, then inverse
        bind_pose_matrix = armature.matrix_world @ bone.matrix_local
        write_matrix(file, bind_pose_matrix.inverted())

        # Parent index
        file.write(struct.pack("<i", parent_index))

      # Keyframe section
      old_frame = bpy.context.scene.frame_current
      for keyframe in keyframes:
        file.write(struct.pack("<f", keyframe)) # Keyframe time
        bpy.context.scene.frame_set(keyframe) # TODO: This needs an int but gets a float, causing a warning
        for armature, bone, pose_bone, parent_index in bones:
          transform = pose_bone.matrix
          if pose_bone.parent:
            transform = transform @ pose_bone.parent.matrix.inverted()
          write_matrix(file, transform)
      bpy.context.scene.frame_set(old_frame)

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