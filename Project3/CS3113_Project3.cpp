// ==================================================
// Process Control Block Simulation Project 3  - working commented version
// ==================================================

#include <iostream>  // for input/output operations
#include <queue>     // for queue data structures (used for process queues)
#include <vector>    // for dynamic arrays (used for memory)
#include <string>    // for string manipulation
#include <unordered_map>  // for hash maps (fast key-value lookups)
#include <list>      // for linked lists (used for memory block management)
using namespace std;

// define process states as an enum for better type safety
// using enums makes the code more readable and prevents invalid states
enum ProcessState {
    STATE_NEW = 1,        // process has been created but not yet loaded into memory
    STATE_READY = 2,      // process is in memory and ready to run
    STATE_RUNNING = 3,    // process is currently executing on the CPU
    STATE_TERMINATED = 4, // process has finished execution
    STATE_IOWAITING = 5   // process is waiting for an IO operation to complete
};

// define instruction types as an enum
// this makes it easier to understand what each instruction does
enum InstructionType {
    COMPUTE = 1, // performs a computational task for a specific number of cycles
    PRINT = 2,   // performs an IO operation (printing) for a specific number of cycles
    STORE = 3,   // stores a value at a specific memory address
    LOAD = 4     // loads a value from a specific memory address
};

// structure to represent an instruction
// each instruction has a type and a list of parameters
struct Instruction {
    InstructionType type;     // what kind of instruction (compute, print, store, load)
    vector<int> parameters;   // parameters for the instruction (e.g., cycles, memory addresses)
    
    // constructor to initialize an instruction with a specific type
    // this makes it easier to create new instructions
    Instruction(InstructionType t) : type(t) {}
};

// memory block structure for the memory management linked list
// we use this to keep track of free and allocated memory blocks
struct MemoryBlock {
    int processID;      // which process owns this block (-1 if free)
    int startAddress;   // the starting address of this memory block
    int size;           // how big this memory block is
    
    // constructor for easier initialization of memory blocks
    // this allows us to create a new block with one line of code
    MemoryBlock(int id, int start, int sz) : 
        processID(id), startAddress(start), size(sz) {}
};

// process control block (PCB) structure
// this holds all information about a process, like a process's ID card
struct PCB {
    int processID;       // unique identifier for this process
    ProcessState state;  // current state of the process (new, ready, running, etc.)
    int programCounter;  // index of the next instruction to execute
    int instructionBase; // starting address of the process's instructions in memory
    int dataBase;        // starting address of the process's data in memory
    int memoryLimit;     // maximum memory this process can use
    int cpuCyclesUsed;   // how many CPU cycles this process has consumed so far
    int registerValue;   // the current value stored in the process's CPU register
    int maxMemoryNeeded; // how much memory the process needs in total
    int mainMemoryBase;  // starting address of the process in main memory
    vector<Instruction> instructions;  // list of instructions for this process
    int paramOffset;     // keeps track of the current parameter offset during execution
    
    // constructor with default initialization
    // this sets up a new PCB with safe default values
    PCB() : processID(0), state(STATE_NEW), programCounter(0), 
            instructionBase(0), dataBase(0), memoryLimit(0),
            cpuCyclesUsed(0), registerValue(0), maxMemoryNeeded(0), 
            mainMemoryBase(0), paramOffset(0) {}
};

// custom structure for tracking IO waiting processes
// this holds information about a process that's waiting for IO to complete
struct IOWaitItem {
    int pcbAddress;       // memory address where the PCB is stored
    int waitDuration;     // how long this IO operation should take
    int entryTime;        // when the process started waiting for IO
    
    // constructor for easier initialization
    // this creates a new IO wait record with one line of code
    IOWaitItem(int address, int duration, int time) :
        pcbAddress(address), waitDuration(duration), entryTime(time) {}
};

// global memory management linked list
// this keeps track of all memory blocks (free and allocated)
list<MemoryBlock> memoryBlocks;

