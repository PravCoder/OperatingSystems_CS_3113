#include <iostream>
#include <sstream>
#include <vector>
#include <string>
using namespace std;

int main() {
    int main_memory, num_processes;

    // Read main memory and number of processes
    cin >> main_memory >> num_processes;
    cin.ignore(); // Skip the newline after the second integer


    // Process each line for the number of processes
    for (int i = 0; i < num_processes; i++) {
        // read in process
        string cur_process_line;
        getline(cin, cur_process_line); 

        // split cur-process-line into tokens by space, since each int is seprated by space
        vector<string> tokens;
        stringstream ss(cur_process_line);
        string token;

        while (ss >> token) {
            tokens.push_back(token);
        }

        // 1st, 2nd, 3rd elements of process
        int process_id = stoi(tokens[0]);
        int max_memory_needed = stoi(tokens[1]);
        int num_instructions = stoi(tokens[2]);

        cout << "Main Memory: " << main_memory << endl;
        cout << "Number of Processes: " << num_processes << endl;
        cout << "Process Line: " << cur_process_line << endl;
        cout << "1)process ID: " << process_id << endl;
        cout << "2)max memory needed: " << max_memory_needed << endl;
        cout << "3)number of instructions: " << num_instructions << endl;
        
    }

    return 0;
}

// compile: g++ -o process_simulator debug.cpp
// run: ./process_simulator < input1.txt
