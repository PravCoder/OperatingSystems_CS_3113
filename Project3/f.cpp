#include <iostream>
#include <queue>
#include <vector>
#include <fstream>
#include <string>
#include <unordered_map>
using namespace std;

struct PCB
{
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
};

// Custom structure to replace tuple in IOWaitingQueue
struct IOWaitQueueItem {
  PCB process;
  int startAddress;
  int waitTime;
  int timeEnteredIO;
  
  // Constructor for easier initialization
  IOWaitQueueItem(PCB p, int start, int wait, int time)
    : process(p), startAddress(start), waitTime(wait), timeEnteredIO(time) {}
};

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

// Key: processID, Value: value of the global clock the first time the process entered runnning state
unordered_map<int, int> processStartTimes;

// Helps keep track of each process's current parameter offset now that it's needed across multiple scopes
unordered_map<int, int> paramOffsets; // Key: processID, Value: paramOffset

int globalClock = 0;
bool timeoutOccurred = false;
bool memoryFreed = false;
queue<IOWaitQueueItem> IOWaitingQueue; // Queue of IOWaitQueueItem
int contextSwitchTime, CPUAllocated;

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
    int totalMemoryNeeded = process.maxMemoryNeeded + 10;
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

      // Store opcodes: [[opcode1, param1, param2], [opcode2, param1, param2], ...]
      vector<vector<int> > instructions = processInstructions[process.processID];
      int writeIndex = process.instructionBase;

      for (size_t j = 0; j < instructions.size(); j++)
      {
        const vector<int>& instr = instructions[j];
        mainMemory[writeIndex++] = instr[0];
      }

      // Store parameters
      for (size_t j = 0; j < instructions.size(); j++)
      {
        const vector<int>& instr = instructions[j];
        for (int i = 1; i < instr.size(); i++)
        {
          mainMemory[writeIndex++] = instr[i];
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

void executeCPU(int startAddress, int *mainMemory, MemoryBlock *&memoryHead, queue<PCB> &newJobQueue, queue<int> &readyQueue)
{
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

  if (process.programCounter == 0) // First time running this process
  {
    process.programCounter = process.instructionBase;   // Initialize PC if it's our first time running this process
    paramOffsets[process.processID] = 0;                // Initialize paramOffset for this process
    processStartTimes[process.processID] = globalClock; // Store the time the process first entered running state
  }

  process.state = "RUNNING";
  mainMemory[startAddress + 1] = stateEncoding[process.state]; // Update state in mainMemory
  mainMemory[startAddress + 2] = process.programCounter;       // Update program counter in mainMemory
  cout << "Process " << process.processID << " has moved to Running." << endl;

  int paramOffset = paramOffsets[process.processID];

  // Execute loop over instruction section of mainMemory (all the opcodes)
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
      break;
    }
    case 2: // Print
    {
      int cycles = mainMemory[process.dataBase + paramOffset];
      cout << "Process " << process.processID << " issued an IOInterrupt and moved to the IOWaitingQueue." << endl;
      IOWaitingQueue.push(IOWaitQueueItem(process, startAddress, cycles, globalClock)); // Move process to IOWaitingQueue
      process.state = "IOWAITING";
      mainMemory[startAddress + 1] = stateEncoding[process.state]; // Update state in mainMemory
      return;                                                      // Exit function to allow other processes to run while this one waits in IOWaitingQueue
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
      break;
    }
    default:
      break;
    }

    // Increment program counter and paramOffset for next instruction
    process.programCounter++;
    mainMemory[startAddress + 2] = process.programCounter;
    paramOffset += opcodeParams[opcode];
    paramOffsets[process.processID] = paramOffset;

    // Check if process has exceeded CPU allocation AND if it's not on the very last instruction
    if (cpuCyclesThisRun >= CPUAllocated && process.programCounter < process.dataBase)
    {
      cout << "Process " << process.processID << " has a TimeOUT interrupt and is moved to the ReadyQueue." << endl;
      process.state = "READY";                                     // Reset state to READY
      mainMemory[startAddress + 1] = stateEncoding[process.state]; // Update state in mainMemory
      timeoutOccurred = true;                                      // Set flag for timeout interrupt
      return;                                                      // Exit function to allow other processes to run
    }
  }

  process.programCounter = process.instructionBase - 1; // Reset program counter to instructionBase - 1
  process.state = "TERMINATED";
  mainMemory[startAddress + 2] = process.programCounter;       // Update program counter in mainMemory
  mainMemory[startAddress + 1] = stateEncoding[process.state]; // Update state in mainMemory

  int freedStart = process.mainMemoryBase;
  int freedSize = process.maxMemoryNeeded + 10;
  freeMemory(memoryHead, mainMemory, process.processID); // Free memory allocated to this process
  memoryFreed = true;

  int totalExecutionTime = globalClock - processStartTimes[process.processID];

  // Output the details of the process that just completed execution
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
  cout << "Total CPU Cycles Consumed: " << totalExecutionTime << endl;

  cout << "Process " << process.processID
       << " terminated. Entered running state at: "
       << processStartTimes[process.processID]
       << ". Terminated at: "
       << globalClock
       << ". Total Execution Time: "
       << totalExecutionTime
       << "." << endl;

  cout << "Process " << process.processID << " terminated and released memory from "
       << freedStart << " to "
       << (freedStart + freedSize - 1) << "." << endl;
}

