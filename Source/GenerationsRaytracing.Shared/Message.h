#pragma once

#define DEFINE_MSG_ID \
    static constexpr int ID = __LINE__; \
    int id

#define MSG_ALIGNMENT 0x10

#define MSG_ALIGN(x) \
    (((x) + MSG_ALIGNMENT - 1) & ~(MSG_ALIGNMENT - 1))

#define MSG_DATA_PTR(x) \
    ((char*)x + MSG_ALIGN(sizeof(*x)))

struct MsgInitSwapChain
{
    DEFINE_MSG_ID;
    unsigned int width;
    unsigned int height;
    unsigned int bufferCount;
    unsigned int scaling;
    unsigned int handle;
    unsigned int surface;
};

struct MsgPresent
{
    DEFINE_MSG_ID;
};

struct MsgCreateTexture
{
    DEFINE_MSG_ID;
    unsigned int width;
    unsigned int height;
    unsigned int levels;
    unsigned int usage;
    unsigned int format;
    unsigned int texture;
};

struct MsgCreateVertexBuffer
{
    DEFINE_MSG_ID;
    unsigned int length;
    unsigned int vertexBuffer;
};

struct MsgCreateIndexBuffer
{
    DEFINE_MSG_ID;
    unsigned int length;
    unsigned int format;
    unsigned int indexBuffer;
};

struct MsgCreateRenderTarget
{
    DEFINE_MSG_ID;
    unsigned int width;
    unsigned int height;
    unsigned int format;
    unsigned int surface;
};

struct MsgCreateDepthStencilSurface
{
    DEFINE_MSG_ID;
    unsigned int width;
    unsigned int height;
    unsigned int format;
    unsigned int surface;
};

struct MsgSetRenderTarget
{
    DEFINE_MSG_ID;
    unsigned int index;
    unsigned int surface;
};

struct MsgSetDepthStencilSurface
{
    DEFINE_MSG_ID;
    unsigned int surface;
};

struct MsgClear
{
    DEFINE_MSG_ID;
    unsigned int flags;
    unsigned int color;
    float z;
    unsigned int stencil;
};

struct MsgSetViewport
{
    DEFINE_MSG_ID;
    unsigned int x;
    unsigned int y;
    unsigned int width;
    unsigned int height;
    float minZ;
    float maxZ;
};

struct MsgSetRenderState
{
    DEFINE_MSG_ID;
    unsigned int state;
    unsigned int value;
};

struct MsgSetTexture
{
    DEFINE_MSG_ID;
    unsigned int stage;
    unsigned int texture;
};

struct MsgSetSamplerState
{
    DEFINE_MSG_ID;
    unsigned int sampler;
    unsigned int type;
    unsigned int value;
};

struct MsgSetScissorRect
{
    DEFINE_MSG_ID;
    unsigned int left;
    unsigned int top;
    unsigned int right;
    unsigned int bottom;
};

struct MsgDrawPrimitive
{
    DEFINE_MSG_ID;
    unsigned int primitiveType;
    unsigned int startVertex;
    unsigned int primitiveCount;
};

struct MsgDrawIndexedPrimitive
{
    DEFINE_MSG_ID;
    unsigned int primitiveType;
    int baseVertexIndex;
    unsigned int startIndex;
    unsigned int primitiveCount;
};

struct MsgDrawPrimitiveUP
{
    DEFINE_MSG_ID;
    unsigned int primitiveType;
    unsigned int primitiveCount;
    unsigned int vertexStreamZeroSize;
    unsigned int vertexStreamZeroStride;
};

struct MsgCreateVertexDeclaration
{
    DEFINE_MSG_ID;
    unsigned int vertexElementCount;
    unsigned int vertexDeclaration;
};

struct MsgSetVertexDeclaration
{
    DEFINE_MSG_ID;
    unsigned int vertexDeclaration;
};

struct MsgSetFVF
{
    DEFINE_MSG_ID;
    unsigned int vertexDeclaration;
    unsigned int fvf;
};

struct MsgCreateVertexShader
{
    DEFINE_MSG_ID;
    unsigned int shader;
    unsigned int functionSize;
};

