// ==================================================
// Process Control Block Simulation Project 2 
// ==================================================
#include <iostream>
#include <vector> 
#include <string>
#include <fstream>
#include <sstream>
#include <queue>
#include <limits>
#include <unordered_map>
using namespace std;

struct PCB
{
    int processID;       // identifier of process
    int state;           // 1="new", 2="ready", 3="run", 4="terminated"
    int programCounter;  // index of next instruction to be executed, in logical memory
    int instructionBase; // starting address of instructions for this process
    int dataBase;        // address where the data for the instructions starts in logical memory
    int memoryLimit;     // total size of logical memory allocated to the process    
    int cpuCyclesUsed;   // number of cpu cycles process as consumed so far
    int registerValue;   // value of cpu register associated with process
    int maxMemoryNeeded; // max logical memory required by process as defined in input file
    int mainMemoryBase;  // starting address in main memory where process, PCB+logical_memory is loaded.  

    vector<vector<int> > instructions; // each arr is a instruction, each element in vector is data associated with instruction
    
    // Constructor to initialize values - match working version exactly
    PCB() : programCounter(0), cpuCyclesUsed(0), registerValue(0) {}
};

// Global variables for timing and process management
int cpuAllocated;         // Maximum CPU time before timeout
int globalClock = 0;      // Global system clock
int contextSwitchTime;    // Time required for context switching
int IOWaitTime = 0;       // Time required for IO operations
bool timeOutInterrupt = false;  // Flag for timeout
bool IOInterrupt = false;      // Flag for IO interrupts

// Map to track process execution times
unordered_map<int, int> processStartTimes;

// Structure to hold IO waiting processes [startAddress, timeEntered, timeNeeded]
struct IOWaitInfo {
    int startAddress;
    int timeEntered;
    int timeNeeded;
};

void printPCBFromMainMemory(int startAddress, vector<int>& mainMemory) {
    // Determine the state string
    string state;
    int stateInt = mainMemory[startAddress + 1];
    if (stateInt == 1) {
        state = "NEW";
    } else if (stateInt == 2) {
        state = "READY";
    } else if (stateInt == 3) {
        state = "RUNNING";
    } else {
        state = "TERMINATED";
    }

    // Print PCB fields
    cout << "Process ID: " << mainMemory[startAddress] << "\n";
    cout << "State: " << state << "\n";
    cout << "Program Counter: " << mainMemory[startAddress + 2] << "\n";
    cout << "Instruction Base: " << mainMemory[startAddress + 3] << "\n";
    cout << "Data Base: " << mainMemory[startAddress + 4] << "\n";
    cout << "Memory Limit: " << mainMemory[startAddress + 5] << "\n";
    cout << "CPU Cycles Used: " << mainMemory[startAddress + 6] << "\n";
    cout << "Register Value: " << mainMemory[startAddress + 7] << "\n";
    cout << "Max Memory Needed: " << mainMemory[startAddress + 8] << "\n";
    cout << "Main Memory Base: " << mainMemory[startAddress + 9] << "\n";
    
    // Calculate total CPU cycles consumed
    cout << "Total CPU Cycles Consumed: " << globalClock - processStartTimes[mainMemory[startAddress]] << "\n";
}

