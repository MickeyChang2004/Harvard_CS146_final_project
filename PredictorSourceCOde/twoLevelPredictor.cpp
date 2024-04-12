#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <map>
#include <cstdlib>
#include "pin.H"

using std::ofstream;
using std::cout;
using std::cerr;
using std::endl;
using std::string;

/* ===================================================================== */
/* Commandline Switches */
/* ===================================================================== */

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE,         "pintool",
                            "o", "hw3.out", "specify output file name");

KNOB<BOOL>   KnobPid(KNOB_MODE_WRITEONCE,                "pintool",
                            "i", "0", "append pid to output");

KNOB<UINT64> KnobBranchLimit(KNOB_MODE_WRITEONCE,        "pintool",
                            "l", "0", "set limit of branches analyzed");

KNOB<UINT64> KnobHHRTSize(KNOB_MODE_WRITEONCE, "pintool", "hhrtSize", "512", "specify the size of the HHRT");
KNOB<UINT64> KnobPTSize(KNOB_MODE_WRITEONCE, "pintool", "ptSize", "4096", "specify the size of the PT");
/* ===================================================================== */
/* Global Variables */
/* ===================================================================== */
UINT64 CountSeen = 0;
UINT64 CountTaken = 0;
UINT64 CountCorrect = 0;

/* ===================================================================== */
/* 1-bit Branch predictor (using Branch Prediction Buffer),              */
/* which is mentioned in cs246-lecture-speculation.pdf but with 1-bit    */
/* ===================================================================== */
//TBD: HHRT, hash(), PT
UINT64 SIZE1 = 512;//HHRTsize
UINT64 SIZE2 = 4096;//PTsize
UINT64 K = 12;
UINT64 mask1 = SIZE1-1;
UINT64 mask2 = SIZE2-1;

struct hrt_entry
{
    int* hrt_bit;
};

struct pt_entry
{
    bool prediction;
    int status;
};

hrt_entry* HHRT = NULL;
pt_entry* PT = NULL;

//initialize HHRT
VOID HHRT_init()
{
    SIZE1 = KnobHHRTSize.Value();
    mask1 = SIZE1 - 1;
    HHRT = new hrt_entry[SIZE1];
    for(UINT64 i = 0;i < SIZE1; i++)
    {
        HHRT[i].hrt_bit = new int[K];
        for(UINT64 j = 0; j < K; j++)
        {
            HHRT[i].hrt_bit[j] = 0;
        }
    }
}
//initialize PT
VOID PT_init()
{
    SIZE2 = KnobPTSize.Value();
    K = 0;
    UINT64 i = SIZE2;
    while((i/2)!=0)
    {
        K++;
        i /= 2;
    }
    mask2 = SIZE2 - 1;
    PT = new pt_entry[SIZE2];
    for(UINT64 i = 0; i < SIZE2; i++)
    {
        PT[i].prediction = false;
        PT[i].status = 0;
    }
}
//hash the conditional branch's address into the table
UINT64 HHRT_hash_address(ADDRINT ins_ptr)
{
    UINT64 HHRT_hash_address = ins_ptr;
    HHRT_hash_address ^= (HHRT_hash_address >> 33);
    HHRT_hash_address *= 0xFF51AFD7ED558CCDLLU;// I came across an algorithm named "MurmurHash3 algorithm" which introduces this method to hash address into the hash table
    HHRT_hash_address ^= (HHRT_hash_address >> 33);
    HHRT_hash_address *= 0xC4CEB9FE1A85EC53LLU;
    HHRT_hash_address ^= (HHRT_hash_address >> 33);
    HHRT_hash_address = mask1 & HHRT_hash_address;
    return HHRT_hash_address;
}

//convert HHRT[] to a decimal number
UINT64 convert_PT_index(int bin_bit[])
{
    UINT64 result = 0;
    int j = 1;
    for(UINT64 i = K - 1; i > 0; i--)
    {
        result = result + (bin_bit[i] * j);
        j *= 2;
    }
    result = result + (bin_bit[0] * j);
    return result;
}
//make prediction
bool HHRT_prediction(ADDRINT ins_ptr)
{
    UINT64 HHRT_index = HHRT_hash_address(ins_ptr);
    UINT64 PT_index = convert_PT_index(HHRT[HHRT_index].hrt_bit);
    return PT[PT_index].prediction;
}

