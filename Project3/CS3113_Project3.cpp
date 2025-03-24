// ==================================================
// Process Control Block Simulation Project 3  
// ==================================================
#include <iostream>
#include <queue>
#include <vector>
#include <string>
#include <unordered_map>
#include <list>
using namespace std;

// Memory block structure for linked list
struct MemoryBlock {
    int processID;      // Process ID (-1 if free)
    int startAddress;   // Starting address of the block
    int size;           // Size of the block
    
    // Constructor for easier initialization
    MemoryBlock(int id, int start, int sz) : 
        processID(id), startAddress(start), size(sz) {}
};

// PCB structure
struct PCB {
    int processID;
    string state;
    int programCounter;
    int instructionBase;
    int dataBase;
    int memoryLimit;
    int cpuCyclesUsed;
    int registerValue;
    int maxMemoryNeeded;
    int mainMemoryBase;
    
    // Constructor with default initialization
    PCB() : programCounter(0), cpuCyclesUsed(0), registerValue(0), state("NEW") {}
};

// Custom structure for IOWaitingQueue items
struct IOWaitQueueItem {
    PCB process;
    int startAddress;
    int waitTime;
    int timeEnteredIO;
    
    // Constructor for easier initialization
    IOWaitQueueItem(PCB p, int start, int wait, int time) :
        process(p), startAddress(start), waitTime(wait), timeEnteredIO(time) {}
};

// Maps to store process data
unordered_map<int, int> opcodeParams; // Maps opcode to number of parameters
unordered_map<string, int> stateEncoding; // Maps state string to numeric code
unordered_map<int, vector<vector<int> > > processInstructions; // Stores instruction data for each process
unordered_map<int, int> processStartTimes; // Records when each process first enters running state
unordered_map<int, int> paramOffsets; // Tracks current parameter offset for each process

// Global variables
int globalClock = 0;
bool timeoutOccurred = false;
bool memoryFreed = false;
int contextSwitchTime, CPUAllocated;

// Initialize opcode parameter mappings
void initOpcodeParams() {
    opcodeParams[1] = 2; // Compute: iterations, cycles
    opcodeParams[2] = 1; // Print: cycles
    opcodeParams[3] = 2; // Store: value, address
    opcodeParams[4] = 1; // Load: address
}

// Initialize state encoding mappings
void initStateEncoding() {
    stateEncoding["NEW"] = 1;
    stateEncoding["READY"] = 2;
    stateEncoding["RUNNING"] = 3;
    stateEncoding["TERMINATED"] = 4;
    stateEncoding["IOWAITING"] = 5;
}

// Allocate memory for a process
int allocateMemory(list<MemoryBlock>& memoryBlocks, int processID, int size) {
    list<MemoryBlock>::iterator current = memoryBlocks.begin();
    list<MemoryBlock>::iterator prev = memoryBlocks.end();

    while (current != memoryBlocks.end()) {
        if (current->processID == -1 && current->size >= size) {
            // Found a free block big enough to allocate
            int allocatedAddress = current->startAddress;

            // If the block is exactly the right size, allocate it
            if (current->size == size) {
                current->processID = processID;
            }
            else { // Split the block into allocated and free parts
                list<MemoryBlock>::iterator newBlock = memoryBlocks.insert(current, 
                                                        MemoryBlock(processID, current->startAddress, size));

                current->startAddress += size;
                current->size -= size;
                return allocatedAddress;
            }

            return allocatedAddress;
        }

        prev = current;
        ++current;
    }

    return -1; // No block big enough found
}

// Free memory used by a process
void freeMemory(list<MemoryBlock>& memoryBlocks, vector<int>& mainMemory, int processID) {
    list<MemoryBlock>::iterator current = memoryBlocks.begin();

    // Find the memory block with this processID
    while (current != memoryBlocks.end()) {
        if (current->processID == processID) {
            // Clear memory by setting to -1
            for (int i = current->startAddress; i < current->startAddress + current->size; i++) {
                mainMemory[i] = -1;
            }

            // Mark the block as free
            current->processID = -1;
            return;
        }

        ++current;
    }
}

