// Memory manager functions
// J Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "mm.h"

#define HEAP_SIZE       28672
#define SUBREGIONS      4
#define NULL            0
#define MAX_SIZE        13

#define BLOCK_SIZE1     512
#define BLOCK_SIZE2     1024
#define BLOCK_SIZE3     1536

#define ROS_BASE_ADD     0x20000000
#define ROS_END_ADD      0x20000FFF

#define R4K1_BASE_ADD    0x20001000
#define R4K1_END_ADD     0x200017FF

#define R1_5K_BASE_ADD   0x20001E00
#define R1_5K_END_ADD    0x20001FFF

#define R8K1_BASE_ADD    0x20002400
#define R8K1_END_ADD     0x20003FFF

#define R8K2_BASE_ADD    0x20004000
#define R8K2_END_ADD     0x20005FFF

#define R8K3_BASE_ADD    0x20006000
#define R8K3_END_ADD     0x20007FFF

#define B4K1_START_INDEX 0
#define B4K1_END_INDEX   6

#define B1_5K_START_INDEX 7
#define B1_5K_END_INEX    8

#define B8K1_START_INDEX 9
#define B8K1_END_INDEX   15

#define B8K2_START_INDEX 16
#define B8K2_END_INDEX   23

#define B8K3_START_INDEX 24
#define B8K3_END_INDEX   31

#define NVIC_MPU_NUMBER_FLASH       0x00000001
#define NVIC_MPU_ATTR_AP_KERNEL     0x01000000
#define NVIC_MPU_ATTR_AP_FULL       0x03000000
#define NVIC_MPU_ATTR_TEX_NORMAL    0x00000000
#define NVIC_MPU_ATTR_SIZE_FULL     (31 << 1)
#define NVIC_MPU_ATTR_SIZE_FLASH    (17 << 1)
#define NVIC_MPU_ATTR_SIZE_8K       (12 << 1)
#define NVIC_MPU_ATTR_SIZE_4K       (11 << 1)

#define NVIC_MPU_R4_8K3    0x00000006  // SRAM region 4
#define NVIC_MPU_R3_8K2    0x00000005  // SRAM region 3
#define NVIC_MPU_R2_8K1    0x00000004  // SRAM region 2
#define NVIC_MPU_R1_4K1    0x00000003  // SRAM region 1
#define NVIC_MPU_R0_OS     0x00000002  // SRAM region 0

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

typedef struct METADATA_
{
    void* baseAddress;
    uint8_t pid;
    uint8_t allocated;
}memoryMetadata[MAX_SIZE];

memoryMetadata heapAttributes = {{NULL,}, {0,}, {0,}};

bool subRegionFree[32];
int8_t procIndex = -1;

// Allocate memory based on the requirement
void* allocateMemory(uint8_t no_of_Sub_Regions, uint32_t baseAdd, uint8_t startIndex, uint8_t endIndex, uint16_t blockSize)
{
    uint8_t i, isFree = 0, firstFree = 0;

    for(i = startIndex; i <= endIndex; i++)
    {
        if(subRegionFree[i] == 0)
        {
            if(isFree == 0)
                firstFree = i;
            isFree++;
            if(isFree == no_of_Sub_Regions)
            {
                while(isFree > 0)
                {
                    subRegionFree[i] = 1;            // indicate that the sub-region has been allotted
                    isFree--;
                    i--;
                }
                heapAttributes[procIndex].pid = procIndex;
                heapAttributes[procIndex].allocated = no_of_Sub_Regions;
                heapAttributes[procIndex].baseAddress = (void*)(baseAdd + ((firstFree - startIndex) * blockSize));
                return (void*) heapAttributes[procIndex].baseAddress;
            }
        }
        else
            isFree = 0;
    }
    return (void*) NULL;
}

