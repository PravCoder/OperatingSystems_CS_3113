// ==================================================
// Process Control Block Simulation Project 3
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

// Memory block structure for linked list
struct MemoryBlock
{
  int processID;     // -1 if free, otherwise stores the process ID
  int startAddress;  // Starting memory address of this block
  int size;          // Size of this memory block
  MemoryBlock *next; // Pointer to the next block in the list

  // Constructor for easier initialization
  MemoryBlock(int id, int start, int sz)
      : processID(id), startAddress(start), size(sz), next(nullptr) {}
};

struct PCB
{
  int processID;       // identifier of process
  string state;        // "NEW", "READY", "RUNNING", "TERMINATED", "IOWAITING"
  int programCounter;  // index of next instruction to be executed, in logical memory
  int instructionBase; // starting address of instructions for this process
  int dataBase;        // address where the data for the instructions starts in logical memory
  int memoryLimit;     // total size of logical memory allocated to the process    
  int cpuCyclesUsed;   // number of cpu cycles process has consumed so far
  int registerValue;   // value of cpu register associated with process
  int maxMemoryNeeded; // max logical memory required by process as defined in input file
  int mainMemoryBase;  // starting address in main memory where process, PCB+logical_memory is loaded.
  
  // Constructor to initialize values
  PCB() : programCounter(0), cpuCyclesUsed(0), registerValue(0), state("NEW") {}
};

// Custom structure for IOWaitingQueue items
struct IOWaitQueueItem {
  PCB process;
  int startAddress;
  int waitTime;
  int timeEnteredIO;
  
  // Constructor for easier initialization
  IOWaitQueueItem(PCB p, int start, int wait, int time)
    : process(p), startAddress(start), waitTime(wait), timeEnteredIO(time) {}
};

// Key: opcode, Value: number of parameters
unordered_map<int, int> opcodeParams;
void initOpcodeParams() {
    opcodeParams[1] = 2; // Compute: iterations, cycles
    opcodeParams[2] = 1; // Print: cycles
    opcodeParams[3] = 2; // Store: value, address
    opcodeParams[4] = 1; // Load: address
}

// Key: state, Value: encoding
unordered_map<string, int> stateEncoding;
void initStateEncoding() {
    stateEncoding["NEW"] = 1;
    stateEncoding["READY"] = 2;
    stateEncoding["RUNNING"] = 3;
    stateEncoding["TERMINATED"] = 4;
    stateEncoding["IOWAITING"] = 5;
}

// Key: processID, Value: instructions
unordered_map<int, vector<vector<int> > > processInstructions;

// Key: processID, Value: value of the global clock the first time the process entered running state
unordered_map<int, int> processStartTimes;

// Helps keep track of each process's current parameter offset across multiple executions
unordered_map<int, int> paramOffsets; // Key: processID, Value: paramOffset

// Global variables for timing and process management
int globalClock = 0;
bool timeoutOccurred = false;
bool memoryFreed = false;
queue<IOWaitQueueItem> IOWaitingQueue; // Queue of IOWaitQueueItem
int contextSwitchTime, CPUAllocated;

// Function to allocate memory for a process
int allocateMemory(MemoryBlock *&memoryHead, int processID, int size)
{
  MemoryBlock *current = memoryHead;
  MemoryBlock *prev = nullptr;

  while (current)
  {
    if (current->processID == -1 && current->size >= size) // Found a free block big enough to allocate
    {
      int allocatedAddress = current->startAddress;

      // If the block is exactly the right size, allocate it, otherwise split it into two blocks (allocated and free)
      if (current->size == size)
      {
        current->processID = processID;
      }
      else
      {
        MemoryBlock *newBlock = new MemoryBlock(processID, current->startAddress, size);
        newBlock->next = current;

        if (prev)
        {
          prev->next = newBlock;
        }
        else
        {
          memoryHead = newBlock;
        }

        current->startAddress += size;
        current->size -= size;
        return allocatedAddress;
      }

      return allocatedAddress;
    }

    prev = current;
    current = current->next;
  }

  return -1; // No block big enough to allocate
}

// Function to free memory when a process terminates
void freeMemory(MemoryBlock *&memoryHead, int *mainMemory, int processID)
{
  MemoryBlock *current = memoryHead;

  // Loop over memory blocks to find the one to free
  while (current)
  {
    if (current->processID == processID)
    {
      // Free the memory block by setting all its values to -1
      for (int i = current->startAddress; i < current->startAddress + current->size; i++)
      {
        mainMemory[i] = -1;
      }

      current->processID = -1; // Mark the block as free
      return;                  // Done freeing memory
    }

    current = current->next; // Current block is not the one to free, move to the next block
  }
}