//update PT
VOID PT_update(ADDRINT ins_ptr, bool taken)
{
    UINT64 PT_index = convert_PT_index(HHRT[HHRT_hash_address(ins_ptr)].hrt_bit);
    if(taken)//taken
    {
        if(PT[PT_index].status != 3)
            PT[PT_index].status++;
    }
    else//not taken
    {
        if(PT[PT_index].status != 0)
            PT[PT_index].status--;
    }

    if(PT[PT_index].status == 0 || PT[PT_index].status == 1)
        PT[PT_index].prediction = false;//not taken
    else
        PT[PT_index].prediction = true;//taken
}

//update HHRT
VOID HHRT_update(ADDRINT ins_ptr, bool taken)
{
    UINT64 HHRT_index = HHRT_hash_address(ins_ptr);
    //move to left
    for(UINT64 i = 1; i < K ; i++)
    {
        HHRT[HHRT_index].hrt_bit[i - 1] = HHRT[HHRT_index].hrt_bit[i];
    }
    //add the most recent result to the least significant bit
    if(taken)
        HHRT[HHRT_index].hrt_bit[K-1] = 1;
    else
        HHRT[HHRT_index].hrt_bit[K-1] = 0;
}

/* ===================================================================== */

/* ===================================================================== */

static INT32 Usage()
{
    cerr << "This pin tool collects a profile of jump/branch/call instructions for an application\n";

    cerr << KNOB_BASE::StringKnobSummary();

    cerr << endl;
    return -1;
}

/* ===================================================================== */

VOID write_results(bool limit_reached)
{
    string output_file = KnobOutputFile.Value();
    if(KnobPid) output_file += "." + decstr(getpid());
    
    std::ofstream out(output_file.c_str());

    if(limit_reached)
        out << "Reason: limit reached\n";
    else
        out << "Reason: fini\n";
    out << "Count Seen: " << CountSeen << endl;
    out << "Count Taken: " << CountTaken << endl;
    out << "Count Correct: " << CountCorrect << endl;
    out.close();
}

/* ===================================================================== */

VOID br_predict(ADDRINT ins_ptr, INT32 taken)
{
    //count the number of branches seen
	CountSeen++;
	//count the take branches
	if (taken){
			CountTaken++;
	}
	
	
	//count the correctly predicted branches
	if(HHRT_prediction(ins_ptr) == taken) 
		CountCorrect++;

	//update HHRT and PT
    PT_update(ins_ptr, taken);
	HHRT_update(ins_ptr, taken);

    if(CountSeen == KnobBranchLimit.Value())
    {
        write_results(true);
        exit(0);
    }
} 


/* ===================================================================== */
// Do not need to change instrumentation code here. Only need to modify the analysis code. 
VOID Instruction(INS ins, void *v)
{

// The subcases of direct branch and indirect branch are
// broken into "call" or "not call".  Call is for a subroutine
// These are left as subcases in case the programmer wants
// to extend the statistics to see how sub cases of branches behave

    if( INS_IsRet(ins) )
    {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) br_predict, 
            IARG_INST_PTR, IARG_BRANCH_TAKEN,  IARG_END);
    }
    else if( INS_IsSyscall(ins) )
    {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) br_predict, 
            IARG_INST_PTR, IARG_BRANCH_TAKEN,  IARG_END);
    }
    else if (INS_IsBranch(ins))
    {
        if( INS_IsCall(ins) ) {
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) br_predict, 
                IARG_INST_PTR, IARG_BRANCH_TAKEN,  IARG_END);
        }
        else {
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) br_predict, 
                IARG_INST_PTR, IARG_BRANCH_TAKEN,  IARG_END);
        }
    }
    
}

/* ===================================================================== */

#define OUT(n, a, b) out << n << " " << a << setw(16) << CountSeen. b  << " " << setw(16) << CountTaken. b << endl

VOID Fini(int n, void *v)
{
    write_results(false);
}


/* ===================================================================== */


/* ===================================================================== */

int main(int argc, char *argv[])
{
    
    if( PIN_Init(argc,argv) )
    {
        return Usage();
    }
    PT_init();
    HHRT_init();

        
    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    PIN_StartProgram();
    
    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