// REQUIRED: add your malloc code here and update the SRD bits for the current thread
void * mallocFromHeap(uint32_t size_in_bytes)
{
    void* allocAdd;
    uint8_t number;
    procIndex++;
    if(size_in_bytes <= BLOCK_SIZE1)
    {
        size_in_bytes = BLOCK_SIZE1;
        number = size_in_bytes / BLOCK_SIZE1;
        allocAdd = allocateMemory(number, R4K1_BASE_ADD, B4K1_START_INDEX, B4K1_END_INDEX, BLOCK_SIZE1);
        return allocAdd;
    }
    else if((size_in_bytes > BLOCK_SIZE1) && (size_in_bytes <= BLOCK_SIZE2))
    {
        size_in_bytes = BLOCK_SIZE2;
        number = size_in_bytes / BLOCK_SIZE2;
        allocAdd = allocateMemory(number, R8K1_BASE_ADD, B8K1_START_INDEX, B8K3_END_INDEX, BLOCK_SIZE2);
        return allocAdd;
    }
    else
    {
        if(size_in_bytes == BLOCK_SIZE3)
        {
            size_in_bytes = BLOCK_SIZE3;
            allocAdd = allocateMemory(2, R1_5K_BASE_ADD, B1_5K_START_INDEX, B8K1_START_INDEX, BLOCK_SIZE1);
            return (void*) allocAdd;
        }
        else
        {
            number = (size_in_bytes + BLOCK_SIZE2 - 1) / BLOCK_SIZE2;
            allocAdd = allocateMemory(number, R8K1_BASE_ADD, B8K1_START_INDEX, B8K3_END_INDEX, BLOCK_SIZE2);
            return allocAdd;
        }
    }
}

// REQUIRED: add your free code here and update the SRD bits for the current thread
void freeToHeap(void *pMemory)
{
    uint8_t i, j;

    // Find the metadata entry for the given address
    for(i = 0; i < MAX_SIZE; i++)
    {
        if (heapAttributes[i].baseAddress == pMemory && heapAttributes[i].allocated)
        {
            uint32_t baseAdd = (uint32_t)heapAttributes[i].baseAddress;
            uint8_t no_of_Sub_Regions = heapAttributes[i].allocated;
            uint32_t regionStart;

            // Determine the correct region and subregion size based on the address
            if (baseAdd >= R4K1_BASE_ADD && baseAdd < R4K1_END_ADD)
            {
                regionStart = B4K1_START_INDEX;
            }
            else if(baseAdd >= R1_5K_BASE_ADD && baseAdd < R8K1_BASE_ADD)
            {
                regionStart = B1_5K_START_INDEX;
            }
            else if (baseAdd >= R8K1_BASE_ADD && baseAdd < R8K2_BASE_ADD)
            {
                regionStart = B8K1_START_INDEX;
            }
            else if (baseAdd >= R8K2_BASE_ADD && baseAdd < R8K3_BASE_ADD)
            {
                regionStart = B8K2_START_INDEX;
            }
            else if (baseAdd >= R8K3_BASE_ADD && baseAdd < R8K3_END_ADD)
            {
                regionStart = B8K3_START_INDEX;
            }
            else
                continue;

            // Mark sub-regions as free
            for (j = regionStart; j < regionStart + no_of_Sub_Regions; j++)
            {
                if (j < 32)                         // to make sure the free action doesnt exceed the subregion array size
                {
                    subRegionFree[j] = 0;           // mark the sub-region as freed
                }
            }

            // Reset metadata
            heapAttributes[i].baseAddress = NULL;
            heapAttributes[i].pid = 0;
            heapAttributes[i].allocated = 0;
            procIndex--;
            break;
        }
    }
}

// Set the background rules
void setBackgroundRule()
{
    // set the background region rule
    NVIC_MPU_NUMBER_R |= NVIC_MPU_NUMBER_S;

    // set the base address of the background region
    NVIC_MPU_BASE_R |= NVIC_MPU_BASE_ADDR_M;
    NVIC_MPU_BASE_R &= ~(NVIC_MPU_BASE_VALID);

    // set the attributes into the attributes register
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_XN | NVIC_MPU_ATTR_SHAREABLE | NVIC_MPU_ATTR_CACHEABLE | NVIC_MPU_ATTR_BUFFRABLE | NVIC_MPU_ATTR_ENABLE | NVIC_MPU_ATTR_AP_FULL | NVIC_MPU_ATTR_SIZE_FULL;
}

