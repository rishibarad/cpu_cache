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
    int validBit;
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
    stateType state;
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
        cache.blocks[i].lruLabel = 0;
        cache.blocks[i].set = 0;
        cache.blocks[i].tag = 0;
    }

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
        memBitties = state.mem[state.pc];
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
void printCache()
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


/*
About set bit

e.g. if blockSizeInWords=4 numberOfSets=8 blocksPerSet=16

then if we store mem address= 55 data into the cache,

55=0110111

size of block offset: log2(4)=2

size of set bits:log2(8)=3

so the block offset is the last 2 bits: 11

the set bits are next 3 bits:101

the tag bits are remaining 01
*/
// Properly simulates the cache for a load from
// memory address "addr". Returns the loaded value for loads. Return value meaningless for stores
int lwSw(int addr, int data, char instruction) {
    int all_1 = 0xFFFFFFFF;
    int tempAddr = addr;
    int blockOffsetSize = log2(cache.blockSize);
    int blockOffset;
    int setBitsSize = log2(cache.numSets);
    int setBits;
    int tagBits;
    int i = 0;
    bool hit = false;
    bool emptySpot = true;
    //adding data to cache:
    //  cache.blocks[set].data[BO] = data;
    
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
    
    for (i = 0; i < 256; i += cache.blockSize) {
        
    }
    //four if statements
    
    //hit
    if (hit) {
        
    }
    //miss. empty cache block available
    else if (emptySpot) {
        
    }
    //miss. full but no dirty block (evict case)

    //miss. full with  dirty block (evict case)
    
    
    
    return 0;

}




/*
After transferring an instruction from cache to the processor, for example transferring instruction 13 from cache to processor, if later on you have an instruction like, lw 0 2 13, would you transfer 13 from the cache to the processor again?
        -Yes you would need to transfer from the cache to processor again
*/

/*
 fully associative and direct mapped
 Just to clarify, the only way to have a fully associative cache is when there's only one set right? And for direct mapped, there can be any number of sets as long as the blocks per set is 1?
        -Yes. Yes. For direct map, number of set = number of blocks in cache.
 */
/*
 CacheToProcessor vs ProcessorToCache
 Im a bit confused as to when something is CTP or PTC. My logic is if you are taking something from reg to mem(or cache) it would be a PTC and the opposite for CTP. This does not line up in the spec example as it states cacheToProcessor for the sw instruction and processorToCache for the lw instruction. Could someone explain when something would be PTC vs CTP?
        - Your understanding is correct. For sw in example, we have "@@@ transferring word [6-6] from the processor to the cache", the data is from processor to cache. To fetch the instruction, we need cache to processor. And for every instruction, we need to fetch it before doing the operation.
 */

/*
 Reject the lru
 If I don't find an empty space in a set, I need to reject the lru block and load data into it.

 However, in the Cachestruct, we only have one lru, but what we need to know is the lru in the

 corresponding set where the data should belong.

 I am wondering how to fix it?
        -You don't need to use lru in the cacheStruct. Basically, you can loop through certain blocks and find max lru label in these blocks each time. It works for fully associative, direct mapped, and n-way associative cache.
 */
/*
 Size of accessible memory
 I’m confused on how the size of the memory we will access data from lines up with the specified number of sets, blocks per set, and words per block. If we were to have say, 1 word per block, 1 block per set, and 1 set, would this mean that there is only 1 location in memory we can actually read from and pull into the cache?
        -The only thing that changes the size of the data we bring in from memory is block size. If the block size is 2 words, we are moving groups of 2 words in and out of the cache to/from memory.

        -1 word per block, 1 block per set, and 1 set all all things that describe our cache, not our memory. You can still think of memory like one long array as you have in the past projects.

        -This setup means there only one block in the cache at the time, this block can be a chunk of memory from 0,6500 (or whatever that max number is)
 */
/*
 In the SPEC of project 4, we have an example at the end.

 I wonder whether 6, 23, and 30 are empty memory and are all set to be 0?

 Also, in the first line, we transferring word [0-3] from mem to cache. Is it due to we are trying to read the first instruction?
        -Yes, they are pointing to the empty part of the memory. For your second question, yes.

        Also, yes, you should have enough knowledge to start. You are absolutely correct that you and everyone should start early for every class.

 
 Question about Project Spec
 In the project spec example, the first line says

 @@@ transferring word [0-3] from the memory to the cache

  

 Are we supposed to transfer all the instructions from memory to the cache right at the beginning? Or were we just trying to read instruction 0 and we just happened to transfer all the other instructions as well since the block size is 4?
        -We're just trying to read instr 0, and we transfer the 0-3, so if we were trying to read instr 5, we would read 4-7!

 */
/*
 number of sets
 Is it true that there must be at least 2 sets in the cache?
        -No (how many sets are in a fully associative cache?)
            Just one and one is considered pow(2,0), so there must be at least 1 set in the cache right? In this way, the set index is 0?
                    -Exactly!
 */
/*
 Project 4 cache writeback
 In project 4, do we need to write all cache blocks with dirty bit into memory after the program halts?
        -Nope! You can leave your cache dirty.
 */
/*
 LRU in project 4
 Im a little confused as to how to go about implementing the LRU in the project.

 I understand that we have an int LRU in each block that would technically hold the address (within the set) of the least recently used block, actually keeping track of it is confusing me.

 My initial thought process would be to have a stack-like structure to hold the information as we add/evict blocks, but there isnt really a stack in c is there?

 Any tips on how to go about implementing this?
 
        -At any given time, each of the n blocks in a set should be ranked from 0 (most recently used) to n-1 (least recently used). It is up to you to figure out how to keep those rankings updated properly
 */

