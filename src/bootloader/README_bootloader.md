# UART bootloader

## Memory map

- Bootloader: `0x08000000..0x0801FFFF` (128 KiB, Flash Bank 1 sector 0).
- Application: `0x08100000..0x081FFFFF` (1024 KiB, Flash Bank 2 sectors 0..7).
- The application must be linked for `0x08100000`; in this project use the
  `blinker_flash` environment, which uses `system/h745cm7_flash_bank2_app.ld`.

## Startup logic

- Reset always starts the bootloader.
- Button PC13 released: jump to the application if its vector table is valid.
- Button PC13 pressed: stay in programming mode, blink LED PE1, wait on USART3.
- USART3 pins are PD8 TX and PD9 RX, `115200 8N1`.

## Binary protocol

All multibyte fields are little-endian. Responses are one byte:

- `O`: command accepted.
- `E`: command failed.
- `?`: unknown command.

Commands:

- `0x01 ERASE`
  - Request: `01`
  - Response: `O` or `E`
  - Erases application sectors 0..7 in Flash Bank 2.

- `0x02 WRITE`
  - Request: `02 <offset:u32> <length:u32> <data>`
  - Response: `O` or `E`
  - `offset` is relative to `0x08100000`.
  - Data is programmed in 32-byte flash words. The last partial block is padded
    with `0xFF` by the PC uploader.

- `0x03 READ`
  - Request: `03 <offset:u32> <length:u32>`
  - Response: `O <data>` or `E`

- `0x04 JUMP`
  - Request: `04`
  - Response: `O` then software jump, or `E` if no valid application exists.

## Upload example

```powershell
python src\bootloader\py_uploader.py .pio\build\blinker_flash\firmware.bin --port COM5
```