// Coalesce adjacent free memory blocks
void coalesceMemory(list<MemoryBlock>& memoryBlocks) {
    list<MemoryBlock>::iterator current = memoryBlocks.begin();

    // Merge adjacent free blocks
    while (current != memoryBlocks.end() && current != --memoryBlocks.end()) {
        list<MemoryBlock>::iterator next = current;
        ++next;

        if (next == memoryBlocks.end()) {
            break;
        }

        if (current->processID == -1 && next->processID == -1) {
            // Merge these two blocks
            current->size += next->size;
            memoryBlocks.erase(next);
            // Continue without advancing iterator to check for more merges
        }
        else {
            // Move to next block
            ++current;
        }
    }
}

// Load jobs from NewJobQueue into memory and add to ReadyQueue
void loadJobsToMemory(queue<PCB>& newJobQueue, queue<int>& readyQueue,
    vector<int>& mainMemory, list<MemoryBlock>& memoryBlocks) {
    
    int newJobQueueSize = newJobQueue.size();
    queue<PCB> tempQueue; // For jobs that can't be loaded this cycle

    for (int i = 0; i < newJobQueueSize; i++) {
        PCB process = newJobQueue.front();
        newJobQueue.pop();

        bool coalescedForThisProcess = false;
        int totalMemoryNeeded = process.maxMemoryNeeded + 10; // +10 for PCB metadata
        int allocatedAddress = allocateMemory(memoryBlocks, process.processID, totalMemoryNeeded);

        // If allocation failed, try coalescing memory
        if (allocatedAddress == -1) {
            cout << "Insufficient memory for Process " << process.processID << ". Attempting memory coalescing." << endl;
            coalesceMemory(memoryBlocks);

            // Try allocating again
            allocatedAddress = allocateMemory(memoryBlocks, process.processID, totalMemoryNeeded);
            coalescedForThisProcess = (allocatedAddress != -1);

            // If still no memory, put back in queue
            if (allocatedAddress == -1) {
                cout << "Process " << process.processID << " waiting in NewJobQueue due to insufficient memory." << endl;

                // Return this process to the queue
                tempQueue.push(process);

                // Also return any remaining processes
                for (int j = i + 1; j < newJobQueueSize; j++) {
                    tempQueue.push(newJobQueue.front());
                    newJobQueue.pop();
                }

                break; // Exit the loop
            }
        }

        // If memory allocation succeeded
        if (allocatedAddress != -1) {
            if (coalescedForThisProcess) {
                cout << "Memory coalesced. Process " << process.processID << " can now be loaded." << endl;
            }

            // Update process metadata
            process.mainMemoryBase = allocatedAddress;
            process.instructionBase = allocatedAddress + 10; // PCB takes 10 slots
            process.dataBase = process.instructionBase + processInstructions[process.processID].size();

            // Store PCB in main memory
            mainMemory[allocatedAddress + 0] = process.processID;
            mainMemory[allocatedAddress + 1] = stateEncoding[process.state];
            mainMemory[allocatedAddress + 2] = process.programCounter;
            mainMemory[allocatedAddress + 3] = process.instructionBase;
            mainMemory[allocatedAddress + 4] = process.dataBase;
            mainMemory[allocatedAddress + 5] = process.memoryLimit;
            mainMemory[allocatedAddress + 6] = process.cpuCyclesUsed;
            mainMemory[allocatedAddress + 7] = process.registerValue;
            mainMemory[allocatedAddress + 8] = process.maxMemoryNeeded;
            mainMemory[allocatedAddress + 9] = process.mainMemoryBase;

            // Store opcodes first
            vector<vector<int> > instructions = processInstructions[process.processID];
            int writeIndex = process.instructionBase;

            for (size_t j = 0; j < instructions.size(); j++) {
                const vector<int>& instr = instructions[j];
                mainMemory[writeIndex++] = instr[0]; // Store opcode
            }

            // Then store parameters
            for (size_t j = 0; j < instructions.size(); j++) {
                const vector<int>& instr = instructions[j];
                for (size_t k = 1; k < instr.size(); k++) {
                    mainMemory[writeIndex++] = instr[k]; // Store parameter
                }
            }

            cout << "Process " << process.processID << " loaded into memory at address "
                 << allocatedAddress << " with size " << totalMemoryNeeded << "." << endl;

            // Add to ready queue
            readyQueue.push(process.mainMemoryBase);
        }
    }

    // Return unloaded processes to newJobQueue
    while (!tempQueue.empty()) {
        newJobQueue.push(tempQueue.front());
        tempQueue.pop();
    }
}

