from enum import Enum
import socket
import time

class States(Enum):
    RESTARTING = 1
    READY = 2
    SAFE_MODE = 4
    BBQ_MODE = 8

class Commands(Enum):
    SAFE_MODE_ENABLE = 0b00001100
    SAFE_MODE_DISABLE = 0b00010010
    NUM_CMDS_RECEIVED = 0b00101100
    NUM_SAFE_MODES = 0b00111000
    SHOW_UP_TIME = 0b01001100
    RESET_CMD_COUNT = 0b01011100
    SHUTDOWN = 0b01100000
    INVALID = 0xff

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

def send_gcs_command(s: socket.socket, addr, command: Commands) -> bytes:
    print(f"Sending {command.name}")
    s.sendto(command.value.to_bytes(1, 'little'), addr)
    time.sleep(0.1)

    try:
        msg, _ = s.recvfrom(64)
        return msg
    except socket.timeout:
        print("Request Timed Out")
        return None

if __name__ == '__main__':
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.settimeout(0.5)
    server_addr = ('127.0.0.1', 8080)

    time.sleep(1)
    try:
        s.recvfrom(64)
    except socket.timeout:
        pass

    commands_list = [command for command in Commands]




    for command in Commands:
        msg = send_gcs_command(s, server_addr, command)
        print(f"Recieved: {msg}")
        if msg is not None:
            state = States(int(msg[-1]))
            print(f"state: {state.name}")
            
        time.sleep(1)