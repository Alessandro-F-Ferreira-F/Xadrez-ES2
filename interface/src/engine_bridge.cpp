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
#ifdef _WIN32
    : win_stdin_write(nullptr),
      win_stdout_read(nullptr),
      win_process_handle(nullptr),
      win_thread_handle(nullptr)
#else
    : pipe_engine{-1, -1},
      pipe_interface{-1, -1},
      engine_pid{-1}
#endif
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
        if (win_process_handle != nullptr) {
            stop();
        }

        SECURITY_ATTRIBUTES sa{};
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = nullptr;

        HANDLE stdinRead = nullptr;
        HANDLE stdinWrite = nullptr;
        if (!CreatePipe(&stdinRead, &stdinWrite, &sa, 0)) {
            return false;
        }

        HANDLE stdoutRead = nullptr;
        HANDLE stdoutWrite = nullptr;
        if (!CreatePipe(&stdoutRead, &stdoutWrite, &sa, 0)) {
            CloseHandle(stdinRead);
            CloseHandle(stdinWrite);
            return false;
        }

        // As pontas que ficam com a interface não devem ser herdadas pelo processo filho
        if (!SetHandleInformation(stdinWrite, HANDLE_FLAG_INHERIT, 0) ||
            !SetHandleInformation(stdoutRead, HANDLE_FLAG_INHERIT, 0)) {
            CloseHandle(stdinRead);
            CloseHandle(stdinWrite);
            CloseHandle(stdoutRead);
            CloseHandle(stdoutWrite);
            return false;
        }

        STARTUPINFOA si{};
        si.cb = sizeof(STARTUPINFOA);
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdInput = stdinRead;
        si.hStdOutput = stdoutWrite;
        si.hStdError = GetStdHandle(STD_ERROR_HANDLE);

        PROCESS_INFORMATION pi{};
        std::string cmdLine = "\"" + engineFilePath + "\"";

        BOOL created = CreateProcessA(
            nullptr,
            &cmdLine[0],
            nullptr,
            nullptr,
            TRUE,
            CREATE_NO_WINDOW,
            nullptr,
            nullptr,
            &si,
            &pi
        );

        // O processo pai fecha suas cópias das pontas usadas pelo filho para garantir EOF correto
        CloseHandle(stdinRead);
        CloseHandle(stdoutWrite);

        if (!created) {
            CloseHandle(stdinWrite);
            CloseHandle(stdoutRead);
            return false;
        }

        win_stdin_write = stdinWrite;
        win_stdout_read = stdoutRead;
        win_process_handle = pi.hProcess;
        win_thread_handle = pi.hThread;
        read_buffer.clear();
        return true;
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
        if (win_stdin_write == nullptr) {
            return false;
        }

        size_t totalWritten = 0;
        while (totalWritten < cmd.size()) {
            DWORD bytesWritten = 0;
            BOOL ok = WriteFile(
                static_cast<HANDLE>(win_stdin_write),
                cmd.data() + totalWritten,
                static_cast<DWORD>(cmd.size() - totalWritten),
                &bytesWritten,
                nullptr
            );
            if (!ok || bytesWritten == 0) {
                return false;
            }
            totalWritten += static_cast<size_t>(bytesWritten);
        }
        return true;
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
        if (win_stdout_read == nullptr) {
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
            DWORD bytesRead = 0;
            BOOL ok = ReadFile(
                static_cast<HANDLE>(win_stdout_read),
                buf,
                sizeof(buf),
                &bytesRead,
                nullptr
            );

            if (ok && bytesRead > 0) {
                read_buffer.append(buf, static_cast<size_t>(bytesRead));
            } else {
                std::string line = read_buffer;
                read_buffer.clear();
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }
                return line;
            }
        }
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
        if (win_process_handle != nullptr) {
            HANDLE proc = static_cast<HANDLE>(win_process_handle);
            DWORD waitRes = WaitForSingleObject(proc, 0);

            if (waitRes == WAIT_TIMEOUT && win_stdin_write != nullptr) {
                sendCommand("quit");
            }
            if (win_stdin_write != nullptr) {
                CloseHandle(static_cast<HANDLE>(win_stdin_write));
                win_stdin_write = nullptr;
            }
            if (win_stdout_read != nullptr) {
                CloseHandle(static_cast<HANDLE>(win_stdout_read));
                win_stdout_read = nullptr;
            }
            if (waitRes == WAIT_TIMEOUT) {
                if (WaitForSingleObject(proc, 2000) == WAIT_TIMEOUT) {
                    TerminateProcess(proc, 1);
                    WaitForSingleObject(proc, 1000);
                }
            }
            CloseHandle(proc);
            win_process_handle = nullptr;
        }
        if (win_thread_handle != nullptr) {
            CloseHandle(static_cast<HANDLE>(win_thread_handle));
            win_thread_handle = nullptr;
        }
        if (win_stdin_write != nullptr) {
            CloseHandle(static_cast<HANDLE>(win_stdin_write));
            win_stdin_write = nullptr;
        }
        if (win_stdout_read != nullptr) {
            CloseHandle(static_cast<HANDLE>(win_stdout_read));
            win_stdout_read = nullptr;
        }
        read_buffer.clear();
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

