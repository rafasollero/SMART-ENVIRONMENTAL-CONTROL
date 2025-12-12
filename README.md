 SMART ENV. CONTROL SYSTEM

1. Project Overview & 

 This platform is your autonomous, dedicated guardian for a closed environment—think a precision greenhouse or a specialized lab. Instead of relying on guesswork, this project uses a robust suite of sensors and smart logic to automatically manage temperature, humidity, lighting, and safety, ensuring the environment remains within optimal, life-supporting parameters. It’s about building a responsive, intelligent ecosystem.

 2. Hardware and Component List

The project runs on an Arduino UNO/Nano and requires a dense wiring setup. **Please pay close attention to the pin reassignments below!**

2.1. Physical Components

| Type | Component | Pin Assignment | Purpose |
| :--- | :--- | :--- | :--- |
| **Microcontroller** | Arduino UNO/Nano | N/A | The brain of the operation. |
| **Analog Sensors**| LDR, LM35 (or TMP36), Capacitive Hygrometer, MQ-2 (Gas) | A0, A3, A1, A2 | Readings for Light, Temp, Soil Moisture, and Air Quality. |
| **Digital Sensors** | PIR HC-SR501, Push Button | D2, D3 | Motion Detection and Manual Emergency Stop (via Interrupt). |
| **Actuators** | Servomotor, Relay Modules (x3) | D9, D4, D5, D6 | Window/Vent opening, Fan, Water Pump, Artificial Lighting. |
| **Interface** | LCD 16x2 (Direct), Buzzer | Varios, A4 | Status Display and Audible Safety Alarm. |

2.2. Critical Pin Reassignment Summary

We had to move several components to avoid conflicts with the crucial Analog Pins (A0-A3) needed for sensors. **Ensure your wiring reflects these new assignments:**

| Component | Old Pin | **New Pin** | Type |
| :--- | :--- | :--- | :--- |
| **Buzzer (Alarm)** | D10 | **A4** | Analog/Digital |
| **LED Status (Heartbeat)** | D13 | **D0 (RX)** | Digital |
| **LED Alert (Failure)** | D12 | **D1 (TX)** | Digital |
| **LCD 16x2 (Data)** | A0, A1, A2 | **D10, D12, D13** | Digital |

💻 3. Software Requirements

* Arduino IDE
* `Servo.h` (Standard)
* `Wire.h` (Standard)
* `LiquidCrystal.h` (Standard, for 4-bit parallel connection).

---

⚠️ 4. Essential Note for Tinkercad Simulation

To simplify testing and prevent the need for complex external power supplies, the high-power components are simulated using simple LEDs and Potentiometers:

| Real-World Actuator | Control Pin | Component in **Tinkercad** (Simulation) |
| :--- | :--- | :--- |
| **Fan / Ventilation** | D4 | **LED** (Wired as Normally Closed Relay Logic) |
| **Water Pump / Irrigation** | D5 | **LED** (Wired as Normally Closed Relay Logic) |
| **Artificial Lights** | D6 | **LED** (Wired as Normally Closed Relay Logic) |
| **Soil Hygrometer (A1)** | A1 | **Potentiometer** (Vary resistance to simulate wet/dry soil) |
| **Gas Sensor (A2)** | A2 | **Potentiometer** (Vary resistance to simulate gas concentration) |

> **REMINDER:** The relays use **Inverse Logic**. When the code sends a **LOW** signal to a pin (D4, D5, D6), the LED in your simulation should turn **ON**.

---

 5. Technical Appendices

5.1. Temperature Conversion Formula

The code reads the raw analog value (`tempVal`) from the LM35/TMP36 sensor on A3 and converts it to Celsius using standard calculations.

$$
\text{Voltage} = \text{tempVal} \times \frac{5.0}{1023.0} \\
\text{Temperature } (^{\circ}\text{C}) = \text{Voltage} \times 100
$$

5.2. I2C LCD Connection (Alternative Method)

If you decide to switch from the standard `LiquidCrystal.h` to the I2C backpack version (highly recommended for saving pins), you would only need to use two pins:

* **SDA** (Serial Data) $\rightarrow$ **A4**
* **SCL** (Serial Clock) $\rightarrow$ **A5**

You would then replace `LiquidCrystal.h` with `LiquidCrystal_I2C.h` and use the I2C object declaration (`LiquidCrystal_I2C lcd(0x27, 16, 2);`).

5.3. Calibration Thresholds

These values define the control logic (found in the `// Calibration` section):

* `TEMP_MAXIMA = 30` (If exceeded, fan/window activates).
* `UMBRAL_HUMEDAD_MIN = 400` (If exceeded, irrigation starts).
* `UMBRAL_GAS_PELIGRO = 150` (If exceeded, initiates critical alert and forced ventilation).

 6. Next Steps

1.  Complete the component wiring in Tinkercad based on the **New Pin Assignments**.
2.  Upload the code and start the simulation.
3.  Test the core functions: activate the Emergency Button (D3) and manipulate the Potentiometers (A1/A2) and the temperature sensor (A3) to ensure the system responds correctly.
