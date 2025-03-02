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
#include <fstream> 
#include <cstdio>


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

    int cur_address = 0;
    while (!newJobQueue.empty()) {
        PCB cur_process = newJobQueue.front();  // access front elemen
        //show_PCB(cur_process); 
        newJobQueue.pop();  

        cur_process.mainMemoryBase = cur_address;
        cur_process.instructionBase = cur_address + 10; 
        cur_process.dataBase = cur_process.instructionBase + cur_process.instructions.size();

        mainMemory[cur_address] = cur_process.processID;
        mainMemory[cur_address + 1] = 1;
        mainMemory[cur_address + 2] = cur_process.programCounter;
        mainMemory[cur_address + 3] = cur_process.instructionBase;
        mainMemory[cur_address + 4] = cur_process.dataBase;
        mainMemory[cur_address + 5] = cur_process.memoryLimit;
        mainMemory[cur_address + 6] = cur_process.cpuCyclesUsed;
        mainMemory[cur_address + 7] = cur_process.registerValue;
        mainMemory[cur_address + 8] = cur_process.maxMemoryNeeded;
        mainMemory[cur_address + 9] = cur_process.mainMemoryBase;

        int num_instructions = cur_process.instructions.size();
        //cout << "num-instructions for job: " << num_instructions << endl;
        //cout << "size of main memory: " << mainMemory.size() << endl;

        int instructionAddress = cur_process.instructionBase;
        int dataAddress = cur_process.dataBase;
        for (int i=0; i<num_instructions; i++) {
            vector<int> cur_instruction = cur_process.instructions[i];
            int opcode = cur_instruction[0];
           
            mainMemory[instructionAddress++] = opcode; // access then post-increment 
            if (opcode == 1) {  // compute
                //cout << "FLAG1: "<< cur_instruction.size() << endl;
                mainMemory[dataAddress++] = cur_instruction[1];
                mainMemory[dataAddress++] = cur_instruction[2];
            }
            else if (opcode == 2) {  // print
                mainMemory[dataAddress++] = cur_instruction[1];
                //cout << "FLAG4" << endl;
            }
            else if (opcode == 3) {  // store
                mainMemory[dataAddress++] = cur_instruction[1];
                mainMemory[dataAddress++] = cur_instruction[2];
                //cout << "FLAG5" << endl;
            }
            else if (opcode == 4) {  // load
                mainMemory[dataAddress++] = cur_instruction[1];
                //cout << "FLAG6" << endl;
            }
        }
        cur_address = cur_process.instructionBase + cur_process.maxMemoryNeeded;  // change next process jump formula
        readyQueue.push(cur_process.mainMemoryBase);  // add the base-address of process to readQueue

        
    }
}

