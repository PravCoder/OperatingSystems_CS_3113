// The code is made by Shone George Kutty Renjan and my head hurts.
#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <limits>
#include <sstream>

using namespace std;
struct PCB {
    // Define PCB fields as described earlier
    int processID;  // Unique identifier for the process
    int state;  // Current status of the process will be one of the following: NEW, READY, RUNNING, or TERMINATED
    int programCounter; // index of the next instruction to be executed within the process's logical memory
    int instructionBase;    // Specifies the starting address of the instructions in the logical memory
    int memoryLimit;  // total size of logical memory allocated to the process
    int dataBase;   // Points to the beginning of the data segment within the logical memory
    int cpuCyclesUsed;  // Accumulates the total CPU cycles consumed by the process during its execution
    int registerValue;  // Simulated register used to store intermediate values during load and store operations
    int maxMemoryNeeded;    // Specifies the maximum memory required by the process as defined in the input file
    int mainMemoryBase; // Denotes the starting address in main memory where the process, including its PCB and logical memory, is loade
    vector<vector<int> > instruction; // each arr is a instruction, each element in vector is data assoicated with instruction, 1st element being opcode


};
// Function to print the instruction details
void printInstruction(const vector<int>& instruction) {
    if (instruction.empty()) {
        cout << "Invalid instruction" << endl;
        return;
    }

    switch (instruction[0]) {
        case 1: // Compute instruction
            cout << "Compute Instruction | Iterations: " << instruction[1] 
                 << " | Cycles: " << instruction[2] << endl;
            break;
        case 2: // Simple instruction
            cout << "Simple Instruction | Cycles: " << instruction[1] << endl;
            break;
        case 3: // Store instruction
            cout << "Store Instruction | Value: " << instruction[1] 
                 << " | Address: " << instruction[2] << endl;
            break;
        case 4: // Load instruction
            cout << "Load Instruction | Address: " << instruction[1] << endl;
            break;
        default:
            cout << "Unknown instruction type: " << instruction[0] << endl;
    }
}





// Function to load jobs into memory
void loadJobsToMemory(queue<PCB>& jobQueue, queue<int>& readyQueue, vector<int>& memory, int maxMemory) {
    const int METADATA_SIZE = 10;  // Size of process metadata block
    int currentMemoryAddress = 0;

    while (!jobQueue.empty()) {
        PCB currentJob = jobQueue.front();
        jobQueue.pop();

        // Calculate total memory needed for instructions and data
        int totalInstructionsSize = currentJob.instruction.size();
        int totalDataSize = 0;
        
        // Calculate data size based on instruction types
        for (size_t i = 0; i < currentJob.instruction.size(); i++) {
            vector<int> instruction = currentJob.instruction[i];
            switch (instruction[0]) {
                case 1: totalDataSize += 2; break;  // Compute: needs 2 data slots
                case 2: totalDataSize += 1; break;  // Print: needs 1 data slot
                case 3: totalDataSize += 2; break;  // Store: needs 2 data slots
                case 4: totalDataSize += 1; break;  // Load: needs 1 data slot
            }
        }

        // Calculate memory segments
        currentJob.mainMemoryBase = currentMemoryAddress;
        currentJob.instructionBase = currentJob.mainMemoryBase + METADATA_SIZE;
        currentJob.dataBase = currentJob.instructionBase + totalInstructionsSize;
        int jobEndAddress = currentJob.dataBase + totalDataSize;

        // Verify memory limits
        if (jobEndAddress > maxMemory) {
            // Handle memory overflow - could throw exception or handle error
            continue;
        }

        // Store process metadata
        memory[currentMemoryAddress] = currentJob.processID;
        memory[currentMemoryAddress + 1] = 1;  // Process status
        memory[currentMemoryAddress + 2] = currentJob.programCounter;
        memory[currentMemoryAddress + 3] = currentJob.instructionBase;
        memory[currentMemoryAddress + 4] = currentJob.dataBase;
        memory[currentMemoryAddress + 5] = currentJob.memoryLimit;
        memory[currentMemoryAddress + 6] = currentJob.cpuCyclesUsed;
        memory[currentMemoryAddress + 7] = currentJob.registerValue;
        memory[currentMemoryAddress + 8] = currentJob.maxMemoryNeeded;
        memory[currentMemoryAddress + 9] = currentJob.mainMemoryBase;

        // Load instructions and data
        int instructionMemoryAddress = currentJob.instructionBase;
        int dataMemoryAddress = currentJob.dataBase;

        for (size_t i = 0; i < currentJob.instruction.size(); i++) {
            vector<int> instruction = currentJob.instruction[i];
            int opcode = instruction[0];
            memory[instructionMemoryAddress++] = opcode;

            // Handle different instruction types
            switch (opcode) {
                case 1:  // Compute instruction
                    memory[dataMemoryAddress++] = instruction[1];
                    memory[dataMemoryAddress++] = instruction[2];
                    break;
                    
                case 2:  // Print instruction
                    memory[dataMemoryAddress++] = instruction[1];
                    break;
                    
                case 3:  // Store instruction
                    memory[dataMemoryAddress++] = instruction[1];
                    memory[dataMemoryAddress++] = instruction[2];
                    break;
                    
                case 4:  // Load instruction
                    memory[dataMemoryAddress++] = instruction[1];
                    break;
            }
        }

        // Update memory address for next job
        currentMemoryAddress = currentJob.instructionBase + currentJob.maxMemoryNeeded;
        
        // Add job to ready queue
        readyQueue.push(currentJob.mainMemoryBase);
    }
}

