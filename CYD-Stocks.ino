#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h> 
#include <SPI.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

const char* ssid     = "<ssid>";
const char* password = "<pass>";

String symbols[] = {
  "XEQT.TO", "VCN.TO", "XEF.TO", "XEC.TO", "VIDY.TO", 
  "GC=F", "BTC-CAD", "ETH-CAD", "DOGE-CAD", "SHIB-CAD", 
  "SOL-CAD", "XRP-CAD", "NVDA", "SPY"
};
const int numStocks = 14;

TFT_eSPI tft = TFT_eSPI(); 
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -14400); 

unsigned long lastUpdate = 0;

void setup() {
  tft.init();
  tft.setRotation(3); 
  tft.invertDisplay(false); 
  tft.fillScreen(TFT_BLACK); 
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
  timeClient.begin();
  drawStaticUI();
}

void loop() {
  timeClient.update();
  if (millis() - lastUpdate > 60000 || lastUpdate == 0) {
    updateAllStocks();
    lastUpdate = millis();
  }
  drawStatusBar();
  delay(1000);
}

void drawStaticUI() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextFont(2); 
  
  tft.setCursor(5, 2);   tft.print("SYMBOL");
  tft.setCursor(85, 2);  tft.print("PRICE");
  tft.setCursor(180, 2); tft.print("DAY%");
  tft.setCursor(255, 2); tft.print("YTD%");
  
  tft.drawFastHLine(0, 18, 320, TFT_DARKGREY); 
}

void updateAllStocks() {
  for (int i = 0; i < numStocks; i++) {
    fetchYahooData(symbols[i], i);
    delay(200); 
  }
}

void fetchYahooData(String symbol, int index) {
  HTTPClient http;
  // We use range=ytd to get the full year's daily closes
  String url = "https://query1.finance.yahoo.com/v8/finance/chart/" + symbol + "?interval=1d&range=ytd";
  http.begin(url);
  
  if (http.GET() == 200) {
    String payload = http.getString();
    
    StaticJsonDocument<256> filter;
    filter["chart"]["result"][0]["meta"]["regularMarketPrice"] = true;
    filter["chart"]["result"][0]["indicators"]["quote"][0]["close"] = true;

    DynamicJsonDocument doc(6144); 
    deserializeJson(doc, payload, DeserializationOption::Filter(filter));

    JsonArray closePrices = doc["chart"]["result"][0]["indicators"]["quote"][0]["close"];
    float currentPrice = doc["chart"]["result"][0]["meta"]["regularMarketPrice"];
    
    float ytdStartPrice = 0;
    float dayPrevClose = 0;

    // 1. Find YTD Start (First non-null value in the array)
    for (size_t i = 0; i < closePrices.size(); i++) {
      if (!closePrices[i].isNull() && closePrices[i] > 0) {
        ytdStartPrice = closePrices[i];
        break;
      }
    }

    // 2. Find Day Previous Close
    // We look for the last closed price that isn't the current data point
    int lastIdx = closePrices.size() - 1;
    // If the last price in the array is today's price, we want the one before it
    // We loop backwards to find the first valid 'historical' close
    int count = 0;
    for (int i = lastIdx; i >= 0; i--) {
      if (!closePrices[i].isNull()) {
        if (count == 1) { // This is the 'Previous' day
          dayPrevClose = closePrices[i];
          break;
        }
        count++;
      }
    }
    
    // Fallback: if array is too short, use current price (shows 0%)
    if (dayPrevClose == 0) dayPrevClose = currentPrice;

    float dayChangeP = ((currentPrice - dayPrevClose) / dayPrevClose) * 100.0;
    float ytdChangeP = (ytdStartPrice > 0) ? ((currentPrice - ytdStartPrice) / ytdStartPrice) * 100.0 : 0;

    // UI Drawing
    int yPos = 22 + (index * 14); 
    tft.fillRect(0, yPos, 320, 14, TFT_BLACK); 

    String disp = symbol;
    if (symbol == "GC=F") disp = "GOLD";
    else if (symbol.indexOf("-CAD") > 0) disp = symbol.substring(0, symbol.indexOf("-"));
    else if (symbol.endsWith(".TO")) disp = symbol.substring(0, symbol.indexOf("."));

    tft.setTextFont(2); 
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(5, yPos); tft.print(disp);
    
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(85, yPos); 
    if (currentPrice < 1) tft.print(currentPrice, 4);
    else tft.print(currentPrice, 2);
    
    // Day % Column
    tft.setCursor(180, yPos);
    if (dayChangeP >= 0) { tft.setTextColor(TFT_GREEN, TFT_BLACK); tft.print("+"); }
    else { tft.setTextColor(TFT_RED, TFT_BLACK); }
    tft.print(dayChangeP, 2); tft.print("%");

    // YTD % Column
    tft.setCursor(255, yPos);
    if (ytdChangeP >= 0) { tft.setTextColor(TFT_GREEN, TFT_BLACK); tft.print("+"); }
    else { tft.setTextColor(TFT_RED, TFT_BLACK); }
    tft.print(ytdChangeP, 2); tft.print("%");
  }
  http.end();
}

void drawStatusBar() {
  int hh = timeClient.getHours();
  int mm = timeClient.getMinutes();
  String suffix = (hh >= 12) ? "PM" : "AM";
  hh = (hh > 12) ? hh - 12 : (hh == 0 ? 12 : hh);
  String timeStr = "Toronto -  " + String(hh) + ":" + (mm < 10 ? "0" : "") + String(mm) + " " + suffix;
  
  tft.drawFastHLine(0, 222, 320, TFT_DARKGREY);
  tft.fillRect(0, 223, 320, 17, TFT_BLACK);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(100, 224);
  tft.print(timeStr);
}