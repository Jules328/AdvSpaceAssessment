#include <stdint.h>
#include <stdio.h>
#include <sys/socket.h>
#include <time.h>

typedef enum states {
    RESTARTING = 0,
    READY,
    SAFE_MODE,
    BBQ_MODE
} states_t;

typedef enum commands {
    INVALID = -1,
    SAFE_MODE_ENABLE,
    SAFE_MODE_DISABLE,
    NUM_CMDS_RECEIVED,
    NUM_SAFE_MODES,
    SHOW_UP_TIME,
    RESET_CMD_COUNT,
    SHUTDOWN
} commands_t;

int main() {    
    /* Initialize state machine and FSW */
    states_t state = RESTARTING;

    /* Open socket for commanding */
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    /* Configure Socket Options */
    struct timeval loop_delay;
    loop_delay.tv_sec = 0;
    loop_delay.tv_usec = 20000;

    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &loop_delay, sizeof(loop_delay));

    /* Bind Socket to specific address */
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    bind(sockfd, (struct sockaddr*) &address, sizeof(address));

    /* Main Loop */
    while (1) {
        int in = 0;
        /* Check for commands */
        recvfrom(sockfd, &in, sizeof(in), 0, 0, 0);

        /* Perform State */
        switch (state){
            case RESTARTING:
                printf("RESTARTING\n");
                break;
            case READY:
                printf("READY\n");
                break;
            case SAFE_MODE:
                printf("SAFE_MODE\n");
                break;
            case BBQ_MODE:
                printf("BBQ_MODE\n");
                break;
        }
    }
    close(sockfd);

    return 0;
}