#pragma once

#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

class EngineBridge{

    private:
#ifdef _WIN32
        HANDLE engine_write;
        HANDLE engine_read;
        HANDLE process;
        HANDLE process_thread;
        std::string read_buffer;
#else
        int pipe_engine[2];
        int pipe_interface[2];
        int engine_pid;
        std::string read_buffer;
#endif

    public:
        EngineBridge();
        ~EngineBridge();


        bool start (const std::string &engineFilePath); //Cria o processo da engine de acordo com o caminho passado

        bool sendCommand(const std::string &command);

        std::string readCommand();

        bool stop();
        
};