#pragma once

#define MATERIAL_FLAG_NONE                0
#define MATERIAL_FLAG_ADDITIVE            (1 << 0)
#define MATERIAL_FLAG_CONST_TEX_COORD     (1 << 1)
#define MATERIAL_FLAG_REFLECTION          (1 << 2)
#define MATERIAL_FLAG_DOUBLE_SIDED        (1 << 3)
#define MATERIAL_FLAG_SOFT_EDGE           (1 << 4)
#define MATERIAL_FLAG_NO_SHADOW           (1 << 5)
#define MATERIAL_FLAG_VIEW_Z_ALPHA_FADE   (1 << 6)
#define MATERIAL_FLAG_HAS_DIFFUSE_TEXTURE (1 << 7)
#define MATERIAL_FLAG_FULBRIGHT           (1 << 8)
#define MATERIAL_FLAG_BLEND_FLIP          (1 << 9)
#define MATERIAL_FLAG_HAS_METALNESS       (1 << 10)