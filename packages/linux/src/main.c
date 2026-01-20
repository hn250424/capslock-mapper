#include <string.h>

#include "process.h"
// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <signal.h>
// #include <sys/types.h>
// #include <sys/stat.h>
// #include <fcntl.h>
// #include <errno.h>

// #define APP_NAME "cm"
// #define VERSION "1.0.0"
// #define PID_FILE "/tmp/cm.pid"
// #define RUNNER_PATH "./runner"  // 현재 디렉토리의 runner

// // PID 파일에서 프로세스 ID 읽기
// int read_pid() {
//     FILE *fp = fopen(PID_FILE, "r");
//     if (!fp) return -1;
    
//     int pid;
//     if (fscanf(fp, "%d", &pid) != 1) {
//         fclose(fp);
//         return -1;
//     }
    
//     fclose(fp);
//     return pid;
// }

// // 프로세스가 실행 중인지 확인
// int is_running() {
//     int pid = read_pid();
//     if (pid < 0) return 0;
    
//     // /proc/[pid] 존재 여부로 확인
//     char path[64];
//     snprintf(path, sizeof(path), "/proc/%d", pid);
    
//     return access(path, F_OK) == 0;
// }

// // PID 파일 작성
// int write_pid(int pid) {
//     FILE *fp = fopen(PID_FILE, "w");
//     if (!fp) {
//         perror("Cannot create PID file");
//         return -1;
//     }
    
//     fprintf(fp, "%d\n", pid);
//     fclose(fp);
//     return 0;
// }

// // 러너 시작 (데몬화)
// int on_runner() {
//     if (is_running()) {
//         printf("Already running.\n");
//         return 0;
//     }
    
//     // 현재 디렉토리에서 러너의 절대 경로 구하기
//     char runner_abs_path[1024];
//     if (realpath(RUNNER_PATH, runner_abs_path) == NULL) {
//         perror("Cannot find runner binary");
//         return 1;
//     }
    
//     pid_t pid = fork();
    
//     if (pid < 0) {
//         perror("Fork failed");
//         return 1;
//     }
    
//     if (pid > 0) {
//         // 부모 프로세스: PID만 저장하고 즉시 리턴
//         // 자식은 자동으로 init이 회수
//         printf("Program has started. (PID: %d)\n", pid);
//         write_pid(pid);
//         return 0;
//     }
    
//     // 자식 프로세스: 데몬화
    
//     // 1. 부모와 완전히 독립
//     if (setsid() < 0) {
//         exit(1);
//     }
    
//     // 2. Double fork로 완전한 데몬화
//     pid_t pid2 = fork();
//     if (pid2 > 0) {
//         // 첫 번째 자식 종료 (부모는 이것만 기다림)
//         exit(0);
//     }
//     if (pid2 < 0) {
//         exit(1);
//     }
    
//     // 3. 손자 프로세스 - 진짜 데몬
//     // 여기서 실제 PID를 저장해야 하지만, 부모는 이미 종료됨
//     // 그래서 write_pid를 여기서 다시 호출
//     write_pid(getpid());
    
//     // 4. 표준 입출력 닫기
//     close(STDIN_FILENO);
//     close(STDOUT_FILENO);
//     close(STDERR_FILENO);
    
//     // 5. /dev/null로 리다이렉트
//     open("/dev/null", O_RDONLY);
//     open("/dev/null", O_WRONLY);
//     open("/dev/null", O_WRONLY);
    
//     // 6. 러너 실행
//     execv(runner_abs_path, (char *[]){runner_abs_path, NULL});
    
//     // execv 실패 시
//     exit(1);
// }

// // 러너 종료
// int off_runner() {
//     int pid = read_pid();
    
//     if (pid < 0 || !is_running()) {
//         printf("Program was not running.\n");
//         return 0;
//     }
    
//     // SIGTERM 전송
//     if (kill(pid, SIGTERM) == 0) {
//         // PID 파일 삭제
//         unlink(PID_FILE);
//         printf("Program has stopped.\n");
//         return 0;
//     } else {
//         perror("Failed to stop program");
//         return 1;
//     }
// }

// // 상태 확인
// int show_status() {
//     int pid = read_pid();
//     int running = is_running();
    
//     printf("[Running status]    %s", running ? "\033[0;32mRunning" : "\033[0;31mNot running");
//     if (running && pid > 0) {
//         printf(" (PID: %d)", pid);
//     }
//     printf("\033[0m\n");
    
//     // 러너 바이너리 존재 확인
//     printf("[Runner binary]     %s\n", 
//            access(RUNNER_PATH, X_OK) == 0 ? "\033[0;32mFound\033[0m" : "\033[0;31mNot found\033[0m");
    
//     return 0;
// }

// // 버전 출력
// int show_version() {
//     printf("%s version %s\n", APP_NAME, VERSION);
//     return 0;
// }

// // 도움말 출력
// int show_help() {
//     printf("Usage:\n");
//     printf("  on                       - Start mapper process\n");
//     printf("  off                      - Terminate mapper process\n");
//     printf("  status | s               - Check mapper status\n\n");
    
//     printf("  --help    | -h           - Show this help message\n");
//     printf("  --version | -v           - Show version info\n");
    
//     return 0;
// }

// // 잘못된 명령어
// int show_help_invalid() {
//     printf("Invalid command. Use '--help' to see available options.\n");
//     return 1;
// }

int handleArgs(char* arg1, char* arg2) {
    for (int i = 0; commandWithOptions[i].command.name != NULL || commandWithOptions[i].options[0].name != NULL; i++) {
        int isArg1Cmd = commandWithOptions[i].command.name && ( (strcmp(arg1, commandWithOptions[i].command.name) == 0) || (strcmp(arg1, commandWithOptions[i].command.alias) == 0) );
        int isArg2Cmd = commandWithOptions[i].command.name && ( (strcmp(arg2, commandWithOptions[i].command.name) == 0) || (strcmp(arg2, commandWithOptions[i].command.alias) == 0) );

        if (isArg1Cmd || isArg2Cmd) {
            char* opt = isArg1Cmd ? arg2 : arg1;

            for (int j = 0; commandWithOptions[i].options[j].name != NULL; j++) {
                if (strcmp(opt, commandWithOptions[i].options[j].name) == 0 || strcmp(opt, commandWithOptions[i].options[j].alias) == 0) {
                    return commandWithOptions[i].options[j].handler();
                }
            }
        }
    }

    return show_help_invalid();
}

int handleArg(char* arg) {
    for (int i = 0; commandWithOptions[i].command.name != NULL || commandWithOptions[i].options[0].name != NULL; i++) {
        if (commandWithOptions[i].command.name != NULL) {
            if (strcmp(arg, commandWithOptions[i].command.name) == 0 || strcmp(arg, commandWithOptions[i].command.alias) == 0) {
                // Command may have a NULL handler, unlike options.
                return commandWithOptions[i].command.handler ? commandWithOptions[i].command.handler() : show_help_invalid();
            }
        } else {
            for (int j = 0; commandWithOptions[i].options[j].name != NULL; j++) {
                if (strcmp(arg, commandWithOptions[i].options[j].name) == 0 || strcmp(arg, commandWithOptions[i].options[j].alias) == 0) {
                    return commandWithOptions[i].options[j].handler();
                }
            }
        }
    }

    return show_help_invalid();
}

int main(int argc, char* argv[]) {
    if (argc == 2) return handleArg(argv[1]);
    else if (argc == 3) return handleArgs(argv[1], argv[2]);
    else return show_help_invalid();
}
