#pragma once

#include <string>


class EngineBridge{

    private:
#ifdef _WIN32
        void* win_stdin_write;
        void* win_stdout_read;
        void* win_process_handle;
        void* win_thread_handle;
#else
        int pipe_engine[2];
        int pipe_interface[2];
        int engine_pid;
#endif
        std::string read_buffer;

    public:
        EngineBridge();
        ~EngineBridge();


        bool start (const std::string &engineFilePath); //Cria o processo da engine de acordo com o caminho passado

        bool sendCommand(const std::string &command);

        std::string readCommand();

        bool stop();
        
};