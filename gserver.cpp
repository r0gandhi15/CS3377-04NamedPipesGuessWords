#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iterator>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include "LineInfo.h"

using namespace std;

// Global constants
const int READ = 0;
const int WRITE = 1;
const int PIPE_ERROR = -1;
const int FORK_ERROR = -1;
const int CHILD_PID = 0;
const int MAX_PIPE_MESSAGE_SIZE = 100;
const int MAX_WORD_SIZE = 100;

static const char* const KNOWN_PIPE = "known_pipe";

// Read words from file into vector<string>
void getWords(vector<string>& words, char* filename) {
    ifstream file(filename);
    if (!file)
        throw domain_error(LineInfo("file open error", __FILE__, __LINE__));
    copy(istream_iterator<string>(file),
         istream_iterator<string>(),
         back_inserter(words));
}

// Server child process: plays the game
void executeChildProcess(int client_fd, vector<string>& words) {
    srand(time(0));

    string word = words[rand() % words.size()];
    string guess(word.size(), '-');

    int tries = 0;
    char buffer[MAX_PIPE_MESSAGE_SIZE] = {0};

    // send initial try count
    sprintf(buffer, "%d", tries);
    if (write(client_fd, buffer, MAX_PIPE_MESSAGE_SIZE) == PIPE_ERROR)
        throw domain_error(LineInfo("write error", __FILE__, __LINE__));

   // sleep(3); // wait for client to read

    // send the random word
    if (write(client_fd, word.c_str(), MAX_PIPE_MESSAGE_SIZE) == PIPE_ERROR)
        throw domain_error(LineInfo("write error", __FILE__, __LINE__));

    // create unique server pipe for reading client guesses
    char serverPipeName[MAX_PIPE_MESSAGE_SIZE];
    sprintf(serverPipeName, "server_pipe_%d", getpid());
    mkfifo(serverPipeName, 0666);

    // send server pipe name to client
    if (write(client_fd, serverPipeName, MAX_PIPE_MESSAGE_SIZE) == PIPE_ERROR)
        throw domain_error(LineInfo("write error", __FILE__, __LINE__));

    int server_fd = open(serverPipeName, O_RDONLY);
    if (server_fd == PIPE_ERROR)
        throw domain_error(LineInfo("open error", __FILE__, __LINE__));

    // game loop
    while (tries < 12) {
        // send current guess word to client
        if (write(client_fd, guess.c_str(), MAX_PIPE_MESSAGE_SIZE) == PIPE_ERROR)
            throw domain_error(LineInfo("write error", __FILE__, __LINE__));

        if(guess == word)
                break;
        // read letter from client
        char letter;
        if (read(server_fd, &letter, sizeof(letter)) == PIPE_ERROR)
            throw domain_error(LineInfo("read error", __FILE__, __LINE__));

        // update guess word
        for (size_t i = 0; i < word.size(); i++) {
            if (word[i] == letter)
                guess[i] = letter;
        }

        tries++;
    }

    close(server_fd);
    close(client_fd);
    unlink(serverPipeName);
    cout << "Server Child Done" << endl;
}

// Server parent process
void executeParentProcess(vector<string>& words) {
    mkfifo(KNOWN_PIPE, 0666);

    int known_fd = open(KNOWN_PIPE, O_RDONLY);
    if (known_fd == PIPE_ERROR)
        throw domain_error(LineInfo("open error", __FILE__, __LINE__));

    char clientPipeName[MAX_PIPE_MESSAGE_SIZE] = {0};
    if (read(known_fd, clientPipeName, MAX_PIPE_MESSAGE_SIZE) == PIPE_ERROR)
        throw domain_error(LineInfo("read error", __FILE__, __LINE__));

    close(known_fd);
    unlink(KNOWN_PIPE);

    int client_fd = open(clientPipeName, O_WRONLY);
    if (client_fd == PIPE_ERROR)
        throw domain_error(LineInfo("open error", __FILE__, __LINE__));

    pid_t pid = fork();
    if (pid == FORK_ERROR)
        throw domain_error(LineInfo("fork error", __FILE__, __LINE__));
    else if (pid == CHILD_PID)
        executeChildProcess(client_fd, words);
    else {
        close(client_fd);
        cout << "Server Parent Done" << endl;
    }
}

int main(int argc, char* argv[]) {
    try {
        if (argc != 2)
            throw domain_error(LineInfo("Usage: ./gserver words.txt", __FILE__, __LINE__));

        vector<string> words;
        getWords(words, argv[1]);
        executeParentProcess(words);

    } catch (exception& e) {
        cout << e.what() << endl;
        cout << endl << "Press enter to exit..." << endl;
        cin.ignore(); cin.get();
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
