# CYD-Stocks
Stock ticker display on the cheap yellow display ESP32.

Code is based off: https://github.com/mike-rankin/ESP32_Stock_Ticker but has been modified to use yahoo finance to allow support of Canadian stocks without subscription. I've also changed some of the formatting.

Follow the setup guide here: https://github.com/LextZip/Deskbuddy/blob/main/SETUP_GUIDE.md specifically this:

## 1. Install ESP32 Board Support
1. Open **Arduino IDE**
2. Go to **Tools > Board > Boards Manager**
3. Search for `ESP32`
4. Install **ESP32 by Espressif Systems**

## 2. Install Required Libraries

Open:

**Sketch > Include Library > Manage Libraries**

Install these libraries:

- `TFT_eSPI`
- `Ntpclient`
- `XPT2046_Touchscreen`

## 3. Copy User_Setup.h into the library dir

- typically `C:\Users\USER\Documents\Arduino\libraries\TFT_eSPI\`



