/*
    Adam Elkhanoufi
    03/01/2023
    Multithreaded Sudoku Validator using POSIX pthreads
    C++ 11
*/

#include <iostream>
#include <fstream>
#include <pthread.h>
#include <sstream>
#include <vector>
#include <queue>
#include <cstddef>
using namespace std;

#define GRID_SIZE 9         // Size of the Sudoku grid
#define SUBGRID_SIZE 3      // Size of each subgrid (relational to the grid size)
#define NUM_THREADS 3       // Number of threads being used (more can be used, but this is the minimum)
#define BUFFER_SIZE 1024    // Size of the buffer for wrting to the output file

int sudoku[GRID_SIZE][GRID_SIZE];                                       // Grid of the inputted sudoku solution
char buffer[BUFFER_SIZE];                                               // Buffer for writing to the output file
int valid_rows, valid_cols, valid_subgrids, nextID, taskCount = 0;      // Valid rows, columns, and subgrids counters
bool isValid = true;                                                    // Flag for the validity of the entire grid
static int bufferPos = 0;                                               // Position in the buffer

// Function prototypes
bool validate_row(int);
bool validate_column(int);
bool validate_subgrid(int, int);

static pthread_mutex_t output_mtx;         // Mutex to protect the buffer from simultaneous access
static pthread_cond_t condition_flag;      // Condition flag to signal the writer thread
static bool allDone;                       // Flag to signal that all tasks have been completed

// Structure to hold the arguments for each thread
struct THREAD_ARGS {
    int startRow, startCol;
    bool result;
    string func;
    int threadID;
};

// Thread pool class
class THREAD_POOL {
    public:
        queue<THREAD_ARGS> taskQueue;     // Queue of tasks to be completed
        vector<pthread_t> threads;        // Vector of threads
        
        // Constructor to initialize the thread pool with the given number of threads
        THREAD_POOL() {
            allDone = false;
            for (int i = 0; i < NUM_THREADS; i++) {
                pthread_t thread;
                pthread_create(&thread, NULL, WORKER_THREAD, this);
                threads.push_back(thread);
            }
            pthread_mutex_init(&queueMutex, NULL);
            pthread_cond_init(&queueCondition, NULL);
        }

        // Destructor
        ~THREAD_POOL() {
            for (int i = 0; i < threads.size(); i++) {
                pthread_join(threads[i], NULL);
            }
            
            pthread_mutex_destroy(&queueMutex);
            pthread_cond_destroy(&queueCondition);
        }

        // Add a task to the queue
        void ADD_TASK(THREAD_ARGS task) {
            pthread_mutex_lock(&queueMutex);
            if (nextID > 0){
                task.threadID = nextID++ % NUM_THREADS;
            }else {
                task.threadID = nextID++;
            }
            taskQueue.push(task);
            pthread_cond_signal(&queueCondition);
            pthread_mutex_unlock(&queueMutex);
        }

        // Wait for all tasks to be completed
        void WAIT_FOR_TASKS() {
            pthread_mutex_lock(&queueMutex);
            while (!taskQueue.empty() || !allDone) {
                pthread_cond_wait(&queueCondition, &queueMutex);
            }
            pthread_mutex_unlock(&queueMutex);
            pthread_cond_broadcast(&queueCondition);
        }

        // Function to write to the output buffer
        void WriteToBuffer(const string& threadOutput) {
            pthread_mutex_lock(&output_mtx);
            int len = threadOutput.length();

            // Check if the buffer has enough space to accommodate the output
            if (bufferPos + len > BUFFER_SIZE) {
                cerr << "Output buffer is full!" << endl;
                exit(-1);
            }

            // Check for null bytes in the output string
            if (threadOutput.find('\0') != string::npos) {
                string outputCopy = threadOutput;
                size_t found = outputCopy.find_last_of('\0');
                outputCopy.erase(remove(outputCopy.begin(), outputCopy.end(), '\0'), outputCopy.end());
                const_cast<string&>(threadOutput) = outputCopy;
            }

            // Copy the output to the buffer
            copy(threadOutput.begin(), threadOutput.end(), buffer + bufferPos);
            bufferPos += len;

            // If the buffer is full, wakeup a thread to write it to the output file
            if (bufferPos == BUFFER_SIZE) {
                pthread_cond_signal(&condition_flag);
            }
            pthread_mutex_unlock(&output_mtx);
        }

