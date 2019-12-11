#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#define MAX_CACHE_SIZE 256
#define MAX_BLOCK_SIZE 256

#define NUMMEMORY 65536 /* maximum number of words in memory */
#define NUMREGS 8 /* number of machine registers */
#define MAXLINELENGTH 1000


typedef struct stateStruct {
    int pc;
    int mem[NUMMEMORY];
    int reg[NUMREGS];
    int numMemory;
} stateType;

enum actionType
{
  cacheToProcessor,
  processorToCache,
  memoryToCache,
  cacheToMemory,
  cacheToNowhere
};

typedef struct blockStruct
{
  int data[MAX_BLOCK_SIZE];
  bool isDirty;
  int lruLabel;
  int set;
  int tag;
    //new code
    bool validBit;
} blockStruct;

typedef struct cacheStruct
{
  blockStruct blocks[MAX_CACHE_SIZE];
  int blocksPerSet;
  int blockSize;
  int lru; //num lru bits needed
  int numSets;
} cacheStruct;

/* Global Cache variable */
cacheStruct cache;
stateType state;

/*void printState(stateType *);*/
int convertNum(int);
int offset2sComp(int);
int lwSw(int, int, char);
void printAction(int, int, enum actionType);

int
main(int argc, char *argv[])
{
    //Starter declarations
    char line[MAXLINELENGTH];
    //stateType state;
    FILE *filePtr;
    
    
    //My declarations (p1)
    int i;
    int opcode;
    int regTemp;
    int offsetTemp;
    int regA;
    int regB;
    int destReg;
    int offsetField;
    int decimalOffset;
    int memBitties;
    int numExecutions;
    
    //My declarations (p4)
    int ctr;
    
    if (argc != 5) {
        printf("error: usage: %s <machine-code file>\n", argv[0]);
        exit(1);
    }

    filePtr = fopen(argv[1], "r");
    if (filePtr == NULL) {
        printf("error: can't open file %s", argv[1]);
        perror("fopen");
        exit(1);
    }
    
    cache.blockSize = atoi(argv[2]);
    if (cache.blockSize > 256 || cache.blockSize < 0) {
        printf("error: block Size error %s", argv[2]);
        exit(1);
    }
    cache.numSets = atoi(argv[3]);
    if (cache.numSets > 256 || cache.numSets < 0 ) {
        printf("error: num Sets error %s", argv[3]);
        exit(1);
    }
    cache.blocksPerSet = atoi(argv[4]);
    if (cache.blocksPerSet > 256 || cache.blocksPerSet < 0) {
        printf("error: blocks per set error %s", argv[4]);
        exit(1);
    }
    
    //because blocksPerSet == number of ways
    cache.lru = log2(cache.blocksPerSet);
    
    //initialize valid and dirty bits
    for (i = 0; i < MAX_CACHE_SIZE; i++) {
        cache.blocks[i].isDirty = false;
        cache.blocks[i].validBit = false;
        cache.blocks[i].lruLabel = -36;
        //cache.blocks[i].set = 0;
        cache.blocks[i].tag = -36;
    }
    ctr = 0;
    for (i = 0; i < cache.numSets * cache.blocksPerSet; i++) {
        cache.blocks[i].set = ctr;
        if ((i % cache.blocksPerSet) == 0) {
            ctr++;
        }
    }
    /*
    while (i < cache.numSets) {
        for (j = 0; j < cache.blocksPerSet; j++) {
            cache.blocks[ctr].set = i;
            ctr += cache.blockSize;
        }
        i++;
    }
     */

    /* read the entire machine-code file into memory */
    for (state.numMemory = 0; fgets(line, MAXLINELENGTH, filePtr) != NULL;
            state.numMemory++) {

        if (sscanf(line, "%d", state.mem+state.numMemory) != 1) {
            printf("error in reading address %d\n", state.numMemory);
            exit(1);
        }
        printf("memory[%d]=%d\n", state.numMemory, state.mem[state.numMemory]);
    }

    

    //initialize all registers to 0
    for (i = 0; i < 8; ++i) {
        state.reg[i] = convertNum(0);
    }
    
    state.pc    = 0;
    regTemp     = 0b00000000000000000000000000000111;
    offsetTemp  = 0b00000000000000001111111111111111;
    numExecutions = 0;
    
    while (state.pc < state.numMemory) {
        numExecutions++;
        //printState(&state);
        //memBitties = state.mem[state.pc];
        memBitties = lwSw(state.pc, 0, 'l');
        opcode = memBitties >> 22;
        
        regA = memBitties >> 19;
        regA = regA & regTemp;
        
        regB = memBitties >> 16;
        regB = regB & regTemp;
        
        destReg = memBitties & regTemp;
        
        offsetField = memBitties & offsetTemp;
        decimalOffset = offset2sComp(offsetField);
        
        /*R-Types*/
        //add
        if (opcode == 0b000) {
            state.reg[destReg] = state.reg[regA] + state.reg[regB];
            ++state.pc;

        }
        //nor
        else if (opcode == 0b001) {
            state.reg[destReg] = ~state.reg[regA] & ~state.reg[regB];
            ++state.pc;

        }
        /*I-Types*/
        //lw
        else if (opcode == 0b010) {
            /*
            state.reg[regB] = state.mem[state.reg[regA]+decimalOffset];
             */
            state.reg[regB] = lwSw(state.reg[regA]+decimalOffset, 0, 'l');
            ++state.pc;
        }
        //sw
        else if (opcode == 0b011) {
            /*
            state.mem[state.reg[regA]+decimalOffset] = state.reg[regB];
             */
            lwSw(state.reg[regA]+decimalOffset, state.reg[regB], 's');
            ++state.pc;
        }
        //beq
        else if (opcode == 0b100) {
            if(state.reg[regA] == state.reg[regB]) {
                state.pc = state.pc + 1 + decimalOffset;
            } else {
                ++state.pc;
            }
        }
        /*J-Type*/
        //jalr
        else if (opcode == 0b101) {
            state.reg[regB] = state.pc + 1;
            state.pc = state.reg[regA];
        }
        
        //halt
        else if (opcode == 0b110) {
            printf("machine halted\n");
            printf("total of %d instructions executed\n", numExecutions);
            ++state.pc;
            break;
        }
        //noop noop
        else if (opcode == 0b111) {
            //become the newest member of the vindicators
            ++state.pc;
        }
        //error?
        else {
            return(1);
        }
    }

    //print resulting registers
    printf("final state of machine:\n");
    //printState(&state);

    return(0);
}
/*
void
printState(stateType *statePtr)
{
    int i;
    printf("\nstate:\n");
    printf("\tpc %d\n", statePtr->pc);
    printf("\tmemory:\n");
    for (i=0; i<statePtr->numMemory; i++) {
              printf("\t\tmem[ %d ] %d\n", i, statePtr->mem[i]);
    }
    printf("\tregisters:\n");
    for (i=0; i<NUMREGS; i++) {
              printf("\t\treg[ %d ] %d\n", i, statePtr->reg[i]);
    }
    printf("end state\n");
}
*/
int
convertNum(int num)
{
    /* convert a 16-bit number into a 32-bit Linux integer */
    if (num & (1<<15) ) {
        num -= (1<<16);
    }
    return(num);
}


