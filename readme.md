# nGPT

Tiny proof-of-concept ChatGPT on a TI-Nspire calculator using an ESP32 and the calculator's mini USB port.

Tested on:

- TI-Nspire CX II

## What this does

This project lets a TI-Nspire send simple serial commands to an ESP32. The ESP32 connects to Wi-Fi, sends prompts to the OpenAI Responses API, and sends the answer back to the calculator.

Right now it is very much a hacky POC, but it works.

## What you need

- A TI-Nspire calculator
- An ESP32 dev board
- A USB OTG adapter for the calculator
- A USB-to-UART adapter
  - `CP2102` is tested
  - `FTDI FT232` and similar adapters might work, but are untested
- PlatformIO or PIOArduino
- An OpenAI API key

## Setup

1. Create a `secrets.ini` file in the project root.
2. Put this in it:

```ini
[env:esp32dev]
build_flags =
    -D OPENAI_API_KEY=\"API KEY GOES HERE\"
```

3. Replace `API KEY GOES HERE` with your real key.
4. Check `platformio.ini` and change `upload_port` / `monitor_port` if your serial device is different.
5. Build and flash the ESP32 with PlatformIO or PIOArduino.

## Hookup

Connect the calculator to a USB OTG adapter, then connect that to your USB-to-UART adapter, then connect that adapter to the ESP32 serial pins used by this project.

Current firmware settings:

- Baud rate: `115200`
- RX pin: `16`
- TX pin: `17`

## Power notes

In my testing, letting the calculator power the ESP32 directly was not reliable. When the ESP32 tried to connect to Wi-Fi, the power draw spike was enough to make it cut out.

If that happens to you, power the ESP32 separately instead of trying to run it only from the calculator.

Ways to try that:

- Power the ESP32 from its USB port
- Power the ESP32 from its power pins / GPIO-side power input

If you use separate power, make sure the serial connection still shares ground with the ESP32.

## Optional hardware shortcut

If your ESP32 board already has built-in USB-to-UART, you might be able to skip the separate USB-to-UART adapter entirely. I have not tested that, but it seems likely to work.

If you try that route:

- You will probably still want to power the ESP32 separately
- You may need to change the firmware to use `Serial` instead of `HardwareSerial`
- The large commented-out code block in `src/main.cpp` can be used as a rough starting point, but it will need edits rather than just being uncommented as-is
- A practical approach would be to comment out the current transport code and adapt the older commented code to use `Serial`

## Calculator commands

The firmware currently understands these commands:

- `WIFI SSID|PASSWORD`
- `ASK your prompt here`
- `PING`
- `ISTI`

Notes:

- For public Wi-Fi, leave the password blank but still include the `|`.
- Example: `WIFI MyNetwork|supersecretpassword`
- Example: `WIFI SchoolGuest|`

`ISTI` is useful if `Send` is acting weird. If you run into trouble, send `ISTI` first and then try again.

## Basic use on the calculator

Open Scratchpad or a new document and use a flow like this:

```text
Send "WIFI SSID|PASSWORD"
GetStr a
a
```

Then, once Wi-Fi is connected:

```text
Send "ASK prompt"
GetStr a
a
```

Example:

```text
Send "ASK what is 2+2"
GetStr a
a
```

## What to expect

- Wi-Fi connect replies with `WIFI OK` or `WIFI FAIL`
- `PING` replies with `PONG`
- AI requests can take around 15 to 60 seconds depending on Wi-Fi and API response time
- Responses are sent back in 30-character lines so they are easier to read on the calculator

## Troubleshooting

- If `Send` is not behaving right, try `Send "ISTI"` first
- If nothing comes back, check the LED activity on your USB-to-UART adapter
- If Wi-Fi fails, double-check the `SSID|PASSWORD` format
- If public Wi-Fi is used, keep the trailing `|`
- If the ESP32 is not uploading, make sure the serial ports in `platformio.ini` match your machine
- If the calculator is acting stuck or not sending anything, unplug power from the ESP32 and unplug the OTG cable directly from the calculator, then turn the calculator off or press the reset button on the back, then plug everything back in and try again
- If the ESP32 cuts out while connecting to Wi-Fi, power the ESP32 externally instead of relying on the calculator to power it

## Implementation notes

- Uses the OpenAI Responses API
- Current model in code: `gpt-4.1-mini`
- Responses are cleaned up a bit before being sent back to the calculator
- Long responses are capped in firmware so they do not grow forever

## Disclaimer

This is a personal experiment, not a polished product. Expect rough edges.
