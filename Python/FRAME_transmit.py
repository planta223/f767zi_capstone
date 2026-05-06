# PC에서 STM32로 명령 전송 (frame 단위)
# ESC 키를 누르면 어느 입력 구간에서든 즉시 E-STOP frame 송신

import struct
import threading
import time
import math
import serial
import keyboard

SOF1 = 0xAA
SOF2 = 0x55

MSG_TYPE_VW = 0x02
MSG_TYPE_DROPOFF_START = 0x03
MSG_TYPE_HEARTBEAT = 0x05
MSG_TYPE_ESTOP = 0x06

CMD_V_MAX_MPS = 0.12
CMD_W_MAX_RADPS = 0.30

PORT = "COM6"
BAUD = 115200

running = True
heartbeat_enabled = True
estop_sent = False
ser = None

serial_lock = threading.Lock()
estop_lock = threading.Lock()


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


def build_estop_frame() -> bytes:
    # [0xAA][0x55][0x06][checksum]
    # checksum = 0xAA ^ 0x55 ^ 0x06 = 0xF9
    frame = bytearray([SOF1, SOF2, MSG_TYPE_ESTOP])
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

    with serial_lock:
        if ser is None or not ser.is_open:
            raise RuntimeError("serial port is not open")

        ser.write(frame)
        ser.flush()


def trigger_estop():
    """
    ESC hotkey callback.
    어느 입력 구간에 있든 호출됨.
    """
    global estop_sent, heartbeat_enabled

    with estop_lock:
        if estop_sent:
            return

        estop_sent = True

    try:
        # 선택 사항:
        # heartbeat를 계속 보내도 STM32가 E-STOP latch 상태라면 구동명령은 무시됨.
        # 다만 테스트 중 상태를 명확히 하려면 heartbeat를 끄지 않는 편이 낫다.
        # heartbeat_enabled = False

        send_frame(build_estop_frame())
        print("\n[!!! E-STOP !!!] MSG_TYPE_ESTOP(0x06) sent")
        print("[INFO] STM32 should latch E-STOP and ignore drive/dropoff commands.")
        print("[INFO] Reset STM32 or power-cycle to recover, if release frame is not implemented.")

    except Exception as e:
        print(f"\n[E-STOP ERR] {e}")


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
    print("긴급정지: ESC")

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
    print("긴급정지: ESC")

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
    print("긴급정지: ESC")
    print("엔터를 누르면 메인 메뉴로 돌아갑니다.")
    input()


def send_stop_once():
    try:
        send_frame(build_vw_frame(0.0, 0.0))
        print("[TX] stop VW sent")
    except Exception as e:
        print(f"[STOP ERR] {e}")


def send_estop_once_from_menu():
    trigger_estop()


def main():
    global ser, running, heartbeat_enabled

    ser = serial.Serial(PORT, BAUD, timeout=0.1)
    print(f"[INFO] opened {PORT} @ {BAUD}")

    # 전역 E-STOP hotkey 등록
    keyboard.add_hotkey("esc", trigger_estop)
    print("[INFO] E-STOP hotkey registered: ESC")

    hb_thread = threading.Thread(target=heartbeat_loop, daemon=True)
    hb_thread.start()

    try:
        while True:
            print("\nSelect mode:")
            print("1: VW mode")
            print("2: Dropoff start mode")
            print("3: Heartbeat stop mode")
            print("4: Heartbeat resume")
            print("5: E-STOP send once")
            print("q: Quit")
            print("긴급정지: ESC")

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

            elif mode == "5":
                send_estop_once_from_menu()

            elif mode == "q":
                break

            else:
                print("invalid input")

    finally:
        running = False

        # hotkey 해제
        try:
            keyboard.unhook_all_hotkeys()
        except Exception:
            pass

        # 일반 종료 시에는 stop VW만 송신.
        # 단, 이미 E-STOP 상태이면 STM32에서 어차피 무시해야 정상.
        send_stop_once()

        time.sleep(0.15)

        if ser is not None and ser.is_open:
            ser.close()

        print("[INFO] closed")


if __name__ == "__main__":
    main()