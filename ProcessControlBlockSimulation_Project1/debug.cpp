#include <iostream>
#include <queue>
#include <vector>
using namespace std;

// PCB Structure
struct PCB
{
    int processID;
    int state; // 0=NEW, 1=READY, 2=RUNNING, 3=TERMINATED
    int programCounter;
    int instructionBase;
    int dataBase;
    int memoryLimit;
    int cpuCyclesUsed;
    int registerValue;
    int maxMemoryNeeded;
    int mainMemoryBase;
    vector<int> instructions;
    vector<int> data;
};

// Load Jobs into Memory
void loadJobsToMemory(queue<PCB> &newJobQueue, queue<int> &readyQueue, vector<int> &mainMemory, int maxMemory)
{
    int currentAddress = 0;
    while (!newJobQueue.empty())
    {
        PCB process = newJobQueue.front();
        newJobQueue.pop();

        process.mainMemoryBase = currentAddress;
        process.instructionBase = currentAddress + 10;
        process.dataBase = process.instructionBase + process.instructions.size();

        // Store PCB in mainMemory
        mainMemory[currentAddress] = process.processID;
        mainMemory[currentAddress + 1] = 1; // READY
        mainMemory[currentAddress + 2] = process.programCounter;
        mainMemory[currentAddress + 3] = process.instructionBase;
        mainMemory[currentAddress + 4] = process.dataBase;
        mainMemory[currentAddress + 5] = process.memoryLimit;
        mainMemory[currentAddress + 6] = process.cpuCyclesUsed;
        mainMemory[currentAddress + 7] = process.registerValue;
        mainMemory[currentAddress + 8] = process.maxMemoryNeeded;
        mainMemory[currentAddress + 9] = process.mainMemoryBase;

        // Store Instructions and Data separately
        int instructionAddress = process.instructionBase;
        int dataAddress = process.dataBase;
        for (size_t i = 0; i < process.instructions.size(); i++)
        {
            int opcode = process.instructions[i];
            mainMemory[instructionAddress++] = opcode;

            if (opcode == 1) // Compute
            {
                mainMemory[dataAddress++] = process.instructions[++i]; // Iterations
                mainMemory[dataAddress++] = process.instructions[++i]; // Cycles
            }
            else if (opcode == 2) // Print
            {
                mainMemory[dataAddress++] = process.instructions[++i]; // Cycles
            }
            else if (opcode == 3) // Store
            {
                mainMemory[dataAddress++] = process.instructions[++i]; // Value
                mainMemory[dataAddress++] = process.instructions[++i]; // Address
            }
            else if (opcode == 4) // Load
            {
                mainMemory[dataAddress++] = process.instructions[++i]; // Address
            }
        }
        currentAddress += (10 + process.memoryLimit);
        readyQueue.push(process.mainMemoryBase);
    }
}

void executeCPU(queue<int> &readyQueue, vector<int> &mainMemory)
{
    while (!readyQueue.empty())
    {
        int baseAddress = readyQueue.front();
        readyQueue.pop();

        int instructionBase = mainMemory[baseAddress + 3];
        int dataBase = mainMemory[baseAddress + 4];
        int programCounter = instructionBase;
        int cpuCyclesUsed = mainMemory[baseAddress + 6];
        int registerValue = 0;

        while (programCounter < dataBase)
        {
            cout << "pc: " << programCounter << " database: " << dataBase << endl;
            int opCode = mainMemory[programCounter];

            if (opCode == 1) // Compute
            {
                int iterations = mainMemory[dataBase + (programCounter - instructionBase)];
                int cycles = mainMemory[dataBase + (programCounter - instructionBase) + 1];
                cpuCyclesUsed += iterations * cycles;
                cout << "compute" << endl;
                programCounter += 3;
            }
            else if (opCode == 2) // Print
            {
                cout << "print" << endl;
                programCounter += 2;
            }
            else if (opCode == 3) // Store
            {
                int value = mainMemory[dataBase + (programCounter - instructionBase)];
                int address = mainMemory[dataBase + (programCounter - instructionBase) + 1];
                if (address < mainMemory[baseAddress + 5])
                {
                    mainMemory[dataBase + address] = value;
                    cout << "stored" << endl;
                }
                else
                {
                    cout << "store error!" << endl;
                }
                programCounter += 3;
            }
            else if (opCode == 4) // Load
            {
                int address = mainMemory[dataBase + (programCounter - instructionBase)];
                if (address < mainMemory[baseAddress + 5])
                {
                    registerValue = mainMemory[dataBase + address];
                    cout << "loaded" << endl;
                }
                else
                {
                    cout << "load error!" << endl;
                }
                programCounter += 2;
            }
            else
            {
                cout << "Invalid Instruction!" << endl;
                break;
            }
        }
        mainMemory[baseAddress + 1] = 3; // TERMINATED
        cout << "Process ID: " << mainMemory[baseAddress] << endl;
        cout << "State: TERMINATED" << endl;
        cout << "Program Counter: " << programCounter << endl;
        cout << "Instruction Base: " << instructionBase << endl;
        cout << "Data Base: " << dataBase << endl;
        cout << "Memory Limit: " << mainMemory[baseAddress + 5] << endl;
        cout << "CPU Cycles Used: " << cpuCyclesUsed << endl;
        cout << "Register Value: " << registerValue << endl;
        cout << "Max Memory Needed: " << mainMemory[baseAddress + 8] << endl;
        cout << "Main Memory Base: " << mainMemory[baseAddress + 9] << endl;
        cout << "Total CPU Cycles Consumed: " << cpuCyclesUsed << endl;
    }
}

void print_memory(vector<int> &mainMemory)
{
    for (size_t i = 0; i < 100; i++)
    {
        cout << i << ": " << mainMemory[i] << endl;
    }
    cout << endl;
}

int main()
{
    int maxMemory, numProcesses;
    vector<int> mainMemory;
    queue<int> readyQueue;
    queue<PCB> newJobQueue;

    cin >> maxMemory >> numProcesses;
    mainMemory.assign(maxMemory, -1);
    for (int i = 0; i < numProcesses; i++)
    {
        PCB process;
        cin >> process.processID >> process.maxMemoryNeeded;
        process.state = 0;
        process.programCounter = 0;
        process.memoryLimit = process.maxMemoryNeeded;
        process.cpuCyclesUsed = 0;
        process.registerValue = 0;
        int numInstructions;
        cin >> numInstructions;
        for (int j = 0; j < numInstructions; j++)
        {
            int opcode;
            cin >> opcode;
            process.instructions.push_back(opcode);
        }
        newJobQueue.push(process);
    }
    loadJobsToMemory(newJobQueue, readyQueue, mainMemory, maxMemory);

    cout << "After loading jobs to memory" << endl;
    print_memory(mainMemory);

    cout << endl << endl;

    executeCPU(readyQueue, mainMemory);
    print_memory(mainMemory);
    return 0;
}

// g++ -o debug debug.cpp
// ./debug < input1.txt