// map to track when each process first entered the running state
// used to calculate total execution time
unordered_map<int, int> processStartTimes;

// map to track memory blocks allocated to processes
// allows quick access to a process's memory block by its ID
unordered_map<int, list<MemoryBlock>::iterator> processMemoryBlocks;

// global variables
int globalClock = 0;           // tracks the current system time
bool timeoutOccurred = false;  // indicates if a process timed out
bool memoryFreed = false;      // indicates if memory was freed during execution
int contextSwitchTime;         // time required to switch between processes
int CPUAllocated;              // maximum CPU time a process can use before timeout
queue<IOWaitItem> ioWaitingQueue; // queue of processes waiting for IO

// - HELPER FUNCTION
// converts a process state enum to a string for output
// this makes the output more readable for humans
string stateToString(ProcessState state) {
    switch (state) {
        case STATE_NEW: return "NEW";
        case STATE_READY: return "READY";
        case STATE_RUNNING: return "RUNNING";
        case STATE_TERMINATED: return "TERMINATED";
        case STATE_IOWAITING: return "IOWAITING";
        default: return "UNKNOWN";
    }
}

// - HELPER FUNCTION
// gets the number of parameters for a specific instruction type
// this is used when reading instructions and calculating parameter offsets
int getParameterCount(InstructionType type) {
    switch (type) {
        case COMPUTE: return 2; // compute needs iterations and cycles
        case PRINT: return 1;   // print only needs cycles
        case STORE: return 2;   // store needs value and address
        case LOAD: return 1;    // load only needs address
        default: return 0;      // unknown instruction type has no parameters
    }
}

// allocates memory for a process
// this finds a free memory block of sufficient size and assigns it to a process
int allocateMemory(int processID, int size) {
    // start at the beginning of the memory blocks list
    list<MemoryBlock>::iterator current = memoryBlocks.begin();
    list<MemoryBlock>::iterator prev = memoryBlocks.end();

    // search through all memory blocks
    while (current != memoryBlocks.end()) {
        // check if this block is free and big enough
        if (current->processID == -1 && current->size >= size) {
            // found a suitable block, remember its starting address
            int allocatedAddress = current->startAddress;

            // if the block is exactly the right size, just mark it as allocated
            if (current->size == size) {
                current->processID = processID;
                // store a reference to this block for quick access later
                processMemoryBlocks[processID] = current;
            }
            else { 
                // if the block is larger than needed, split it into two parts
                // first part is allocated to the process, second part remains free
                
                // create a new block for the allocated portion
                list<MemoryBlock>::iterator newBlock = memoryBlocks.insert(current, 
                                                        MemoryBlock(processID, current->startAddress, size));
                
                // store a reference to this block for quick access later
                processMemoryBlocks[processID] = newBlock;

                // adjust the remaining free portion
                current->startAddress += size;
                current->size -= size;
                
                // return the starting address of the allocated block
                return allocatedAddress;
            }

            // return the starting address of the allocated block
            return allocatedAddress;
        }

        // move to the next block
        prev = current;
        ++current;
    }

    // if we get here, no suitable block was found
    return -1;
}

// - HELPER FUNCTION
// frees memory that was allocated to a process
// this marks the memory as available for other processes
void freeMemory(vector<int>& mainMemory, int processID) {
    // check if this process has an allocated memory block
    if (processMemoryBlocks.find(processID) == processMemoryBlocks.end()) {
        return; // process doesn't have any allocated memory
    }
    
    // get the memory block for this process
    list<MemoryBlock>::iterator blockIt = processMemoryBlocks[processID];
    int startAddress = blockIt->startAddress;
    int size = blockIt->size;
    
    // clear memory by setting all values to -1
    for (int i = startAddress; i < startAddress + size; i++) {
        mainMemory[i] = -1;
    }
    
    // mark the block as free
    blockIt->processID = -1;
    
    // remove the reference to this block from the map
    processMemoryBlocks.erase(processID);
}

