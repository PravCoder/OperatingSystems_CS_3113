#include <iostream>
#include <queue>
#include <vector> // We need vector for mainMemory
//#include <fstream>
using namespace std;

// ---------------------------------------------------------------------
// PCB Structure
// ---------------------------------------------------------------------
struct PCB
{
    int processID;
    int state;           // 0=NEW, 1=READY, 2=RUNNING, 3=TERMINATED
    int programCounter;  // Next instruction index
    int instructionBase; // Where instructions begin in logical memory
    int dataBase;        // Where data starts in logical memory
    int memoryLimit;     // Total size of logical memory for this process
    int cpuCyclesUsed;   // Number of CPU cycles needed for the process
    int registerValue;   // the value to be stored in the register and wrote back to memory
    int maxMemoryNeeded; // the maximum memory reserved for the process
    int mainMemoryBase;  // Where the process (PCB + instructions + data) starts in main memory

    // For simplicity, store the raw instructions here *before* loading to main memory.
    // The project’s final design might store them differently.
    vector<int> instructions;
};

//ofstream out("Out.txt");

// ---------------------------------------------------------------------
// loadJobsToMemory
//   - Moves processes from newJobQueue into mainMemory if space is available
//   - Writes PCB fields + instructions into mainMemory
//   - Enqueues each process’s start address in readyQueue
// ---------------------------------------------------------------------
void loadJobsToMemory(queue<PCB> &newJobQueue,queue<int> &readyQueue,vector<int> &mainMemory,int maxMemory)
{
    int currentAddress = 0;

    while(!newJobQueue.empty())
    {
        PCB process = newJobQueue.front();
        newJobQueue.pop();

        // Assign the start address address for the process
        process.mainMemoryBase = currentAddress;
        process.instructionBase = currentAddress + 10;

        // read the number of instructions 
        int numInstructions;
        cin >> numInstructions;

        process.dataBase = process.instructionBase + numInstructions;
        mainMemory[currentAddress + 4] = process.dataBase;
        

        // Store the PCB in memory, memory in this case is sequential so id is at 0, state is at 1 and so on
        // for a total of 10 indexes hence the magic number 10 in assignment of the instruction base (line 44)
        mainMemory[currentAddress] = process.processID;
        mainMemory[currentAddress + 1] = 1; // 1 is the code for READY
        mainMemory[currentAddress + 2] = process.programCounter;
        mainMemory[currentAddress + 3] = process.instructionBase;
        mainMemory[currentAddress + 4] = process.dataBase;
        mainMemory[currentAddress + 5] = process.memoryLimit;
        mainMemory[currentAddress + 6] = process.cpuCyclesUsed;
        mainMemory[currentAddress + 7] = process.registerValue;
        mainMemory[currentAddress + 8] = process.maxMemoryNeeded;
        mainMemory[currentAddress + 9] = process.mainMemoryBase;


        // store these instructions
        int instructionAddress = process.instructionBase;
        int dataAddress = process.dataBase;

        for(int i = 0; i < numInstructions; i++)
        {
            int opcode;
            cin >> opcode;
            mainMemory[instructionAddress++] = opcode;

            if(opcode == 1) // compute
            {
                int iterations, cycles;
                cin >> iterations >> cycles;

                mainMemory[dataAddress++] = iterations;
                mainMemory[dataAddress++] = cycles;
            }
            else if(opcode == 2) // print
            {
                int cycles;
                cin >> cycles;
                mainMemory[dataAddress++] = cycles;
            }
            else if(opcode == 3) // store
            {
                int value, address;
                cin >> value >> address;

                mainMemory[dataAddress++] = value;
                mainMemory[dataAddress++] = address;
            }
            else if(opcode == 4) // Load
            {
                int address;
                cin >> address;
                mainMemory[dataAddress++] = address;
            }
        }// END FOR LOOP

        // We now know the instruction end so we can set the database
        //process.dataBase = dataAddress;
        //mainMemory[currentAddress + 4] = process.dataBase;

        // Move to the next full memory block
        currentAddress += (10 + process.memoryLimit);

        // push to the ready queue
        readyQueue.push(process.mainMemoryBase);

    }// END WHILE LOOP
}// END FUNCTION



void printMemoryTable(const vector<int> mainMemory, int maxMemory)
{
    for(int i = 0; i <maxMemory; i++)
    {
        cout << i << " : " << mainMemory[i] << endl;
        //out << i << " : " << mainMemory[i] << endl;
    }
}

