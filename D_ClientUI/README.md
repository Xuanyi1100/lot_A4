# D - Client UI (browser dashboard)

Single file: `index.html`. No install, no build step.

1. Open `index.html` in Chrome, Edge, Firefox or Safari (internet is needed:
   MQTT.js and Chart.js load from a CDN).
2. Type the MQTT username (default `atu8211`) and its password, press **Connect**.
3. The page connects to the EMQX broker with MQTT over secure WebSocket
   (`wss://p07da41d.ala.us-east-1.emqxsl.com:8084/mqtt`) and subscribes to
   `SENG3030/Wednesday/<A|B|C>/#`.

What it shows: device status (A/B/C online/offline), A's data switch set by C,
all sensor values with their age (red = older than 5 s), live charts
(temperature, acceleration, gyroscope), audio clips from A (play / save .wav),
and the latest MQTT messages.

Usernames and topics are in the `CONFIG` block at the top of the script.
They must match `shared/topics.h`.