// merges adjacent free memory blocks to create larger free blocks
// this helps reduce memory fragmentation
void coalesceMemory() {
    // start at the beginning of the memory blocks list
    list<MemoryBlock>::iterator current = memoryBlocks.begin();

    // scan through the list looking for adjacent free blocks
    while (current != memoryBlocks.end() && current != --memoryBlocks.end()) {
        // get the next block
        list<MemoryBlock>::iterator next = current;
        ++next;

        // check if we've reached the end of the list
        if (next == memoryBlocks.end()) {
            break;
        }

        // if both current and next blocks are free, merge them
        if (current->processID == -1 && next->processID == -1) {
            // add next block's size to current block
            current->size += next->size;
            // remove the next block since it's now part of the current block
            memoryBlocks.erase(next);
            // don't move to the next block yet, check if we can merge more
        }
        else {
            // blocks can't be merged, move to the next block
            ++current;
        }
    }
}

// - HELPER FUNCTION
// loads a PCB from main memory
// this reconstructs a PCB from its memory representation
PCB loadPCBFromMemory(vector<int>& mainMemory, int address) {
    PCB pcb;
    
    // load all PCB fields from memory
    pcb.processID = mainMemory[address];
    pcb.state = static_cast<ProcessState>(mainMemory[address + 1]);
    pcb.programCounter = mainMemory[address + 2];
    pcb.instructionBase = mainMemory[address + 3];
    pcb.dataBase = mainMemory[address + 4];
    pcb.memoryLimit = mainMemory[address + 5];
    pcb.cpuCyclesUsed = mainMemory[address + 6];
    pcb.registerValue = mainMemory[address + 7];
    pcb.maxMemoryNeeded = mainMemory[address + 8];
    pcb.mainMemoryBase = mainMemory[address + 9];
    
    // calculate parameter offset based on executed instructions
    if (pcb.programCounter > 0) {
        pcb.paramOffset = 0;
        
        // go through all executed instructions and add up their parameter counts
        for (int i = pcb.instructionBase; i < pcb.programCounter; i++) {
            int opcode = mainMemory[i];
            pcb.paramOffset += getParameterCount(static_cast<InstructionType>(opcode));
        }
    } else {
        // process hasn't executed any instructions yet
        pcb.paramOffset = 0;
    }
    
    return pcb;
}

// - HELPER FUNCTION
// saves a PCB to main memory
// this stores a PCB in memory so it can be retrieved later
void savePCBToMemory(vector<int>& mainMemory, int address, const PCB& pcb) {
    // store all PCB fields in memory
    mainMemory[address] = pcb.processID;
    mainMemory[address + 1] = static_cast<int>(pcb.state);
    mainMemory[address + 2] = pcb.programCounter;
    mainMemory[address + 3] = pcb.instructionBase;
    mainMemory[address + 4] = pcb.dataBase;
    mainMemory[address + 5] = pcb.memoryLimit;
    mainMemory[address + 6] = pcb.cpuCyclesUsed;
    mainMemory[address + 7] = pcb.registerValue;
    mainMemory[address + 8] = pcb.maxMemoryNeeded;
    mainMemory[address + 9] = pcb.mainMemoryBase;
}

