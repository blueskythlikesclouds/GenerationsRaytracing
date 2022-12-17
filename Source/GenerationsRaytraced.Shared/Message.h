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
};

struct MsgWriteBuffer
{
    DEFINE_MSG_ID;
    unsigned int buffer;
    unsigned int size;
};

struct MsgExit
{
    DEFINE_MSG_ID;
};

struct MsgReleaseResource
{
    DEFINE_MSG_ID;
    int resource;
};