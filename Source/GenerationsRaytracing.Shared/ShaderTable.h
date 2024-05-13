#pragma once

#define RAY_GENERATION_PRIMARY           0
#define RAY_GENERATION_SECONDARY         1
#define RAY_GENERATION_NUM               2

#define HIT_GROUP_PRIMARY                0
#define HIT_GROUP_PRIMARY_TRANSPARENT    1
#define HIT_GROUP_SECONDARY              2
#define HIT_GROUP_NUM                    3

#define MISS_PRIMARY                     0
#define MISS_PRIMARY_TRANSPARENT         1
#define MISS_SECONDARY                   2
#define MISS_SECONDARY_ENVIRONMENT_COLOR 3
#define MISS_NUM                         4

#define INSTANCE_MASK_DEFAULT            1

#define INSTANCE_TYPE_OPAQUE             0
#define INSTANCE_TYPE_TRANSPARENT        1
#define INSTANCE_TYPE_NUM                2