void executeCPU(int startAddress, vector<int>& mainMemory) {
    // Retrieve PCB data from memory
    int procID = mainMemory[startAddress];
    int status = mainMemory[startAddress + 1];
    int instrPtr = mainMemory[startAddress + 2];
    int instrStart = mainMemory[startAddress + 3];
    int dataOffset = mainMemory[startAddress + 4];
    int memLimit = mainMemory[startAddress + 5];
    int execDuration = mainMemory[startAddress + 6];
    int regStore = mainMemory[startAddress + 7];
    int allocLimit = mainMemory[startAddress + 8];
    int baseMem = mainMemory[startAddress + 9];

    int totalInstr = dataOffset - instrStart;
    int logicIdx = 0;

    while (instrPtr < totalInstr) {
        int opcode = mainMemory[instrStart + instrPtr];
        vector<int> params;

        // Fetch instruction parameters
        int numParams = (opcode == 1 || opcode == 3) ? 2 : (opcode == 2 || opcode == 4) ? 1 : 0;
        for (int i = 0; i < numParams; i++) {
            if (logicIdx < memLimit - totalInstr) {
                params.push_back(mainMemory[dataOffset + logicIdx++]);
            } else {
                params.push_back(-1);
            }
        }

        // Execute operation
        switch (opcode) {
            case 1: // Arithmetic operation
                execDuration += params[1];
                cout << "compute" << endl;
                break;
            case 2: // Output
                execDuration += params[0];
                cout << "print" << endl;
                break;
            case 3: // Store
                if (params[1] + instrStart >= instrStart && params[1] + instrStart < allocLimit + instrStart) {
                    mainMemory[params[1] + instrStart] = params[0];
                    regStore = params[0];
                    cout << "stored" << endl;
                } else {
                    regStore = params[0]; // STORE REGISTER VALUE EVEN AFTER STORE ERROR
                    cout << "store error!" << endl;
                }
                execDuration++;
                break;
            case 4: // Load
                if (params[0] + instrStart >= instrStart && params[0] + instrStart < allocLimit + instrStart) {
                    regStore = mainMemory[params[0] + instrStart];
                    cout << "loaded" << endl;
                } else {
                    cout << "load error!" << endl;
                }
                execDuration++;
                break;
            default:
                cerr << "ERROR: Invalid opcode " << opcode << endl;
        }

        instrPtr++;
    }

    // Update process status
    mainMemory[startAddress + 1] = 4;              // terminate process
    mainMemory[startAddress + 2] = instrStart - 1;  
    mainMemory[startAddress + 6] = execDuration;
    mainMemory[startAddress + 7] = regStore;

    // Output process execution details
    cout << "Process ID: " << procID << "\n";
    cout << "State: TERMINATED" << endl;
    cout << "Program Counter: " << mainMemory[startAddress + 2] << endl;
    cout << "Instruction Base: " << instrStart << endl;
    cout << "Data Base: " << dataOffset << endl;
    cout << "Memory Limit: " << memLimit << endl;
    cout << "CPU Cycles Used: " << execDuration << endl;
    cout << "Register Value: " << regStore << endl;
    cout << "Max Memory Needed: " << allocLimit << endl;
    cout << "Main Memory Base: " << baseMem << endl;
    cout << "Total CPU Cycles Consumed: " << execDuration << endl;
}



