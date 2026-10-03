#include "engine_bridge.hpp"

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif

EngineBridge::EngineBridge()
    : pipe_engine{-1, -1},
      pipe_interface{-1, -1},
      engine_pid{-1}   
{

}

bool EngineBridge::start(const std::string &engineFilePath){

    #ifdef _WIN32
        //codigo windows
    
    #else

        pipe(pipe_engine);
        pipe(pipe_interface);

        //O pid é diferente pro pai e pro filho. O retorno é feito depois que o 2 processo é criado
        pid_t pid = fork()

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

