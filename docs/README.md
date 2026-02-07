# TI-32 Documentation Index

Welcome to the documentation for the TI-32 project.

## 📁 Folders

- [**Plans**](./plans/): Implementation plans for various features.
  - [Power Management](./plans/power_management_plan.md)
  - [WiFi/Ngrok Config](./plans/wifi_ngrok_config_plan.md)
  - [IP Address Display](./plans/ip_address_display_plan_v2.md)

## 📄 Key Documents

- [**Implementation Summary**](./IMPLEMENTATION_SUMMARY.md): A summary of the core architectural changes.
- [**IP Address Usage**](./IP_ADDRESS_DISPLAY_USAGE.md): Details on how to use the IP address display features.

## 🌐 Network Architecture (v0.2)

The project now uses a **Mailbox/Polling** system.
- **Polling Interval**: 5 seconds.
- **Server Route**: `/esp32`
- **Dashboard**: `http://localhost:8080/esp32.html`

### How to use the Mailbox system:
1. Ensure the ESP32 is connected to WiFi.
2. The ESP32 will periodically poll the server's `/esp32/poll` endpoint.
3. Use the Web Dashboard to queue commands.
4. The ESP32 will pick up the commands, execute them, and send back results.