// first version
void executeCPU(int startAddress, vector<int>&  mainMemory) {  // given starting-address of a pcb-struct
    // TODO: Implement CPU instruction execution
    
    // extract attributes of pcb from main memory because they are stored sequentially
    int processID = mainMemory[startAddress];
    int state = mainMemory[startAddress + 1];
    int programCounter = mainMemory[startAddress + 2];  // create temporary variables that do no modify memory just yet!!
    int instructionBase = mainMemory[startAddress + 3];
    int dataBase = mainMemory[startAddress + 4];
    int memoryLimit = mainMemory[startAddress + 5];
    int cpuCyclesUsed = mainMemory[startAddress + 6];
    int registerValue = mainMemory[startAddress + 7];
    int maxMemoryNeeded = mainMemory[startAddress + 8];
    int mainMemoryBase = mainMemory[startAddress + 9];

    // number of instructions or opcodes is distance betwen database and isntructionbase
    int num_instructions = dataBase - instructionBase; 
    int data_size = memoryLimit - num_instructions; // size of data-segment


    // index in the data segment of instructions
    int logical_memory_indx = 0; 

    // iterate number of instructions or opcodes
    for (int i=0; programCounter < num_instructions; i++) {
        int cur_opcode = mainMemory[instructionBase + i]; // get current opcode in memory, using address of thwere instructions start plus ith instruction
        vector<int> cur_instruction_data;  // each element is the parameters for the current instruction/opcode

        // compute - has 2 arguments
        if (cur_opcode == 1) {
            if (logical_memory_indx + 1 >= data_size) {  // if not enough parameters to do a compute operation push -1 not possible
                cur_instruction_data.push_back(-1);
                cur_instruction_data.push_back(-1);
            } else {
                cur_instruction_data.push_back(mainMemory[dataBase + logical_memory_indx]);  // iterations
                cur_instruction_data.push_back(mainMemory[dataBase + logical_memory_indx + 1]);  // cycles
            }
            logical_memory_indx += 2;
        }

        // store - has 2 arguments
        else if (cur_opcode == 3) {
            if (logical_memory_indx + 1 >= data_size) {
                cur_instruction_data.push_back(-1);
                cur_instruction_data.push_back(-1);
            } else {
                cur_instruction_data.push_back(mainMemory[dataBase + logical_memory_indx]);   // value
                cur_instruction_data.push_back(mainMemory[dataBase + logical_memory_indx + 1]);  // address
            }
            logical_memory_indx += 2;
        }

        // load - has 1 arguments
        else if (cur_opcode == 4) {
            if (logical_memory_indx >= data_size) {
                cur_instruction_data.push_back(-1);
            } else {
                cur_instruction_data.push_back(mainMemory[dataBase + logical_memory_indx]);  // address
            }
            logical_memory_indx += 1;
        }

        // print - has 1 arguments
        else if (cur_opcode == 2) {
            if (logical_memory_indx >= data_size) {
                cur_instruction_data.push_back(-1);
            } else {
                cur_instruction_data.push_back(mainMemory[dataBase + logical_memory_indx]);
            }
            logical_memory_indx += 1;
        }


        // process each instruction opcode and modify the variables
        // COMPUTE
        if (cur_opcode == 1) {
            cpuCyclesUsed += cur_instruction_data[1];
            cout << "compute" << "\n";
        }
        // PRINT
        else if (cur_opcode == 2) {
            cpuCyclesUsed += cur_instruction_data[0];
            cout << "print" << "\n";
        }
        // STORE
        else if (cur_opcode == 3) {
            // check if we are sindide data segment
            if (cur_instruction_data[1] + instructionBase >= instructionBase && (cur_instruction_data[1] + instructionBase) < maxMemoryNeeded + instructionBase) { 
                mainMemory[cur_instruction_data[1] + instructionBase] = cur_instruction_data[0];
                registerValue = cur_instruction_data[0];
                cout << "stored" << "\n";
            } else {
                registerValue = cur_instruction_data[0];
                cout << "store error!" << "\n";
            }
            cpuCyclesUsed++;
        }
        // LOAD
        else if (cur_opcode == 4) {
            // check if we are sindide data segment
            if ((cur_instruction_data[0] + instructionBase) >= instructionBase && (cur_instruction_data[0] + instructionBase) < (maxMemoryNeeded + instructionBase)) {
                registerValue = mainMemory[cur_instruction_data[0] + instructionBase];
                cout << "loaded" << "\n";
            } else{
                cout << "load error!" << "\n";
            }
            cpuCyclesUsed++;
        }
        // some opcode error
        else {
            cerr << "ERROR: Invalid opcode " << cur_opcode << "\n";
        }

        programCounter++; // increment program counter to go to next instruction
    }

    mainMemory[startAddress + 1] = 4;              // terminate process
    mainMemory[startAddress + 2] = instructionBase - 1; // update program counter for this pcb, to be before instructionBase
    mainMemory[startAddress + 6] = cpuCyclesUsed;
    mainMemory[startAddress + 7] = registerValue;

    // Print PCB information.
    cout << "Process ID: " << processID << "\n";
    cout << "State: TERMINATED\n";
    cout << "Program Counter: " << mainMemory[startAddress + 2] << "\n";
    cout << "Instruction Base: " << instructionBase << "\n";
    cout << "Data Base: " << dataBase << "\n";
    cout << "Memory Limit: " << memoryLimit << "\n";
    cout << "CPU Cycles Used: " << cpuCyclesUsed << "\n";
    cout << "Register Value: " << registerValue << "\n";
    cout << "Max Memory Needed: " << maxMemoryNeeded << "\n";
    cout << "Main Memory Base: " << mainMemoryBase << "\n";
    cout << "Total CPU Cycles Consumed: " << cpuCyclesUsed << "\n";



}




void show_main_memory(vector<int> &mainMemory, int rows) {
    for (int i = 0; i < rows; i++){
        cout << i << ": " << mainMemory[i] << endl;
    }
    cout << endl;
}


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
    //cout << "main memory: " << maxMemory << "\n";
    //cout << "number of process: " << num_processes << "\n";
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
    //cout << "Main Memory After Loading Processes:" << endl;
    //show_main_memory(mainMemory, 400);
    for (int i = 0; i < mainMemory.size(); i++){
        cout << i << " : " << mainMemory[i] << "\n";
    }



    // // Step 4: Process execution
    while (!readyQueue.empty())
    {
        int pcd_start_addy = readyQueue.front();
        readyQueue.pop();
        executeCPU(pcd_start_addy, mainMemory);
    }


    return 0;
}


/* 
g++ -o CS3113_Project1 CS3113_Project1.cpp
./CS3113_Project1  < input1.txt
*/