// Allow Flash Access
void allowFlashAccess(void)
{
    NVIC_MPU_NUMBER_R   |= NVIC_MPU_NUMBER_FLASH;

    NVIC_MPU_BASE_R     |= 0x00000000;

    NVIC_MPU_ATTR_R     |= NVIC_MPU_ATTR_AP_FULL | NVIC_MPU_ATTR_CACHEABLE | NVIC_MPU_ATTR_SIZE_FLASH | NVIC_MPU_ATTR_ENABLE;
}

//void allowPeripheralAccess(void)
//{
//}

// setup SRAM access
void setupSramAccess(void)
{
    // for the OS region
    NVIC_MPU_NUMBER_R = NVIC_MPU_R0_OS;
    NVIC_MPU_BASE_R = ROS_BASE_ADD;
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_XN | NVIC_MPU_ATTR_AP_KERNEL | NVIC_MPU_ATTR_TEX_NORMAL | NVIC_MPU_ATTR_SHAREABLE | NVIC_MPU_ATTR_CACHEABLE | 0 | NVIC_MPU_ATTR_SIZE_4K | NVIC_MPU_ATTR_ENABLE;

    // for the 4K1 region
    NVIC_MPU_NUMBER_R   = NVIC_MPU_R1_4K1;
    NVIC_MPU_BASE_R     = R4K1_BASE_ADD;
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_XN | NVIC_MPU_ATTR_AP_KERNEL | NVIC_MPU_ATTR_TEX_NORMAL | NVIC_MPU_ATTR_SHAREABLE | NVIC_MPU_ATTR_CACHEABLE | 0 | NVIC_MPU_ATTR_SIZE_4K | NVIC_MPU_ATTR_ENABLE;

    // for the 8K1 region
    NVIC_MPU_NUMBER_R   = NVIC_MPU_R2_8K1;
    NVIC_MPU_BASE_R     = R8K1_BASE_ADD;
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_XN | NVIC_MPU_ATTR_AP_KERNEL | NVIC_MPU_ATTR_TEX_NORMAL | NVIC_MPU_ATTR_SHAREABLE | NVIC_MPU_ATTR_CACHEABLE | 0 | NVIC_MPU_ATTR_SIZE_8K | NVIC_MPU_ATTR_ENABLE;

    // for the 8K2 region
    NVIC_MPU_NUMBER_R   = NVIC_MPU_R3_8K2;
    NVIC_MPU_BASE_R     = R8K2_BASE_ADD;
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_XN | NVIC_MPU_ATTR_AP_KERNEL | NVIC_MPU_ATTR_TEX_NORMAL | NVIC_MPU_ATTR_SHAREABLE | NVIC_MPU_ATTR_CACHEABLE | 0 | NVIC_MPU_ATTR_SIZE_8K | NVIC_MPU_ATTR_ENABLE;

    // for the 8K3 region
    NVIC_MPU_NUMBER_R = NVIC_MPU_R4_8K3;
    NVIC_MPU_BASE_R = R8K3_BASE_ADD;
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_XN |NVIC_MPU_ATTR_AP_KERNEL | NVIC_MPU_ATTR_TEX_NORMAL | NVIC_MPU_ATTR_SHAREABLE | NVIC_MPU_ATTR_CACHEABLE | (0x80 << 8) | NVIC_MPU_ATTR_SIZE_8K | NVIC_MPU_ATTR_ENABLE;
}

// create no sram access
uint64_t createNoSramAccessMask(void)
{
    return 0x00000000;
}

