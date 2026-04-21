#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdlib>
#include "LineInfo.h"

using namespace std;

// Global constants
const int READ = 0;
const int WRITE = 1;
const int PIPE_ERROR = -1;
const int MAX_PIPE_MESSAGE_SIZE = 100;
static const char* const KNOWN_PIPE = "known_pipe";

void executeClientProcess() {
    // create unique client pipe
    char clientPipeName[MAX_PIPE_MESSAGE_SIZE];
    sprintf(clientPipeName, "client_pipe_%d", getpid());
    mkfifo(clientPipeName, 0666);

    // write client pipe name to known pipe
    int known_fd = open(KNOWN_PIPE, O_WRONLY);
    if (known_fd == PIPE_ERROR)
        throw domain_error(LineInfo("open error", __FILE__, __LINE__));

    if (write(known_fd, clientPipeName, MAX_PIPE_MESSAGE_SIZE) == PIPE_ERROR)
        throw domain_error(LineInfo("write error", __FILE__, __LINE__));
    close(known_fd);

    // open client pipe for reading server messages
    int client_fd = open(clientPipeName, O_RDONLY);
    if (client_fd == PIPE_ERROR)
        throw domain_error(LineInfo("open error", __FILE__, __LINE__));

    char buffer[MAX_PIPE_MESSAGE_SIZE] = {0};

    // read initial try count
    if (read(client_fd, buffer, MAX_PIPE_MESSAGE_SIZE) == PIPE_ERROR)
        throw domain_error(LineInfo("read error", __FILE__, __LINE__));

    int tries = atoi(buffer);

    // read random word (for game length)
    memset(buffer, 0, MAX_PIPE_MESSAGE_SIZE);
    if (read(client_fd, buffer, MAX_PIPE_MESSAGE_SIZE) == PIPE_ERROR)
        throw domain_error(LineInfo("read error", __FILE__, __LINE__));

    string word = buffer;

    // read server child pipe name
    memset(buffer, 0, MAX_PIPE_MESSAGE_SIZE);
    if (read(client_fd, buffer, MAX_PIPE_MESSAGE_SIZE) == PIPE_ERROR)
        throw domain_error(LineInfo("read error", __FILE__, __LINE__));

    char serverPipeName[MAX_PIPE_MESSAGE_SIZE];
    strcpy(serverPipeName, buffer);

    // open server child pipe for writing guesses
    int server_fd = open(serverPipeName, O_WRONLY);
    if (server_fd == PIPE_ERROR)
        throw domain_error(LineInfo("open error", __FILE__, __LINE__));

    string guess(word.size(), '-');

    cout << "Game Start" << endl;
    cout << "You have 12 letter guesses to win" << endl;

    // game loop
    while (true) {
      // read updated guess word from server
        memset(buffer, 0, MAX_PIPE_MESSAGE_SIZE);
        if (read(client_fd, buffer, MAX_PIPE_MESSAGE_SIZE) == PIPE_ERROR)
            throw domain_error(LineInfo("read error", __FILE__, __LINE__));

        guess = buffer;

        if (guess == word) {
            cout << word << endl;
            cout << "You Win!" << endl;
            break;
        }
        if (tries >= 12) {
            cout << "Out of tries: 12 allowed" << endl;
            cout << "The word is: " << word << endl;
            break;
        }

        cout << "Current try number: " << tries + 1 << endl;
        cout << "(Guess) Enter a letter in the word: " << guess << endl;

        char letter;
        cin >> letter;

        if(write(server_fd, &letter, sizeof(letter)) == PIPE_ERROR)
                throw domain_error(LineInfo("write error", __FILE__, __LINE__));

        tries++;
        }

    close(client_fd);
    close(server_fd);
    unlink(clientPipeName);
}

int main() {
    try {
        executeClientProcess();
    } catch (exception& e) {
        cout << e.what() << endl;
        cout << endl << "Press enter to exit..." << endl;
        cin.ignore(); cin.get();
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}
