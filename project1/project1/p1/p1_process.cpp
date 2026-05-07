#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <cstdio>
#include <cmath>
#include <unistd.h>
#include <sys/wait.h>

#include "p1_process.h"
#include "p1_threads.h"

using namespace std;

// This file implements the multi-processing logic for the project




// This function should be called in each child process right after forking
// The input vector should be a subset of the original files vector
void process_classes(vector<string> classes, int num_threads) {
  printf("Child process is created. (pid: %d)\n", getpid());
  // Each process should use the sort function which you have defined  		
  // in the p1_threads.cpp for multithread sorting of the data. 

  for (int i = 0; i < classes.size(); i++) {
    // get all the input/output file names here
    string class_name = classes[i];
    char buffer[40];
    snprintf(buffer, sizeof(buffer), "input/%s.csv", class_name.c_str());
    string input_file_name(buffer);

    snprintf(buffer, sizeof(buffer), "output/%s_sorted.csv", class_name.c_str());
    string output_sorted_file_name(buffer);

    snprintf(buffer, sizeof(buffer), "output/%s_stats.csv", class_name.c_str());
    string output_stats_file_name(buffer);

    vector<student> students;
    // Your implementation goes here, you will need to implement:
    // File I/O
    //  - This means reading the input file, and creating a list of students,
    //  see p1_process.h for the definition of the student struct
    //
    //  - Also, once the sorting is done and the statistics are generated, this means
    //  creating the appropritate output files
    //
    // Multithreaded Sorting
    //  - See p1_thread.cpp and p1_thread.h
    //
    //  - The code to run the sorter has already been provided
    //
    // Generating Statistics
    //  - This can be done after sorting or during

    // Run multi threaded sort
    ParallelMergeSorter sorter(students, num_threads);
    vector<student> sorted = sorter.run_sort();
  }

  // child process done, exit the program
  printf("Child process is terminated. (pid: %d)\n", getpid());
  exit(0);
}


void create_processes_and_sort(vector<string> class_names, int num_processes, int num_threads) {
  vector<pid_t> child_pids;
  int classes_per_process = max(class_names.size() / num_processes, 1ul);

  // This code is provided to you to test your sorter, this code does not use any child processes
  // Remove this later on
  process_classes(class_names, 1);

  // Your implementation goes here, you will need to implement:
  // Splitting up work
  //   - Split up the input vector of classes into num_processes sublists
  //
  //   - Make sure all classes are included, remember integer division rounds down
  //
  // Creating child processes
  //   - Each child process will handle one of the sublists from above via process_classes
  //   
  // Waiting for all child processes to terminate
}