// ---------------------------------------------------------------------
// executeCPU
//   - Simulates execution of the process whose PCB is at 'startAddress'
//   - Decodes instructions from mainMemory, updates PCB fields
//   - Prints final results
// ---------------------------------------------------------------------
void executeCPU(queue<PCB> &newJobQueue, queue<int> &readyQueue, vector<int> &mainMemory, int maxMemory)
{
    while(!readyQueue.empty())
    {
        int baseAddress = readyQueue.front();
        readyQueue.pop();

        int processID = mainMemory[baseAddress];
        int instructionBase = mainMemory[baseAddress + 3];
        int database = mainMemory[baseAddress + 4];
        int programCounter = mainMemory[baseAddress + 2];
        int cpuCyclesUsed = mainMemory[baseAddress + 6];

        while(programCounter < database)
        {
            int opCode = mainMemory[instructionBase + (programCounter - instructionBase)];

            if(opCode == 1) // compute
            {
                int iterations = mainMemory[database + (programCounter - instructionBase) + 1];
                int cycles = mainMemory[database + (programCounter - instructionBase) + 2];

                cpuCyclesUsed += iterations * cycles;
                
                cout << "Compute " << endl;
                //out << "Compute " << endl;
                programCounter += 3;
            }
            else if(opCode == 2) // print
            {
                int cycles = mainMemory[database + (programCounter - instructionBase) + 1];
                cpuCyclesUsed += cycles;

                cout << "print " << endl;
                //out << "print " << endl;
                programCounter += 2;
            }
            else if(opCode == 3) // store
            {
                int value = mainMemory[database + (programCounter - instructionBase) + 1];
                int address = mainMemory[database + (programCounter - instructionBase) + 2];

                if(address < mainMemory[baseAddress + 5])
                {
                    mainMemory[database + address] = value;
                    cout << "Stored " << endl;
                    //out << "Stored " << endl;
                }
                else
                {
                    cout << "Could not store!" << endl;
                    //out << "could not store!" << endl;
                }

                cpuCyclesUsed += 1;
                programCounter += 3;
            }
            else if(opCode == 4) // load
            {
                int address = mainMemory[database + (programCounter - instructionBase) + 1];
                
                if(address < mainMemory[baseAddress + 5])
                {
                    mainMemory[baseAddress + 7] = mainMemory[database + address];
                    cout << "Loaded" << endl;
                    //out << "Loaded" << endl;
                }
                else
                {
                    cout << "Load error!" << endl;
                    //out << "Load error!" << endl;
                }

                cpuCyclesUsed += 1;
                programCounter += 2;
            }
            else
            {
                cout << "Invalid Instruction!" << endl;
                //out << "Invalid Instruction!" << endl;
                break;
            }

            mainMemory[baseAddress + 2] = programCounter;

        }// END < WHILE

        // check the processes as ran
        mainMemory[baseAddress + 1] = 3;
        mainMemory[database + 2] = programCounter;
        mainMemory[database + 6] = cpuCyclesUsed;

        // Print the PCB final state
        cout << "Process ID: " << processID << endl;
        cout << "State: TERMINATED" << endl;
        cout << "Program counter: " << programCounter << endl;
        cout << "Total CPU cycles: " << cpuCyclesUsed << endl;

        /*
        out << "Process ID: " << processID << endl;
        out << "State: TERMINATED" << endl;
        out << "Program counter: " << programCounter << endl;
        out << "Total CPU cycles: " << cpuCyclesUsed << endl;
        */
        
        readyQueue.pop();
    }
}

// ---------------------------------------------------------------------
// main
// compile: g++ -o debug debug.cpp
// run: ./debug < input1.txt
// ---------------------------------------------------------------------
int main()
{
    // read the input via cin
    int maxMemory, numProcesses;
    vector<int> mainMemory;
    queue<int> readyQueue;
    queue<PCB> newJobQueue;


    cin >> maxMemory;
    cin >> numProcesses;
// this is read twice once here and once in load jobs
    // initialize the vector as was suggested in TA email
    mainMemory.resize(maxMemory, -1);

    for(int i = 0; i < numProcesses; i++)
    {
        PCB process;
        cin >> process.processID >> process.maxMemoryNeeded;
        process.state = 0; // this is encoded to NEW
        process.programCounter = 0;
        process.memoryLimit = process.maxMemoryNeeded;
        process.cpuCyclesUsed = 0;
        process.registerValue = 0;
        newJobQueue.push(process);
        cout << "processid: " << process.processID << " maxmmem: " << process.maxMemoryNeeded << endl;
    }

    // Load the jobs into memory
    loadJobsToMemory(newJobQueue, readyQueue, mainMemory, maxMemory);

    // print memory table
    //printMemoryTable(mainMemory, maxMemory);

    cout << "BACK IN MAIN!" << endl;

    // execute the CPU
    executeCPU(newJobQueue, readyQueue, mainMemory, maxMemory);

    cout << "BACK IN MAIN AGAIN!" << endl;

    //out.close();
    return 0;
}