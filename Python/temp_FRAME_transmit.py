# PC에서 STM32로 명령 전송 (frame 단위)

import struct
import threading
import time
import math
import serial

SOF1 = 0xAA
SOF2 = 0x55

MSG_TYPE_VW = 0x02
MSG_TYPE_DROPOFF_START = 0x03
MSG_TYPE_HEARTBEAT = 0x05

CMD_V_MAX_MPS = 0.12
CMD_W_MAX_RADPS = 0.30

PORT = "COM6"
BAUD = 115200

running = True
heartbeat_enabled = True
ser = None

serial_lock = threading.Lock()

def checksum_xor(data: bytes) -> int:
    x = 0
    for b in data:
        x ^= b
    return x & 0xFF


def build_heartbeat_frame() -> bytes:
    frame = bytearray([SOF1, SOF2, MSG_TYPE_HEARTBEAT])
    frame.append(checksum_xor(frame))
    return bytes(frame)


def build_vw_frame(v_mps: float, w_radps: float) -> bytes:
    frame = bytearray([SOF1, SOF2, MSG_TYPE_VW])
    frame += struct.pack("<f", v_mps)
    frame += struct.pack("<f", w_radps)
    frame.append(checksum_xor(frame))
    return bytes(frame)


def build_dropoff_frame(target_id: int) -> bytes:
    frame = bytearray([SOF1, SOF2, MSG_TYPE_DROPOFF_START, target_id & 0xFF])
    frame.append(checksum_xor(frame))
    return bytes(frame)


def clamp_float(x: float, lo: float, hi: float) -> float:
    return max(lo, min(hi, x))


def sanitize_float(x: float, name: str) -> float:
    if not math.isfinite(x):
        raise ValueError(f"{name} must be finite")
    return x


def send_frame(frame: bytes):
    global ser
    if ser is None or not ser.is_open:
        raise RuntimeError("serial port is not open")
    ser.write(frame)
    ser.flush()


def heartbeat_loop():
    global running, heartbeat_enabled
    while running:
        try:
            if heartbeat_enabled:
                send_frame(build_heartbeat_frame())
        except Exception as e:
            print(f"[HB ERR] {e}")
        time.sleep(0.1)


def vw_mode():
    print("\n[VW mode]")
    print("입력 형식: v w")
    print("예: 0.10 0.00")
    print("추가 명령: stop / exit")

    while True:
        try:
            s = input("vw> ").strip().lower()

            if s == "exit":
                break

            if s == "stop":
                send_frame(build_vw_frame(0.0, 0.0))
                print("[TX] VW sent: v=0.000, w=0.000")
                continue

            parts = s.split()
            if len(parts) != 2:
                print("입력 형식 오류. 예: 0.10 0.00")
                continue

            v = float(parts[0])
            w = float(parts[1])

            v = sanitize_float(v, "v")
            w = sanitize_float(w, "w")

            v = clamp_float(v, -CMD_V_MAX_MPS, CMD_V_MAX_MPS)
            w = clamp_float(w, -CMD_W_MAX_RADPS, CMD_W_MAX_RADPS)

            send_frame(build_vw_frame(v, w))
            print(f"[TX] VW sent: v={v:.3f}, w={w:.3f}")

        except Exception as e:
            print(f"[ERR] {e}")


def dropoff_mode():
    print("\n[DROPOFF mode]")
    print("입력: target_id (0~255)")
    print("추가 명령: exit")

    while True:
        try:
            s = input("dropoff> ").strip().lower()

            if s == "exit":
                break

            target_id = int(s)
            if not (0 <= target_id <= 255):
                raise ValueError("target_id must be 0..255")

            send_frame(build_dropoff_frame(target_id))
            print(f"[TX] DROPOFF_START sent: target_id={target_id}")

        except Exception as e:
            print(f"[ERR] {e}")


def heartbeat_stop_mode():
    global heartbeat_enabled
    heartbeat_enabled = False
    print("\n[HEARTBEAT STOP mode]")
    print("heartbeat 전송을 중지했습니다.")
    print("failsafe timeout 확인용입니다.")
    print("엔터를 누르면 메인 메뉴로 돌아갑니다.")
    input()


def send_stop_once():
    try:
        send_frame(build_vw_frame(0.0, 0.0))
        print("[TX] stop VW sent")
    except Exception as e:
        print(f"[STOP ERR] {e}")


def main():
    global ser, running, heartbeat_enabled

    ser = serial.Serial(PORT, BAUD, timeout=0.1)
    print(f"[INFO] opened {PORT} @ {BAUD}")

    hb_thread = threading.Thread(target=heartbeat_loop, daemon=True)
    hb_thread.start()

    try:
        while True:
            print("\nSelect mode:")
            print("1: VW mode")
            print("2: Dropoff start mode")
            print("3: Heartbeat stop mode")
            print("4: Heartbeat resume")
            print("q: Quit")

            mode = input("> ").strip().lower()

            if mode == "1":
                vw_mode()
            elif mode == "2":
                dropoff_mode()
            elif mode == "3":
                heartbeat_stop_mode()
            elif mode == "4":
                heartbeat_enabled = True
                print("[INFO] heartbeat resumed")
            elif mode == "q":
                break
            else:
                print("invalid input")

    finally:
        running = False
        send_stop_once()
        time.sleep(0.15)
        if ser is not None and ser.is_open:
            ser.close()
        print("[INFO] closed")


if __name__ == "__main__":
    main()
