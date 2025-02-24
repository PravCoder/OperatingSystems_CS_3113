#include <iostream>
#include <vector> 
#include <string>
#include <fstream>
#include <sstream>
#include <queue>
#include <limits>
using namespace std;

// Global clock and timing values
int globalClock = 0;
int cpuTimeAllocation = 0;
int contextSwitchTime = 0;

struct PCB {
    int processID;          // Identifier of process
    int state;             // NEW=0, READY=1, RUNNING=2, IOWAITING=3, TERMINATED=4
    int programCounter;     // Index of next instruction to execute
    int instructionBase;    // Starting address of instructions
    int dataBase;          // Starting address of data segment
    int memoryLimit;       // Total size of logical memory for process
    int cpuCyclesUsed;     // CPU cycles consumed
    int registerValue;      // Current register value
    int maxMemoryNeeded;   // Maximum memory required
    int mainMemoryBase;    // Starting address in main memory
    int runStartTime;      // Time when process first started running
    vector<vector<int> > instructions;  // Process instructions
};

void printPCB(const PCB& pcb) {
    cout << "Process ID: " << pcb.processID << endl
         << "State: " << pcb.state << endl
         << "Program Counter: " << pcb.programCounter << endl
         << "Instruction Base: " << pcb.instructionBase << endl
         << "Data Base: " << pcb.dataBase << endl
         << "Memory Limit: " << pcb.memoryLimit << endl
         << "CPU Cycles Used: " << pcb.cpuCyclesUsed << endl
         << "Register Value: " << pcb.registerValue << endl
         << "Max Memory Needed: " << pcb.maxMemoryNeeded << endl
         << "Main Memory Base: " << pcb.mainMemoryBase << endl;
}

void checkIOQueue(queue<PCB>& ioWaitingQueue, queue<int>& readyQueue, vector<int>& mainMemory) {
    int queueSize = ioWaitingQueue.size();
    
    // Process all jobs in IO queue simultaneously
    for(int i = 0; i < queueSize; i++) {
        PCB currentProcess = ioWaitingQueue.front();
        ioWaitingQueue.pop();
        
        // Check if IO operation is complete
        if(currentProcess.cpuCyclesUsed <= globalClock) {
            cout << "Process " << currentProcess.processID 
                 << " completed I/O and is moved to ReadyQueue." << endl;
            
            // Update process state in main memory
            mainMemory[currentProcess.mainMemoryBase + 1] = 1;  // Set to READY
            readyQueue.push(currentProcess.mainMemoryBase);
        } else {
            ioWaitingQueue.push(currentProcess);
        }
    }
}

void loadJobsToMemory(queue<PCB>& newJobQueue, queue<int>& readyQueue, vector<int>& mainMemory) {
    int currentAddress = 0;
    
    while(!newJobQueue.empty()) {
        PCB currentProcess = newJobQueue.front();
        newJobQueue.pop();
        
        // Calculate memory addresses
        currentProcess.mainMemoryBase = currentAddress;
        currentProcess.instructionBase = currentAddress + 10;
        currentProcess.dataBase = currentProcess.instructionBase + currentProcess.instructions.size();
        
        // Store PCB metadata in memory
        mainMemory[currentAddress] = currentProcess.processID;
        mainMemory[currentAddress + 1] = 1;  // Set to READY state
        mainMemory[currentAddress + 2] = currentProcess.programCounter;
        mainMemory[currentAddress + 3] = currentProcess.instructionBase;
        mainMemory[currentAddress + 4] = currentProcess.dataBase;
        mainMemory[currentAddress + 5] = currentProcess.memoryLimit;
        mainMemory[currentAddress + 6] = currentProcess.cpuCyclesUsed;
        mainMemory[currentAddress + 7] = currentProcess.registerValue;
        mainMemory[currentAddress + 8] = currentProcess.maxMemoryNeeded;
        mainMemory[currentAddress + 9] = currentProcess.mainMemoryBase;
        
        // Store instructions
        int instructionAddress = currentProcess.instructionBase;
        int dataAddress = currentProcess.dataBase;
        
        for(size_t i = 0; i < currentProcess.instructions.size(); i++) {
            vector<int>& currentInstruction = currentProcess.instructions[i];
            int opcode = currentInstruction[0];
            
            mainMemory[instructionAddress++] = opcode;
            
            // Store instruction data based on opcode
            switch(opcode) {
                case 1: // Compute
                    mainMemory[dataAddress++] = currentInstruction[1]; // iterations
                    mainMemory[dataAddress++] = currentInstruction[2]; // cycles
                    break;
                case 2: // Print
                    mainMemory[dataAddress++] = currentInstruction[1]; // cycles
                    break;
                case 3: // Store
                    mainMemory[dataAddress++] = currentInstruction[1]; // value
                    mainMemory[dataAddress++] = currentInstruction[2]; // address
                    break;
                case 4: // Load
                    mainMemory[dataAddress++] = currentInstruction[1]; // address
                    break;
            }
        }
        
        currentAddress = currentProcess.instructionBase + currentProcess.maxMemoryNeeded;
        readyQueue.push(currentProcess.mainMemoryBase);
    }
}