// Function to coalesce adjacent free memory blocks
void coalesceMemory(MemoryBlock *&memoryHead)
{
  MemoryBlock *current = memoryHead;

  // Loop over memory blocks to coalesce adjacent free blocks
  while (current && current->next)
  {
    MemoryBlock *next = current->next;

    if (current->processID == -1 && next->processID == -1)
    {
      current->size += next->size; // Add the size of the next block to the current block
      current->next = next->next;  // Make the current block point to the block after the next block
      delete next;                 // Free the memory used by the next block
      continue;                    // Continue checking for more adjacent free blocks
    }

    current = current->next; // Move to the next memory block
  }
}

// Print PCB contents from main memory
void printPCBFromMainMemory(int startAddress, int *mainMemory) {
    // Determine the state string
    string state;
    int stateInt = mainMemory[startAddress + 1];
    if (stateInt == 1) {
        state = "NEW";
    } else if (stateInt == 2) {
        state = "READY";
    } else if (stateInt == 3) {
        state = "RUNNING";
    } else if (stateInt == 4) {
        state = "TERMINATED";
    } else {
        state = "IOWAITING";
    }

    // Output the details of the process
    cout << "Process ID: " << mainMemory[startAddress] << endl;
    cout << "State: " << state << endl;
    cout << "Program Counter: " << mainMemory[startAddress + 2] << endl;
    cout << "Instruction Base: " << mainMemory[startAddress + 3] << endl;
    cout << "Data Base: " << mainMemory[startAddress + 4] << endl;
    cout << "Memory Limit: " << mainMemory[startAddress + 5] << endl;
    cout << "CPU Cycles Used: " << mainMemory[startAddress + 6] << endl;
    cout << "Register Value: " << mainMemory[startAddress + 7] << endl;
    cout << "Max Memory Needed: " << mainMemory[startAddress + 8] << endl;
    cout << "Main Memory Base: " << mainMemory[startAddress + 9] << endl;
    
    // Calculate and print total execution time
    int processID = mainMemory[startAddress];
    cout << "Total CPU Cycles Consumed: " << globalClock - processStartTimes[processID] << endl;
}

// Load jobs from NewJobQueue to memory and add to ReadyQueue
void loadJobsToMemory(queue<PCB> &newJobQueue, queue<int> &readyQueue,
                      int *mainMemory, MemoryBlock *&memoryHead)
{
  int newJobQueueSize = newJobQueue.size();
  queue<PCB> tempQueue; // Stores jobs that couldn't be loaded this cycle

  for (int i = 0; i < newJobQueueSize; i++)
  {
    PCB process = newJobQueue.front();
    newJobQueue.pop();

    bool coalescedForThisProcess = false;
    int totalMemoryNeeded = process.maxMemoryNeeded + 10; // +10 for PCB metadata
    int allocatedAddress = allocateMemory(memoryHead, process.processID, totalMemoryNeeded);

    // If there's not enough memory, try coalescing and then allocating again
    if (allocatedAddress == -1)
    {
      cout << "Insufficient memory for Process " << process.processID << ". Attempting memory coalescing." << endl;
      coalesceMemory(memoryHead); // Coalesce memory blocks to free up more memory

      allocatedAddress = allocateMemory(memoryHead, process.processID, totalMemoryNeeded); // Try allocating memory again
      coalescedForThisProcess = (allocatedAddress != -1);                                  // Track if coalescing worked

      // If still not enough memory, put the process back in newJobQueue and try again next cycle
      if (allocatedAddress == -1)
      {
        cout << "Process " << process.processID << " waiting in NewJobQueue due to insufficient memory." << endl;

        // Store the failed job
        tempQueue.push(process);

        // Also move the rest of the unprocessed jobs from newJobQueue into tempQueue
        for (int j = i + 1; j < newJobQueueSize; j++)
        {
          tempQueue.push(newJobQueue.front());
          newJobQueue.pop();
        }

        break; // Break the loop so we retry all jobs next time in original order
      }
    }

    // If memory was successfully allocated, load the process into mainMemory
    if (allocatedAddress != -1)
    {
      if (coalescedForThisProcess)
      {
        cout << "Memory coalesced. Process " << process.processID << " can now be loaded." << endl;
      }

      // Update PCB metadata
      process.mainMemoryBase = allocatedAddress;
      process.instructionBase = allocatedAddress + 10;
      process.dataBase = process.instructionBase + processInstructions[process.processID].size();

      // Store PCB metadata in mainMemory
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

      // Store opcodes first, then parameters
      vector<vector<int> > instructions = processInstructions[process.processID];
      int writeIndex = process.instructionBase;

      // First store all opcodes
      for (size_t j = 0; j < instructions.size(); j++)
      {
        const vector<int>& instr = instructions[j];
        mainMemory[writeIndex++] = instr[0];
      }

      // Then store parameters
      for (size_t j = 0; j < instructions.size(); j++)
      {
        const vector<int>& instr = instructions[j];
        for (size_t k = 1; k < instr.size(); k++)
        {
          mainMemory[writeIndex++] = instr[k];
        }
      }

      cout << "Process " << process.processID << " loaded into memory at address "
           << allocatedAddress << " with size " << totalMemoryNeeded << "." << endl;

      // Add to ready queue
      readyQueue.push(process.mainMemoryBase);
    }
  }

  // Put failed jobs back in newJobQueue for the next cycle
  while (!tempQueue.empty())
  {
    newJobQueue.push(tempQueue.front());
    tempQueue.pop();
  }
}

