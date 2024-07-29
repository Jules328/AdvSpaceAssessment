from enum import Enum
import socket
import time

# States values
class States(Enum):
    RESTARTING = 1
    READY = 2
    SAFE_MODE = 4
    BBQ_MODE = 8

# Command values
class Commands(Enum):
    SAFE_MODE_ENABLE = 0b00001100
    SAFE_MODE_DISABLE = 0b00010010
    NUM_CMDS_RECEIVED = 0b00101100
    NUM_SAFE_MODES = 0b00111000
    SHOW_UP_TIME = 0b01001100
    RESET_CMD_COUNT = 0b01011100
    SHUTDOWN = 0b01100000
    INVALID = 0xff

CMD_ACC = 0x55
CMD_NO_ACC = 0xAA

# Commands that have returned values and a description of the returned data
commands_with_return = {Commands.NUM_CMDS_RECEIVED: "Number of correct commands recieved", 
                        Commands.NUM_SAFE_MODES:    "Number of safe modes", 
                        Commands.SHOW_UP_TIME:      "Seconds since FSW start"}

# function to send a command to gcs and grab the returned data and return it (if not timed out)
def send_gcs_command(s: socket.socket, addr, command: Commands) -> bytes:
    print(f"Sending {command.name}")
    s.sendto(command.value.to_bytes(1, 'little'), addr)

    # try to receive message from FSW
    try:
        msg, _ = s.recvfrom(64)
        return msg
    except socket.timeout:
        print("Request Timed Out")
        return None

def command_fsw(s, server_addr, command):
    msg = send_gcs_command(s, server_addr, command)
    if msg is not None:
        # print(f"Recieved: {msg}")

        # pull the state from the bytes recieved
        state = States(int(msg[-1]))
        acc = msg[-2]

        if acc == CMD_ACC:
            print("Command Accepted")
        elif acc == CMD_NO_ACC:
            print("Command Rejected")

        # specially handling the Shutdown command
        if command is Commands.SHUTDOWN and acc == CMD_ACC:
            print(f"FSW Shutting Down, Last State: {state.name}")

        # handle all other commands
        else:
            # pull other data from msg, if commands returns data
            if command in commands_with_return:
                desc = commands_with_return[command]
                val = int.from_bytes(msg[:-2], 'little', signed=True)
                print(f"{desc}: {val}")
            
            # prints the returned state
            print(f"Current State: {state.name}")

# displays all available commands by index
def print_commands():
    print("This is a full list of sendable commands:")
    for idx, command in enumerate(Commands):
        print(f"\t{idx} - {command.name}")

if __name__ == '__main__':
    # prints welcome message
    print("Hello! Welcome to the Advanced Space GCS application.")
    print('Type "quit" to exit, or "help" to recieve a list of commands to send')

    # establish socket
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.settimeout(0.1) # sets a small timeout for receive calls

    # get user input of target address
    user_input = input("Input FSW IP address and port (xxx.xxx.xxx.xxx:yyy, defaults to local host): ")
    try:
        split_input = user_input.split(':')
        server_addr = (split_input[0], int(split_input[-1]))
        socket.inet_aton(server_addr[0]) # attempts to parse given address to bytes
        print(f"Using {server_addr[0]}:{server_addr[1]}")
    except: # multiple possible reasons to end up here
        print("Using localhost and port 8080")
        server_addr = ('127.0.0.1', 8080)

    # parse commands to make them indexable
    commands_list = [command for command in Commands]

    # display a list of commands
    print_commands()

    while True:
        user_input = input("Input a command send (by number): ")
        
        # exit loop
        if user_input.lower() == 'quit':
            break
        # get a list of commands to send
        elif user_input.lower() == 'help':
            print_commands()
        # tries to send selected command
        else:
            try:
                command_index = int(user_input)
                command_fsw(s, server_addr, commands_list[command_index])
            except: # catches Parse errors
                print("Invalid Input")