bool executeCPU(int startAddress, vector<int>& mainMemory, queue<int>& readyQueue, 
                queue<PCB>& ioWaitingQueue) {
    // Extract process information from memory
    int processID = mainMemory[startAddress];
    int& state = mainMemory[startAddress + 1];
    int& programCounter = mainMemory[startAddress + 2];
    int instructionBase = mainMemory[startAddress + 3];
    int dataBase = mainMemory[startAddress + 4];
    int& cpuCyclesUsed = mainMemory[startAddress + 6];
    int& registerValue = mainMemory[startAddress + 7];
    int maxMemoryNeeded = mainMemory[startAddress + 8];
    
    static int lastStartTime = -1;
    
    // Handle process state transition
    if(state == 1) { // READY to RUNNING
        globalClock += contextSwitchTime;
        cout << "Process " << processID 
             << " moved from ReadyQueue to Running state at time " 
             << globalClock << endl;
        
        if(lastStartTime == -1) {
            lastStartTime = globalClock;
        }
        state = 2; // Set to RUNNING
    }
    
    int currentCPUTime = 0;
    int dataIndex = 0;
    
    while(true) {
        // Get current instruction
        int opcode = mainMemory[instructionBase + programCounter];
        
        // Execute instruction (atomic operation)
        switch(opcode) {
            case 1: { // Compute
                int iterations = mainMemory[dataBase + dataIndex++];
                int cycles = mainMemory[dataBase + dataIndex++];
                
                // Execute compute operation atomically
                cpuCyclesUsed += cycles;
                currentCPUTime += cycles;
                globalClock += cycles;
                
                // Check for timeout after atomic operation
                if(currentCPUTime > cpuTimeAllocation) {
                    cout << "Process " << processID 
                         << " has a TimeOUT interrupt and is moved to the ReadyQueue." 
                         << endl;
                    state = 1; // Back to READY
                    readyQueue.push(startAddress);
                    checkIOQueue(ioWaitingQueue, readyQueue, mainMemory);
                    return false;
                }
                break;
            }
            
            case 2: { // Print (I/O operation)
                int ioCycles = mainMemory[dataBase + dataIndex++];
                
                // Create PCB for IO waiting
                PCB ioProcess;
                ioProcess.processID = processID;
                ioProcess.mainMemoryBase = startAddress;
                ioProcess.cpuCyclesUsed = globalClock + ioCycles;
                
                cout << "Process " << processID 
                     << " issued an IOInterrupt and moved to IOWaitingQueue." << endl;
                
                state = 3; // Set to IOWAITING
                ioWaitingQueue.push(ioProcess);
                checkIOQueue(ioWaitingQueue, readyQueue, mainMemory);
                return false;
            }
            
            case 3: { // Store
                int value = mainMemory[dataBase + dataIndex++];
                int address = mainMemory[dataBase + dataIndex++];
                
                if(address >= 0 && address < maxMemoryNeeded) {
                    mainMemory[instructionBase + address] = value;
                    registerValue = value;
                }
                
                cpuCyclesUsed++;
                currentCPUTime++;
                globalClock++;
                
                if(currentCPUTime > cpuTimeAllocation) {
                    cout << "Process " << processID 
                         << " has a TimeOUT interrupt and is moved to the ReadyQueue." 
                         << endl;
                    state = 1;
                    readyQueue.push(startAddress);
                    checkIOQueue(ioWaitingQueue, readyQueue, mainMemory);
                    return false;
                }
                break;
            }
            
            case 4: { // Load
                int address = mainMemory[dataBase + dataIndex++];
                
                if(address >= 0 && address < maxMemoryNeeded) {
                    registerValue = mainMemory[instructionBase + address];
                }
                
                cpuCyclesUsed++;
                currentCPUTime++;
                globalClock++;
                
                if(currentCPUTime > cpuTimeAllocation) {
                    cout << "Process " << processID 
                         << " has a TimeOUT interrupt and is moved to the ReadyQueue." 
                         << endl;
                    state = 1;
                    readyQueue.push(startAddress);
                    checkIOQueue(ioWaitingQueue, readyQueue, mainMemory);
                    return false;
                }
                break;
            }
        }
        
        programCounter++;
        
        // Check if process has completed
        if(programCounter >= dataBase - instructionBase) {
            state = 4; // TERMINATED
            globalClock += contextSwitchTime;
            
            cout << "Process " << processID 
                 << " terminated. Entered running state at: " << lastStartTime 
                 << ". Terminated at: " << globalClock 
                 << ". Total Execution Time: " << (globalClock - lastStartTime) << endl;
            
            checkIOQueue(ioWaitingQueue, readyQueue, mainMemory);
            return true;
        }
    }
}

