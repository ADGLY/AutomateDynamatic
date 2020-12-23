#include <unistd.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <libexplain/execlp.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "file.h"
#include "hdl_data.h"
#include "axi_files.h"
#include "tcl.h"





//https://stackoverflow.com/questions/6171552/popen-simultaneous-read-and-write
void piping() {
    pid_t pid = 0;
    int inpipefd[2];
    int outpipefd[2];
    char buf[1024];
    int status;

    pipe(inpipefd);
    pipe(outpipefd);
    pid = fork();
    if (pid == 0)
    {
        // Child
        dup2(outpipefd[0], STDIN_FILENO);
        dup2(inpipefd[1], STDOUT_FILENO);
        dup2(inpipefd[1], STDERR_FILENO);

        //ask kernel to deliver SIGTERM in case the parent dies
        prctl(PR_SET_PDEATHSIG, SIGTERM);

        //replace tee with your process
        execlp("vivado", "vivado", "-mode", "tcl", (char*) NULL);
        // Nothing below this line should be executed by child process. If so, 
        // it means that the execl function wasn't successfull, so lets exit:
        exit(1);
    }
    // The code below will be executed only by parent. You can write and read
    // from the child using pipefd descriptors, and you can send signals to 
    // the process using its pid by kill() function. If the child process will
    // exit unexpectedly, the parent process will obtain SIGCHLD signal that
    // can be handled (e.g. you can respawn the child process).

    //close unused pipe ends
    close(outpipefd[0]);
    close(inpipefd[1]);

    // Now, you can write to outpipefd[1] and read from inpipefd[0] :
    char msg[256];
    char script_path[MAX_NAME_LENGTH];
    char* result = getcwd(script_path, MAX_NAME_LENGTH);
    if(result == NULL) {
        fprintf(stderr, "getcwd error !\n");
        return;
    }

    sprintf(msg, "source %s/generate_project.tcl\n", script_path);
    write(outpipefd[1], msg, strlen(msg));
    const char* exit = "exit\n";
    write(outpipefd[1], exit, strlen(exit));
    read(inpipefd[0], buf, 256);
   // printf("Received answer: %s\n", buf);

    //kill(pid, SIGKILL); //send SIGKILL signal to the child process
    waitpid(pid, &status, 0);
}


//histogram_elaborated_optimized.vhd
///home/antoine/Documents/Dynamatic/HistogramInt/hdl
int main(void) {
    hdl_source_t hdl_source;
    project_t project;
    hdl_create(&hdl_source);
    parse_hdl(&hdl_source);

    create_project(&project, &hdl_source);
    generate_AXI_script(&project);
    generate_MAIN_script(&project);

    //int err = execlp("vivado", "vivado", "-mode", "tcl", NULL);
    piping();
    read_axi_files(&(project.axi_ip));
    update_files(&project);
    hdl_free(&hdl_source);
    return 0;
}