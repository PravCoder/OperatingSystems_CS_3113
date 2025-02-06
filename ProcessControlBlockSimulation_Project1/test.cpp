#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;

// Simulated process structure
struct SimulatedProcess {
    int pid;            // Process ID
    int ppid;           // Parent Process ID
    int value;          // Integer value
    int pc;             // Program counter
    vector<string> program;  // Instruction program
    bool blocked;       // Blocked state
    bool terminated;    // Termination state
};

// Process Manager class
class ProcessManager {
private:
    vector<SimulatedProcess> processes;  // Array of simulated processes
    queue<int> readyQueue;               // Queue of ready processes
    queue<int> blockedQueue;             // Queue of blocked processes
    int runningProcessIdx;               // Index of currently running process

public:
    ProcessManager() : runningProcessIdx(-1) {}

    // Load program from file into a simulated process
    void loadProgramFromFile(const string& filename, SimulatedProcess& process) {
        ifstream file(filename);
        if (file.is_open()) {
            string line;
            while (getline(file, line)) {
                process.program.push_back(line);
            }
            file.close();
        }
    }

    // Create the initial process
    void createInitialProcess(const string& filename) {
        SimulatedProcess initialProcess;
        initialProcess.pid = 0;
        initialProcess.ppid = -1; // No parent for the initial process
        initialProcess.value = 0;
        initialProcess.pc = 0;
        initialProcess.blocked = false;
        initialProcess.terminated = false;
        loadProgramFromFile(filename, initialProcess);
        processes.push_back(initialProcess);
        readyQueue.push(initialProcess.pid);
    }

    // Execute next instruction of a process
    void executeNextInstruction(SimulatedProcess& process) {
        if (process.pc < process.program.size()) {
            string instruction = process.program[process.pc];
            istringstream iss(instruction);
            string opcode;
            iss >> opcode;
            
            if (opcode == "S") {
                int value;
                iss >> value;
                process.value = value;
            } else if (opcode == "A") {
                int value;
                iss >> value;
                process.value += value;
            } else if (opcode == "D") {
                int value;
                iss >> value;
                process.value -= value;
            } else if (opcode == "B") {
                process.blocked = true;
                readyQueue.pop(); // Remove from ready queue
                blockedQueue.push(process.pid); // Add to blocked queue
            } else if (opcode == "E") {
                process.terminated = true;
                readyQueue.pop(); // Remove from ready queue
            }
            
            process.pc++; // Move to next instruction
        }
    }

    // Schedule the next process to run
    void schedule() {
        if (runningProcessIdx != -1) {
            SimulatedProcess& runningProcess = processes[runningProcessIdx];
            executeNextInstruction(runningProcess);

            // Check if process should be unblocked
            if (runningProcess.blocked) {
                runningProcess.blocked = false;
                blockedQueue.pop(); // Remove from blocked queue
                readyQueue.push(runningProcess.pid); // Add back to ready queue
            }

            // Check if process should be terminated
            if (runningProcess.terminated) {
                runningProcessIdx = -1; // No process running
            }
        }

        if (runningProcessIdx == -1 && !readyQueue.empty()) {
            // Get the next process from the ready queue to run
            runningProcessIdx = readyQueue.front();
            readyQueue.pop();
        }
    }

    // Print current system state
    void printState() const {
        cout << "****************************************************************" << endl;
        cout << "The current system state is as follows:" << endl;
        cout << "****************************************************************" << endl;
        cout << "CURRENT TIME: <time>" << endl;

        // Print running process (if any)
        if (runningProcessIdx != -1) {
            const SimulatedProcess& runningProcess = processes[runningProcessIdx];
            cout << "RUNNING PROCESS:" << endl;
            cout << "pid: " << runningProcess.pid << ", ppid: " << runningProcess.ppid
                 << ", value: " << runningProcess.value << ", pc: " << runningProcess.pc
                 << ", blocked: " << (runningProcess.blocked ? "Yes" : "No")
                 << ", terminated: " << (runningProcess.terminated ? "Yes" : "No") << endl;
        }

        // Print blocked processes
        if (!blockedQueue.empty()) {
            cout << "BLOCKED PROCESSES:" << endl;
            queue<int> blockedCopy = blockedQueue;
            while (!blockedCopy.empty()) {
                int idx = blockedCopy.front();
                const SimulatedProcess& blockedProcess = processes[idx];
                cout << "pid: " << blockedProcess.pid << ", ppid: " << blockedProcess.ppid
                     << ", value: " << blockedProcess.value << ", pc: " << blockedProcess.pc
                     << ", blocked: " << (blockedProcess.blocked ? "Yes" : "No")
                     << ", terminated: " << (blockedProcess.terminated ? "Yes" : "No") << endl;
                blockedCopy.pop();
            }
        }

        // Print ready processes (grouped by priority)
        cout << "PROCESSES READY TO EXECUTE:" << endl;
        // Implement priority-based queue processing (not shown in this simplified example)
        cout << "****************************************************************" << endl;
    }
};

int main() {
    ProcessManager manager;

    // Create the initial process with program loaded from file "init"
    manager.createInitialProcess("init");

    char command;
    while (true) {
        cout << "Enter command (P to print state, Q to quit): ";
        cin >> command;

        // Convert command to uppercase for case-insensitive comparison
        command = toupper(command);

        if (command == 'P') {
            manager.printState();
        } else if (command == 'Q') {
            break;  // Exit the loop and terminate the program
        } else {
            cout << "Invalid command. Please try again." << endl;
        }

        // Simulate process scheduling after each command (not shown in this simplified example)
        // manager.schedule();
    }

    return 0;
}
// g++ -o test test.cpp
// ./test < input1.txt