void loadJobsToMemory(queue<PCB>& newJobQueue, queue<int>& readyQueue, vector<int>& mainMemory, int maxMemory) {
    int memoryIndex = 0;
    
    while (!newJobQueue.empty()) {
        PCB pcb = newJobQueue.front();
        newJobQueue.pop();
        
        // Record starting memory position for this PCB
        int startingMemory = memoryIndex;
        
        // Store PCB metadata in main memory - match working version exactly
        mainMemory[memoryIndex++] = pcb.processID;
        mainMemory[memoryIndex++] = 1;  // State is NEW (1)
        mainMemory[memoryIndex++] = pcb.programCounter;
        mainMemory[memoryIndex++] = startingMemory + 10;  // instructionBase starts after 10 PCB fields
        // We'll update dataBase later after loading instructions
        int dataBaseIndex = memoryIndex;  // Save position to update later
        memoryIndex++;  // Skip over dataBase field for now
        mainMemory[memoryIndex++] = pcb.memoryLimit;
        mainMemory[memoryIndex++] = pcb.cpuCyclesUsed;
        mainMemory[memoryIndex++] = pcb.registerValue;
        mainMemory[memoryIndex++] = pcb.maxMemoryNeeded;
        mainMemory[memoryIndex++] = startingMemory;  // mainMemoryBase points to start of PCB
        
        // Create queue for data values
        queue<int> dataQueue;
        
        // Load instructions to memory and collect data values
        int numberOfInstructions = 0;
        for (int i = 0; i < pcb.instructions.size(); i++) {
            vector<int> instruction = pcb.instructions[i];
            int opcode = instruction[0];
            
            // Store the opcode
            mainMemory[memoryIndex++] = opcode;
            numberOfInstructions++;
            
            // Process data based on opcode
            if (opcode == 1) {  // compute
                dataQueue.push(instruction[1]);  // iterations
                dataQueue.push(instruction[2]);  // cycles
            }
            else if (opcode == 2) {  // print
                dataQueue.push(instruction[1]);  // cycles
            }
            else if (opcode == 3) {  // store
                dataQueue.push(instruction[1]);  // value
                dataQueue.push(instruction[2]);  // address
            }
            else if (opcode == 4) {  // load
                dataQueue.push(instruction[1]);  // address
            }
        }
        
        // Now update the dataBase field with current memory position
        mainMemory[dataBaseIndex] = memoryIndex;
        
        // Load data values to memory
        int dataSize = dataQueue.size();
        for (int i = 0; i < dataSize; i++) {
            mainMemory[memoryIndex++] = dataQueue.front();
            dataQueue.pop();
        }
        
        // Skip remaining memory allocated to the process
        int remainingMemory = pcb.memoryLimit - dataSize - numberOfInstructions;
        memoryIndex += remainingMemory;
        
        // Add starting address to ready queue
        readyQueue.push(startingMemory);
    }
    
    // Print the entire main memory after loading all processes
    for (int i = 0; i < mainMemory.size(); i++) {
        cout << i << " : " << mainMemory[i] << "\n";
    }
}

// Check for IO waiting processes that have completed
void checkIOWaitingQueue(queue<int>& readyQueue, vector<int>& mainMemory, queue<IOWaitInfo>& IOWaitingQueue) {
    int size = IOWaitingQueue.size();
    for (int i = 0; i < size; i++) {
        IOWaitInfo waitInfo = IOWaitingQueue.front();
        IOWaitingQueue.pop();
        
        // Get process ID from the start address
        int processID = mainMemory[waitInfo.startAddress];
        
        // Check if IO wait time has elapsed
        if (globalClock - waitInfo.timeEntered >= waitInfo.timeNeeded) {
            readyQueue.push(waitInfo.startAddress);
            cout << "print" << endl;
            cout << "Process " << processID << " completed I/O and is moved to the ReadyQueue." << endl;
        } else {
            // Put back in queue if still waiting
            IOWaitingQueue.push(waitInfo);
        }
    }
}