// Check IO waiting queue for completed operations
void checkIOWaitingQueue(queue<int>& readyQueue, vector<int>& mainMemory,
    queue<IOWaitQueueItem>& IOWaitingQueue) {
    
    int size = IOWaitingQueue.size();
    for (int i = 0; i < size; i++) {
        IOWaitQueueItem item = IOWaitingQueue.front();
        IOWaitingQueue.pop();

        PCB process = item.process;
        int startAddress = item.startAddress;
        int waitTime = item.waitTime;
        int timeEnteredIO = item.timeEnteredIO;

        // Check if wait time has elapsed
        if (globalClock - timeEnteredIO >= waitTime) {
            int paramOffset = paramOffsets[process.processID];
            int cycles = mainMemory[process.dataBase + paramOffset];

            // Execute print operation
            cout << "print" << endl;
            process.cpuCyclesUsed += cycles;
            mainMemory[startAddress + 6] = process.cpuCyclesUsed;

            // Update program counter and parameter offset
            process.programCounter++;
            mainMemory[startAddress + 2] = process.programCounter;
            paramOffset += opcodeParams[2]; // 2 is code for print
            paramOffsets[process.processID] = paramOffset;

            // Update state to READY
            process.state = "READY";
            mainMemory[startAddress + 1] = stateEncoding[process.state];

            cout << "Process " << process.processID << " completed I/O and is moved to the ReadyQueue." << endl;
            readyQueue.push(startAddress);
        }
        else {
            // Not enough time elapsed, put back in queue
            IOWaitingQueue.push(item);
        }
    }
}

