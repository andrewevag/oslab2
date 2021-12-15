#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/wait.h>



int main(int argc, char** argv){
    if(argc > 2){
        printf("Usage is ./test [<random_word>]\nif random word is given then we run it with a child doing the same things\n");
        exit(EXIT_FAILURE);
    }
    
    char buf[BUFSIZ];
    const int father = getpid();
    int cid;

    if(argc == 2){
        //make the child only if requested
  
        cid = fork();
        if(cid < 0){
            printf("fork failed\n");
            exit(EXIT_FAILURE);
        }
    }

    int batts[2], temps[2], lights[2];
    for(int i = 0 ; i < 2; i ++){
        sprintf(buf, "/dev/lunix%d-batt", i);
        batts[i] = open(buf, 0);
        if(batts[i] < 0){
            printf("failed to open battery at linux%d\n", i);
            exit(EXIT_FAILURE);
        }
        
        sprintf(buf, "/dev/lunix%d-temp", i);
        temps[i] = open(buf, 0);
        if(temps[i] < 0){
            printf("failed to open temp at linux%d\n", i);
            exit(EXIT_FAILURE);
        }
        
        sprintf(buf, "/dev/lunix%d-light", i);
        lights[i] = open(buf, 0);
        if(lights[i] < 0){
            printf("failed to open battery at linux%d\n", i);
            exit(EXIT_FAILURE);
        }
    }
    int rv, wv;
    char helperbuf[BUFSIZ];
    int logfile = open("lunix-logfile", O_CREAT | O_RDWR | O_TRUNC, S_IRWXU);
    if(logfile < 0){
        printf("opening logfile failed\n");
        exit(EXIT_FAILURE);
    }
    for(int i = 0 ; i < 10; i ++){
        for(int j = 0; j < 2; j ++){
            rv = read(batts[j], buf, sizeof(buf));
            if(rv < 0){
                printf("read from batt of lunix%d failed\n", j);
                exit(EXIT_FAILURE);
            }
            sprintf(helperbuf, "[PID = %d] lunix%d-batt %s\n", getpid(), j, buf);
            wv = write(logfile, helperbuf, rv);
            if(wv != rv){
                printf("writing value from lunix%d-batt less than requested bytes = %d\n", j, wv);
            }

            rv = read(temps[j], buf, sizeof(buf));
            if(rv < 0){
                printf("read from temp of lunix%d failed\n", j);
                exit(EXIT_FAILURE);
            }

            sprintf(helperbuf, "[PID = %d] lunix%d-temp %s\n", getpid(), j, buf);
            wv = write(logfile, helperbuf, rv);
            if(wv != rv){
                printf("writing value from lunix%d-temp less than requested bytes = %d\n", j, wv);
            }



            rv = read(lights[j], buf, sizeof(buf));
            if(rv < 0){
                printf("read from light of lunix%d failed\n", j);
                exit(EXIT_FAILURE);
            }

            sprintf(helperbuf, "[PID = %d] lunix%d-light %s\n", getpid(), j, buf);
            wv = write(logfile, helperbuf, rv);
            if(wv != rv){
                printf("writing value from lunix%d-light less than requested bytes = %d\n", j, wv);
            }

        }


        sleep(1);
    }
    if(argc == 2){
        if(cid > 0){
            //father
            int ret, status;
            ret = wait(NULL);
            if(ret < 0){
                printf("Wait failed\n");
                exit(EXIT_FAILURE);
            }
            return 0;
        }
        else return 0;
    }
    else return 0;
}