// Process I/O waiting queue to check for completed operations
void checkIOWaitingQueue(queue<int> &readyQueue, int *mainMemory)
{
  int size = IOWaitingQueue.size();
  for (int i = 0; i < size; i++)
  {
    IOWaitQueueItem item = IOWaitingQueue.front();
    PCB process = item.process;
    int startAddress = item.startAddress;
    int waitTime = item.waitTime;
    int timeEnteredIO = item.timeEnteredIO;
    IOWaitingQueue.pop();

    if (globalClock - timeEnteredIO >= waitTime) // Enough time has passed for I/O to complete
    {
      int paramOffset = paramOffsets[process.processID];
      int cycles = mainMemory[process.dataBase + paramOffset];

      // Execute print operation
      cout << "print" << endl;
      process.cpuCyclesUsed += cycles;
      mainMemory[startAddress + 6] = process.cpuCyclesUsed; // Update CPU cycles used in mainMemory

      // Increment program counter and paramOffset for next instruction
      process.programCounter++;
      mainMemory[startAddress + 2] = process.programCounter;
      paramOffset += opcodeParams[2]; // 2 is print opcode, which has 1 parameter
      paramOffsets[process.processID] = paramOffset;

      // Reset state to READY
      process.state = "READY";
      mainMemory[startAddress + 1] = stateEncoding[process.state]; // Update state in mainMemory

      cout << "Process " << process.processID << " completed I/O and is moved to the ReadyQueue." << endl;
      readyQueue.push(startAddress);
    }
    else
    {
      // Not enough time has elapsed, put back in queue
      IOWaitingQueue.push(IOWaitQueueItem(process, startAddress, waitTime, timeEnteredIO));
    }
  }
}

