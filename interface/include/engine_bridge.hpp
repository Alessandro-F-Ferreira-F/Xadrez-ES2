#pragma once

#include <string>


class EngineBridge{

    private:
        int pipe_engine[2];
        int pipe_interface[2];
        int engine_pid;
        std::string read_buffer;

        // Tira uma linha completa de read_buffer (sem '\n' nem '\r'); false se ainda nao ha
        bool takeBufferedLine(std::string &out);

    public:
        // Resultado de tryReadLine
        enum class ReadStatus { Line, Pending, Closed };

        EngineBridge();
        ~EngineBridge();


        bool start (const std::string &engineFilePath); //Cria o processo da engine de acordo com o caminho passado

        bool sendCommand(const std::string &command);

        // Bloqueia ate chegar uma linha inteira
        std::string readCommand();

        // Nao bloqueia: Line (linha em out), Pending (ainda nao chegou) ou Closed (motor encerrou)
        ReadStatus tryReadLine(std::string &out);

        bool stop();

};
