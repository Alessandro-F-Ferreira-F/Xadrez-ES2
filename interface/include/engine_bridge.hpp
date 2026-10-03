#pragma once

#include <string>


class EngineBridge{

    private:
        int pipe_engine[2];
        int pipe_interface[2];
        int engine_pid;

    public:
        EngineBridge();


        bool start (const std::string &engineFilePath); //Cria o processo da engine de acordo com o caminho passado

        bool sendCommand(const std::string &command);

        std::string readCommand();

        bool stop();
        
};