// loads jobs from the new job queue into memory and adds them to the ready queue
// this is called when we want to load waiting processes into memory
void loadJobsToMemory(queue<PCB>& pendingJobs, queue<int>& executionQueue, vector<int>& systemMemory) {
    // we'll keep track of jobs that can't be loaded right now
    vector<PCB> deferredJobs;
    bool memoryCombined = false;
    
    // process jobs until the queue is empty or we can't load any more
    while (!pendingJobs.empty()) {
        // get the next job from the queue
        PCB currentJob = pendingJobs.front();
        pendingJobs.pop();
        
        // calculate how much memory this job needs (including PCB overhead)
        int requiredSpace = currentJob.maxMemoryNeeded + 10;  // 10 for PCB metadata
        
        // try to allocate memory for this job
        int assignedLocation = allocateMemory(currentJob.processID, requiredSpace);
        
        // if allocation failed, try to coalesce memory and try again
        if (assignedLocation == -1) {
            cout << "Insufficient memory for Process " << currentJob.processID 
                 << ". Attempting memory coalescing." << endl;
            
            // try to combine adjacent free memory blocks
            coalesceMemory();
            memoryCombined = true;
            
            // try allocating memory again
            assignedLocation = allocateMemory(currentJob.processID, requiredSpace);
            
            // if still no space, defer this job and all remaining jobs
            if (assignedLocation == -1) {
                cout << "Process " << currentJob.processID 
                     << " waiting in NewJobQueue due to insufficient memory." << endl;
                
                // save this job for later
                deferredJobs.push_back(currentJob);
                
                // also save all remaining jobs
                while (!pendingJobs.empty()) {
                    deferredJobs.push_back(pendingJobs.front());
                    pendingJobs.pop();
                }
                
                // stop processing jobs for now
                break;
            }
        }
        
        // if we successfully allocated memory
        if (assignedLocation != -1) {
            // if we had to coalesce memory, print a message
            if (memoryCombined) {
                cout << "Memory coalesced. Process " << currentJob.processID 
                     << " can now be loaded." << endl;
                memoryCombined = false;  // reset for next job
            }
            
            // set up memory layout for this job
            currentJob.mainMemoryBase = assignedLocation;
            currentJob.instructionBase = assignedLocation + 10;  // PCB takes 10 slots
            
            // calculate where data starts (after all instructions)
            int instructionCount = currentJob.instructions.size();
            currentJob.dataBase = currentJob.instructionBase + instructionCount;
            
            // save the PCB to memory
            savePCBToMemory(systemMemory, assignedLocation, currentJob);
            
            // first store all instruction opcodes
            int memoryIndex = currentJob.instructionBase;
            for (size_t i = 0; i < currentJob.instructions.size(); i++) {
                systemMemory[memoryIndex++] = static_cast<int>(currentJob.instructions[i].type);
            }
            
            // then store all instruction parameters
            for (size_t i = 0; i < currentJob.instructions.size(); i++) {
                const vector<int>& paramList = currentJob.instructions[i].parameters;
                for (size_t j = 0; j < paramList.size(); j++) {
                    systemMemory[memoryIndex++] = paramList[j];
                }
            }
            
            // print a message that job was loaded
            cout << "Process " << currentJob.processID << " loaded into memory at address "
                 << assignedLocation << " with size " << requiredSpace << "." << endl;
            
            // add to execution queue (ready queue)
            executionQueue.push(currentJob.mainMemoryBase);
        }
    }
    
    // return deferred jobs to the pending queue in original order
    for (size_t i = 0; i < deferredJobs.size(); i++) {
        pendingJobs.push(deferredJobs[i]);
    }
}

// checks for completed IO operations and moves processes back to the ready queue
// this is called regularly to see if any waiting IO operations have finished
void checkIOWaitingQueue(queue<int>& readyQueue, vector<int>& mainMemory) {
    // create a temporary queue for operations that need to keep waiting
    queue<IOWaitItem> stillWaiting;
    
    // process all items in the IO waiting queue
    while (!ioWaitingQueue.empty()) {
        // get the next IO operation
        IOWaitItem currentItem = ioWaitingQueue.front();
        ioWaitingQueue.pop();
        
        // check if this operation has been waiting long enough
        bool isCompleted = (globalClock - currentItem.entryTime >= currentItem.waitDuration);
        
        if (isCompleted) {
            // the IO operation is complete, get the process
            int pcbAddress = currentItem.pcbAddress;
            PCB process = loadPCBFromMemory(mainMemory, pcbAddress);
            
            // print a message that the operation is complete
            cout << "print" << endl;
            
            // update the process state
            int waitCycles = mainMemory[process.dataBase + process.paramOffset];
            process.cpuCyclesUsed += waitCycles;
            process.programCounter++;
            process.paramOffset += getParameterCount(PRINT);
            process.state = STATE_READY;
            
            // save the updated process state
            savePCBToMemory(mainMemory, pcbAddress, process);
            
            // print a message that the process is ready
            cout << "Process " << process.processID << " completed I/O and is moved to the ReadyQueue." << endl;
            
            // add the process to the ready queue
            readyQueue.push(pcbAddress);
        } else {
            // the IO operation is not complete yet, keep waiting
            stillWaiting.push(currentItem);
        }
    }
    
    // restore the IO waiting queue with operations that need to keep waiting
    ioWaitingQueue = stillWaiting;
}

