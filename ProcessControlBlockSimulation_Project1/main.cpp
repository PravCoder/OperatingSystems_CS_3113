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


int main() {
    ifstream input_file("input1.txt");
    int main_memory;
    int num_processes;
    input_file >> main_memory;
    input_file >> num_processes;
    cout << "main memory: " << main_memory << "\n";
    cout << "number of process: " << num_processes << "\n";
    
    
    input_file.ignore(); // ignore new line character after each line
    for (int i=0; i<num_processes; i++) {
        string cur_process_line;
        getline(input_file, cur_process_line); // read in each line into cur-process-line
        istringstream stream(cur_process_line);

        vector<string> tokens;
        string token;


        // loop split process-str by 
        while (stream >> token) {
            tokens.push_back(token);
        }

        // get 1st, 2nd
        int process_id = std::stoi(tokens[0]);        
        int max_memory_needed = std::stoi(tokens[1]); 
        int num_instructions = std::stoi(tokens[2]);

       
        cout << "Process #" << i << ": "<< cur_process_line <<"\n";
        cout << "id: " << process_id << " max-mem: " << max_memory_needed << " num_instru: " << num_instructions <<"\n";
    }

    input_file.close();
    return 0;
}