struct MsgSetVertexShader
{
    DEFINE_MSG_ID;
    unsigned int shader;
};

struct MsgSetVertexShaderConstantF
{
    DEFINE_MSG_ID;
    unsigned int startRegister;
    unsigned int vector4fCount;
};

struct MsgSetVertexShaderConstantB
{
    DEFINE_MSG_ID;
    unsigned int startRegister;
    unsigned int boolCount;
};

struct MsgSetStreamSource
{
    DEFINE_MSG_ID;
    unsigned int streamNumber;
    unsigned int streamData;
    unsigned int offsetInBytes;
    unsigned int stride;
};

struct MsgSetStreamSourceFreq
{
    DEFINE_MSG_ID;
    unsigned int streamNumber;
    unsigned int setting;
};

struct MsgSetIndices
{
    DEFINE_MSG_ID;
    unsigned int indexData;
};

struct MsgCreatePixelShader
{
    DEFINE_MSG_ID;
    unsigned int shader;
    unsigned int functionSize;
};

struct MsgSetPixelShader
{
    DEFINE_MSG_ID;
    unsigned int shader;
};

struct MsgSetPixelShaderConstantF
{
    DEFINE_MSG_ID;
    unsigned int startRegister;
    unsigned int vector4fCount;
};

struct MsgSetPixelShaderConstantB
{
    DEFINE_MSG_ID;
    unsigned int startRegister;
    unsigned int boolCount;
};

struct MsgMakePicture
{
    DEFINE_MSG_ID;
    unsigned int texture;
    unsigned int size;
    wchar_t name[256];
};

struct MsgWriteBuffer
{
    DEFINE_MSG_ID;
    unsigned int buffer;
    unsigned int size;
};

struct MsgWriteTexture
{
    DEFINE_MSG_ID;
    unsigned int texture;
    unsigned int size;
    unsigned int pitch;
};

struct MsgCreateMesh
{
    DEFINE_MSG_ID;
    unsigned int model;
    unsigned int element;
    unsigned int group;
    bool opaque;
    bool punchThrough;
    unsigned int vertexBuffer;
    unsigned int vertexOffset;
    unsigned int vertexCount;
    unsigned int vertexStride;
    unsigned int indexBuffer;
    unsigned int indexOffset;
    unsigned int indexCount;
    unsigned int normalOffset;
    unsigned int tangentOffset;
    unsigned int binormalOffset;
    unsigned int texCoordOffset;
    unsigned int colorOffset;
    unsigned int colorFormat;
    unsigned int blendWeightOffset;
    unsigned int blendIndicesOffset;
    unsigned int material;
    unsigned int nodeNum;
};

struct MsgCreateModel
{
    DEFINE_MSG_ID;
    unsigned int model;
    unsigned int element;
    unsigned int matrixNum;
};

#define INSTANCE_MASK_NONE         0
#define INSTANCE_MASK_OPAQ_PUNCH   1
#define INSTANCE_MASK_TRANS_WATER  2
#define INSTANCE_MASK_DEFAULT      (INSTANCE_MASK_OPAQ_PUNCH | INSTANCE_MASK_TRANS_WATER)
#define INSTANCE_MASK_SKY          4

struct MsgCreateInstance
{
    DEFINE_MSG_ID;
    float transform[3][4];
    float prevTransform[3][4];
    unsigned int model;
    unsigned int element;
};

struct MsgNotifySceneTraversed
{
    DEFINE_MSG_ID;
    unsigned int resetAccumulation;
};

struct MsgCreateMaterial
{
    DEFINE_MSG_ID;
    unsigned int material;
    char shader[256];
    unsigned int textures[16];
    float parameters[16][4];
};

struct MsgReleaseResource
{
    DEFINE_MSG_ID;
    unsigned int resource;
};

struct MsgReleaseElement
{
    DEFINE_MSG_ID;
    unsigned int model;
    unsigned int element;
};

struct MsgCopyVelocityMap
{
    DEFINE_MSG_ID;
    unsigned int enableBoostBlur;
};