//.fill instructions always come after halt

int
offset2sComp(int num) {
    int tempNum     = num;
    int testNum     = 0b00000000000000001000000000000000;
    int adjustedNum = 0b00000000000000000111111111111111;
    
    tempNum = num & testNum;
    tempNum = tempNum >> 15;
    if (tempNum != 1) {
        return num;
    }
    num = num & adjustedNum;
    num = -32768 + num;
    return num;
}


/*
 * Log the specifics of each cache action.
 *
 * address is the starting word address of the range of data being transferred.
 * size is the size of the range of data being transferred.
 * type specifies the source and destination of the data being transferred.
 *  -	cacheToProcessor: reading data from the cache to the processor
 *  -	processorToCache: writing data from the processor to the cache
 *  -	memoryToCache: reading data from the memory to the cache
 *  -	cacheToMemory: evicting cache data and writing it to the memory
 *  -	cacheToNowhere: evicting cache data and throwing it away
 */
void printAction(int address, int size, enum actionType type)
{
	printf("@@@ transferring word [%d-%d] ", address, address + size - 1);

	if (type == cacheToProcessor) {
		printf("from the cache to the processor\n");
	}
	else if (type == processorToCache) {
		printf("from the processor to the cache\n");
	}
	else if (type == memoryToCache) {
		printf("from the memory to the cache\n");
	}
	else if (type == cacheToMemory) {
		printf("from the cache to the memory\n");
	}
	else if (type == cacheToNowhere) {
		printf("from the cache to nowhere\n");
	}
}

/*
 * Prints the cache based on the configurations of the struct
 */

