#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

typedef enum states {
    RESTARTING  = 1U << 0,
    READY       = 1U << 1,
    SAFE_MODE   = 1U << 2,
    BBQ_MODE    = 1U << 3
} states_t;

typedef enum commands { /* Command Structure 0bxxxxyyyy, xxxx - cmd id, yyyy - invalid state flags (BBQ, SAFE, READY, RESTARTING, in order)*/
    SAFE_MODE_ENABLE    = 0b00001100,
    SAFE_MODE_DISABLE   = 0b00010010,
    NUM_CMDS_RECEIVED   = 0b00101100,
    NUM_SAFE_MODES      = 0b00111000,
    SHOW_UP_TIME        = 0b01001100,
    RESET_CMD_COUNT     = 0b01011100,
    SHUTDOWN            = 0b01100000,
    INVALID             = 0xff /* flag for a command not listed or in wrong mode */
} commands_t;

int open_command_interface(int port, int to_sec, int to_usec) {
    int sockfd;
    struct timeval timeout;
    struct sockaddr_in address;

    /* Open socket for commanding */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket creation failed");
        return -1;
    }

    /* Configure Socket Parameters */
    /* Receiving timeout */
    timeout.tv_sec = to_sec;
    timeout.tv_usec = to_usec;

    /* Addresses the */
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    /* Set Socket Options */
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) { /* Adds a recv timeout */
        perror("setting socket options failed");
        return -1;
    }

    /* Bind Socket to specific address */    
    if (bind(sockfd, (struct sockaddr*) &address, sizeof(address)) == -1) {
        perror("binding socket failed");
        return -1;
    }

    return sockfd;
}

/* Function to subtract timeval structs (from https://www.gnu.org/software/libc/manual/html_node/Calculating-Elapsed-Time.html) */
int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y) {
    int nsec = 0;

    /* Perform the carry for the later subtraction by updating y. */
    if (x->tv_usec < y->tv_usec) {
        nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
        y->tv_usec -= 1000000 * nsec;
        y->tv_sec += nsec;
    }
    if (x->tv_usec - y->tv_usec > 1000000) {
        nsec = (x->tv_usec - y->tv_usec) / 1000000;
        y->tv_usec += 1000000 * nsec;
        y->tv_sec -= nsec;
    }

    /* Compute the time remaining to wait. tv_usec is certainly positive. */
    result->tv_sec = x->tv_sec - y->tv_sec;
    result->tv_usec = x->tv_usec - y->tv_usec;

    /* Return 1 if result is negative. */
    return x->tv_sec < y->tv_sec;
}


int main(void) {
    /* State Variables */
    uint8_t should_shutdown = 0;
    uint8_t state = RESTARTING;
    uint8_t command = INVALID;
    uint8_t num_cmds_rejected = 0;
    uint16_t num_cmds_recieved = 0;
    uint16_t num_safe_modes = 0;
    struct timeval start, now, diff;

    /* Messaging Variables */
    int command_socket;
    struct sockaddr_in client_addr;
    socklen_t client_addr_size;
    uint8_t buf[64] = {};
    uint8_t buf_len = 0;

    
    printf("Flight Software Initializing!\n");

    /* Opens communication between GCS and FSW */
    command_socket = open_command_interface(8080, 0, 200000); /* Listening on UDP port 8080 */
    if (command_socket == -1) {
        perror("Failed to Create Server Socket. Exiting");
    }
    printf("Established Command Interface!\n");

    /* Find the start time of the main loop */
    printf("Flight Software Initialized!\n");
    printf("Transitioning to RESTARTING!\n");
    gettimeofday(&start, 0);

    /* Main Loop */
    while (!should_shutdown) {
        /* Perform State Actions */
        switch (state){
            case RESTARTING:
                sleep(10);
                printf("Transitioning to READY\n");
                state = READY;
                break;
            case READY:
                if (num_cmds_rejected == 5) {
                    printf("Transitioning to SAFE_MODE\n");
                    state = SAFE_MODE;
                    ++num_safe_modes;
                }
                break;
            case SAFE_MODE:
                if (num_cmds_rejected == 3) {
                    printf("Transitioning to BBQ_MODE\n");
                    state = BBQ_MODE;
                }
                break;
            case BBQ_MODE:
                break;
        }

        /* Check for commands */
        if (recvfrom(command_socket, &command, sizeof(command), 0, (struct sockaddr*) &client_addr, &client_addr_size) != -1) {
            printf("Recieved a Command: %u!\n", command);

            /* reset response buffer */
            memset(&buf, 0, buf_len);
            buf_len = 0;

            /* Check if command is being used in invalid state */
            if ((command & state) != 0) {
                printf("Command used in wrong state. command: %d, state %d\n", command, state);
                command = INVALID;
            }

            /* Perform Commanded Actions */
            switch (command) {
                case SAFE_MODE_ENABLE:
                    printf("Transitioning to SAFE_MODE\n");
                    state = SAFE_MODE;
                    num_cmds_rejected = 0;
                    ++num_safe_modes;
                    break;
                case SAFE_MODE_DISABLE:
                    printf("Transitioning to READY\n");
                    state = READY;
                    num_cmds_rejected = 0;
                    break;
                case NUM_CMDS_RECEIVED:
                    printf("Returning number of commands recieved: %d\n", num_cmds_recieved);
                    memcpy(buf + buf_len, &num_cmds_recieved, sizeof(num_cmds_recieved));
                    buf_len += sizeof(num_cmds_recieved);
                    break;
                case NUM_SAFE_MODES:
                    printf("Returning number of safe modes: %d\n", num_safe_modes);
                    memcpy(buf + buf_len, &num_safe_modes, sizeof(num_safe_modes));
                    buf_len += sizeof(num_safe_modes);
                    break;
                case SHOW_UP_TIME:
                    gettimeofday(&now, 0); /* get current time */
                    timeval_subtract(&diff, &now, &start);
                    printf("Returning number of seconds since starting: %ld\n", diff.tv_sec);
                    memcpy(buf + buf_len, &diff.tv_sec, sizeof(diff.tv_sec));
                    buf_len += sizeof(diff.tv_sec);
                    break;
                case RESET_CMD_COUNT:
                    printf("Resetting Command count\n");
                    num_cmds_recieved = 0;
                    break;
                case SHUTDOWN:
                    printf("Shutting Down\n");
                    should_shutdown = 1;
                    memset(buf + buf_len, 0xff, 1); /* Sends 1 0xff byte to signal shutting down */
                    ++buf_len;
                    break;
                default: /* catches improper commands*/
                    command = INVALID;
                    break;
            }

            /* Increment correct counters */
            if (command == INVALID) {
                printf("Recieved Invalid Command\n");
                ++num_cmds_rejected;
            } else {
                ++num_cmds_recieved;
            }

            /* Add state info to the response message buffer */
            memcpy(buf + buf_len, &state, sizeof(state));
            buf_len += sizeof(state);

            /* Send response to client */
            sendto(command_socket, buf, buf_len, 0, (struct sockaddr*) &client_addr, client_addr_size);
        }
    }
    return 0;
}