        // Function to write the buffer to the output file
        static void *WriteToFile(void *arg) {
            ofstream *outputFile = static_cast<ofstream*>(arg);

            
            pthread_mutex_lock(&output_mtx);

            // Wait for the buffer to be full or for all threads to be done
            while (!allDone){
                pthread_cond_wait(&condition_flag, &output_mtx);
            }

            // Write the buffer to the output file
            (*outputFile).write(buffer, bufferPos);

            // Reset the buffer position
            bufferPos = 0;

            pthread_mutex_unlock(&output_mtx);
           

            return nullptr;
        }

    private:
        pthread_mutex_t queueMutex;
        pthread_cond_t queueCondition;

        // Function assigning tasks to threads using Queues
        static void *WORKER_THREAD(void* arg) {
            THREAD_POOL *pool = (THREAD_POOL*)arg;
            while (true) {
                // Check if all tasks have been completed
                pthread_mutex_lock(&pool->queueMutex);
                if (taskCount >= GRID_SIZE * 3) {
                    allDone = true;
                    pthread_mutex_unlock(&pool->queueMutex);
                    break;
                }
                pthread_mutex_unlock(&pool->queueMutex);
                
                // Wait for a task to become available
                THREAD_ARGS task;
                pthread_mutex_lock(&pool->queueMutex);
                if (!pool->taskQueue.empty()){
                    task = pool->taskQueue.front();
                    pool->taskQueue.pop();
                    
                    // Execute the task depending on the function name
                    if (task.func == "validate_row"){
                        task.result = validate_row(task.startRow);
                        pthread_mutex_unlock(&pool->queueMutex);
                        pthread_mutex_lock(&output_mtx);
                        taskCount++;
                        string output = "[Thread " + to_string(task.threadID+1) + "] Row " + to_string(task.startRow + 1) + ": " + (task.result ? "Valid" : "Invalid") + "\n";
                        pool->WriteToBuffer(output);
                        pthread_mutex_unlock(&output_mtx);
                    }else if (task.func == "validate_column"){
                        task.result = validate_column(task.startCol);
                        pthread_mutex_unlock(&pool->queueMutex);
                        pthread_mutex_lock(&output_mtx);
                        taskCount++;
                        string output = "[Thread " + to_string(task.threadID+1) + "] Column " + to_string(task.startCol + 1) + ": " + (task.result ? "Valid" : "Invalid") + "\n";
                        pool->WriteToBuffer(output);
                        pthread_mutex_unlock(&output_mtx);
                    }else if (task.func == "validate_subgrid") {
                        task.result = validate_subgrid(task.startRow, task.startCol);
                        pthread_mutex_unlock(&pool->queueMutex);
                        pthread_mutex_lock(&output_mtx);
                        taskCount++;
                        string output = 
                            "[Thread " + to_string(task.threadID+1) + "] Subgrid R" + to_string(task.startRow + 1) + to_string(task.startRow + SUBGRID_SIZE)
                            + "C" + to_string(task.startCol + 1) + to_string(task.startCol + SUBGRID_SIZE) + ": " + (task.result ? "Valid" : "Invalid") + "\n";
                        pool->WriteToBuffer(output); 
                        pthread_mutex_unlock(&output_mtx);      
                    }
                }else if (allDone){
                    pthread_mutex_unlock(&pool->queueMutex);
                    break;
                }else{
                    pthread_mutex_unlock(&pool->queueMutex);
                }
                pthread_cond_signal(&pool->queueCondition);
            }
            return NULL;
        }
};

 // Function to validate a subgrid
