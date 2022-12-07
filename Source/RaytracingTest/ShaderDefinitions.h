#ifndef SHADER_DEFINITIONS_H
#define SHADER_DEFINITIONS_H

#define MESH_TYPE_OPAQUE				(1 << 0)
#define MESH_TYPE_TRANS					(1 << 1)
#define MESH_TYPE_PUNCH					(1 << 2)
#define MESH_TYPE_SPECIAL				(1 << 3)

#define INSTANCE_MASK_OPAQUE_OR_PUNCH   (1 << 0)
#define INSTANCE_MASK_TRANS_OR_SPECIAL  (1 << 1)
#define INSTANCE_MASK_SKY				(1 << 2)

#endif
