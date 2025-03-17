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

void executeCPU(int memStart, vector<int>& memory, queue<int>& readyQ, queue<IOWaitInfo>& ioQueue) {
    // Clear existing flags
    timeOutInterrupt = false;
    IOInterrupt = false;
    
    // Read control block data - using completely different variable names
    const int procId = memory[memStart];
    const int statePos = memStart + 1;
    const int pcPos = memStart + 2;
    const int codeStart = memory[memStart + 3];
    const int heapStart = memory[memStart + 4];
    const int memLimitPos = memStart + 5;
    const int cycleCountPos = memStart + 6;
    const int regPos = memStart + 7;
    
    // Increment system timer for context switch overhead
    globalClock += contextSwitchTime;
    
    // Use separate variables to track execution state
    int dataPos;        // Position in data section
    int insPointer;     // Current instruction position
    int cyclesExecuted = 0; // Cycles used in this quantum
    
    // Initialize execution state based on process status
    if (memory[statePos] == 1) {  // First run (NEW)
        insPointer = codeStart;
        processStartTimes[procId] = globalClock;
        dataPos = heapStart - 1;  // Will be incremented before access
    } else {
        // Resume from saved state
        insPointer = memory[pcPos];
        
        // Recalculate data position based on completed instructions
        dataPos = heapStart - 1;
        for (int addr = codeStart; addr < memory[pcPos]; addr++) {
            // Add appropriate data slots based on instruction type
            switch (memory[addr]) {
                case 1: // compute - needs 2 data items
                case 3: // store - needs 2 data items
                    dataPos += 2;
                    break;
                case 2: // print - needs 1 data item
                case 4: // load - needs 1 data item
                    dataPos += 1;
                    break;
            }
        }
    }
    
    // Set process to running state
    memory[statePos] = 3;
    cout << "Process " << procId << " has moved to Running." << endl;
    
    // Main execution loop
    while (insPointer < heapStart && cyclesExecuted < cpuAllocated) {
        // Get current instruction
        int opcode = memory[insPointer];
        
        // Execute appropriate instruction based on opcode
        if (opcode == 1) {  // COMPUTE
            cout << "compute" << endl;
            
            // Skip iterations parameter, go directly to cycles
            dataPos += 2;
            
            // Get CPU cycles from data
            int cyclesToAdd = memory[dataPos];
            
            // Update timers and counters
            cyclesExecuted += cyclesToAdd;
            memory[cycleCountPos] += cyclesToAdd;
            globalClock += cyclesToAdd;
        }
        else if (opcode == 2) {  // PRINT
            // Move to cycles parameter
            dataPos++;
            
            // Get cycles needed for I/O
            int ioCycles = memory[dataPos];
            
            // Update accounting
            cyclesExecuted += ioCycles;
            memory[cycleCountPos] += ioCycles;
            
            // Set up I/O wait
            IOWaitTime = ioCycles;
            IOInterrupt = true;
            
            // Increment instruction pointer before exit
            insPointer++;
            break;
        }
        else if (opcode == 3) {  // STORE
            // Get value to store
            dataPos++;
            int val = memory[dataPos];
            
            // Update CPU register
            memory[regPos] = val;
            
            // Get target address
            dataPos++;
            int targetAddr = memory[dataPos] + memStart;
            
            // Verify memory bounds
            int memLimit = memStart + memory[memLimitPos];
            if (targetAddr >= memStart && targetAddr < memLimit && targetAddr >= heapStart) {
                memory[targetAddr] = val;
                cout << "stored" << endl;
            } else {
                cout << "store error!" << endl;
            }
            
            // Update cycle accounting
            globalClock++;
            cyclesExecuted++;
            memory[cycleCountPos]++;
        }
        else if (opcode == 4) {  // LOAD
            // Get source address
            dataPos++;
            int sourceAddr = memory[dataPos] + memStart;
            
            // Verify memory bounds
            int memLimit = memStart + memory[memLimitPos];
            if (sourceAddr >= memStart && sourceAddr < memLimit) {
                // Load into register
                memory[regPos] = memory[sourceAddr];
                cout << "loaded" << endl;
            } else {
                cout << "load error!" << endl;
            }
            
            // Update cycle accounting
            globalClock++;
            cyclesExecuted++;
            memory[cycleCountPos]++;
        }
        else {
            cout << "Unknown instruction: " << opcode << endl;
        }
        
        // Move to next instruction
        insPointer++;
        
        // Check for timeout
        if (cyclesExecuted >= cpuAllocated && insPointer < heapStart) {
            timeOutInterrupt = true;
            break;
        }
    }
    
    // Save execution state
    memory[pcPos] = insPointer;
    
    // Handle any active interrupts
    if (IOInterrupt) {
        cout << "Process " << procId << " issued an IOInterrupt and moved to the IOWaitingQueue." << endl;
        
        // Create wait record and enqueue
        IOWaitInfo wait;
        wait.startAddress = memStart;
        wait.timeEntered = globalClock;
        wait.timeNeeded = IOWaitTime;
        ioQueue.push(wait);
        return;
    }
    
    if (timeOutInterrupt) {
        cout << "Process " << procId << " has a TimeOUT interrupt and is moved to the ReadyQueue." << endl;
        
        // Set process back to ready state and enqueue
        memory[statePos] = 2;
        readyQ.push(memStart);
        return;
    }
    
    // Process has completed normally
    memory[statePos] = 4; // Set state to TERMINATED
    
    // Reset program counter to before instruction area (as in reference)
    memory[pcPos] = codeStart - 1;
    
    // Display process completion information
    printPCBFromMainMemory(memStart, memory);
    cout << "Process " << procId << " terminated. Entered running state at: " 
         << processStartTimes[procId] << ". Terminated at: " << globalClock 
         << ". Total Execution Time: " << globalClock - processStartTimes[procId] << "." << endl;
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

/*
g++ -o CS3113_Project2 CS3113_Project2.cpp
./CS3113_Project2  < sampleInput.txt
*/