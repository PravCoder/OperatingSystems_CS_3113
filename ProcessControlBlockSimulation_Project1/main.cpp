// ==================================================
// Process Control Block Simulation Project 1
// What is struct. 
// ==================================================
#include <iostream>
using namespace std;
#include <vector> 
#include <string>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <queue>
#include <limits>


struct PCB
{
    int processID;       // identifer of process
    int state;        // "new", "ready", "run","terminated"
    int programCounter;  // index of next instruction to be executed, in logical memory
    int instructionBase; // starting address of instructions for this process
    int dataBase;        // address where the data for the instructions starts in logical memory
    int memoryLimit;     // total size of logical memory allocated to the process    
    int cpuCyclesUsed;   // number of cpu cycles process as consumed so far
    int registerValue;   // value of cpu register associated with process
    int maxMemoryNeeded; // max logical memory required by process as defined in input file
    int mainMemoryBase;  // starting address in main memory where process, PCB+logical_memory is laoded.  

    vector<vector<int> > instructions; // each arr is a instruction, each element in vector is data assoicated with instruction, 1st element being opcode
};
void show_PCB(PCB process) {
    cout << "PROCESS ["<<process.processID<<"] " << "maxMemoryNeeded: " << process.maxMemoryNeeded << endl;
    cout << "num-instructions: " << process.instructions.size() << endl;
    for (int i=0; i<process.instructions.size(); i++) {
        vector<int> cur_instruction = process.instructions[i];
        if (cur_instruction[0] == 1) { // compute
            cout << "instruction: " << cur_instruction[0] << " iter: " << cur_instruction[1] << " cycles: " << cur_instruction[2] << endl;
        }
        if (cur_instruction[0] == 2) {
            cout << "instruction: " << cur_instruction[0] << " cycles: " << cur_instruction[1] << endl;
        }
        if (cur_instruction[0] == 3) {
            cout << "instruction: " << cur_instruction[0] << " value: " << cur_instruction[1] << " address: " << cur_instruction[2] << endl;
        }
        if (cur_instruction[0] == 4) {
            cout << "instruction: " << cur_instruction[0] << " address: " << cur_instruction[1] << endl;
        }
        
    }
}


/*
First 10 address hold PCB metadata 10 fields.
Next address contain all instructions opcodes for process. 
Remaining addresses contain data associated with each of instructions in order (cycles, value, address, iterations)
pcb.instructionBase points to the starting address of this isntruction segemnt in main memory
pcb.dataBase points to the address where the data for each instruction starts.
Add pcb start address to readyQueue. 
*/
void loadJobsToMemory(queue<PCB>& newJobQueue, queue<int>& readyQueue, vector<int>& mainMemory, int maxMemory) {
    // TODO: Implement loading jobs into main memory, store process meta data and instructions

    while (!newJobQueue.empty()) {
        PCB cur_process = newJobQueue.front();  // Access front element
        show_PCB(cur_process);
        newJobQueue.pop();  
    }
}


void executeCPU(int startAddress, int* mainMemory) {
    // TODO: Implement CPU instruction execution
}


/* 
Compile: g++ -o main main.cpp
Run: ./main < input1.txt
*/
int main() {
    // Step 1: Read and parse input file
    // TODO: Implement input parsing and populate newJobQueue
    // iterate number of process and extract each
    // Step 1: Read and parse input file
    int maxMemory;
    int num_processes;
    vector<int> mainMemory;
    queue<int> readyQueue;
    queue<PCB> newJobQueue;
    cin >> maxMemory;
    cin >> num_processes;
    cout << "main memory: " << maxMemory << "\n";
    cout << "number of process: " << num_processes << "\n";
    mainMemory.resize(maxMemory, -1);

    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    for (int i = 0; i < num_processes; i++) {
        string line;
        getline(cin, line);  // read entire process line
        istringstream ss(line);
        //cout << "Raw Line: " << line << endl;

        int cur_process_id, cur_process_max_memory_needed, cur_process_num_instructions;  
        ss >> cur_process_id >> cur_process_max_memory_needed >> cur_process_num_instructions;   // read in first 3 variables of process

        PCB process;
        process.processID = cur_process_id;
        process.maxMemoryNeeded = cur_process_max_memory_needed;
        process.state = 0; 
        process.programCounter = 0;
        process.memoryLimit = process.maxMemoryNeeded;
        process.cpuCyclesUsed = 0;
        process.registerValue = 0;
        
        //cout << "CUR-PROCESS: ID: " << process.processID << " max_mem: " << process.maxMemoryNeeded << " num_instructions: " << cur_process_num_instructions << "\n";

        // just iterate instructions of each cur-process
        for (int j = 0; j < cur_process_num_instructions; j++) {
            int instruction_opcode;  // read in opcode based on that read in data of instruction
            ss >> instruction_opcode;
            vector<int> cur_instruction;  // instruction-arr for cur-instruction
            cur_instruction.push_back(instruction_opcode); // add opcode as first element for this instruction-arr

            if (instruction_opcode == 1) { // compute
                int iterations, cycles;
                ss >> iterations >> cycles;
                cur_instruction.push_back(iterations);
                cur_instruction.push_back(cycles);
                //cout << "instruction: " << instruction_opcode << " iter: " << iterations << " cycles: " << cycles << endl;
            } 
            if (instruction_opcode == 2) {  // print
                int cycles;
                ss >> cycles;
                cur_instruction.push_back(cycles);
                //cout << "instruction: " << instruction_opcode << " cycles: " << cycles << endl;
            } 
            if (instruction_opcode == 3) {   //store
                int value, address;
                ss >> value >> address;
                cur_instruction.push_back(value);  
                cur_instruction.push_back(address);
                //cout << "instruction: " << instruction_opcode << " value: " << value << " address: " << address << endl;
            }
            if (instruction_opcode == 4) {  // load
                int address;
                ss >> address;
                cur_instruction.push_back(address);
                //cout << "instruction: " << instruction_opcode << " address: " << address << endl;
            }
            //cout << "cur-instruction size: " << cur_instruction.size() << endl;
            process.instructions.push_back(cur_instruction);
        }
        // push cur-pcb to new-job-queue
        newJobQueue.push(process);  
    }

    

    
    // Step 2: Load jobs into main memory
    loadJobsToMemory(newJobQueue, readyQueue, mainMemory, maxMemory);





    // Step 3: After you load the jobs in the queue go over the main memory
    // and print the content of mainMemory. It will be in the table format
    // three columns as I had provided you earlier.






    // // Step 4: Process execution
    // while (!readyQueue.empty()) {
    // int startAddress = readyQueue.front();
    // //readyQueue contains start addresses w.r.t main memory for jobs
    // readyQueue.pop();
    // // Execute job
    // executeCPU(startAddress, mainMemory);
    // // Output Job that just completed execution – see example below
    // }




    return 0;
}
