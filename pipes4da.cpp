#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include <iostream>
#include <string>
#include <array>
#include <algorithm>

const int rows = 3;
const int columns = 7;

bool pipeRead(int pipe, int& value)
{
    int readed = 0;
    readed = read(pipe, &value, sizeof(value));
    if (readed == -1)
    {
        perror("Error reading pipe.");
        exit(EXIT_FAILURE);
    }
    else if (readed == 0)
        return false;
    return true;
}

void pipeWrite(int pipe, int value)
{
    if (write(pipe, &value, sizeof(value)) == -1)
    {
        perror("Error writing pipe.");
        exit(EXIT_FAILURE);
    }
}

std::vector<std::array<int, 2>> createPipes(int count)
{
    std::vector<std::array<int, 2>> pipes;
    for (int i = 0; i < count; ++i)
    {
        std::array<int, 2> newPipe;
        if (pipe(newPipe.data()) == -1)
        {
            perror("Error creating pipe.");
            exit(EXIT_FAILURE);
        }
        pipes.push_back(newPipe);
    }
    return pipes;
}

void computeAverageForRow(std::array<int, columns> values, int p)
{
    int sum = 0;
    for (auto v : values)
        sum += v;

    pipeWrite(p, sum);       // Write sum to pipe
    pipeWrite(p, values.size()); // Write number of elements (size) to pipe
    close(p);  // Close the pipe after writing
}

int main(int argc, char* argv[])
{
    std::array<std::array<int, columns>, rows> values = { { {3,4,6,8,9,7,8}, {6,2,6,1,7,9,0}, {0,9,0,2,4,1,2} } };

    // Create pipes for each row
    std::vector<std::array<int, 2>> pipes = createPipes(rows);

    // parent process reads the pipes
    int read_pipes[rows]; 

    for (int i = 0; i < rows; ++i)
    {
        int p[2];
        pipe(p);
        
        if (fork() == 0)
        {
            // Process child
            close(pipes[i][0]); // Close the read end of the pipe in child process
            computeAverageForRow(values[i], pipes[i][1]); // Compute the sum and size of row and write to pipe
            exit(0); // Exit child process after completing task
        }
        else
        {
            // Parent process
            read_pipes[i] = pipes[i][0];  // Save the read end of the pipe in parent process
            close(pipes[i][1]); // Close the write end in parent
        }
    }

    // parent process reads the data from the pipes
    int sum = 0;
    int count = 0;
    for (int i = 0; i < rows; ++i)
    {
        int value;
        pipeRead(read_pipes[i], value);  // Read sum from pipe
        sum += value;

        pipeRead(read_pipes[i], value);  // Read count from pipe
        count += value;

        close(read_pipes[i]);  // Close the read end after reading data
    }

    float average = 0.0f;
    if (count > 0)
        average = (float)sum / count;

    std::cout << "Average: " << average << "\n";
    
    // Wait for all child processes to finish
    for (int i = 0; i < rows; ++i)
        wait(nullptr);  // Wait for each child process to terminate
    
    return 0;
}
