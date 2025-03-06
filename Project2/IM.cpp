// PROJECT 2 WORKING VERSION
#include <iostream>
#include <queue>
#include <string>
#include <array>
#include <sstream>
#include <unordered_map>
#include <vector>
using namespace std;

struct PCB
{
    int processID;
    int state; // 1 is NEW
    int programCounter; // Removed default initialization
    int instructionBase;
    int dataBase;
    int memoryLimit;
    int cpuCyclesUsed; // Removed default initialization
    int registerValue; // Removed default initialization
    int maxMemoryNeeded;
    int mainMemoryBase;
    vector<int> processInstructions;
    
    // Constructor to initialize values
    PCB() : programCounter(0), cpuCyclesUsed(0), registerValue(0) {}
};

//global variables to be accessed from anywhere in the program
int cpuAllocated;
int globalClock = 0;
int contextSwitchTime;
int IOWaitTime = 0;
bool timeOutInterrupt = false;
bool IOInterrupt = false;

// Key: processID, Value: value of the global clock the first time the process entered runnning state
unordered_map<int, int> processStartTimes;


void printPCBFromMainMemory(int startAddress, int *mainMemory)
{
    // code block to decode the integer representing state
    string state;
    int stateInt = mainMemory[startAddress + 1];
    if (stateInt == 1)
    {
        state = "NEW";
    }
    else if (stateInt == 2)
    {
        state = "READY";
    }
    else if (stateInt == 3)
    {
        state = "RUNNING";
    }
    else
    {
        state = "TERMINATED";
    }

    // cout all of the necessary fields of the PCB struct
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


void loadJobsToMemory(queue<PCB> &newJobQueue, queue<int> &readyQueue, int *mainMemory, int maxMemory)
{
    int memoryIndex = 0;

    // TODO: Implement loading jobs into main memory
    int originalJobQueueSize = newJobQueue.size(); // used to store newJobQueue size before popping off elements

    for (int i = 0; i < originalJobQueueSize; i++)
    {
        PCB pcb = newJobQueue.front();
        newJobQueue.pop();

        // load all PCB contents to mainMemory
        int startingMemory = memoryIndex;
        mainMemory[memoryIndex++] = pcb.processID;                                        // store processID
        mainMemory[memoryIndex++] = 1;                                                    // store state, 2 is "READY"
        mainMemory[memoryIndex++] = pcb.programCounter;                                   // store programCounter
        mainMemory[memoryIndex++] = startingMemory + 10;                                  // store instructionBase
        mainMemory[memoryIndex++] = startingMemory + 10 + pcb.processInstructions.size(); // store dataBase
        mainMemory[memoryIndex++] = pcb.memoryLimit;                                      // store memoryLimit
        mainMemory[memoryIndex++] = pcb.cpuCyclesUsed;                                    // store cpuCyclesUsed
        mainMemory[memoryIndex++] = pcb.registerValue;                                    // store registerValue
        mainMemory[memoryIndex++] = pcb.maxMemoryNeeded;                                  // store maxMemoryNeeded
        mainMemory[memoryIndex++] = startingMemory;                                       // the first address correlating to this PCB in memory

        queue<int> dataQueue;
        // load instructions to mainMemory
        int numberOfInstructions = 0;
        for (int i = 0; i < pcb.processInstructions.size(); i++)
        {
            mainMemory[memoryIndex++] = pcb.processInstructions[i];
            if(pcb.processInstructions[i] == 1){
                //two values to store for a COMPUTE operation
                dataQueue.push(pcb.processInstructions[++i]);
                dataQueue.push(pcb.processInstructions[++i]);
            }
            else if(pcb.processInstructions[i] == 2){

                //one value to store for a PRINT operation
                dataQueue.push(pcb.processInstructions[++i]);
            }
            else if(pcb.processInstructions[i] == 3){
                //two values to store for a STORE operation
                dataQueue.push(pcb.processInstructions[++i]);
                dataQueue.push(pcb.processInstructions[++i]);
            }
            else if(pcb.processInstructions[i] == 4){
                //one value to store for a LOAD operation
                dataQueue.push(pcb.processInstructions[++i]);
            }
            numberOfInstructions++;
            
        }
        mainMemory[startingMemory + 4] = memoryIndex;
        int originalDataQueueSize = dataQueue.size();
        for(int i = 0; i < originalDataQueueSize; i++){
            mainMemory[memoryIndex++] = dataQueue.front();
            dataQueue.pop();
        }
        for (int i = 0; i < pcb.memoryLimit - originalDataQueueSize - numberOfInstructions; i++)
        {
            memoryIndex++;
        }
        readyQueue.push(startingMemory); // store the mainMemoryBase in readyQueue to denote where different processes start
    }

    //after loading all processes into mainMemory, output all of mainMemory
    for(int i = 0; i < maxMemory; i++){
        cout << i << " : " << mainMemory[i] << "\n";
    }
}

void executeCPU(int startAddress, int *mainMemory)
{
    //variables to store all of the relevant PCB indices for readability
    int stateIndex = startAddress + 1;
    int programCounterIndex = startAddress + 2;
    int instructionIndex = mainMemory[startAddress + 3]; //index where start of instructions is stored
    int dataBaseIndex = startAddress + 4;
    int memoryLimitIndex = startAddress + 5;
    int cpuCyclesConsumedIndex = startAddress + 6; //index where cpuCyclesConsumed is stored
    int registerIndex = startAddress + 7; //index where registerValue is stored

    int metaDataIndex = mainMemory[dataBaseIndex] - 1; //decremented to be at the correct location after incrementing
    int programCounter;
    int opcodeValue;

    globalClock += contextSwitchTime; //increment globalClock by contextSwitchTime before process moves to running
    
    //checks to see if this is the first time this process has been run
    if(mainMemory[stateIndex] == 1){
        programCounter = instructionIndex; //set programCounter to the first instruction
        processStartTimes[mainMemory[startAddress]] = globalClock;
    }
    else{
        programCounter = mainMemory[programCounterIndex];

        //calculate how deep into the metaData we are with the programCounter
        int instructionsCompleted = programCounter - instructionIndex;
        for(int i = instructionIndex; i < programCounter; i++){
            int tempOpcode = mainMemory[i];
            if(tempOpcode == 1 || tempOpcode == 3){
                metaDataIndex++;
                metaDataIndex++;
            }
            else{
                metaDataIndex++;
            }        
        }
    }

    mainMemory[stateIndex] = 3; //update state to 3, representing "RUNNING"
    cout << "Process " << mainMemory[startAddress] << " has moved to Running.\n"; //output to denote state
    

    opcodeValue = mainMemory[programCounter]; // first instruction's opcode

    //variable for  dataBase for readability
    int dataBase = mainMemory[dataBaseIndex];

    int cpuCyclesConsumed = 0; //var to keep track of the total cpu cycles used


    //loop to go through each instruction sequentially
    while(programCounter < dataBase && cpuCyclesConsumed < cpuAllocated){
        if(opcodeValue == 1){ //COMPUTE
            //print to denote instruction type
            cout << "compute\n";
            
            metaDataIndex++; // iterations is the next value in mainMemory, but we don't need that right now
            metaDataIndex++; // now we are at CPU cycles consumed, which we do need
            // update the amount of cycles consumed
            globalClock += mainMemory[metaDataIndex];
            cpuCyclesConsumed += mainMemory[metaDataIndex];
            mainMemory[cpuCyclesConsumedIndex] += mainMemory[metaDataIndex];

        }
        else if(opcodeValue == 2){ //PRINT
             
            metaDataIndex++; // now we are at CPU cycles consumed

            // update the amount of cycles consumed
            cpuCyclesConsumed += mainMemory[metaDataIndex];
            mainMemory[cpuCyclesConsumedIndex] += mainMemory[metaDataIndex];
            IOWaitTime = mainMemory[metaDataIndex];
            IOInterrupt = true;
            break;
        }
        else if(opcodeValue == 3){ //STORE
            
            metaDataIndex++; // the value to be stored is at this index

            mainMemory[registerIndex] = mainMemory[metaDataIndex]; //temporarily stores the value in the register

            metaDataIndex++; // the location to store the value is at this index
            
            int locationToBeStored = mainMemory[metaDataIndex] + startAddress;
            int endOfCurrentMemory = startAddress + mainMemory[memoryLimitIndex];
            if(locationToBeStored >= endOfCurrentMemory || locationToBeStored < mainMemory[dataBaseIndex]){
                cout << "store error!\n";
            }
            else{
                //store the register value at the desired location
                mainMemory[locationToBeStored] = mainMemory[registerIndex];
                cout << "stored\n";
            }
            globalClock++;
            cpuCyclesConsumed += 1; 
            mainMemory[cpuCyclesConsumedIndex] += 1;
   
        }
        else if(opcodeValue == 4){ //LOAD
            metaDataIndex++; // location of value to be loaded into the register
            int endOfCurrentMemory = startAddress + mainMemory[memoryLimitIndex];
            if(mainMemory[metaDataIndex] + startAddress >= endOfCurrentMemory){
                cout << "load error!\n";
            }
            else{
                //load requested value into the register
                mainMemory[registerIndex] = mainMemory[mainMemory[metaDataIndex]+ startAddress];
                cout << "loaded\n";
            }
            globalClock++;
            cpuCyclesConsumed += 1;
            mainMemory[cpuCyclesConsumedIndex] += 1;
        }

        programCounter++; //increment our programCounter
        opcodeValue = mainMemory[programCounter]; //update the opcode

    }

    
    //IO INTERRUPT
    if(IOInterrupt){
        programCounter++;
        mainMemory[programCounterIndex] = programCounter; //save program counter to access later
        return;
    }

    //TIMEOUT INTERRUPT
    if(cpuCyclesConsumed >= cpuAllocated && programCounter < dataBase){
        timeOutInterrupt = true;
        mainMemory[programCounterIndex] = programCounter; //save program counter to access later
        return;
    }

    mainMemory[programCounterIndex] = mainMemory[startAddress + 3] - 1; //reset programCounter to be one before where instructions start

    mainMemory[stateIndex] = 4; //update state to 4, representing "TERMINATED"

}

PCB loadPCB(string processDetails, int *mainMemory)
{

    // initiallizing variables
    PCB newPCB;
    vector<int> intVector;
    stringstream stream(processDetails);
    string temp;

    // loop to separate processDetails into an int Vector
    while (getline(stream, temp, ' '))
    {
        intVector.push_back(stoi(temp));
    }

    newPCB.processID = intVector.at(0);       // store processID
    newPCB.maxMemoryNeeded = intVector.at(1); // store maxMemoryNeeded
    newPCB.memoryLimit = intVector.at(1);     // store memoryLimit
    newPCB.state = 1;                         // 1 stands for NEW

    newPCB.mainMemoryBase = 0;  // initialize to 0, add in later
    newPCB.instructionBase = 0; // initialize to 0, add in later
    newPCB.dataBase = 0;        // initialize to 0, add in later

    vector<int> newVector; // stores the instructions for the PCB

    // loop to store the actual instructions in newVector
    for (int i = 3; i < intVector.size(); i++)
    {
        newVector.push_back(intVector.at(i));
    }

    newPCB.processInstructions = newVector;
    return newPCB;
}

// Fixed the template parameter syntax with parentheses
void checkIOWaitingQueue(queue<int> &readyQueue, int *mainMemory, queue<array<int, 3> > &IOWaitingQueue){
    int size = IOWaitingQueue.size();
    for(int i = 0; i < size; i++){
        array<int, 3> IOArray = IOWaitingQueue.front();
        IOWaitingQueue.pop();
        int startAddress = IOArray[0];
        int timeEntered = IOArray[1];
        int timeNeeded = IOArray[2];

        if(globalClock - timeEntered >= timeNeeded){
            readyQueue.push(startAddress);
            cout << "print\n";
            cout << "Process " << mainMemory[startAddress] << " completed I/O and is moved to the ReadyQueue.\n";
        }
        else{
            IOWaitingQueue.push(IOArray);
        }
    }
}


int main()
{
    int maxMemory;
    int numProcesses;
    queue<PCB> newJobQueue;
    queue<array<int, 3> > IOWaitingQueue; //items in queue will be of form [startAddress, timeEntered, timeNeeded]
    queue<int> readyQueue;

    // Step 1: Read and parse input file
    // TODO: Implement input parsing and populate newJobQueue
    //freopen("sampleInput.txt", "r", stdin);
    cin >> maxMemory;    // stores the max amount of memory the program will use
    cin >> cpuAllocated; //time a process can be in the CPU before it times out
    cin >> contextSwitchTime; //time it takes to switch to a new process
    cin >> numProcesses; // stores the total number of processes for the program

    int *mainMemory = new int[maxMemory]; // dynamic array to store all mainMemory

    for (int i = 0; i < maxMemory; i++)
    {
        mainMemory[i] = -1;
    }

    cin.ignore(1, '\n'); // removes a newline character before reading in the rest of the lines

    // loop that reads the file line by line and loads the PCB structs with the relevant data
    for (int i = 0; i < numProcesses; i++)
    {
        string line;
        string processID, memoryLimit, numInstructions;
        cin >> processID >> memoryLimit >> numInstructions;
        line += processID + " ";
        line += memoryLimit + " ";
        line += numInstructions + " ";
        for(int j = 0; j < stoi(numInstructions); j++){
            string currentInstruction;
            string metaData;
            cin >> currentInstruction;
            line += currentInstruction + " ";
            if(stoi(currentInstruction) == 1 || stoi(currentInstruction) == 3){ //opcodes with two metadata fields
                
                cin >> metaData;
                line+=metaData + " ";
                cin >> metaData;
                line+=metaData + " ";
            }
            else{  //opcodes with one metadata field
                cin >> metaData;
                line+=metaData + " ";
            }
            cin.ignore(1, '\n');
        }
        PCB newPCB = loadPCB(line, mainMemory);
        newJobQueue.push(newPCB);
    }

    // Step 2: Load jobs into main memory
    loadJobsToMemory(newJobQueue, readyQueue, mainMemory, maxMemory);
    // Step 3: After you load the jobs in the queue go over the main memory
    // and print the content of mainMemory. It will be in the table format
    // three columns as I had provided you earlier.

    // Step 4: Process execution
    //globalClock += contextSwitchTime; //increment by contextSwitchTime to simulate coming in from a previous process
    while (!readyQueue.empty() || !IOWaitingQueue.empty())
    {
        if(!readyQueue.empty()){
            int startAddress = readyQueue.front();
            // readyQueue contains start addresses w.r.t main memory for jobs
            readyQueue.pop();
            // Execute job
            executeCPU(startAddress, mainMemory);

            if(timeOutInterrupt){
                cout << "Process " << mainMemory[startAddress] << " has a TimeOUT interrupt and is moved to the ReadyQueue.\n";
                readyQueue.push(startAddress);
                timeOutInterrupt = false;
                checkIOWaitingQueue(readyQueue, mainMemory, IOWaitingQueue);
                continue;
            }
            if(IOInterrupt){
                cout << "Process " << mainMemory[startAddress] << " issued an IOInterrupt and moved to the IOWaitingQueue.\n";
                array<int, 3> ioArray = {startAddress, globalClock, IOWaitTime};
                IOWaitingQueue.push(ioArray);
                IOInterrupt = false;
                checkIOWaitingQueue(readyQueue, mainMemory, IOWaitingQueue);
                continue;
            }
            // Output Job that just completed execution – see example below
            printPCBFromMainMemory(startAddress, mainMemory);
            cout << "Process " << mainMemory[startAddress] << " terminated. Entered running state at: " << processStartTimes[mainMemory[startAddress]] << ". Terminated at: " << globalClock << ". Total Execution Time: " << globalClock - processStartTimes[mainMemory[startAddress]] << ".\n";
        }
        else{
            globalClock += contextSwitchTime; //advance globalClock until wait is over
            
        }
        checkIOWaitingQueue(readyQueue, mainMemory, IOWaitingQueue);
    }
    
    globalClock += contextSwitchTime; //increment by contextSwitchTime to simulate going into a new process
    cout << "Total CPU time used: " << globalClock << ".";
    
    // Don't forget to free dynamically allocated memory
    delete[] mainMemory;
    
    return 0;
}


/* 
g++ -o IM IM.cpp
./IM  < sampleInput.txt
*/