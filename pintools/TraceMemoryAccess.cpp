/*BEGIN_LEGAL 
Intel Open Source License 

Copyright (c) 2002-2016 Intel Corporation. All rights reserved.
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */
/*
 *  This file contains an ISA-portable PIN tool for tracing memory accesses.
 */

#include <stdio.h>
#include "pin.H"


FILE * trace;
FILE * inst;
FILE * maps;

UINT32 inscount = 0;
bool outflag=false;


#define THRESHOLD 3110000

#define LEN_ADDR 8
const string routine_name="main";
typedef enum region_type {
    GLOBAL = 0,
    HEAP,
    STACK,
    NO_REGION
}region_type;

typedef struct region_info {
    region_type type;
    ADDRINT base;
    ADDRINT end;
}region_info;

// size changes with region num
region_info regions[3];

//char regin_type_str[4][6]={"uknow","stack"," heap"," glob"};
int GetReginFromAddr(VOID * poi){
    ADDRINT addr=(ADDRINT)poi;
    int type=0;
    if(addr<regions[STACK].end && addr>regions[STACK].base){
        type=2;
    }else if(addr<regions[HEAP].end && addr>regions[HEAP].base){
        type=3;
    }else if(addr<regions[GLOBAL].end && addr>regions[GLOBAL].base){
        type=1;
    }
    return type;
}
void get_region_info()
{
    INT pid = PIN_GetPid();
    //printf("Pid : %d\n", pid);

    FILE *procmaps;
    char path[20];
    char line[100];

    sprintf(path, "/proc/%d/maps", pid);

    procmaps = fopen(path, "r"); 
    int times = 0;

    while(fgets(line, 100, procmaps) != NULL) {
	//printf("%s", line);
        if (times == 0 && strstr(line, "rwxp")) {
            sscanf(strndup(line, 1+2*LEN_ADDR), "%x-%x", &regions[GLOBAL].base, &regions[GLOBAL].end);
    	    //printf("%x and %x\n", regions[GLOBAL].base, regions[GLOBAL].end);
            regions[GLOBAL].type = GLOBAL;
            times++;
        }
        if (strstr(line, "[stack")) {
            sscanf(strndup(line, 1+2*LEN_ADDR), "%x-%x", &regions[STACK].base, &regions[STACK].end);
            regions[STACK].type = STACK;
	    //printf("%x and %x\n", regions[STACK].base, regions[STACK].end);
        }
        if (strstr(line, "[heap")) {
            sscanf(strndup(line, 1+2*LEN_ADDR), "%x-%x", &regions[HEAP].base, &regions[HEAP].end);
            regions[HEAP].type = HEAP;
	    //printf("%x and %x\n", regions[HEAP].base, regions[HEAP].end);
        } 
    } 

    fprintf(maps, "%x;%x\n", regions[GLOBAL].base, regions[GLOBAL].end);
    fprintf(maps, "%x;%x\n", regions[STACK].base, regions[STACK].end);
    fprintf(maps, "%x;%x", regions[HEAP].base, regions[HEAP].end);
}

VOID RecordMem(VOID * ip, VOID * addr)
{

    fprintf(trace, "%p;%p\n", ip, addr);
}

VOID LogInst(VOID * ip, UINT32 size, UINT32 num,VOID * addr) {
    if(outflag==false){
        return;
    }
    inscount++;

    //fprintf(inst, "%p;%d;", ip, size);

    char *pi = (char *)malloc(size);
    PIN_SafeCopy(pi, (void *)ip, size);

    for (UINT32 id=0; id<size; id++){  
        fprintf(inst, "%02x", (unsigned char)(pi[id])); 
        fprintf(trace, "%d", GetReginFromAddr(addr));
    }
    fprintf(inst, "\n");
    fprintf(trace, "\n");
    free(pi);
    pi = NULL;

/*
    if (!num){
        fprintf(inst, ";N\n");
    }else{
        fprintf(inst, ";%d\n", num);
    }
*/
    if (inscount % THRESHOLD == 0) {
        get_region_info();

    	fflush(trace);
    	fflush(maps);
    	fflush(inst);

    	fclose(trace);
    	fclose(maps);
    	fclose(inst);

        char new_name[10];

    	sprintf(new_name, "%s%d", "memtrace", inscount/THRESHOLD);
            trace = fopen(new_name, "w");
    	sprintf(new_name, "%s%d", "inst", inscount/THRESHOLD);
            inst = fopen(new_name, "w");
    	sprintf(new_name, "%s%d", "maps", inscount/THRESHOLD);
            maps = fopen(new_name, "w");
    }
}

VOID  begintrace()
{   
        outflag=true;
}
VOID  endtrace()
{   
        outflag=false;
}
// Is called for every instruction and instruments reads and writes
VOID RTNInstrumentation(RTN rtn, VOID *v)
{
    RTN_Open(rtn);

    string name=RTN_Name(rtn);
    if(routine_name.compare(name)==0){
        INS InsHead=RTN_InsHead(rtn);
        INS InsTail=RTN_InsTail(rtn);
        if(INS_Valid(InsHead))
            INS_InsertPredicatedCall(InsHead, IPOINT_BEFORE, AFUNPTR(begintrace),IARG_END);
        if(INS_Valid(InsTail))
            INS_InsertPredicatedCall(InsTail, IPOINT_BEFORE, AFUNPTR(endtrace),IARG_END);
         
    }
    for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins)){
        
            if(INS_IsMemoryWrite(ins)||INS_IsMemoryRead(ins)){
            UINT32 memOperands = INS_MemoryOperandCount(ins);
            UINT32 memOp=0;
            for (memOp = 0; memOp < memOperands; memOp++)
            {
                if (INS_MemoryOperandIsRead(ins, memOp)||INS_MemoryOperandIsWritten(ins, memOp)){
                    break;
                }
            }
            
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)LogInst, 
                IARG_INST_PTR,
                IARG_UINT32, INS_Size(ins),
                IARG_UINT32, memOperands,
                IARG_MEMORYOP_EA, memOp,
                IARG_END);
            }
    }
    RTN_Close(rtn);
    
}

VOID Fini(INT32 code, VOID *v)
{
    fclose(trace);
    fclose(maps);
    fclose(inst);
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */
   
INT32 Usage()
{
    PIN_ERROR( "This Pintool prints a trace of memory addresses\n" 
              + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
    PIN_InitSymbols();
    if (PIN_Init(argc, argv)) return Usage();

    trace = fopen("memtrace0", "w");
    inst = fopen("inst0", "w");
    maps = fopen("maps0", "w");
    get_region_info();
    //INS_AddInstrumentFunction(Instruction, 0);
    RTN_AddInstrumentFunction(RTNInstrumentation, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();
    
    return 0;
}
