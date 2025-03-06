#include <iostream>
#include <queue>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <array>
#include <limits>
using namespace std;

struct PCB {
    int processID;
    int state; // 1: NEW, 2: READY, 3: RUNNING, 4: TERMINATED
    int programCounter;
    int instructionBase;
    int dataBase;
    int memoryLimit;
    int cpuCyclesUsed;
    int registerValue;
    int maxMemoryNeeded;
    int mainMemoryBase;
    vector<vector<int> > instructions; // Fixed: Use space between nested template arguments
};

// Global variables
int cpuAllocated;
int globalClock = 0;
int contextSwitchTime;
int IOWaitTime = 0;
bool timeOutInterrupt = false;
bool IOInterrupt = false;

unordered_map<int, int> processStartTimes; // Track when processes enter the RUNNING state

void displayPCB(int startAddress, vector<int>& mainMemory) {
    string state;
    int stateInt = mainMemory[startAddress + 1];
    if (stateInt == 1) state = "NEW";
    else if (stateInt == 2) state = "READY";
    else if (stateInt == 3) state = "RUNNING";
    else state = "TERMINATED";

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
    cout << "Total CPU Cycles Consumed: " << globalClock - processStartTimes[mainMemory[startAddress]] << "\n";
}

void loadJobsToMemory(queue<PCB>& newJobQueue, queue<int>& readyQueue, vector<int>& mainMemory, int maxMemory) {
    int memoryIndex = 0;

    while (!newJobQueue.empty()) {
        PCB pcb = newJobQueue.front();
        newJobQueue.pop();

        int startingMemory = memoryIndex;
        mainMemory[memoryIndex++] = pcb.processID;
        mainMemory[memoryIndex++] = 1; // State: NEW
        mainMemory[memoryIndex++] = pcb.programCounter;
        mainMemory[memoryIndex++] = startingMemory + 10;
        mainMemory[memoryIndex++] = startingMemory + 10 + pcb.instructions.size();
        mainMemory[memoryIndex++] = pcb.memoryLimit;
        mainMemory[memoryIndex++] = pcb.cpuCyclesUsed;
        mainMemory[memoryIndex++] = pcb.registerValue;
        mainMemory[memoryIndex++] = pcb.maxMemoryNeeded;
        mainMemory[memoryIndex++] = startingMemory;

        int numInstructions = pcb.instructions.size();
        int instructionAddress = startingMemory + 10;
        int dataAddress = startingMemory + 10 + numInstructions;

        for (int i = 0; i < numInstructions; i++) {
            vector<int> curInstruction = pcb.instructions[i];
            mainMemory[instructionAddress++] = curInstruction[0]; // Opcode

            if (curInstruction[0] == 1) { // Compute
                mainMemory[dataAddress++] = curInstruction[1]; // Iterations
                mainMemory[dataAddress++] = curInstruction[2]; // Cycles
            } else if (curInstruction[0] == 2) { // Print
                mainMemory[dataAddress++] = curInstruction[1]; // Cycles
            } else if (curInstruction[0] == 3) { // Store
                mainMemory[dataAddress++] = curInstruction[1]; // Value
                mainMemory[dataAddress++] = curInstruction[2]; // Address
            } else if (curInstruction[0] == 4) { // Load
                mainMemory[dataAddress++] = curInstruction[1]; // Address
            }
        }

        readyQueue.push(startingMemory);
        memoryIndex = startingMemory + 10 + numInstructions + pcb.maxMemoryNeeded;
    }

    for (int i = 0; i < maxMemory; i++) {
        cout << i << " : " << mainMemory[i] << "\n";
    }
}

void executeCPU(int startAddress, vector<int>& mainMemory) {
    int stateIndex = startAddress + 1;
    int programCounterIndex = startAddress + 2;
    int instructionIndex = mainMemory[startAddress + 3];
    int dataBaseIndex = mainMemory[startAddress + 4];
    int memoryLimitIndex = mainMemory[startAddress + 5];
    int cpuCyclesConsumedIndex = startAddress + 6;
    int registerIndex = startAddress + 7;

    int programCounter;
    int opcodeValue;

    globalClock += contextSwitchTime;

    if (mainMemory[stateIndex] == 1) {
        programCounter = instructionIndex;
        processStartTimes[mainMemory[startAddress]] = globalClock;
    } else {
        programCounter = mainMemory[programCounterIndex];
    }

    mainMemory[stateIndex] = 3; // State: RUNNING
    cout << "Process " << mainMemory[startAddress] << " has moved to Running.\n";

    opcodeValue = mainMemory[programCounter];
    int cpuCyclesConsumed = 0;

    while (programCounter < dataBaseIndex && cpuCyclesConsumed < cpuAllocated) {
        if (opcodeValue == 1) { // Compute
            cout << "compute\n";
            int cycles = mainMemory[dataBaseIndex + 1];
            cpuCyclesConsumed += cycles;
            globalClock += cycles;
            mainMemory[cpuCyclesConsumedIndex] += cycles;
            dataBaseIndex += 2;
        } else if (opcodeValue == 2) { // Print
            cout << "print\n";
            int cycles = mainMemory[dataBaseIndex];
            cpuCyclesConsumed += cycles;
            globalClock += cycles;
            mainMemory[cpuCyclesConsumedIndex] += cycles;
            IOWaitTime = cycles;
            IOInterrupt = true;
            dataBaseIndex++;
            break;
        } else if (opcodeValue == 3) { // Store
            int value = mainMemory[dataBaseIndex];
            int address = mainMemory[dataBaseIndex + 1];
            if (address + startAddress < startAddress + memoryLimitIndex) {
                mainMemory[address + startAddress] = value;
                cout << "stored\n";
                mainMemory[registerIndex] = value; // Update register value
            } else {
                cout << "store error!\n";
            }
            cpuCyclesConsumed++;
            globalClock++;
            mainMemory[cpuCyclesConsumedIndex]++;
            dataBaseIndex += 2;
        } else if (opcodeValue == 4) { // Load
            int address = mainMemory[dataBaseIndex];
            if (address + startAddress < startAddress + memoryLimitIndex) {
                mainMemory[registerIndex] = mainMemory[address + startAddress];
                cout << "loaded\n";
            } else {
                cout << "load error!\n";
            }
            cpuCyclesConsumed++;
            globalClock++;
            mainMemory[cpuCyclesConsumedIndex]++;
            dataBaseIndex++;
        }

        programCounter++;
        opcodeValue = mainMemory[programCounter];
    }

    if (IOInterrupt) {
        programCounter++;
        mainMemory[programCounterIndex] = programCounter;
        return;
    }

    if (cpuCyclesConsumed >= cpuAllocated && programCounter < dataBaseIndex) {
        timeOutInterrupt = true;
        mainMemory[programCounterIndex] = programCounter;
        return;
    }

    mainMemory[programCounterIndex] = instructionIndex - 1;
    mainMemory[stateIndex] = 4; // State: TERMINATED
}

