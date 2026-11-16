#include "engine_bridge.hpp"
#include <cstdlib>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <cerrno>
    #include <csignal>
    #include <sys/wait.h>
    #include <unistd.h>
#endif

EngineBridge::EngineBridge()
    : pipe_engine{-1, -1},
      pipe_interface{-1, -1},
      engine_pid{-1}   
{
    #ifndef _WIN32
        // Evita que escrita em pipe fechado pelo filho mate a interface com SIGPIPE
        signal(SIGPIPE, SIG_IGN);
    #endif
}

EngineBridge::~EngineBridge() {
    stop();
}

bool EngineBridge::start(const std::string &engineFilePath){

    #ifdef _WIN32
        // TODO: Implementar criação de pipes e processo Win32
        (void)engineFilePath;
        return false;
    #else
        if (engine_pid > 0) {
            stop();
        }

        if (pipe(pipe_engine) < 0) {
            return false;
        }
        if (pipe(pipe_interface) < 0) {
            close(pipe_engine[0]);
            close(pipe_engine[1]);
            pipe_engine[0] = pipe_engine[1] = -1;
            return false;
        }

        //O pid é diferente pro pai e pro filho. O retorno é feito depois que o 2 processo é criado
        pid_t pid = fork();


        if (pid < 0){
            return false;
        }

        else if (pid == 0) {

            //Fluxo do filho(engine)

            //No momento, a entrada e saída padrão do processo filho ainda são o teclado e a tela. O pipe já está criado, mas não é o padrão. Substituimos os file_descriptor padrão pelos dos pipes criados.
            dup2(pipe_engine[0], 0);
            dup2(pipe_interface[1], 1);

            //Fecga cópia dos pipes
            close(pipe_engine[0]);
            close(pipe_engine[1]);
            close(pipe_interface[0]);
            close(pipe_interface[1]);

            //Substitui o binário da interface sendo executado pelo da engine
            execlp(engineFilePath.c_str(), engineFilePath.c_str(), NULL);

            //Só executa se o execlp der errado
            exit(1);

        }

        else{

            engine_pid = pid;

            close(pipe_engine[0]); //Interface nunca le do pipe da engine
            close(pipe_interface[1]); //Interface nunca escreve no pipe da interface


        }

        return true;

    #endif
    
}

bool EngineBridge::sendCommand(const std::string &command){

    std::string cmd = command + "\n";

    #ifdef _WIN32
        // TODO: Implementar envio via WriteFile no Windows
        (void)cmd;
        return false;
    #else
        if (pipe_engine[1] < 0) {
            return false;
        }
        ssize_t bytes_enviado = write(pipe_engine[1], cmd.c_str(), cmd.size());

        if (bytes_enviado > 0) {
            return true;
        }
        
        return false;
    #endif
}

std::string EngineBridge::readCommand() {
    #ifdef _WIN32
        // TODO: Implementar leitura via ReadFile no Windows
        return "";
    #else
        if (pipe_interface[0] < 0) {
            return "";
        }

        while (true) {
            size_t nl = read_buffer.find('\n');
            if (nl != std::string::npos) {
                std::string line = read_buffer.substr(0, nl);
                read_buffer.erase(0, nl + 1);
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }
                return line;
            }

            char buf[512];
            ssize_t n = read(pipe_interface[0], buf, sizeof(buf));
            if (n > 0) {
                read_buffer.append(buf, static_cast<size_t>(n));
            } else if (n == 0) {
                std::string line = read_buffer;
                read_buffer.clear();
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }
                return line;
            } else {
                if (errno == EINTR) {
                    continue;
                }
                return "";
            }
        }
    #endif
}

bool EngineBridge::stop() {
    #ifdef _WIN32
        // TODO: Implementar encerramento do processo no Windows
        return true;
    #else
        if (engine_pid > 0) {
            int status = 0;
            pid_t exited = waitpid(engine_pid, &status, WNOHANG);
            if (pipe_engine[1] >= 0) {
                if (exited == 0) {
                    sendCommand("quit");
                }
                close(pipe_engine[1]);
                pipe_engine[1] = -1;
            }
            if (pipe_interface[0] >= 0) {
                close(pipe_interface[0]);
                pipe_interface[0] = -1;
            }
            if (exited == 0) {
                waitpid(engine_pid, &status, 0);
            }
            engine_pid = -1;
        }
        read_buffer.clear();
        return true;
    #endif
}

