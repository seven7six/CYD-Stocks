#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h> 
#include <SPI.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// --- USER SETTINGS: FILL THESE IN ---
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
  
  // --- THE ULTIMATE COLOR FIX ---
  // If your background is WHITE right now, change this to 'false'
  // If your background is BLACK, keep it 'true'
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
  tft.setTextDatum(MC_DATUM);
  
  // Header using Font 2 (Slightly larger/cleaner)
  tft.setTextFont(2); 
  tft.setCursor(10, 2);   tft.print("SYMBOL");
  tft.setCursor(105, 2);  tft.print("PRICE (CAD)");
  tft.setCursor(240, 2);  tft.print("CHANGE%");
  
  tft.drawFastHLine(0, 18, 320, TFT_DARKGREY); 
}

void updateAllStocks() {
  for (int i = 0; i < numStocks; i++) {
    fetchYahooData(symbols[i], i);
    delay(150); 
  }
}

void fetchYahooData(String symbol, int index) {
  HTTPClient http;
  String url = "https://query1.finance.yahoo.com/v8/finance/chart/" + symbol + "?interval=1m&range=1d";
  http.begin(url);
  
  if (http.GET() == 200) {
    String payload = http.getString();
    StaticJsonDocument<200> filter;
    filter["chart"]["result"][0]["meta"]["regularMarketPrice"] = true;
    filter["chart"]["result"][0]["meta"]["chartPreviousClose"] = true;
    DynamicJsonDocument doc(2048);
    deserializeJson(doc, payload, DeserializationOption::Filter(filter));
    
    float price = doc["chart"]["result"][0]["meta"]["regularMarketPrice"];
    float prevClose = doc["chart"]["result"][0]["meta"]["chartPreviousClose"];
    float changeP = (prevClose > 0) ? ((price - prevClose) / prevClose) * 100.0 : 0;

    // Row positioning: 14.5 pixels per row
    int yPos = 22 + (index * 14); 
    tft.fillRect(0, yPos, 320, 14, TFT_BLACK); 

    String disp = symbol;
    if (symbol == "GC=F") disp = "GOLD";
    else if (symbol.indexOf("-CAD") > 0) disp = symbol.substring(0, symbol.indexOf("-"));
    else if (symbol.endsWith(".TO")) disp = symbol.substring(0, symbol.indexOf("."));

    // --- FONT 2: Taller and much clearer than Size 1 ---
    tft.setTextFont(2); 
    
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(10, yPos); tft.print(disp);
    
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(105, yPos); 
    if (price < 0.01) tft.print(price, 5); 
    else if (price < 1) tft.print(price, 4);
    else tft.print(price, 2);
    
    tft.setCursor(240, yPos);
    if (changeP >= 0) {
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.print("+");
    } else {
      tft.setTextColor(TFT_RED, TFT_BLACK);
    }
    tft.print(changeP, 2); tft.print("%");
  }
  http.end();
}

void drawStatusBar() {
  int hh = timeClient.getHours();
  int mm = timeClient.getMinutes();
  String suffix = (hh >= 12) ? "PM" : "AM";
  hh = (hh > 12) ? hh - 12 : (hh == 0 ? 12 : hh);
  String timeStr = "Toronto: " + String(hh) + ":" + (mm < 10 ? "0" : "") + String(mm) + " " + suffix;

  tft.drawFastHLine(0, 222, 320, TFT_DARKGREY);
  tft.fillRect(0, 223, 320, 17, TFT_BLACK);
  
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextFont(2); // Smaller but readable font for status
  tft.setCursor(100, 224);
  tft.print(timeStr);
}