void checkIOWaitingQueue(queue<int>& readyQueue, vector<int>& mainMemory, queue<array<int, 3> >& IOWaitingQueue) {
    int size = IOWaitingQueue.size();
    for (int i = 0; i < size; i++) {
        array<int, 3> IOArray = IOWaitingQueue.front();
        IOWaitingQueue.pop();
        int startAddress = IOArray[0];
        int timeEntered = IOArray[1];
        int timeNeeded = IOArray[2];

        if (globalClock - timeEntered >= timeNeeded) {
            readyQueue.push(startAddress);
            cout << "print\n";
            cout << "Process " << mainMemory[startAddress] << " completed I/O and is moved to the ReadyQueue.\n";
        } else {
            IOWaitingQueue.push(IOArray);
        }
    }
}

int main() {
    int maxMemory;
    int numProcesses;
    queue<PCB> newJobQueue;
    queue<array<int, 3> > IOWaitingQueue; // Fixed: Use space between nested template arguments
    queue<int> readyQueue;

    cin >> maxMemory;
    cin >> cpuAllocated;
    cin >> contextSwitchTime;
    cin >> numProcesses;

    vector<int> mainMemory(maxMemory, -1);

    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    for (int i = 0; i < numProcesses; i++) {
        string line;
        getline(cin, line);
        istringstream ss(line);

        int processID, maxMemoryNeeded, numInstructions;
        ss >> processID >> maxMemoryNeeded >> numInstructions;

        PCB pcb;
        pcb.processID = processID;
        pcb.maxMemoryNeeded = maxMemoryNeeded;
        pcb.state = 1;
        pcb.programCounter = 0;
        pcb.memoryLimit = maxMemoryNeeded;
        pcb.cpuCyclesUsed = 0;
        pcb.registerValue = 0;

        for (int j = 0; j < numInstructions; j++) {
            int opcode;
            ss >> opcode;
            vector<int> instruction;
            instruction.push_back(opcode);

            if (opcode == 1) { // Compute
                int iterations, cycles;
                ss >> iterations >> cycles;
                instruction.push_back(iterations);
                instruction.push_back(cycles);
            } else if (opcode == 2) { // Print
                int cycles;
                ss >> cycles;
                instruction.push_back(cycles);
            } else if (opcode == 3) { // Store
                int value, address;
                ss >> value >> address;
                instruction.push_back(value);
                instruction.push_back(address);
            } else if (opcode == 4) { // Load
                int address;
                ss >> address;
                instruction.push_back(address);
            }

            pcb.instructions.push_back(instruction);
        }

        newJobQueue.push(pcb);
    }

    loadJobsToMemory(newJobQueue, readyQueue, mainMemory, maxMemory);

    while (!readyQueue.empty() || !IOWaitingQueue.empty()) {
        if (!readyQueue.empty()) {
            int startAddress = readyQueue.front();
            readyQueue.pop();
            executeCPU(startAddress, mainMemory);

            if (timeOutInterrupt) {
                cout << "Process " << mainMemory[startAddress] << " has a TimeOUT interrupt and is moved to the ReadyQueue.\n";
                readyQueue.push(startAddress);
                timeOutInterrupt = false;
                checkIOWaitingQueue(readyQueue, mainMemory, IOWaitingQueue);
                continue;
            }

            if (IOInterrupt) {
                cout << "Process " << mainMemory[startAddress] << " issued an IOInterrupt and moved to the IOWaitingQueue.\n";
                array<int, 3> ioArray = {startAddress, globalClock, IOWaitTime};
                IOWaitingQueue.push(ioArray);
                IOInterrupt = false;
                checkIOWaitingQueue(readyQueue, mainMemory, IOWaitingQueue);
                continue;
            }

            displayPCB(startAddress, mainMemory);
            cout << "Process " << mainMemory[startAddress] << " terminated. Entered running state at: " << processStartTimes[mainMemory[startAddress]] << ". Terminated at: " << globalClock << ". Total Execution Time: " << globalClock - processStartTimes[mainMemory[startAddress]] << ".\n";
        } else {
            globalClock += contextSwitchTime;
        }

        checkIOWaitingQueue(readyQueue, mainMemory, IOWaitingQueue);
    }

    globalClock += contextSwitchTime;
    cout << "Total CPU time used: " << globalClock << ".\n";

    return 0;
}
/* 
g++ -o deep deep.cpp
./deep  < sampleInput.txt
*/