int main() {
    // Read initial parameters
    int maxMemory;
    cin >> maxMemory >> cpuTimeAllocation >> contextSwitchTime;
    
    int numProcesses;
    cin >> numProcesses;
    
    // Initialize memory and queues
    vector<int> mainMemory(maxMemory, -1);
    queue<int> readyQueue;
    queue<PCB> newJobQueue;
    queue<PCB> ioWaitingQueue;
    
    // Parse process information
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    
    for(int i = 0; i < numProcesses; i++) {
        string line;
        getline(cin, line);
        istringstream ss(line);
        
        PCB process;
        int numInstructions;
        
        ss >> process.processID >> process.maxMemoryNeeded >> numInstructions;
        
        // Initialize PCB fields
        process.state = 0; // NEW
        process.programCounter = 0;
        process.memoryLimit = process.maxMemoryNeeded;
        process.cpuCyclesUsed = 0;
        process.registerValue = 0;
        process.runStartTime = -1;
        
        // Read instructions
        for(int j = 0; j < numInstructions; j++) {
            int opcode;
            ss >> opcode;
            
            vector<int> instruction;
            instruction.push_back(opcode);
            
            switch(opcode) {
                case 1: { // Compute
                    int iterations, cycles;
                    ss >> iterations >> cycles;
                    instruction.push_back(iterations);
                    instruction.push_back(cycles);
                    break;
                }
                case 2: { // Print
                    int cycles;
                    ss >> cycles;
                    instruction.push_back(cycles);
                    break;
                }
                case 3: { // Store
                    int value, address;
                    ss >> value >> address;
                    instruction.push_back(value);
                    instruction.push_back(address);
                    break;
                }
                case 4: { // Load
                    int address;
                    ss >> address;
                    instruction.push_back(address);
                    break;
                }
            }
            process.instructions.push_back(instruction);
        }
        newJobQueue.push(process);
    }
    
    // Load processes into memory
    loadJobsToMemory(newJobQueue, readyQueue, mainMemory);
    
    // Print initial memory state
    cout << "Initial Memory State:" << endl;
    for(int i = 0; i < mainMemory.size(); i++) {
        cout << i << " : " << mainMemory[i] << endl;
    }
    
    // Initial context switch
    globalClock += contextSwitchTime;
    
    // Execute processes
    bool processingComplete = false;
    while(!processingComplete) {
        // Check if all queues are empty
        if(readyQueue.empty() && ioWaitingQueue.empty()) {
            processingComplete = true;
            continue;
        }
        
        // Process ready queue
        if(!readyQueue.empty()) {
            int processAddress = readyQueue.front();
            readyQueue.pop();
            executeCPU(processAddress, mainMemory, readyQueue, ioWaitingQueue);
        }
        // If ready queue is empty but IO queue isn't, increment clock and check IO
        else if(!ioWaitingQueue.empty()) {
            globalClock += contextSwitchTime;
            checkIOQueue(ioWaitingQueue, readyQueue, mainMemory);
        }
    }
    
    // Final context switch
    globalClock += contextSwitchTime;
    cout << "Total CPU time used: " << globalClock << endl;
    
    return 0;
}

/*
g++ -o CS3113_Project2 CS3113_Project2.cpp
./CS3113_Project2  < sampleInput.txt
*/