// sram access window
void addSramAccessWindow(uint64_t *srdBitMask, uint32_t *baseAdd, uint32_t size_in_bytes)
{
    uint8_t j;
    uint16_t blockSize;
    uint32_t regionBaseAdd = 0;
    uint32_t allocated;
//    uint32_t startIndex = 0;

    // Determine the base address and subregion size based on the input address
    if ((uint32_t)baseAdd >= R4K1_BASE_ADD && (uint32_t)baseAdd < R4K1_END_ADD)
    {
        regionBaseAdd = R4K1_BASE_ADD;
//        startIndex = B4K1_START_INDEX;
        blockSize = BLOCK_SIZE1;
    }
    else if((uint32_t)baseAdd >= R1_5K_BASE_ADD && (uint32_t)baseAdd < R8K1_BASE_ADD)
    {
        regionBaseAdd = R1_5K_BASE_ADD;
//        startIndex = B1_5K_START_INDEX;
        blockSize = BLOCK_SIZE2;
    }
    else if ((uint32_t)baseAdd >= R8K1_BASE_ADD && (uint32_t)baseAdd< R8K1_END_ADD)
    {
        regionBaseAdd = R8K1_BASE_ADD;
//        startIndex = B8K1_START_INDEX;
        blockSize = BLOCK_SIZE2;
    }
    else if ((uint32_t)baseAdd >= R8K2_BASE_ADD && (uint32_t)baseAdd < R8K2_END_ADD)
    {
        regionBaseAdd = R8K2_BASE_ADD;
//        startIndex = B8K2_START_INDEX;
        blockSize = BLOCK_SIZE2;
    }
    else if ((uint32_t)baseAdd >= R8K3_BASE_ADD && (uint32_t)baseAdd < R8K3_END_ADD)
    {
        regionBaseAdd = R8K3_BASE_ADD;
//        startIndex = B8K3_START_INDEX;
        blockSize = BLOCK_SIZE2;
    }
    else
    {
        return;  // Base address does not match any configurable region
    }

    for(j = 0; j < MAX_SIZE; j++)
    {
        if (heapAttributes[j].baseAddress == (void*)baseAdd && heapAttributes[j].allocated)
        {
            allocated = heapAttributes[j].allocated;
            break;
        }
    }

    // Calculate the subregion index range
    uint32_t offset = (((uint32_t) baseAdd) - regionBaseAdd) / blockSize;
//    uint32_t startSubregion = offset;//startIndex; //+ offset;
    uint32_t endSubregion = offset + allocated;

    uint32_t i;

    // Set the corresponding SRD bits to 1 (enable access) for each subregion in the range
    for(i = (offset) /*+ offset)*/; i < endSubregion; i++)
    {
        *srdBitMask |= (1 << i);  // Clear the bit to enable access
    }
}

// apply sram access mask
void applySramAccessMask(uint64_t srdBitMask)
{
    // apply for region 4K1
    uint32_t applyMask = (srdBitMask & 0x000000FF);
    NVIC_MPU_NUMBER_R = NVIC_MPU_R1_4K1;
    NVIC_MPU_ATTR_R &= ~(0x0000FF00);
    NVIC_MPU_ATTR_R |= (applyMask << 8);

    // apply for region 8K1
    applyMask = (srdBitMask & 0x0000FF00);
    applyMask >>= 8;
    NVIC_MPU_NUMBER_R = NVIC_MPU_R2_8K1;
    NVIC_MPU_ATTR_R &= ~(0x0000FF00);
    NVIC_MPU_ATTR_R |= (applyMask << 8);

    // apply for region 8K2
    applyMask = (srdBitMask & 0x00FF0000);
    applyMask >>= 16;
    NVIC_MPU_NUMBER_R = NVIC_MPU_R3_8K2;
    NVIC_MPU_ATTR_R &= ~(0x0000FF00);
    NVIC_MPU_ATTR_R |= (applyMask << 8);

    // apply for region 8K3
    applyMask = (srdBitMask & 0xFF000000);
    applyMask >>= 24;
    NVIC_MPU_NUMBER_R = NVIC_MPU_R4_8K3;
    NVIC_MPU_ATTR_R &= ~(0x0000FF00);
    NVIC_MPU_ATTR_R |= (applyMask << 8);
}

// Initialize MPU
void initMpu(void)
{
    // turn off mpu for safe programming
    NVIC_MPU_CTRL_R &= ~(NVIC_MPU_CTRL_ENABLE);

    setBackgroundRule();
    allowFlashAccess();
    setupSramAccess();

    // enable the MPU
//    NVIC_MPU_CTRL_R |= NVIC_MPU_CTRL_ENABLE;
}