// Execute a process on the CPU
void executeCPU(int startAddress, vector<int>& mainMemory, list<MemoryBlock>& memoryBlocks,
    queue<PCB>& newJobQueue, queue<int>& readyQueue, queue<IOWaitQueueItem>& IOWaitingQueue) {
    
    // Extract PCB from memory
    PCB process;
    int cpuCyclesThisRun = 0;

    process.processID = mainMemory[startAddress];
    process.state = "READY";
    mainMemory[startAddress + 1] = stateEncoding[process.state];
    process.programCounter = mainMemory[startAddress + 2];
    process.instructionBase = mainMemory[startAddress + 3];
    process.dataBase = mainMemory[startAddress + 4];
    process.memoryLimit = mainMemory[startAddress + 5];
    process.cpuCyclesUsed = mainMemory[startAddress + 6];
    process.registerValue = mainMemory[startAddress + 7];
    process.maxMemoryNeeded = mainMemory[startAddress + 8];
    process.mainMemoryBase = mainMemory[startAddress + 9];

    // Add context switch time
    globalClock += contextSwitchTime;

    // First time running this process?
    if (process.programCounter == 0) {
        process.programCounter = process.instructionBase;
        paramOffsets[process.processID] = 0;
        processStartTimes[process.processID] = globalClock;
    }

    // Set to running state
    process.state = "RUNNING";
    mainMemory[startAddress + 1] = stateEncoding[process.state];
    mainMemory[startAddress + 2] = process.programCounter;
    cout << "Process " << process.processID << " has moved to Running." << endl;

    int paramOffset = paramOffsets[process.processID];

    // Execute process instructions
    while (process.programCounter < process.dataBase && cpuCyclesThisRun < CPUAllocated) {
        int opcode = mainMemory[process.programCounter];

        switch (opcode) {
        case 1: // Compute
        {
            int iterations = mainMemory[process.dataBase + paramOffset];
            int cycles = mainMemory[process.dataBase + paramOffset + 1];
            cout << "compute" << endl;
            
            process.cpuCyclesUsed += cycles;
            mainMemory[startAddress + 6] = process.cpuCyclesUsed;
            cpuCyclesThisRun += cycles;
            globalClock += cycles;
            
            paramOffset += 2; // Compute has 2 parameters
            break;
        }
        case 2: // Print (I/O)
        {
            int cycles = mainMemory[process.dataBase + paramOffset];
            cout << "Process " << process.processID << " issued an IOInterrupt and moved to the IOWaitingQueue." << endl;
            
            // Move to IO waiting queue
            IOWaitingQueue.push(IOWaitQueueItem(process, startAddress, cycles, globalClock));
            
            // Update state to IOWAITING
            process.state = "IOWAITING";
            mainMemory[startAddress + 1] = stateEncoding[process.state];
            
            // Save parameter offset
            paramOffsets[process.processID] = paramOffset;
            
            return; // Exit CPU to let other processes run
        }
        case 3: // Store
        {
            int value = mainMemory[process.dataBase + paramOffset];
            int address = mainMemory[process.dataBase + paramOffset + 1];

            // Update register
            process.registerValue = value;
            mainMemory[startAddress + 7] = process.registerValue;

            // Store only if address is within memory limit
            if (address < process.memoryLimit) {
                mainMemory[process.mainMemoryBase + address] = process.registerValue;
                cout << "stored" << endl;
            }
            else {
                cout << "store error!" << endl;
            }

            process.cpuCyclesUsed++;
            mainMemory[startAddress + 6] = process.cpuCyclesUsed;
            cpuCyclesThisRun++;
            globalClock++;
            
            paramOffset += 2; // Store has 2 parameters
            break;
        }
        case 4: // Load
        {
            int address = mainMemory[process.dataBase + paramOffset];

            // Load only if address is within memory limit
            if (address < process.memoryLimit) {
                process.registerValue = mainMemory[process.mainMemoryBase + address];
                mainMemory[startAddress + 7] = process.registerValue;
                cout << "loaded" << endl;
            }
            else {
                cout << "load error!" << endl;
            }

            process.cpuCyclesUsed++;
            mainMemory[startAddress + 6] = process.cpuCyclesUsed;
            cpuCyclesThisRun++;
            globalClock++;
            
            paramOffset += 1; // Load has 1 parameter
            break;
        }
        }

        // Increment program counter for next instruction
        process.programCounter++;
        mainMemory[startAddress + 2] = process.programCounter;
        
        // Save parameter offset for next time
        paramOffsets[process.processID] = paramOffset;

        // Check for timeout
        if (cpuCyclesThisRun >= CPUAllocated && process.programCounter < process.dataBase) {
            cout << "Process " << process.processID << " has a TimeOUT interrupt and is moved to the ReadyQueue." << endl;
            process.state = "READY";
            mainMemory[startAddress + 1] = stateEncoding[process.state];
            timeoutOccurred = true;
            readyQueue.push(startAddress);
            return;
        }
    }

    // Process has completed all instructions
    process.programCounter = process.instructionBase - 1; // Reset to before instructionBase
    process.state = "TERMINATED";
    mainMemory[startAddress + 2] = process.programCounter;
    mainMemory[startAddress + 1] = stateEncoding[process.state];

    // Print PCB contents
    cout << "Process ID: " << process.processID << endl;
    cout << "State: " << process.state << endl;
    cout << "Program Counter: " << process.programCounter << endl;
    cout << "Instruction Base: " << process.instructionBase << endl;
    cout << "Data Base: " << process.dataBase << endl;
    cout << "Memory Limit: " << process.memoryLimit << endl;
    cout << "CPU Cycles Used: " << process.cpuCyclesUsed << endl;
    cout << "Register Value: " << process.registerValue << endl;
    cout << "Max Memory Needed: " << process.maxMemoryNeeded << endl;
    cout << "Main Memory Base: " << process.mainMemoryBase << endl;
    cout << "Total CPU Cycles Consumed: " << (globalClock - processStartTimes[process.processID]) << endl;

    // Output termination message
    cout << "Process " << process.processID
         << " terminated. Entered running state at: "
         << processStartTimes[process.processID]
         << ". Terminated at: "
         << globalClock
         << ". Total Execution Time: "
         << (globalClock - processStartTimes[process.processID])
         << "." << endl;

    // Free memory
    int freedStart = process.mainMemoryBase;
    int freedSize = process.maxMemoryNeeded + 10;
    freeMemory(memoryBlocks, mainMemory, process.processID);
    
    cout << "Process " << process.processID << " terminated and released memory from "
         << freedStart << " to "
         << (freedStart + freedSize - 1) << "." << endl;
         
    memoryFreed = true;
}