void displayMemoryContents(vector<int> &memoryBlock, int totalRows) {
    for (int index = 0; index < totalRows && index < memoryBlock.size(); index++) {
        cout << index << " : " << memoryBlock[index] << "\n";
    }
    //cout << "End of Memory Display" << endl;
}
// Function Prototypes for provided functions
void loadJobsToMemory(queue<PCB>& jobQueue, queue<int>& readyQueue, vector<int>& memory, int maxMemory);
// 
void executeCPU(int startAddress, vector<int>& mainMemory);
void displayMemoryContents(vector<int>& memoryBlock, int totalRows);

// Function Prototypes for added functions
void parseProcess(istringstream& inputStream, PCB& process);
void parseInstruction(istringstream& inputStream, int instructionOpcode, vector<int>& currentInstruction);
void executeProcesses(queue<int>& readyQueue, vector<int>& mainMemory);

int main() {
    int maxMemoryCapacity, totalProcesses;
    vector<int> mainMemory;
    queue<int> readyQueue;
    queue<PCB> jobQueue;

    cin >> maxMemoryCapacity >> totalProcesses; // Read memory capacity and number of processes
    mainMemory.resize(maxMemoryCapacity, -1);

    cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Ignore the rest of the line
    for (int i = 0; i < totalProcesses; i++) {
        string line;
        getline(cin, line);
        istringstream inputStream(line); // Create an input stream for the line

        PCB process;
        parseProcess(inputStream, process);  // Function to parse the process details and instructions
        jobQueue.push(process);
    }

    loadJobsToMemory(jobQueue, readyQueue, mainMemory, maxMemoryCapacity);

    displayMemoryContents(mainMemory, maxMemoryCapacity);

    executeProcesses(readyQueue, mainMemory);

    return 0;
}

// Function to parse each process details from the input
void parseProcess(istringstream& inputStream, PCB& process) {
    int processID, maxMemoryRequired, numInstructions;
    inputStream >> processID >> maxMemoryRequired >> numInstructions;

    process.processID = processID; // Unique identifier for the process
    process.maxMemoryNeeded = maxMemoryRequired; // Specifies the maximum memory required by the process as defined in the input file
    process.state=0; // Current status of the process will be one of the following: NEW, READY, RUNNING, or TERMINATED
    process.programCounter =0; // index of the next instruction to be executed within the process's logical memory
    process.memoryLimit = process.maxMemoryNeeded; // total size of logical memory allocated to the process
    process.cpuCyclesUsed=0; // Accumulates the total CPU cycles consumed by the process during its execution
    process.registerValue=0; // Simulated register used to store intermediate values during load and store operations

    // Parse instructions
    for (int j = 0; j < numInstructions; j++) {
        int instructionOpcode; // Opcode for the instruction
        inputStream >> instructionOpcode; // Read the opcode
        vector<int> currentInstruction; // Vector to store the instruction details
        currentInstruction.push_back(instructionOpcode); // Add the opcode to the instruction vector
        parseInstruction(inputStream, instructionOpcode, currentInstruction);
        process.instruction.push_back(currentInstruction);
    }
}

// Function to parse each instruction based on opcode
void parseInstruction(istringstream& inputStream, int opcode, vector<int>& instruction) {
    switch (opcode) {
        case 1: { // Compute instruction (opcode 1)
            int iterationCount, cycleCount; // Number of iterations and cycles for the compute instruction
            inputStream >> iterationCount >> cycleCount; // Read the iteration count and cycle count
            instruction.push_back(iterationCount); // Add the iteration count to the instruction vector
            instruction.push_back(cycleCount);  // Add the cycle count to the instruction vector
            break;
        }
        
        case 2: {
            int cycleCount;
            inputStream >> cycleCount; // Read the cycle count
            instruction.push_back(cycleCount); // Add the cycle count to the instruction vector
            break;
        }
        case 3: {
            int value, address;
            inputStream >> value >> address; // Read the value and address
            instruction.push_back(value); // Add the value to the instruction vector
            instruction.push_back(address); // Add the address to the instruction vector
            break;
        }
        case 4: {
            int address;
            inputStream >> address; // Read the address
            instruction.push_back(address); // Add the address to the instruction vector
            break;
        }
        default:
            // Optional: Handle invalid opcode if needed
            cerr << "Error: Unknown opcode " << opcode << endl;
            break;
    }
}

// Function to execute all processes in the ready queue
void executeProcesses(queue<int>& readyQueue, vector<int>& mainMemory) {
    while (!readyQueue.empty()) {
        int startAddress = readyQueue.front();
        readyQueue.pop();
        executeCPU(startAddress, mainMemory);
    }
}


/* 
g++ -o shone shone.cpp
./shone  < input1.txt
*/