// Execute a process on the CPU
void executeCPU(int startAddress, int *mainMemory, MemoryBlock *&memoryHead, queue<PCB> &newJobQueue, queue<int> &readyQueue)
{
  // Extract PCB from memory
  PCB process;
  int cpuCyclesThisRun = 0;

  process.processID = mainMemory[startAddress];
  process.state = "READY";
  mainMemory[startAddress + 1] = stateEncoding[process.state]; // Update state in mainMemory
  process.programCounter = mainMemory[startAddress + 2];
  process.instructionBase = mainMemory[startAddress + 3];
  process.dataBase = mainMemory[startAddress + 4];
  process.memoryLimit = mainMemory[startAddress + 5];
  process.cpuCyclesUsed = mainMemory[startAddress + 6];
  process.registerValue = mainMemory[startAddress + 7];
  process.maxMemoryNeeded = mainMemory[startAddress + 8];
  process.mainMemoryBase = mainMemory[startAddress + 9];

  globalClock += contextSwitchTime; // Increment global clock when process moves to running state

  // First time running this process?
  if (process.programCounter == 0)
  {
    process.programCounter = process.instructionBase;   // Initialize PC
    paramOffsets[process.processID] = 0;                // Initialize paramOffset
    processStartTimes[process.processID] = globalClock; // Store start time
  }

  // Update state to RUNNING
  process.state = "RUNNING";
  mainMemory[startAddress + 1] = stateEncoding[process.state]; // Update state in mainMemory
  mainMemory[startAddress + 2] = process.programCounter;       // Update program counter in mainMemory
  cout << "Process " << process.processID << " has moved to Running." << endl;

  int paramOffset = paramOffsets[process.processID];

  // Execute instructions until completion or timeout
  while (process.programCounter < process.dataBase && cpuCyclesThisRun < CPUAllocated)
  {
    int opcode = mainMemory[process.programCounter];

    switch (opcode)
    {
    case 1: // Compute
    {
      int iterations = mainMemory[process.dataBase + paramOffset];
      int cycles = mainMemory[process.dataBase + paramOffset + 1];
      cout << "compute" << endl;
      process.cpuCyclesUsed += cycles;
      mainMemory[startAddress + 6] = process.cpuCyclesUsed; // Update CPU cycles used in mainMemory
      cpuCyclesThisRun += cycles;
      globalClock += cycles;
      paramOffset += 2; // Compute has 2 parameters
      break;
    }
    case 2: // Print (I/O)
    {
      int cycles = mainMemory[process.dataBase + paramOffset];
      cout << "Process " << process.processID << " issued an IOInterrupt and moved to the IOWaitingQueue." << endl;
      IOWaitingQueue.push(IOWaitQueueItem(process, startAddress, cycles, globalClock)); // Move process to IOWaitingQueue
      process.state = "IOWAITING";
      mainMemory[startAddress + 1] = stateEncoding[process.state]; // Update state in mainMemory
      
      // Save parameter offset
      paramOffsets[process.processID] = paramOffset;
      
      return; // Exit function to allow other processes to run while this one waits in IOWaitingQueue
    }
    case 3: // Store
    {
      int value = mainMemory[process.dataBase + paramOffset];
      int address = mainMemory[process.dataBase + paramOffset + 1];

      process.registerValue = value;
      mainMemory[startAddress + 7] = process.registerValue; // Update register value in mainMemory

      // Store only if address is within memory limit
      if (address < process.memoryLimit)
      {
        mainMemory[process.mainMemoryBase + address] = process.registerValue; // Store value at desired address in mainMemory
        cout << "stored" << endl;
      }
      else
      {
        cout << "store error!" << endl;
      }

      process.cpuCyclesUsed++;                              // Store operation takes 1 cycle
      mainMemory[startAddress + 6] = process.cpuCyclesUsed; // Update CPU cycles used in mainMemory
      cpuCyclesThisRun++;
      globalClock++;
      paramOffset += 2; // Store has 2 parameters
      break;
    }
    case 4: // Load
    {
      int address = mainMemory[process.dataBase + paramOffset];

      // Load only if address is within memory limit
      if (address < process.memoryLimit)
      {
        process.registerValue = mainMemory[process.mainMemoryBase + address]; // Load value from desired address in mainMemory
        mainMemory[startAddress + 7] = process.registerValue;                 // Update register value in mainMemory
        cout << "loaded" << endl;
      }
      else
      {
        cout << "load error!" << endl;
      }

      process.cpuCyclesUsed++;                              // Load operation takes 1 cycle
      mainMemory[startAddress + 6] = process.cpuCyclesUsed; // Update CPU cycles used in mainMemory
      cpuCyclesThisRun++;
      globalClock++;
      paramOffset += 1; // Load has 1 parameter
      break;
    }
    default:
      break;
    }

    // Increment program counter for next instruction
    process.programCounter++;
    mainMemory[startAddress + 2] = process.programCounter;
    
    // Save parameter offset for next time
    paramOffsets[process.processID] = paramOffset;

    // Check for timeout
    if (cpuCyclesThisRun >= CPUAllocated && process.programCounter < process.dataBase)
    {
      cout << "Process " << process.processID << " has a TimeOUT interrupt and is moved to the ReadyQueue." << endl;
      process.state = "READY";                                     // Reset state to READY
      mainMemory[startAddress + 1] = stateEncoding[process.state]; // Update state in mainMemory
      timeoutOccurred = true;                                      // Set flag for timeout
      readyQueue.push(startAddress);                              // Add back to ready queue
      return;                                                      // Exit function to allow other processes to run
    }
  }

  // Process has completed all instructions
  process.programCounter = process.instructionBase - 1; // Reset program counter to instructionBase - 1
  process.state = "TERMINATED";
  mainMemory[startAddress + 2] = process.programCounter;       // Update program counter in mainMemory
  mainMemory[startAddress + 1] = stateEncoding[process.state]; // Update state in mainMemory

  int freedStart = process.mainMemoryBase;
  int freedSize = process.maxMemoryNeeded + 10;
  
  // Calculate total execution time
  int totalExecutionTime = globalClock - processStartTimes[process.processID];

  // Print PCB details
  printPCBFromMainMemory(startAddress, mainMemory);

  // Output termination message
  cout << "Process " << process.processID
       << " terminated. Entered running state at: "
       << processStartTimes[process.processID]
       << ". Terminated at: "
       << globalClock
       << ". Total Execution Time: "
       << totalExecutionTime
       << "." << endl;

  // Free memory
  freeMemory(memoryHead, mainMemory, process.processID);
  
  cout << "Process " << process.processID << " terminated and released memory from "
       << freedStart << " to "
       << (freedStart + freedSize - 1) << "." << endl;
       
  memoryFreed = true;
}