int main() {
    // Initialize maps
    initOpcodeParams();
    initStateEncoding();

    int maxMemory, numProcesses;
    queue<PCB> newJobQueue;
    queue<int> readyQueue;
    queue<IOWaitQueueItem> IOWaitingQueue;
    vector<int> mainMemory;
    list<MemoryBlock> memoryBlocks;

    // Read input parameters
    cin >> maxMemory >> CPUAllocated >> contextSwitchTime >> numProcesses;
    
    // Initialize main memory with -1
    mainMemory.resize(maxMemory, -1);
    
    // Initialize memory blocks list with one big free block
    memoryBlocks.push_back(MemoryBlock(-1, 0, maxMemory));

    // Read processes data
    for (int i = 0; i < numProcesses; i++) {
        PCB process;
        int numInstructions;

        cin >> process.processID >> process.maxMemoryNeeded >> numInstructions;
        process.state = "NEW";
        process.memoryLimit = process.maxMemoryNeeded;

        // Read and store all instructions for the process
        vector<vector<int> > instructions;
        for (int j = 0; j < numInstructions; j++) {
            vector<int> currentInstruction;
            int opcode;

            cin >> opcode;
            currentInstruction.push_back(opcode);
            int numParams = opcodeParams[opcode];

            for (int k = 0; k < numParams; k++) {
                int param;
                cin >> param;
                currentInstruction.push_back(param);
            }

            instructions.push_back(currentInstruction);
        }

        processInstructions[process.processID] = instructions;
        newJobQueue.push(process);
    }

    // Load initial jobs into memory
    loadJobsToMemory(newJobQueue, readyQueue, mainMemory, memoryBlocks);

    // Output initial memory state
    for (int i = 0; i < maxMemory; i++) {
        cout << i << " : " << mainMemory[i] << endl;
    }

    // Main execution loop
    while (!readyQueue.empty() || !IOWaitingQueue.empty() || !newJobQueue.empty()) {
        if (!readyQueue.empty()) {
            // Execute next process from ready queue
            int startAddress = readyQueue.front();
            readyQueue.pop();
            
            executeCPU(startAddress, mainMemory, memoryBlocks, newJobQueue, readyQueue, IOWaitingQueue);

            // If timeout occurred, process is already added back to ready queue
            timeoutOccurred = false;
            
            // If memory was freed, try to load more jobs
            if (memoryFreed) {
                loadJobsToMemory(newJobQueue, readyQueue, mainMemory, memoryBlocks);
                memoryFreed = false;
            }
        }
        else if (!IOWaitingQueue.empty()) {
            // No processes in ready queue, but there are IO waiting processes
            globalClock += contextSwitchTime;
        }
        else {
            // Only have new jobs waiting for memory
            globalClock += contextSwitchTime;
        }

        // Always check IO waiting queue for completed operations
        checkIOWaitingQueue(readyQueue, mainMemory, IOWaitingQueue);
    }

    // Final context switch
    globalClock += contextSwitchTime;
    cout << "Total CPU time used: " << globalClock << "." << endl;

    return 0;
}
/*
g++ -o CS3113_Project3 CS3113_Project3.cpp
./CS3113_Project3 < sampleInput1.txt
./CS3113_Project3 < sampleInput1.txt > output.txt
*/