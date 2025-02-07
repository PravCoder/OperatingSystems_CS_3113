void executeCPU(int startAddress, vector<int>&  mainMemory) {
    // extract attributes of pcb from main memory
    int processID = mainMemory[startAddress];
    int state = mainMemory[startAddress + 1];
    int programCounter = mainMemory[startAddress + 2];
    int instructionBase = mainMemory[startAddress + 3];
    int dataBase = mainMemory[startAddress + 4];
    int memoryLimit = mainMemory[startAddress + 5];
    int cpuCyclesUsed = mainMemory[startAddress + 6];
    int registerValue = mainMemory[startAddress + 7];
    int maxMemoryNeeded = mainMemory[startAddress + 8];
    int mainMemoryBase = mainMemory[startAddress + 9];

    // Calculate number of opcodes and data segment size
    int numOpcodes = dataBase - instructionBase;
    int dataSize = memoryLimit - numOpcodes;

    // Parameter extraction index within the data segment
    int paramIdx = 0;

    // iterate through opcodes
    for (int i = 0; programCounter < numOpcodes; i++) {
        int opcode = mainMemory[instructionBase + i];
        vector<int> params;

        // Extract parameters based on opcode type
        // COMPUTE - has 2 arguments
        if (opcode == 1) {
            if (paramIdx + 1 >= dataSize) {
                params.push_back(-1);
                params.push_back(-1);
            } else {
                params.push_back(mainMemory[dataBase + paramIdx]);
                params.push_back(mainMemory[dataBase + paramIdx + 1]);
            }
            paramIdx += 2;
        }
        // STORE - has 2 arguments
        else if (opcode == 3) {
            if (paramIdx + 1 >= dataSize) {
                params.push_back(-1);
                params.push_back(-1);
            } else {
                params.push_back(mainMemory[dataBase + paramIdx]);
                params.push_back(mainMemory[dataBase + paramIdx + 1]);
            }
            paramIdx += 2;
        }
        // PRINT - has 1 argument
        else if (opcode == 2) {
            if (paramIdx >= dataSize) {
                params.push_back(-1);
            } else {
                params.push_back(mainMemory[dataBase + paramIdx]);
            }
            paramIdx += 1;
        }
        // LOAD - has 1 argument
        else if (opcode == 4) {
            if (paramIdx >= dataSize) {
                params.push_back(-1);
            } else {
                params.push_back(mainMemory[dataBase + paramIdx]);
            }
            paramIdx += 1;
        }

        // process each instruction opcode
        switch (opcode) {
            case 1: // COMPUTE
            {
                cpuCyclesUsed += params[1];
                cout << "compute" << "\n";
                break;
            }

            case 2: // PRINT
            {
                cpuCyclesUsed += params[0];
                cout << "print" << "\n";
                break;
            }

            case 3: // STORE
            {
                if (params[1] + instructionBase >= instructionBase &&
                    (params[1] + instructionBase) < maxMemoryNeeded + instructionBase) {
                    mainMemory[params[1] + instructionBase] = params[0];
                    registerValue = params[0];
                    cout << "stored" << "\n";
                } else {
                    registerValue = params[0];
                    cout << "store error!" << "\n";
                }
                cpuCyclesUsed++;
                break;
            }

            case 4: // LOAD
            {
                if ((params[0] + instructionBase) >= instructionBase &&
                    (params[0] + instructionBase) < (maxMemoryNeeded + instructionBase)) {
                    registerValue = mainMemory[params[0] + instructionBase];
                    cout << "loaded" << "\n";
                } else {
                    cout << "load error!" << "\n";
                }
                cpuCyclesUsed++;
                break;
            }

            default:
            {
                cerr << "ERROR: Invalid opcode " << opcode << "\n";
                break;
            }
        }

        programCounter++;
    }

    // Update PCB fields in mainMemory
    mainMemory[startAddress + 1] = 4;               // TERMINATED state
    mainMemory[startAddress + 2] = instructionBase - 1; // Update program counter
    mainMemory[startAddress + 6] = cpuCyclesUsed;
    mainMemory[startAddress + 7] = registerValue;

    // Print PCB information
    cout << "Process ID: " << processID << "\n";
    cout << "State: TERMINATED\n";
    cout << "Program Counter: " << mainMemory[startAddress + 2] << "\n";
    cout << "Instruction Base: " << instructionBase << "\n";
    cout << "Data Base: " << dataBase << "\n";
    cout << "Memory Limit: " << memoryLimit << "\n";
    cout << "CPU Cycles Used: " << cpuCyclesUsed << "\n";
    cout << "Register Value: " << registerValue << "\n";
    cout << "Max Memory Needed: " << maxMemoryNeeded << "\n";
    cout << "Main Memory Base: " << mainMemoryBase << "\n";
    cout << "Total CPU Cycles Consumed: " << cpuCyclesUsed << "\n";
}