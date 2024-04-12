/*
CS146_FinalProject
Created by:Mickey Chang
----Gshare Predictor----
*/



#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <map>
#include <cstdlib>
#include <cmath>
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

KNOB<UINT64> KnobGHRSize(KNOB_MODE_WRITEONCE, "pintool", "ghrSize", "8", "specify the size of the GHR");
//KNOB<UINT64> KnobPTSize(KNOB_MODE_WRITEONCE, "pintool", "ptSize", "4096", "specify the size of the PT");
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
UINT64 SIZE1 = 8;//GHRsize
//UINT64 SIZE2 = 4096;//PTsize
//UINT64 K = 12;
//UINT64 mask1 = SIZE1-1;
UINT64 mask = 255;//2^8 - 1

int* GHR = NULL;//global history register

struct pt_entry
{
    bool prediction;
    int status;
};


pt_entry* PT = NULL;

//initialize GHR; GHR[0] is the least significant bit
VOID GHR_init()
{
    SIZE1 = KnobGHRSize.Value();
    GHR = new int[SIZE1];
    for(UINT64 i = 0;i < SIZE1; i++)
    {
        GHR[i] = 0;//initialize GHR to 0
    }
}
//initialize PT
VOID PT_init()
{
    UINT64 SIZE2 = pow(2, SIZE1);//SIZE2 = 2^SIZE1
    mask = SIZE2 - 1;

    PT = new pt_entry[SIZE2];
    for(UINT64 i = 0; i < SIZE2; i++)
    {
        PT[i].prediction = false;
        PT[i].status = 0;
    }
}
//XOR the GHR and the address to get the index of the PT
UINT64 XOR_address(ADDRINT ins_ptr)
{
    UINT64 XOR_address = ins_ptr & mask;
    for(UINT64 i = 0; i < SIZE1; i++)
    {
        XOR_address = XOR_address ^ (GHR[i] << i);
    }
    return XOR_address;
}


//make prediction
bool GHR_prediction(ADDRINT ins_ptr)
{
    UINT64 PT_index = XOR_address(ins_ptr);
    return PT[PT_index].prediction;
}

//update PT
VOID PT_update(ADDRINT ins_ptr, bool taken)
{
    UINT64 PT_index = XOR_address(ins_ptr);
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

//update GHR
VOID GHR_update(ADDRINT ins_ptr, bool taken)
{

    //move to left
    for(UINT64 i = 1; i < SIZE1 ; i++)
    {
        GHR[i - 1] = GHR[i];
    }
    //add the most recent result to the least significant bit
    if(taken)
        GHR[SIZE1-1] = 1;
    else
        GHR[SIZE1-1] = 0;
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
	if(GHR_prediction(ins_ptr) == taken) 
		CountCorrect++;

	//update GHR and PT
    PT_update(ins_ptr, taken);
	GHR_update(ins_ptr, taken);

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
    GHR_init();
    PT_init();
    

        
    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    PIN_StartProgram();
    
    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