void executeCPU(int startAddress, vector<int>& mainMemory, queue<int>& readyQueue, queue<IOWaitInfo>& IOWaitingQueue) {
    // Reset interrupt flags
    timeOutInterrupt = false;
    IOInterrupt = false;
    
    // Extract PCB fields from main memory - use consistent naming with working version
    int processID = mainMemory[startAddress];
    int stateIndex = startAddress + 1;
    int programCounterIndex = startAddress + 2;
    int instructionBase = mainMemory[startAddress + 3];
    int dataBase = mainMemory[startAddress + 4];  // This is the actual address where data starts
    int memoryLimitIndex = startAddress + 5;
    int cpuCyclesConsumedIndex = startAddress + 6;
    int registerIndex = startAddress + 7;
    
    // Apply context switch time
    globalClock += contextSwitchTime;
    
    // Initialize metadata index to point to data
    int metaDataIndex = dataBase - 1;  // Will be incremented before first use
    
    // Set initial program counter
    int programCounter;
    if (mainMemory[stateIndex] == 1) {  // NEW state
        // First time running this process
        programCounter = instructionBase;
        processStartTimes[processID] = globalClock;
    } else {
        // Process was interrupted before
        programCounter = mainMemory[programCounterIndex];
        
        // Calculate how deep into the metadata we are based on the program counter
        int instructionsCompleted = programCounter - instructionBase;
        for (int i = instructionBase; i < programCounter; i++) {
            int tempOpcode = mainMemory[i];
            if (tempOpcode == 1 || tempOpcode == 3) {  // compute or store (2 data items)
                metaDataIndex += 2;
            } else {  // print or load (1 data item)
                metaDataIndex += 1;
            }
        }
    }
    
    // Update state to RUNNING
    mainMemory[stateIndex] = 3;
    cout << "Process " << processID << " has moved to Running." << endl;
    
    // Track CPU cycles for this quantum
    int cpuCyclesConsumed = 0;
    
    // Get first opcode
    int opcodeValue = mainMemory[programCounter];
    
    // Execute instructions
    while (programCounter < dataBase && cpuCyclesConsumed < cpuAllocated) {
        // COMPUTE instruction
        if (opcodeValue == 1) {
            cout << "compute" << endl;
            
            metaDataIndex++;  // Skip iterations
            metaDataIndex++;  // Now at cycles
            
            // Update cycle counts - 
            globalClock += mainMemory[metaDataIndex];
            cpuCyclesConsumed += mainMemory[metaDataIndex];
            mainMemory[cpuCyclesConsumedIndex] += mainMemory[metaDataIndex];
        }
        // PRINT instruction (I/O operation)
        else if (opcodeValue == 2) {
            metaDataIndex++;  // Now at cycles
            
            // Update cycle tracking for I/O - match working version
            cpuCyclesConsumed += mainMemory[metaDataIndex];
            mainMemory[cpuCyclesConsumedIndex] += mainMemory[metaDataIndex];
            IOWaitTime = mainMemory[metaDataIndex];
            IOInterrupt = true;
            programCounter++;  // Advance program counter before break
            break;
        }
        // STORE instruction
        else if (opcodeValue == 3) {
            metaDataIndex++;  // Now at value to store
            
            // First update register value
            mainMemory[registerIndex] = mainMemory[metaDataIndex];
            
            metaDataIndex++;  // Now at address
            
            // Calculate physical address
            int locationToBeStored = mainMemory[metaDataIndex] + startAddress;
            int endOfCurrentMemory = startAddress + mainMemory[memoryLimitIndex];
            
            // Check address bounds
            if (locationToBeStored >= endOfCurrentMemory || locationToBeStored < dataBase) {
                cout << "store error!" << endl;
            } else {
                // Store the register value at the specified location
                mainMemory[locationToBeStored] = mainMemory[registerIndex];
                cout << "stored" << endl;
            }
            
            // Update cycle counts
            globalClock++;
            cpuCyclesConsumed += 1;
            mainMemory[cpuCyclesConsumedIndex] += 1;
        }
        // LOAD instruction
        else if (opcodeValue == 4) {
            metaDataIndex++;  // Now at address
            
            // Calculate physical address
            int location = mainMemory[metaDataIndex] + startAddress;
            int endOfCurrentMemory = startAddress + mainMemory[memoryLimitIndex];
            
            // Check address bounds
            if (location >= endOfCurrentMemory) {
                cout << "load error!" << endl;
            } else {
                // Load value from memory into register
                mainMemory[registerIndex] = mainMemory[location];
                cout << "loaded" << endl;
            }
            
            // Update cycle counts
            globalClock++;
            cpuCyclesConsumed += 1;
            mainMemory[cpuCyclesConsumedIndex] += 1;
        }
        
        // Advance to next instruction
        programCounter++;
        
        // Get next opcode
        if (programCounter < dataBase) {
            opcodeValue = mainMemory[programCounter];
            
            // Check if CPU time limit is reached
            if (cpuCyclesConsumed >= cpuAllocated) {
                timeOutInterrupt = true;
                break;
            }
        }
    }
    
    // Save the current program counter
    mainMemory[programCounterIndex] = programCounter;
    
    // Handle interrupts
    if (IOInterrupt) {
        cout << "Process " << processID << " issued an IOInterrupt and moved to the IOWaitingQueue." << endl;
        IOWaitInfo waitInfo = {startAddress, globalClock, IOWaitTime};
        IOWaitingQueue.push(waitInfo);
        return;
    }
    
    if (timeOutInterrupt) {
        cout << "Process " << processID << " has a TimeOUT interrupt and is moved to the ReadyQueue." << endl;
        mainMemory[stateIndex] = 2;  // Set state back to READY
        readyQueue.push(startAddress);
        return;
    }
    
    // If we get here, the process has completed
    mainMemory[stateIndex] = 4;  // TERMINATED
    
    // Reset program counter to one less than instructionBase (important!)
    mainMemory[programCounterIndex] = instructionBase - 1;
    
    printPCBFromMainMemory(startAddress, mainMemory);
    cout << "Process " << processID << " terminated. Entered running state at: " 
         << processStartTimes[processID] << ". Terminated at: " << globalClock 
         << ". Total Execution Time: " << globalClock - processStartTimes[processID] << "." << endl;
}