int main()
{
  // Initialize maps
  initOpcodeParams();
  initStateEncoding();

  // Variables for input and queues
  int maxMemory, numProcesses;
  queue<PCB> newJobQueue;
  queue<int> readyQueue;
  int *mainMemory;

  // Read input parameters
  cin >> maxMemory >> CPUAllocated >> contextSwitchTime >> numProcesses;
  
  // Initialize main memory
  mainMemory = new int[maxMemory];
  MemoryBlock *memoryHead = new MemoryBlock(-1, 0, maxMemory); // Initialize memory list as one big free block

  // Initialize all elements of mainMemory to -1
  for (int i = 0; i < maxMemory; i++)
  {
    mainMemory[i] = -1;
  }

  // Read process information
  for (int i = 0; i < numProcesses; i++)
  {
    PCB process;
    int numInstructions;

    cin >> process.processID >> process.maxMemoryNeeded >> numInstructions;
    process.state = "NEW";
    process.memoryLimit = process.maxMemoryNeeded;
    process.programCounter = 0;
    process.cpuCyclesUsed = 0;
    process.registerValue = 0;

    // Read and store instructions
    vector<vector<int> > instructions;
    for (int j = 0; j < numInstructions; j++)
    {
      vector<int> currentInstruction;
      int opcode;

      cin >> opcode;
      currentInstruction.push_back(opcode);
      int numParams = opcodeParams[opcode];

      for (int k = 0; k < numParams; k++)
      {
        int param;
        cin >> param;
        currentInstruction.push_back(param);
      }

      instructions.push_back(currentInstruction);
    }

    processInstructions[process.processID] = instructions;
    newJobQueue.push(process);
  }

  // Load initial processes into memory
  loadJobsToMemory(newJobQueue, readyQueue, mainMemory, memoryHead);

  // Output memory contents
  for (int i = 0; i < maxMemory; i++)
  {
    cout << i << " : " << mainMemory[i] << endl;
  }

  // Main execution loop
  while (!readyQueue.empty() || !IOWaitingQueue.empty() || !newJobQueue.empty())
  {
    if (!readyQueue.empty())
    {
      // Execute next process from ready queue
      int startAddress = readyQueue.front();
      readyQueue.pop();
      executeCPU(startAddress, mainMemory, memoryHead, newJobQueue, readyQueue);

      // If timeout occurred, process already added back to ready queue
      if (timeoutOccurred)
      {
        timeoutOccurred = false;
      }
      
      // If memory was freed, try to load more processes
      if (memoryFreed)
      {
        loadJobsToMemory(newJobQueue, readyQueue, mainMemory, memoryHead);
        memoryFreed = false;
      }
    }
    else if (!IOWaitingQueue.empty())
    {
      // No processes in ready queue, but there are IO waiting processes
      globalClock += contextSwitchTime;
    }
    else
    {
      // No processes in ready queue or IO waiting queue, but there are jobs in new job queue
      // that can't be loaded yet. Just advance the clock and wait.
      globalClock += contextSwitchTime;
    }

    // Check for completed IO operations
    checkIOWaitingQueue(readyQueue, mainMemory);
  }

  // Final context switch
  globalClock += contextSwitchTime;
  cout << "Total CPU time used: " << globalClock << "." << endl;

  // Free memory
  delete[] mainMemory;
  
  // Clean up memory blocks
  MemoryBlock* current = memoryHead;
  while (current) {
    MemoryBlock* next = current->next;
    delete current;
    current = next;
  }

  return 0;
}
/*
g++ -o c c.cpp
./c < sampleInput1.txt
./c < sampleInput1.txt > output.txt
*/