import serial

PORT = "COM6"
BAUDRATE = 115200

ser = serial.Serial(PORT, BAUDRATE, timeout=1)

try:
    while True:
        cmd = input("cmd (2/4/5/6/8, q=quit): ").strip()

        if cmd == "q":
            break

        if len(cmd) != 1:
            print("한 글자만 입력하십시오.")
            continue

        ser.write(cmd.encode("ascii"))
        ser.flush()
        print(f"sent: {cmd}")

finally:
    ser.close()
