#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_CACHE_SIZE 256
#define MAX_BLOCK_SIZE 256

#define NUMMEMORY 65536 /* maximum number of words in memory */
#define NUMREGS 8 /* number of machine registers */
#define MAXLINELENGTH 1000


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

typedef struct stateStruct {
    int pc;
    int mem[NUMMEMORY];
    int reg[NUMREGS];
    int numMemory;
} stateType;


typedef struct cacheStruct
{
  blockStruct blocks[MAX_CACHE_SIZE];
  int blocksPerSet;
  int blockSize;
  int lru;
  int numSets;
} cacheStruct;

/* Global Cache variable */
cacheStruct cache;

void printState(stateType *);
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
    
    
    //My declarations
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

    
    
    
    //Starter code
    if (argc != 2) {
        printf("error: usage: %s <machine-code file>\n", argv[0]);
        exit(1);
    }

    filePtr = fopen(argv[1], "r");
    if (filePtr == NULL) {
        printf("error: can't open file %s", argv[1]);
        perror("fopen");
        exit(1);
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
    //End Starter Code
    
    /*
     1) Don't modify printState or stateStruct at all
     2) Call printState exactly once before each instruction executes and once
     just before the simulator exits
     3) Dont print the sequence "@@@" anywhere (except where the provided
     "printState" function prints it)
     4) state.numMemory must be equal to the number of lines in the machine
     code file
     5) Initialize all registers to 0
     */
    
    //My Code
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
        printState(&state);
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
            state.reg[regB] = state.mem[state.reg[regA]+decimalOffset];
            ++state.pc;
        }
        //sw
        else if (opcode == 0b011) {
            state.mem[state.reg[regA]+decimalOffset] = state.reg[regB];
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
    printState(&state);

    return(0);
}

void
printState(stateType *statePtr)
{
    int i;
    printf("\n@@@\nstate:\n");
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
Your code should behave exactly the same way as your project 1 code except
for memory accesses, so starting with your project 1 simulator should save
you some work. Additionally, writing the following two
functions and replacing all your memory accesses with these function calls
could simplify your code. The following function takes the address input
variable to access global defined cache data structure:
*/

// Properly simulates the cache for a load from
// memory address "addr". Returns the loaded value.
int lwSw(int addr, int data, char instruction) {
    //four if statements
    
    //if1
    
    //if2
    
    //if3
    
    //if4
    
    
    
    return 0;

}

