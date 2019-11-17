//Initialization of seeds for ray tracing
//number of blocks we want to consider for our calculation
#define BLOCK_SIZE pow(LINE_SIZE, 2)
#define LINE_SIZE 8

/**struct frame_info {
    uint width;
    uint height;
    uint number_blocks_per_row;
};*/

//necessary information about picture
struct frameInfo
{
    uint uintVal; // We're using 3 values. [0]: width [1]: height [2]:number_blocks_per_row
};

StructuredBuffer<frameInfo> gInfo[1];

//we need a texture to store our seeds to!
RWStructuredBuffer<uint> seed_Texture;

//function for generate random numbers
// http://www.reedbeta.com/blog/quick-and-easy-gpu-random-numbers-in-d3d11/
uint wang_hash(uint seed)
{
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return seed;
}

[numthreads(LINE_SIZE, LINE_SIZE, 1)]
void InitSeeds(uint group_Index : SV_GROUPINDEX, uint2 group_ID : SV_GROUPID, uint2 thread_ID : SV_DISPATCHTHREADID) {
    uint width = gInfo[0][1].uintVal;
    uint height = gInfo[0][2].uintVal;
    uint number_blocks_per_row = gInfo[0][3].uintVal;
    uint wang_index = group_Index + (group_ID.y * number_blocks_per_row * BLOCK_SIZE  
                                        + group_ID.x * BLOCK_SIZE);
    seed_Texture[width * thread_ID.y + thread_ID.x] = wang_hash(wang_index);

}