void checkIOWaitingQueue(queue<int> &readyQueue,
                         int *mainMemory)
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
      paramOffset += opcodeParams[2];
      paramOffsets[process.processID] = paramOffset;

      // Reset state to READY and context switch
      process.state = "READY";                                     // Reset state to READY
      mainMemory[startAddress + 1] = stateEncoding[process.state]; // Update state in mainMemory

      cout << "Process " << process.processID << " completed I/O and is moved to the ReadyQueue." << endl;
      readyQueue.push(startAddress);
    }
    else
    {
      IOWaitingQueue.push(IOWaitQueueItem(process, startAddress, waitTime, timeEnteredIO)); // Put back in IOWaitingQueue
    }
  }
}

int main()
{
  // Initialize maps
  initOpcodeParams();
  initStateEncoding();

  int maxMemory, numProcesses;
  queue<PCB> newJobQueue;
  queue<int> readyQueue;
  int *mainMemory;

  // Read from input file (via redirection)
  cin >> maxMemory >> CPUAllocated >> contextSwitchTime >> numProcesses;
  mainMemory = new int[maxMemory];                             // Dynamically allocate memory to serve as main memory
  MemoryBlock *memoryHead = new MemoryBlock(-1, 0, maxMemory); // Initialize memory list as one big free block

  // Initialize all elements of mainMemory to -1
  for (int i = 0; i < maxMemory; i++)
  {
    mainMemory[i] = -1;
  }

  // Loop over each process to build its PCB, store its instructions, and push it to the newJobQueue
  for (int i = 0; i < numProcesses; i++)
  {
    PCB process;
    int numInstructions;

    cin >> process.processID >> process.maxMemoryNeeded >> numInstructions; // Read process details from input file
    process.state = "NEW";
    process.memoryLimit = process.maxMemoryNeeded;
    process.programCounter = 0;
    process.cpuCyclesUsed = 0;
    process.registerValue = 0;

    // Read and store all the instructions for the process
    vector<vector<int> > instructions;
    for (int j = 0; j < numInstructions; j++)
    {
      vector<int> currentInstruction;
      int opcode;

      cin >> opcode;                        // Read opcode from input file
      currentInstruction.push_back(opcode); // Store opcode in instruction vector
      int numParams = opcodeParams[opcode]; // Get number of parameters for opcode

      for (int k = 0; k < numParams; k++)
      {
        int param;
        cin >> param;                        // Read parameters from input file
        currentInstruction.push_back(param); // Store parameters in instruction vector
      }

      instructions.push_back(currentInstruction); // Store instruction vector in instructions 2d vector
    }

    processInstructions[process.processID] = instructions; // Associate processID with its instructions in processInstructions map
    newJobQueue.push(process);                             // Push process to newJobQueue
  }

  loadJobsToMemory(newJobQueue, readyQueue, mainMemory, memoryHead); // Load jobs into main memory

  // Output mainMemory contents
  for (int i = 0; i < maxMemory; i++)
  {
    cout << i << " : " << mainMemory[i] << endl;
  }

  while (!readyQueue.empty() || !IOWaitingQueue.empty())
  {
    if (!readyQueue.empty())
    {
      int startAddress = readyQueue.front();
      readyQueue.pop();
      executeCPU(startAddress, mainMemory, memoryHead, newJobQueue, readyQueue);

      // If a TimeOUT Interrupt occurred, push the process to the back of the readyQueue
      if (timeoutOccurred)
      {
        readyQueue.push(startAddress);
        timeoutOccurred = false;
      }
    }
    else
    {
      // No jobs in readyQueue, but there are jobs in IOWaitingQueue
      globalClock += contextSwitchTime; // Advance global clock while waiting
    }

    if (memoryFreed)
    {
      loadJobsToMemory(newJobQueue, readyQueue, mainMemory, memoryHead); // Load jobs into main memory if there are any still in the newJobQueue
      memoryFreed = false;
    }

    checkIOWaitingQueue(readyQueue, mainMemory); // Always check for I/O completion after an interrupt
  }

  globalClock += contextSwitchTime; // Update global clock
  cout << "Total CPU time used: " << globalClock << "." << endl;

  delete[] mainMemory; // Free dynamically allocated memory
  return 0;
}
/*
g++ -o f f.cpp
./f < sampleInput1.txt
./f < sampleInput1.txt > output.txt
*/