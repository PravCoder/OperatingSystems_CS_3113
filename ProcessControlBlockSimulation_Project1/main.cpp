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



int main() {
    ifstream input_file("input1.txt");
    int main_memory;
    int num_processes;
    input_file >> main_memory;
    input_file >> num_processes;
    
    input_file.ignore(); // ignore new line character after each line
    for (int i=0; i<num_processes; i++) {
        string cur_process_line;
        getline(input_file, cur_process_line); // read in each line into cur-process-line
        
        cout << "process #" << i << ": "<< cur_process_line <<"\n";
    }


    cout << "main memory: " << main_memory << "\n";
    cout << "number of process: " << num_processes << "\n";

    input_file.close();
    return 0;
}