int main() {
    // Initialize variables
    int maxMemory;
    int num_processes;
    vector<int> mainMemory;
    queue<int> readyQueue;
    queue<PCB> newJobQueue;
    queue<IOWaitInfo> IOWaitingQueue;

    // Read input parameters
    cin >> maxMemory;
    cin >> cpuAllocated;
    cin >> contextSwitchTime;
    cin >> num_processes;
    
    // Initialize main memory
    mainMemory.resize(maxMemory, -1);

    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    
    // Read and parse process input
    for (int i = 0; i < num_processes; i++) {
        string line;
        getline(cin, line);
        istringstream ss(line);

        int cur_process_id, cur_process_max_memory_needed, cur_process_num_instructions;
        ss >> cur_process_id >> cur_process_max_memory_needed >> cur_process_num_instructions;

        PCB process;
        process.processID = cur_process_id;
        process.maxMemoryNeeded = cur_process_max_memory_needed;
        process.memoryLimit = cur_process_max_memory_needed;
        
        // Read instruction information
        for (int j = 0; j < cur_process_num_instructions; j++) {
            int instruction_opcode;
            ss >> instruction_opcode;
            vector<int> cur_instruction;
            cur_instruction.push_back(instruction_opcode);

            if (instruction_opcode == 1) { // compute
                int iterations, cycles;
                ss >> iterations >> cycles;
                cur_instruction.push_back(iterations);
                cur_instruction.push_back(cycles);
            } 
            else if (instruction_opcode == 2) {  // print
                int cycles;
                ss >> cycles;
                cur_instruction.push_back(cycles);
            } 
            else if (instruction_opcode == 3) {  // store
                int value, address;
                ss >> value >> address;
                cur_instruction.push_back(value);  
                cur_instruction.push_back(address);
            }
            else if (instruction_opcode == 4) {  // load
                int address;
                ss >> address;
                cur_instruction.push_back(address);
            }
            process.instructions.push_back(cur_instruction);
        }
        
        // Add process to queue
        newJobQueue.push(process);
    }

    // Load jobs into main memory
    loadJobsToMemory(newJobQueue, readyQueue, mainMemory, maxMemory);

    // Process execution loop
    while (!readyQueue.empty() || !IOWaitingQueue.empty()) {
        if (!readyQueue.empty()) {
            int pcb_start_addr = readyQueue.front();
            readyQueue.pop();
            executeCPU(pcb_start_addr, mainMemory, readyQueue, IOWaitingQueue);
        } else {
            // When ready queue is empty but IO waiting queue is not,
            // just advance the clock without extra messaging
            globalClock += contextSwitchTime;
        }
        
        // Check for completed IO operations
        checkIOWaitingQueue(readyQueue, mainMemory, IOWaitingQueue);
    }
    
    // Final context switch
    globalClock += contextSwitchTime;
    cout << "Total CPU time used: " << globalClock << "." << endl;

    return 0;
}