bool validate_subgrid(int startRow, int startCol) {
    bool usedVal[GRID_SIZE] = { false };
    for (int i = startRow; i < startRow + SUBGRID_SIZE; i++) {
        for (int j = startCol; j < startCol + SUBGRID_SIZE; j++) {
            int val = sudoku[i][j];
            if (val < 1 || val > GRID_SIZE || usedVal[val - 1]) {
                isValid = false;
                return false;
            }
            usedVal[val - 1] = true;
        }
    }
    valid_subgrids++;
    return true;
}

// Function to validate a row
bool validate_row(int row) {
    bool usedVals[GRID_SIZE] = {false};
    for (int j = 0; j < GRID_SIZE; j++) {
        int val = sudoku[row][j];
        if (val < 1 || val > GRID_SIZE || usedVals[val-1]) {
            isValid = false;
            return false;
        }
        usedVals[val-1] = true;
    }
    valid_rows++;
    return true;
}

// Function to validate a column
bool validate_column(int col) {
    bool usedVal[GRID_SIZE] = { false };
    for (int i = 0; i < GRID_SIZE; i++) {
        int val = sudoku[i][col];
        if (val < 1 || val > GRID_SIZE || usedVal[val - 1]) {
            
            isValid = false;
            
            return false;
        }
        usedVal[val - 1] = true;
    }
    valid_cols++;
    return true;
}

// Function to validate the entire Sudoku grid
void validate_sudoku(THREAD_POOL &pool) {
    // Validate rows and columns  (seperate into different for loops if you want rows/cols/subgrids outputted respecitvely)
    for (int i = 0; i < GRID_SIZE; i++) {
        THREAD_ARGS row_task = {
            .startRow = i,
            .startCol = 0,
            .func = "validate_row" 
        };
        pool.ADD_TASK(row_task);

        THREAD_ARGS col_task = {
            .startRow = 0, 
            .startCol = i, 
            .func = "validate_column"
        };
        pool.ADD_TASK(col_task);
    }

    // Validate subgrids
    for (int i = 0; i < GRID_SIZE; i += SUBGRID_SIZE) {
        for (int j = 0; j < GRID_SIZE; j += SUBGRID_SIZE) {
            THREAD_ARGS task = {
                .startRow = i,
                .startCol = j,
                .func = "validate_subgrid"
            };
            pool.ADD_TASK(task);
        }
    }
    pool.WAIT_FOR_TASKS();
}

int main(int argc, char *argv[]) {
    // Validate arguments
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <input_file> <output_file>" << endl;
        return -1;
    }

    // Open input and output files
    ifstream inputFile(argv[1]);
    if (!inputFile) {
        cerr << "Failed to open sudoku solution file: " << argv[1] << endl;
        return -1;
    }
    ofstream outputFile(argv[2], ios::app);
    if (!outputFile) {
         cerr << "Failed to open results file: " << argv[2] << endl;
         return -1;
    }

    // Read Sudoku grid from input file
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (!(inputFile >> sudoku[i][j])) {
                cerr << "Failed to read input file: " << argv[1] << endl;
                return -1;
            }
        }
    }
    inputFile.close();
    cout << "Validating Sudoku grid...\n";

    // Create an instance of the thread pool
    THREAD_POOL pool;

    // create thread to write to output file
    pthread_t writerThread;
    pthread_create(&writerThread, NULL, pool.WriteToFile, static_cast<void *>(&outputFile));

    // Validate the Sudoku grid and print to output file
    validate_sudoku(pool);

    // Wait for writer thread to finish
    pthread_join(writerThread, NULL);

    // Add Final Summary
    outputFile << "Valid rows: " << valid_rows << endl;
    outputFile << "Valid columns: " << valid_cols << endl;
    outputFile << "Valid subgrids: " << valid_subgrids << endl;
    outputFile << "This Sudoku solution is: " << (isValid ? "Valid" : "Invalid") << endl;
    outputFile.close();

    cout << "Sudoku Validation Results written to " << argv[2] << endl;
    return 0;
}