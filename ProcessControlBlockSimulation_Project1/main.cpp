// ==================================================
// Process Control Block Simulation Project 1
// What is struct. 
// ==================================================

#include <iostream>
using namespace std;
#include <vector> 
#include <string>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <queue>


struct PCB
{
    int processID;
    string state;        // "new", "ready", "run","terminated"
    int programCounter;  
    int instructionBase; 
    int dataBase;        
    int memoryLimit;     
    int cpuCyclesUsed;   
    int registerValue;   
    int maxMemoryNeeded; 
    int mainMemoryBase;  

    vector<int> instructions;
};


void loadJobsToMemory(queue<PCB>& newJobQueue, queue<int>& readyQueue,
 vector<int>& mainMemory, int maxMemory) {
 // TODO: Implement loading jobs into main memory
}


void executeCPU(int startAddress, int* mainMemory) {
 // TODO: Implement CPU instruction execution
}





/* 
Compile: g++ -o main main.cpp
Run: ./main < input1.txt
*/
int main() {
    // Step 1: Read and parse input file
    // TODO: Implement input parsing and populate newJobQueue
    // iterate number of process and extract each
    // Step 1: Read and parse input file
    int main_memory;
    int num_processes;
    cin >> main_memory;
    cin >> num_processes;
    cout << "main memory: " << main_memory << "\n";
    cout << "number of process: " << num_processes << "\n";

    cin.ignore(); // Ignore leftover newline

    for (int i = 0; i < num_processes; i++) {
        string line;
        getline(cin, line);  // read entire process line
        istringstream ss(line);

        int cur_process_id, cur_process_max_memory_needed, cur_process_num_instructions;
        ss >> cur_process_id >> cur_process_max_memory_needed >> cur_process_num_instructions;

        cout << "CUR-PROCESS: ID: " << cur_process_id << " max_mem: " << cur_process_max_memory_needed << " num_instructions: " << cur_process_num_instructions << "\n";

        vector<int> instructions;
        for (int j = 0; j < cur_process_num_instructions; j++) {
            int instruction_opcode;
            ss >> instruction_opcode;

            if (instruction_opcode == 1) { // compute
                int iterations, cycles;
                ss >> iterations >> cycles;
                cout << "instruction: " << instruction_opcode << " iter: " << iterations << " cycles: " << cycles << endl;
            } 
            if (instruction_opcode == 2) {  // print
                int cycles;
                ss >> cycles;
                cout << "instruction: " << instruction_opcode << " cycles: " << cycles << endl;
            } 
            if (instruction_opcode == 3) {   //store
                int value, address;
                ss >> value >> address;
                cout << "instruction: " << instruction_opcode << " value: " << value << " address: " << address << endl;
            }
            if (instruction_opcode == 4) {  // load
                int address;
                ss >> address;
                cout << "instruction: " << instruction_opcode << " address: " << address << endl;
            }
        }
    }

    

    
    // Step 2: Load jobs into main memory
    // loadJobsToMemory(newJobQueue, readyQueue, mainMemory, maxMemory);





    // Step 3: After you load the jobs in the queue go over the main memory
    // and print the content of mainMemory. It will be in the table format
    // three columns as I had provided you earlier.






    // // Step 4: Process execution
    // while (!readyQueue.empty()) {
    // int startAddress = readyQueue.front();
    // //readyQueue contains start addresses w.r.t main memory for jobs
    // readyQueue.pop();
    // // Execute job
    // executeCPU(startAddress, mainMemory);
    // // Output Job that just completed execution – see example below
    // }




    return 0;
}
