# f767zi_capstone

> **Status:** Completed on 2026-05-26

STM32F767ZI 기반 하위 제어 펌웨어입니다. 현재 저장소 기준으로 `NUCLEO-F767ZI` 보드에서 동작하며, 차동 구동 모터 제어, 엔코더 기반 속도 폐루프, 오도메트리 송신, 드롭오프용 stepper 시퀀스, heartbeat failsafe, software E-STOP을 포함합니다.

## 구현 범위

- `UART3` (`115200`, `8N1`)로 주행 명령(`v`, `w`) 수신
- 좌/우 엔코더 기반 RPM 계산
- PI 속도 제어와 입력 램프 제한
- 차동 구동 오도메트리 계산 (`x`, `y`, `yaw`, `v`, `w`)
- `50 ms` 주기 오도메트리 프레임 송신
- 슬라이더 + 암 stepper 기반 드롭오프 시퀀스
- heartbeat timeout 시 주행/stepper 정지
- software E-STOP latch
- 사용자 버튼(`B1`) 기반 슬라이더 수동 jog
- IWDG reset 감지 시 `LD3` 점등

## 제어 주기

- main loop: heartbeat 확인, UART 프레임 처리, 버튼/LED 처리, stepper 상태 업데이트
- `10 ms`: `Encoder_Update()`, `Control_Update()`, `Odometry_Update()`
- `50 ms`: 오도메트리 UART 송신
- `100 ms`: IWDG refresh
- `500 ms` 초과 heartbeat 미수신 시 정지

## 보드 및 자원 사용

| 항목 | 자원 | 비고 |
| --- | --- | --- |
| MCU/Board | `STM32F767ZITx`, `NUCLEO-F767ZI` | CubeMX `.ioc` 기준 |
| DC motor PWM | `TIM1 CH1/CH2` | 좌/우 모터 PWM |
| DC motor DIR | `PG14`, `PG9` | 좌/우 방향 핀 |
| Encoder | `TIM3`, `TIM4` | 좌/우 엔코더 |
| Slider stepper | `TIM5 CH4` + `PC3` | pulse + dir |
| Arm stepper | `TIM8 CH4` + `PC8` | pulse + dir |
| Timebase | `TIM2` | us timestamp |
| UART | `USART3` (`PD8/PD9`) | ST-LINK VCP |
| LED | `LD1/LD2/LD3` | busy / heartbeat / reset-fault 표시 |

참고:

- `ADC1` 배터리 전압 입력과 `SPI1` IMU 핀은 초기화되어 있지만, 현재 애플리케이션 로직에서는 사용하지 않습니다.
- 프로젝트 설정 파일은 [f767zi.ioc](f767zi.ioc)에 있습니다.

## UART 프레임 프로토콜

모든 프레임은 `[0xAA][0x55][MSG_TYPE]...[XOR checksum]` 형식을 사용합니다.

- checksum: 마지막 1바이트 XOR
- `float`: little-endian 4바이트

### RX: PC -> STM32

| 메시지 | 타입 | 길이 | payload |
| --- | --- | --- | --- |
| `VW` | `0x02` | `12` bytes | `v_mps(float)`, `w_radps(float)` |
| `DROPOFF_START` | `0x03` | `5` bytes | `target_id(uint8)` |
| `HEARTBEAT` | `0x05` | `4` bytes | 없음 |
| `E_STOP` | `0x06` | `4` bytes | 없음 |

### TX: STM32 -> PC

| 메시지 | 타입 | 길이 | payload |
| --- | --- | --- | --- |
| `ODOM` | `0x01` | `28` bytes | `t_us`, `x`, `y`, `yaw`, `v`, `w` |
| `DROPOFF_DONE` | `0x04` | `4` bytes | 없음 |

### 동작 제약

- 부팅 직후 heartbeat 상태는 timeout으로 시작합니다.
- `VW`, `DROPOFF_START` 전에 heartbeat가 먼저 들어와야 합니다.
- `E_STOP` 수신 후에는 latch 상태가 유지되며, 현재 해제 프레임은 없습니다.
- stepper 동작 중에는 주행 명령이 무시됩니다.
- `DROPOFF_START` 수신 시점에는 드롭오프 성공 여부와 무관하게 주행이 먼저 정지됩니다.

## 드롭오프 `target_id`

현재 허용 범위는 `0..4`입니다.

- `0`: init 위치 복귀 (`slider index 0`, `arm center`)
- `1`: slider 위치 1 + arm left
- `2`: slider 위치 1 + arm right
- `3`: slider 위치 2 + arm left
- `4`: slider 위치 2 + arm right

참고:

- 내부 로직상 slider 위치 3까지 계산하는 코드는 존재하지만, 현재 `DROPOFF_TARGET_MAX_ID`는 `4`로 제한되어 있습니다.
- 물리 파라미터와 pulse 상수는 [Core/Inc/config.h](Core/Inc/config.h)에 있습니다.

## 주요 파일

| 파일 | 역할 |
| --- | --- |
| [Core/Src/main.c](Core/Src/main.c) | 메인 루프와 주기 실행 |
| [Core/Src/control.c](Core/Src/control.c) | `v/w -> wheel rpm` 변환, PI 제어, 램프 제한 |
| [Core/Src/encoder.c](Core/Src/encoder.c) | 엔코더 delta/RPM 계산 |
| [Core/Src/odometry.c](Core/Src/odometry.c) | 차동 구동 오도메트리 계산 |
| [Core/Src/protocol.c](Core/Src/protocol.c) | UART 프레임 파싱/송신 |
| [Core/Src/stepper.c](Core/Src/stepper.c) | 슬라이더/암 stepper 시퀀스 |
| [Core/Src/heartbeat.c](Core/Src/heartbeat.c) | heartbeat timeout failsafe |
| [Core/Src/estop.c](Core/Src/estop.c) | software E-STOP latch |

## PC 테스트 스크립트

[Python/FRAME_transmit.py](Python/FRAME_transmit.py)는 현재 프레임 프로토콜 기준 테스트 스크립트입니다. heartbeat thread, `VW`, `DROPOFF_START`, `E-STOP` 전송을 지원합니다.

[Python/ASCII_transmit.py](Python/ASCII_transmit.py)는 단순 ASCII 송신 예제입니다. 현재 펌웨어는 binary frame 프로토콜을 사용하므로, 일반 제어 테스트에는 `FRAME_transmit.py` 사용을 권장합니다.

실행 전에는 스크립트 내부 `COM` 포트를 환경에 맞게 수정해야 합니다.

```bash
pip install pyserial keyboard
python Python/FRAME_transmit.py
```

## 빌드

1. `STM32CubeIDE`에서 프로젝트를 엽니다.
2. 필요 시 [f767zi.ioc](f767zi.ioc)로 CubeMX 설정을 확인합니다.
3. Build 후 보드에 flash 합니다.

## 메모

- 이 README는 현재 저장소 코드 상태를 기준으로 갱신했습니다.
- 향후 IMU, 배터리 모니터링, E-STOP release, dropoff 응답 확장이 들어가면 프로토콜과 기능 범위를 함께 갱신해야 합니다.