//only for debugging purposes
/*void printCache()
{
  printf("\n@@@\ncache:\n");

  for (int set = 0; set < cache.numSets; ++set) {
    printf("\tset %i:\n", set);
    for (int block = 0; block < cache.blocksPerSet; ++block) {
      printf("\t\t[ %i ]: {", block);
      for (int index = 0; index < cache.blockSize; ++index) {
        printf(" %i", cache.blocks[set * cache.blocksPerSet + block].data[index]);
      }
      printf(" }\n");
    }
  }

  printf("end cache\n");
}
*/

// Properly simulates the cache for a load from
// memory address "addr". Returns the loaded value for loads. Return value meaningless for stores
int lwSw(int addr, int data, char instruction) {
    int all_1 = 0xFFFFFFFF;
    int tempAddr = addr;
    int buildAddr;
    int blockOffsetSize = log2(cache.blockSize);
    int blockOffset;
    int setBitsSize = log2(cache.numSets);
    int setBits;
    int tagBits;
    int i = 0;
    int hit = 0;
    int hitSpot = 0;
    int emptySpot = -1;
    int evictSpot = -1;
    int hitSpotLRU;
    int finalVal = 0;

    
    //get blockOffset
    blockOffset = all_1 << blockOffsetSize;
    blockOffset = ~blockOffset;
    blockOffset = blockOffset & tempAddr;
    //right shift temp address by block offset size
    tempAddr = tempAddr >> blockOffsetSize;
    //get setBits
    setBits = all_1 << setBitsSize;
    setBits = ~setBits;
    setBits = setBits & tempAddr;
    //right shift temp address by setBits size
    tempAddr = tempAddr >> setBitsSize;
    //get tagBits
    tagBits = tempAddr;

    //hit or miss
    for (i = 0; i < cache.blocksPerSet; i++) {
        //if tag and set index match
        if (cache.blocks[(setBits*cache.blocksPerSet)+ i].tag == tagBits &&
            cache.blocks[(setBits*cache.blocksPerSet)+ i].validBit) {
            hit = 1;
            hitSpot = setBits*cache.blocksPerSet + i;
        }
        //unused tag
        if (cache.blocks[(setBits*cache.blocksPerSet)+ i].validBit == false
            && emptySpot == -1) {
            emptySpot = setBits*cache.blocksPerSet +i;
        }
    }

    //if hit
    if (hit == 1) {
        //update lru
        hitSpotLRU = cache.blocks[hitSpot].lruLabel;
        cache.blocks[hitSpot].lruLabel = 0;
        for (i = 0; i < cache.blocksPerSet; i++) {
            if ((setBits*cache.blocksPerSet + i) != hitSpot &&
                cache.blocks[(setBits*cache.blocksPerSet)+i].validBit &&
                cache.blocks[(setBits*cache.blocksPerSet)+i].lruLabel < hitSpotLRU) {
                //increment lru that are less than the lru of the block just hit
                cache.blocks[(setBits*cache.blocksPerSet)+i].lruLabel++;
                
            }
        }
        finalVal = cache.blocks[hitSpot].data[blockOffset];

        if (instruction == 'l') {
            printAction(addr, 1, cacheToProcessor);
        } else if (instruction == 's') {
            //assign data to address
            cache.blocks[hitSpot].data[blockOffset] = data;
            //change dirty bit
            cache.blocks[hitSpot].isDirty = true;
            
            printAction(addr, 1, processorToCache);
        }
        return finalVal;
    }
    
    //if miss with empty cache block available
    if (emptySpot >= 0) {
        cache.blocks[emptySpot].lruLabel = 0;
        cache.blocks[emptySpot].set = setBits;
        cache.blocks[emptySpot].tag = tagBits;
        cache.blocks[emptySpot].validBit = true;
        cache.blocks[emptySpot].isDirty = false;
        //will need to bring blockSize number of data values from memory
        addr -= blockOffset;
        for (i = 0; i < cache.blockSize; i++) {
            cache.blocks[emptySpot].data[i] = state.mem[addr + i];
        }
        //updateLRU
        for (i = 0; i < cache.blocksPerSet; i++) {
            if ((setBits*cache.blocksPerSet + i) != emptySpot &&
                cache.blocks[(setBits*cache.blocksPerSet)+i].validBit) {
                //increment lru
                cache.blocks[(setBits*cache.blocksPerSet)+i].lruLabel++;
            }
        }
        //print memory to cache
        printAction(addr, cache.blockSize, memoryToCache);
        
        addr += blockOffset;
        if (instruction == 'l') {
            finalVal = cache.blocks[emptySpot].data[blockOffset];
            //print cache to processor
            printAction(addr, 1, cacheToProcessor);
        } else if (instruction == 's') {
            cache.blocks[emptySpot].data[blockOffset] = data;
            cache.blocks[emptySpot].isDirty = true;
            //print processor to cache
            printAction(addr, 1, processorToCache);
        }

        return finalVal;
    }
    
    //Must be full at this point.
    //find evict block
    for (i = 0; i < cache.blocksPerSet; i++) {
        if (cache.blocks[(setBits*cache.blocksPerSet)+i].validBit &&
            cache.blocks[(setBits*cache.blocksPerSet)+i].lruLabel == cache.blocksPerSet-1) {
            evictSpot = (setBits*cache.blocksPerSet)+i;
        }
    }
    //quick error check
    if (evictSpot == -1) {
        exit(1);
    }
    
    //reconstruct address
    buildAddr = all_1;
    buildAddr = buildAddr & cache.blocks[evictSpot].tag;
    buildAddr = buildAddr << setBitsSize;

    buildAddr = buildAddr | cache.blocks[evictSpot].set;
    buildAddr = buildAddr << blockOffsetSize;

    //if miss with  dirty block (evict case)
    if (cache.blocks[evictSpot].isDirty) {
        //overwrite mem
        for (i = 0; i < cache.blockSize; i++) {
            state.mem[buildAddr+i] = cache.blocks[evictSpot].data[i];
        }
        //print cache to memory
        printAction(buildAddr, cache.blockSize, cacheToMemory);
        //overwrite block
        cache.blocks[evictSpot].lruLabel = 0;
        cache.blocks[evictSpot].set = setBits;
        cache.blocks[evictSpot].tag = tagBits;
        cache.blocks[evictSpot].validBit = true;
        cache.blocks[evictSpot].isDirty = false;
        //will need to bring blockSize number of data values from memory
        addr -= blockOffset;
        for (i = 0; i < cache.blockSize; i++) {
            cache.blocks[evictSpot].data[i] = state.mem[addr + i];
        }
        //updateLRU
        for (i = 0; i < cache.blocksPerSet; i++) {
            if ((setBits*cache.blocksPerSet + i) != evictSpot &&
                cache.blocks[(setBits*cache.blocksPerSet)+i].validBit) {
                //increment lru
                cache.blocks[(setBits*cache.blocksPerSet)+i].lruLabel++;
            }
        }
        
        //print memory to cache
        printAction(addr, cache.blockSize, memoryToCache);
        
        addr += blockOffset;
        if (instruction == 'l') {
            finalVal = cache.blocks[evictSpot].data[blockOffset];
            //print cache to processor
            printAction(addr, 1, cacheToProcessor);
        } else if (instruction == 's') {
            cache.blocks[evictSpot].data[blockOffset] = data;
            cache.blocks[evictSpot].isDirty = true;
            //print processor to cache
            printAction(addr, 1, processorToCache);
        }
        
        return finalVal;
    }
    
    
    
    //if miss without dirty block (evict case)
    printAction(buildAddr, cache.blockSize, cacheToNowhere);

    
    //overwrite block
    cache.blocks[evictSpot].isDirty = false;
    cache.blocks[evictSpot].lruLabel = 0;
    cache.blocks[evictSpot].set = setBits;
    cache.blocks[evictSpot].tag = tagBits;
    cache.blocks[evictSpot].validBit = true;
    //will need to bring blockSize number of data values from memory
    addr -= blockOffset;
    for (i = 0; i < cache.blockSize; i++) {
        cache.blocks[evictSpot].data[i] = state.mem[addr + i];
    }
    //updateLRU
    for (i = 0; i < cache.blocksPerSet; i++) {
        if ((setBits*cache.blocksPerSet + i) != evictSpot &&
            cache.blocks[(setBits*cache.blocksPerSet)+i].validBit) {
            //increment lru
            cache.blocks[(setBits*cache.blocksPerSet)+i].lruLabel++;
        }
    }
    //print memory to cache
    printAction(addr, cache.blockSize, memoryToCache);
    
    addr += blockOffset;
    if (instruction == 'l') {
        finalVal = cache.blocks[evictSpot].data[blockOffset];
        //print cache to processor
        printAction(addr, 1, cacheToProcessor);
    } else if (instruction == 's') {
        cache.blocks[evictSpot].data[blockOffset] = data;
        cache.blocks[evictSpot].isDirty = true;
        //print processor to cache
        printAction(addr, 1, processorToCache);
    }
    return finalVal;
}