// executes a process on the CPU
// this simulates the CPU executing instructions for a process
void executeCPU(int startAddress, vector<int>& mainMemory, queue<PCB>& newJobQueue, queue<int>& readyQueue) {
    
    // load the process from memory
    PCB process = loadPCBFromMemory(mainMemory, startAddress);
    int cpuCyclesThisRun = 0; // track how many cycles we've used in this run
    
    // set the process state to READY (will be changed to RUNNING shortly)
    process.state = STATE_READY;
    savePCBToMemory(mainMemory, startAddress, process);

    // add time for context switching
    globalClock += contextSwitchTime;

    // if this is the first time running this process
    if (process.programCounter == 0) {
        // initialize program counter to the first instruction
        process.programCounter = process.instructionBase;
        // initialize parameter offset to 0
        process.paramOffset = 0;
        // record when the process first starts running
        processStartTimes[process.processID] = globalClock;
    }

    // set the process state to RUNNING
    process.state = STATE_RUNNING;
    savePCBToMemory(mainMemory, startAddress, process);
    cout << "Process " << process.processID << " has moved to Running." << endl;

    // main execution loop - execute instructions until completion or timeout
    while (process.programCounter < process.dataBase && cpuCyclesThisRun < CPUAllocated) {
        // get the current instruction
        int instructionCode = mainMemory[process.programCounter];
        InstructionType currentOperation = static_cast<InstructionType>(instructionCode);
        
        // calculate where this instruction's parameters are located
        int paramStartPos = process.dataBase + process.paramOffset;
        
        // execute the instruction based on its type
        switch (currentOperation) {
            case COMPUTE: {
                // compute instruction: uses iterations and cycles parameters
                int iterationCount = mainMemory[paramStartPos];
                int cycleDuration = mainMemory[paramStartPos + 1];
                
                // print what we're doing
                cout << "compute" << endl;
                
                // update the execution time
                int totalCyclesUsed = cycleDuration;
                process.cpuCyclesUsed += totalCyclesUsed;
                cpuCyclesThisRun += totalCyclesUsed;
                globalClock += totalCyclesUsed;
                
                // move to the next parameters
                process.paramOffset += 2;  // compute has 2 parameters
                break;
            }
            
            case PRINT: {
                // print instruction: uses cycles parameter
                int ioDuration = mainMemory[paramStartPos];
                
                // print a message about the IO interrupt
                cout << "Process " << process.processID 
                     << " issued an IOInterrupt and moved to the IOWaitingQueue." << endl;
                
                // update process state and save
                process.state = STATE_IOWAITING;
                savePCBToMemory(mainMemory, startAddress, process);
                
                // add to IO waiting queue
                ioWaitingQueue.push(IOWaitItem(startAddress, ioDuration, globalClock));
                
                // stop executing this process for now
                return;
            }
            
            case STORE: {
                // store instruction: uses value and address parameters
                int valueToStore = mainMemory[paramStartPos];
                int targetLocation = mainMemory[paramStartPos + 1];
                
                // update the register with the value
                process.registerValue = valueToStore;
                
                // check if the address is valid
                bool validAddress = (targetLocation < process.memoryLimit);
                
                if (validAddress) {
                    // calculate the actual memory address
                    int actualAddress = process.mainMemoryBase + targetLocation;
                    // store the value in memory
                    mainMemory[actualAddress] = valueToStore;
                    cout << "stored" << endl;
                } else {
                    // address is invalid
                    cout << "store error!" << endl;
                }
                
                // update the execution time
                process.cpuCyclesUsed++;
                cpuCyclesThisRun++;
                globalClock++;
                
                // move to the next parameters
                process.paramOffset += 2;  // store has 2 parameters
                break;
            }
            
            case LOAD: {
                // load instruction: uses address parameter
                int sourceLocation = mainMemory[paramStartPos];
                
                // check if the address is valid
                bool validAddress = (sourceLocation < process.memoryLimit);
                
                if (validAddress) {
                    // calculate the actual memory address
                    int actualAddress = process.mainMemoryBase + sourceLocation;
                    // load the value from memory into the register
                    process.registerValue = mainMemory[actualAddress];
                    cout << "loaded" << endl;
                } else {
                    // address is invalid
                    cout << "load error!" << endl;
                }
                
                // update the execution time
                process.cpuCyclesUsed++;
                cpuCyclesThisRun++;
                globalClock++;
                
                // move to the next parameters
                process.paramOffset += 1;  // load has 1 parameter
                break;
            }
        }
        
        // move to the next instruction
        process.programCounter++;
        
        // save the updated process state
        savePCBToMemory(mainMemory, startAddress, process);
        
        // check if we've used up our time slice
        bool timeQuotaExceeded = (cpuCyclesThisRun >= CPUAllocated);
        bool moreInstructions = (process.programCounter < process.dataBase);
        
        // if we've used up our time slice and there are more instructions
        if (timeQuotaExceeded && moreInstructions) {
            // print a message about the timeout
            cout << "Process " << process.processID 
                 << " has a TimeOUT interrupt and is moved to the ReadyQueue." << endl;
            
            // update process state and reschedule
            process.state = STATE_READY;
            savePCBToMemory(mainMemory, startAddress, process);
            timeoutOccurred = true;
            readyQueue.push(startAddress);
            return;
        }
    }

    // if we get here, the process has completed all instructions
    
    // reset program counter to before the instruction base
    process.programCounter = process.instructionBase - 1;
    // set process state to terminated
    process.state = STATE_TERMINATED;
    // save the final process state
    savePCBToMemory(mainMemory, startAddress, process);

    // print the final PCB contents
    cout << "Process ID: " << process.processID << endl;
    cout << "State: " << stateToString(process.state) << endl;
    cout << "Program Counter: " << process.programCounter << endl;
    cout << "Instruction Base: " << process.instructionBase << endl;
    cout << "Data Base: " << process.dataBase << endl;
    cout << "Memory Limit: " << process.memoryLimit << endl;
    cout << "CPU Cycles Used: " << process.cpuCyclesUsed << endl;
    cout << "Register Value: " << process.registerValue << endl;
    cout << "Max Memory Needed: " << process.maxMemoryNeeded << endl;
    cout << "Main Memory Base: " << process.mainMemoryBase << endl;
    cout << "Total CPU Cycles Consumed: " << (globalClock - processStartTimes[process.processID]) << endl;

    // print a termination message with timing information
    cout << "Process " << process.processID<< " terminated. Entered running state at: "<< processStartTimes[process.processID]<< ". Terminated at: "<< globalClock<< ". Total Execution Time: "<< (globalClock - processStartTimes[process.processID])<< "." << endl;

    // free the memory used by this process
    int freedStart = process.mainMemoryBase;
    int freedSize = process.maxMemoryNeeded + 10;
    freeMemory(mainMemory, process.processID);
    
    // print a message about freed memory
    cout << "Process " << process.processID << " terminated and released memory from "
         << freedStart << " to "
         << (freedStart + freedSize - 1) << "." << endl;
         
    // set flag so we know memory was freed
    memoryFreed = true;
}

// main program entry point
int main() {
    // declare variables for memory and process management
    int maxMemory, numProcesses;
    queue<PCB> newJobQueue; // queue for processes waiting to be loaded into memory
    queue<int> readyQueue;  // queue for processes ready to execute
    vector<int> mainMemory; // simulated main memory

    // read input parameters from standard input
    cin >> maxMemory >> CPUAllocated >> contextSwitchTime >> numProcesses;
    
    // initialize main memory with -1 (representing empty memory)
    mainMemory.resize(maxMemory, -1);
    
    // initialize memory blocks list with one big free block
    memoryBlocks.push_back(MemoryBlock(-1, 0, maxMemory));

    // read data for each process
    for (int i = 0; i < numProcesses; i++) {
        // create a new process control block
        PCB process;
        int numInstructions;

        // read process ID, memory needed, and number of instructions
        cin >> process.processID >> process.maxMemoryNeeded >> numInstructions;
        process.state = STATE_NEW;
        process.memoryLimit = process.maxMemoryNeeded;

        // read and store all instructions for this process
        for (int j = 0; j < numInstructions; j++) {
            // read the instruction opcode
            int rawOpcode;
            cin >> rawOpcode;
            // convert to instruction type
            InstructionType operationType = static_cast<InstructionType>(rawOpcode);
            
            // create a new instruction
            Instruction currentCmd(operationType);
            // determine how many parameters this instruction requires
            int requiredArgCount = getParameterCount(operationType);

            // read all parameters for this instruction
            for (int argIndex = 0; argIndex < requiredArgCount; argIndex++) {
                int argValue;
                cin >> argValue;
                currentCmd.parameters.push_back(argValue);
            }

            // add this instruction to the process
            process.instructions.push_back(currentCmd);
        }

        // add this process to the new job queue
        newJobQueue.push(process);
    }

    // attempt to load initial jobs into memory
    loadJobsToMemory(newJobQueue, readyQueue, mainMemory);

    // print the initial state of memory
    for (int i = 0; i < maxMemory; i++) {
        cout << i << " : " << mainMemory[i] << endl;
    }

    // main execution loop
    // continue as long as there are processes in any queue
    while (!readyQueue.empty() || !ioWaitingQueue.empty() || !newJobQueue.empty()) {
        if (!readyQueue.empty()) {
            // if there are processes ready to run
            
            // get the next process to execute
            int startAddress = readyQueue.front();
            readyQueue.pop();
            
            // execute this process
            executeCPU(startAddress, mainMemory, newJobQueue, readyQueue);

            // reset timeout flag
            timeoutOccurred = false;
            
            // if memory was freed, try to load more jobs
            if (memoryFreed) {
                loadJobsToMemory(newJobQueue, readyQueue, mainMemory);
                memoryFreed = false;
            }
        }
        else if (!ioWaitingQueue.empty()) {
            // if there are processes waiting for IO, but none are ready to run
            
            // advance the clock by context switch time
            // this simulates the cpu being idle while waiting for IO
            globalClock += contextSwitchTime;
        }
        else {
            // if there are only new jobs waiting for memory
            // advance the clock by context switch time
            // this simulates the cpu being idle while waiting for memory to become available
            globalClock += contextSwitchTime;
        }

        // always check if any IO operations have completed
        // this allows processes to move from waiting to ready
        checkIOWaitingQueue(readyQueue, mainMemory);
    }

    // add one final context switch time at the end
    // this accounts for system overhead after all processes finish
    globalClock += contextSwitchTime;
    
    // print the total CPU time used by all processes
    cout << "Total CPU time used: " << globalClock << "." << endl;

    return 0;
}
/*
g++ -o CS3113_Project3 CS3113_Project3.cpp
./CS3113_Project3 < sampleInput1.txt
./CS3113_Project3 < sampleInput1.txt > output.txt
*/