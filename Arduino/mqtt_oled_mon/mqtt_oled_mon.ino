/* MQTT monitor with 4 Newheaven 128x64 OLED displays
   more info on www.weigu.lu

  www.weigu.lu
  for UDP, listen on Linux PC (UDP_LOG_PC_IP) with netcat command:
  nc -kulw 0 6464
  more infos: www.weigu.lu/microcontroller/esptoolbox/index.html
  ---------------------------------------------------------------------------
  Copyright (C) 2022 Guy WEILER www.weigu.lu

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
  ---------------------------------------------------------------------------

  ESP8266: LOLIN/WEMOS D1 mini pro
  ESP32:   MH ET LIVE ESP32-MINI-KIT

  MHET    | MHET    - LOLIN        |---| LOLIN      - MHET    | MHET

  GND     | RST     - RST          |---| TxD        - RxD(3)  | GND
   NC     | SVP(36) -  A0          |---| RxD        - TxD(1)  | 27
  SVN(39) | 26      -  D0(16)      |---|  D1(5,SCL) -  22     | 25
   35     | 18      -  D5(14,SCK)  |---|  D2(4,SDA) -  21     | 32
   33     | 19      -  D6(12,MISO) |---|  D3(0)     -  17     | TDI(12)
   34     | 23      -  D7(13,MOSI) |---|  D4(2,LED) -  16     | 4
  TMS(14) | 5       -  D8(15,SS)   |---| GND        - GND     | 0
   NC     | 3V3     - 3V3          |---|  5V        -  5V     | 2
  SD2(9)  | TCK(13)                |---|              TD0(15) | SD1(8)
  CMD(11) | SD3(10)                |---|              SD0(7)  | CLK(6)
*/

/*!!!!!!       Make your changes in config.h (or secrets_xxx.h)      !!!!!!*/

/*------ Comment or uncomment the following line suiting your needs -------*/
#define USE_SECRETS
#define OTA               // if Over The Air update needed (security risk!)
//#define MQTTPASSWORD    // if you want an MQTT connection with password (recommended!!)
#define STATIC            // if static IP needed (no DHCP)

/****** Arduino libraries needed ******/
#include "ESPToolbox.h"            // ESP helper lib (more on weigu.lu)
#include <Wire.h>
#include "SSD1322_I2C.h"
#ifdef USE_SECRETS
  // The file "secrets_xxx.h" has to be placed in a sketchbook libraries
  // folder. Create a folder named "Secrets" in sketchbook/libraries and copy
  // the config.h file there. Rename it to secrets_xxx.h
  #include <secrets_mqtt_oled_mon.h> // things you need to change are here or
#else
  #include "config.h"              // things you need to change are here
#endif // USE_SECRETS
#include <PubSubClient.h>          // for MQTT
#include <ArduinoJson.h>           // convert MQTT messages to JSON
#ifdef BME280_I2C
  #include <Wire.h>                // BME280 on I2C (PU-resistors!)
  #include <BME280I2C.h>
#endif // ifdef BME280_I2C

/****** WiFi and network settings ******/
const char *WIFI_SSID = MY_WIFI_SSID;           // if no secrets, use config.h
const char *WIFI_PASSWORD = MY_WIFI_PASSWORD;   // if no secrets, use config.h
#ifdef STATIC
  IPAddress NET_LOCAL_IP (NET_LOCAL_IP_BYTES);  // 3x optional for static IP
  IPAddress NET_GATEWAY (NET_GATEWAY_BYTES);    // look in config.h
  IPAddress NET_MASK (NET_MASK_BYTES);
  IPAddress NET_DNS (NET_DNS_BYTES);
#endif // ifdef STATIC
#ifdef OTA                                      // Over The Air update settings
  const char *OTA_NAME = MY_OTA_NAME;
  const char *OTA_PASS_HASH = MY_OTA_PASS_HASH; // use the config.h file
#endif // ifdef OTA

IPAddress UDP_LOG_PC_IP(UDP_LOG_PC_IP_BYTES);   // UDP log if enabled in setup

/****** MQTT settings ******/
const short MQTT_PORT = MY_MQTT_PORT;
WiFiClient espClient;
PubSubClient MQTT_Client(espClient);
#ifdef MQTTPASSWORD
  const char *MQTT_USER = MY_MQTT_USER;
  const char *MQTT_PASS = MY_MQTT_PASS;
#endif // MQTTPASSWORD
double mqtt_data[sizeof topic_in / sizeof topic_in[0]];
byte mqtt_flag[sizeof topic_in / sizeof topic_in[0]];

/******* BME280 ******/
float temp(NAN), hum(NAN), pres(NAN);
#ifdef BME280_I2C
  BME280I2C bme;    // Default : forced mode, standby time = 1000 ms
                    // Oversampling = press. ×1, temp. ×1, hum. ×1, filter off
#endif // ifdef BME280_I2C

/******* ESPToolbox ******/
ESPToolbox Tb;                                // Create an ESPToolbox Object

/****** SETUP *************************************************************/

void setup() {
  Tb.set_serial_log(true);
  Tb.set_udp_log(true, UDP_LOG_PC_IP, UDP_LOG_PORT);
  Tb.set_led_log(true); // enable LED logging (pos logic)
  oleds_init();
  #ifdef STATIC
    Tb.set_static_ip(true,NET_LOCAL_IP, NET_GATEWAY, NET_MASK, NET_DNS);
  #endif // ifdef STATIC
  Tb.init_wifi_sta(WIFI_SSID, WIFI_PASSWORD, NET_MDNSNAME, NET_HOSTNAME);
  Tb.init_ntp_time();
  MQTT_Client.setBufferSize(MQTT_MAXIMUM_PACKET_SIZE);
  MQTT_Client.setServer(MQTT_SERVER,MQTT_PORT); //open connection MQTT server
  MQTT_Client.setCallback(mqtt_callback);
  MQTT_CLIENT_ID += String(random(0xffff), HEX);  
  //MQTT_Client.setKeepAlive(90);
  //MQTT_Client.setSocketTimeout(90); 
  #ifdef OTA
    Tb.init_ota(OTA_NAME, OTA_PASS_HASH);
  #endif // ifdef OTA
  #ifdef BME280_I2C
    init_bme280();
  #endif // ifdef BME280_I2C  
  Tb.blink_led_x_times(3);
  Tb.log_ln("Setup done!");
}

/****** LOOP **************************************************************/

void loop() {
  #ifdef OTA
    ArduinoOTA.handle();
  #endif // ifdef OTA
  if (WiFi.status() != WL_CONNECTED) {   // if WiFi disconnected, reconnect
    Tb.init_wifi_sta(WIFI_SSID, WIFI_PASSWORD, NET_MDNSNAME, NET_HOSTNAME);
  }
  if (!MQTT_Client.connected()) {        // reconnect mqtt client, if needed
    Tb.log_ln("MQTT not connected??");
    mqtt_connect();
  }  
  MQTT_Client.loop();                    // make the MQTT live
  switch (Tb.non_blocking_delay_x3(10000, 720000, 50000)) { // 10s, 7200000 = 12 min, 50s  
    case 1: {     
      power_net_sum += (mqtt_data[18] + mqtt_data[19] + mqtt_data[20]); // to calculate mean power
      power_load_sum += (mqtt_data[8] + mqtt_data[9] + mqtt_data[10]);
      power_pv_sum += mqtt_data[4];
      soc_sum += mqtt_data[0];    
      power_array_counter++;
      Tb.log("-C1- Counter " + String(power_array_counter)+ "   ");
      Tb.log(String(power_net_sum) + "   ");      
      Tb.log(String(power_load_sum) + "   ");
      Tb.log(String(power_pv_sum) + "   ");
      Tb.log_ln(String(soc_sum));      

      Tb.log_ln("------------------------- Counter" + String(power_array_counter));
      if (power_array_counter == power_array_nr_values) {
        graph_array[0] = power_net_sum/power_array_nr_values;
        graph_array[1] = power_load_sum/power_array_nr_values;
        graph_array[2] = power_pv_sum/power_array_nr_values;
        graph_array[3] = soc_sum/power_array_nr_values;
        power_array_counter = 0;
        power_net_sum = 0;
        power_load_sum = 0;
        power_pv_sum = 0;
        soc_sum = 0;
      }      
      if (!flag_night) {
        oled_print_net_screen(oled1);
        oled_print_load_screen(oled2);        
        oled_print_pv_screen(oled3);        
        oled_print_bat_screen(oled4);          
      }  
    }  
    break;    
    case 2: {
      draw_graphs();
    }    
    break;    
    case 3: {      
      Tb.get_time();
      if ((Tb.t.hour > hour_awake) && (Tb.t.hour < hour_sleep)) {
        flag_night = false;
      }      
      else {
        flag_night = true;
        Tb.log_ln("+++++++++++++ Good night :)");  
      }      
      if ((Tb.t.hour == hour_sleep) && (Tb.t.minute == 0)) {  
        oleds_clear();
        Tb.log_ln("+++++++++++++ Clearing Screens");  
      }
      if ((Tb.t.hour == hour_awake) && (Tb.t.minute == 0)) {  
        oleds_init();
        Tb.log_ln("+++++++++++++ Reset Screens");  
      }      
    }
    break;
    case 0:
    break;
  }
  delay(1); // needed for the watchdog! (alt. yield())
}

/********** OLED functions ***************************************************/

// init OLEDs
void oleds_init() {
  oled_init_net_screen(oled1);
  oled_print_net_screen(oled1);
  oled_init_load_screen(oled2);  
  oled_print_load_screen(oled2);        
  oled_init_pv_screen(oled3);
  oled_print_pv_screen(oled3);        
  oled_init_battery_screen(oled4);
  oled_print_bat_screen(oled4);          
}

void oleds_clear() {
  oled1.begin();
  oled1.clear();
  oled2.begin();
  oled2.clear();
  oled3.begin();
  oled3.clear();
  oled4.begin();
  oled4.clear();
}  

void draw_graphs() {      
  Tb.get_time();      
  Tb.log("--Case2--" + String(graph_array[0]) + "   ");
  Tb.log(String(graph_array[1]) + "   ");
  Tb.log(String(graph_array[2]) + "   ");
  Tb.log_ln(String(graph_array[3]));            
  
  array_net[120] = int(graph_array[0]/333.3333);
  array_load[120] = int(graph_array[1]/333.3333);
  array_pv[120] = int(graph_array[2]/100);
  array_soc[120] = int(graph_array[3]/2.5);      
        
  clear_vertical_line(oled1, vertical_line_x, 33, 62);
  clear_vertical_line(oled2, vertical_line_x, 33, 62);
  clear_vertical_line(oled3, vertical_line_x, 33, 62);
  clear_vertical_line(oled4, vertical_line_x, 23, 62);
  vertical_line_x = 120 - ((Tb.t.hour*60 + Tb.t.minute)/12);
  for (byte i=0; i<119; i++) { // clear curve
    if (array_net[i]) {
      oled1.setpixel(i + 5, 62-array_net[i], BLACK);    // 10kW max
    }
    if (array_load[i]) {
      oled2.setpixel(i + 5, 62-array_load[i], BLACK);   // 10kW max
    }
    if (array_pv[i]) {            
      oled3.setpixel(i + 5, 62-array_pv[i], BLACK);     // 3kW max  
    }          
    if (array_soc[i]) {
      oled4.setpixel(i + 5, 62-array_soc[i], BLACK);    // 100 %
    }  
    yield();
  }
  for (byte i=0; i<120; i++) { // shift array
    array_net[i] = array_net[i+1];
    array_load[i] = array_load[i+1];
    array_pv[i] = array_pv[i+1];
    array_soc[i] = array_soc[i+1];
  }            
  if (!flag_night) {
    for (byte i=0; i<119; i++) {   
      oled1.setpixel(i + 5, 62-array_net[i], WHITE);    // 10kW max
      oled2.setpixel(i + 5, 62-array_load[i], WHITE);   // 10kW max
      oled3.setpixel(i + 5, 62-array_pv[i], WHITE);     // 3kW max
      oled4.setpixel(i + 5, 62-array_soc[i], WHITE);    // 100 %
      yield();
    }
    // draw vertical time line
    
    draw_vertical_line(oled1, vertical_line_x, 33, 62);
    draw_vertical_line(oled2, vertical_line_x, 33, 62);
    draw_vertical_line(oled3, vertical_line_x, 33, 62);
    draw_vertical_line(oled4, vertical_line_x, 23, 62);
  }
}


void oled_init_net_screen(SSD1322_I2C oled) {
  oled.begin();
  oled.clear();
  oled.print(0, 0, "NET:");  
  oled.print(19, 0, "kW");
  oled.print(0, 1, "L1");     
  oled.print(0, 2, "L2");  
  oled.print(0, 3, "L3");  
  oled.print(6, 1, "V");     
  oled.print(6, 2, "V");  
  oled.print(6, 3, "V");  
  oled.print(13, 1, "A");
  oled.print(13, 2, "A");     
  oled.print(13, 3, "A");
  oled.print(20, 1, "W");     
  oled.print(20, 2, "W");
  oled.print(20, 3, "W"); 
  draw_graph_axis_3_lines(oled, 32);
}

void oled_print_net_screen(SSD1322_I2C oled) {
  double tot_power;
  if ((mqtt_flag[18]) || (mqtt_flag[19]) || (mqtt_flag[20])) {
    tot_power = (mqtt_data[18] + mqtt_data[19] + mqtt_data[20])/1000;  
    display_double_4_digits(oled, 14, 0, tot_power);
    display_int_4_digits(oled, 15, 1, mqtt_data[18]);  
    display_int_4_digits(oled, 15, 2, mqtt_data[19]);    
    display_int_4_digits(oled, 15, 3, mqtt_data[20]);  
  }  
  if (mqtt_flag[21]) {
    display_int_3_voltage(oled, 3, 1, mqtt_data[21]);  
  }
  if (mqtt_flag[22]) {
    display_int_3_voltage(oled, 3, 2, mqtt_data[22]);    
  }
  if (mqtt_flag[23]) {
    display_int_3_voltage(oled, 3, 3, mqtt_data[23]);  
  }
  if (mqtt_flag[24]) {
    display_double_4_digits(oled, 8, 1, mqtt_data[24]);    
  }
  if (mqtt_flag[25]) {
    display_double_4_digits(oled, 8, 2, mqtt_data[25]);    
  }
  if (mqtt_flag[26]) {
    display_double_4_digits(oled, 8, 3, mqtt_data[26]);    
  }  
  for (byte i=18; i<27; i++) {
    mqtt_flag[i] = 0;
  }
}

void oled_init_load_screen(SSD1322_I2C oled) {
  oled.begin();
  oled.clear();
  oled.print(0, 0, "LOAD:");  
  oled.print(10, 0, "Hz");  
  oled.print(19, 0, "kW");  
  oled.print(0, 1, "L1");     
  oled.print(0, 2, "L2");  
  oled.print(0, 3, "L3");  
  oled.print(6, 1, "V");     
  oled.print(6, 2, "V");  
  oled.print(6, 3, "V");  
  oled.print(13, 1, "A");
  oled.print(13, 2, "A");     
  oled.print(13, 3, "A");
  oled.print(20, 1, "W");     
  oled.print(20, 2, "W");
  oled.print(20, 3, "W");  
  draw_graph_axis_3_lines(oled, 32);
}

void oled_print_load_screen(SSD1322_I2C oled) {
  double tot_power;
  if ((mqtt_flag[8]) || (mqtt_flag[9]) || (mqtt_flag[10])) {
    tot_power = (mqtt_data[8] + mqtt_data[9] + mqtt_data[10])/1000;  
    display_double_4_digits(oled, 14, 0, tot_power);
    display_int_4_digits(oled, 15, 1, mqtt_data[8]);  
    display_int_4_digits(oled, 15, 2, mqtt_data[9]);    
    display_int_4_digits(oled, 15, 3, mqtt_data[10]);  
  }  
  if (mqtt_flag[11]) {
    display_int_3_voltage(oled, 3, 1, mqtt_data[11]);  
  }
  if (mqtt_flag[12]) {
    display_int_3_voltage(oled, 3, 2, mqtt_data[12]);    
  }
  if (mqtt_flag[13]) {
    display_int_3_voltage(oled, 3, 3, mqtt_data[13]);  
  }
  if (mqtt_flag[14]) {
    display_double_4_digits(oled, 8, 1, mqtt_data[14]);    
  }
  if (mqtt_flag[15]) {
    display_double_4_digits(oled, 8, 2, mqtt_data[15]);    
  }
  if (mqtt_flag[16]) {
    display_double_4_digits(oled, 8, 3, mqtt_data[16]);    
  }  
  if (mqtt_flag[17]) {
    display_double_4_digits(oled, 5, 0, mqtt_data[17]);    
  }  
  for (byte i=8; i<18; i++) {
    mqtt_flag[i] = 0;
  }
}

void oled_init_pv_screen(SSD1322_I2C oled) {
  oled.begin();
  oled.clear();
  oled.print(0, 1, "PV:");  // first line defective!  
  oled.print(16, 1, "kWh/d");
  oled.print(5, 2, "W");
  oled.print(12, 2, "V");      
  oled.print(19, 2, "A");    
  oled.print(0, 3, "DC");  
  oled.print(7, 3, "W");
  oled.print(9, 3, "LED");  
  oled.print(15, 3, "BST");  
  draw_graph_axis_3_lines(oled, 32);
}

void oled_print_pv_screen(SSD1322_I2C oled) {
  if (mqtt_flag[4]) {  // W
    display_int_4_digits(oled, 0, 2, mqtt_data[4]);  
  }
  if (mqtt_flag[5]) { // V
    display_int_3_voltage(oled, 9, 2, mqtt_data[5]);
  }
  if (mqtt_flag[6]) { // A
    display_double_4_digits(oled, 14, 2, mqtt_data[6]);    
  }  
  if (mqtt_flag[7]) { // Yield
    display_double_4_digits(oled, 11, 1, mqtt_data[7]);
  }
  if (mqtt_flag[27]) {// DC Watt
    display_int_4_digits(oled, 2, 3, mqtt_data[27]);
  }
  if (mqtt_flag[28]) {// LED
    oled.print(12, 3, String(mqtt_data[28],0).c_str());    
  }  
  if (mqtt_flag[29]) { // Batt
    oled.print(18, 3, String(mqtt_data[29],0).c_str());  
  }  
  for (byte i=4; i<8; i++) {
    mqtt_flag[i] = 0;
  }
  for (byte i=27; i<30; i++) {
    mqtt_flag[i] = 0;
  }
}

void oled_init_battery_screen(SSD1322_I2C oled) {
  oled.begin();
  oled.clear();
  oled.print(0, 0, "BAT:");
  oled.print(14, 0, "SOC");    
  oled.print(20, 0, "%");    
  oled.print(5, 1, "V");  
  oled.print(12, 1, "A");  
  oled.print(19, 1, "W");  
  draw_graph_axis_5_lines(oled, 58, 22);
}

void oled_print_bat_screen(SSD1322_I2C oled) {      
  if (mqtt_flag[0]) { // SOC 2 digits
    oled.print(18, 0, String(mqtt_data[0],0).c_str());
  }  
  if (mqtt_flag[2]) { // V
    display_double_4_digits(oled, 0, 1, mqtt_data[2]);
  }  
  if (mqtt_flag[3]) { // A
    display_double_4_digits(oled, 7, 1, mqtt_data[3]);
  }  
  if (mqtt_flag[2]) { // W
    display_int_4_digits(oled, 14, 1, mqtt_data[1]);    
  }  
  for (byte i=0; i<4; i++) {
    mqtt_flag[i] = 0;
  }
}


/********** OLED HELPER functions ********************************************/

// display a double number with 4 digits (xx.x) on the screen (needs 5 places!)
void display_double_4_digits(SSD1322_I2C oled, byte x, byte y, double number) {  
  if (number >= 100) { // clip to 99.99
    number = 99.99;
  }  
  if (number <= -100) { // clip to -99.99
    number = -99.99;
  }    
  if (number >= 0) { // pos
    oled.print(x, y, " ");
    x = x+1;
  }
  // neg
  String str_number = String(number,1);
  // workaround for too long c-strings if 0, e.g. 8.7
  byte index_comma = str_number.indexOf('.');
  str_number.remove(index_comma+2);
  if (abs(number) >= 10) {      
    oled.print(x, y, str_number.c_str());
  }  
  else if (abs(number) < 10) {
    oled.print(x, y, " ");      
    oled.print(x+1, y, str_number.c_str());     
  }  
}

// display an int number with 4 digits correctly on the screen (needs 5 places!)
void display_int_4_digits(SSD1322_I2C oled, byte x, byte y, double number) {  
  if (number > 9999) { // clip to 9999
    number = 9999;    
  }  
  if (number <-9999) { // clip to -9999
    number = -9999;    
  }    
  if (number >= 0) { // pos
    oled.print(x, y, " ");
    x = x+1;    
  }
  // neg
  if (abs(number) > 999) {      
    oled.print(x, y, String(number,0).c_str());    
  }  
  else if ((abs(number) < 1000) && (abs(number) > 99)) {
    oled.print(x, y, " ");  
    oled.print(x+1, y, String(number,0).c_str());         
  }
  else if ((abs(number) < 100) && (abs(number) > 9)) {
    oled.print(x, y, "  ");  
    oled.print(x+2, y, String(number,0).c_str());         
  }
  else if (abs(number) < 10) {
    if (number >= 0) { //workaround because strlen of pos digit gives 2?
      x = x-1;
    }
    oled.print(x, y, "   ");  
    oled.print(x+3, y, String(number,0).c_str());     
  }
}

void display_int_3_voltage(SSD1322_I2C oled, byte x, byte y, double number) {    
  oled.print(x, y, String(number,0).c_str());    
}

void draw_graph_axis_3_lines(SSD1322_I2C oled, byte dottet_line) {    
  oled.drawline(4, 32, 4, 63, WHITE);           // y axes left
  for (byte i=60; i>33; i-=2) {                 // small ticks left
    oled.setpixel(3, i, WHITE);
  }
  for (byte i=62; i>30; i-=6) {                 // big ticks left
    oled.drawline(1, i, 4, i, WHITE);
  }
  oled.drawline(124, 32, 124, 63, WHITE);       // y axes right
  for (byte i=60; i>33; i-=2) {                 // small ticks right
    oled.setpixel(125, i, WHITE);
  }
  for (byte i=62; i>30; i-=6) {                 // big ticks right
    oled.drawline(125, i, 127, i, WHITE);
  }
  oled.drawline(4, 62, 127, 62, WHITE);         // x axes
  for (byte i=4; i<125; i+=5) {                 // x axes ticks 24h
    oled.setpixel(i, 63, WHITE);
  }
  for (byte i=4; i<125; i+=2) {                 // dotted guide line (10%)
    oled.setpixel(i, dottet_line, WHITE);                
  }  
}

void draw_graph_axis_5_lines(SSD1322_I2C oled, byte dottet_line_1, byte dottet_line_2) {    
  oled.drawline(4, 22, 4, 63, WHITE);           // y axes left
  for (byte i=60; i>23; i-=2) {                 // small ticks left
    oled.setpixel(3, i, WHITE);
  }
  for (byte i=62; i>20; i-=8) {                 // big ticks left
    oled.drawline(1, i, 4, i, WHITE);
  }
  oled.drawline(124, 22, 124, 63, WHITE);       // y axes right
  for (byte i=60; i>23; i-=2) {                 // small ticks right
    oled.setpixel(125, i, WHITE);
  }
  for (byte i=62; i>20; i-=8) {                 // big ticks right
    oled.drawline(125, i, 127, i, WHITE);
  }
  oled.drawline(4, 62, 127, 62, WHITE);         // x axes
  for (byte i=4; i<125; i+=5) {                 // x axes ticks 
    oled.setpixel(i, 63, WHITE);
  }
  for (byte i=4; i<125; i+=2) {                 // dotted guide line (10%)
    oled.setpixel(i, dottet_line_1, WHITE);                
  }  
  for (byte i=4; i<125; i+=2) {                 // dotted guide line (100%)
    oled.setpixel(i, dottet_line_2, WHITE);                
  }  
}


void draw_vertical_line(SSD1322_I2C oled, byte line_x, byte line_y_top, byte line_y_bottom) {    
  for (byte i=line_y_top; i<line_y_bottom; i+=3) { 
    oled.setpixel(line_x, i, WHITE);    
  }
}

void clear_vertical_line(SSD1322_I2C oled, byte line_x, byte line_y_top, byte line_y_bottom) {    
  for (byte i=line_y_top; i<line_y_bottom; i+=3) { 
    oled.setpixel(line_x, i, BLACK);
  }
}


/********** MQTT functions ***************************************************/

// connect to MQTT server
void mqtt_connect() {
  while (!MQTT_Client.connected()) { // Loop until we're reconnected
    Tb.log("Attempting MQTT connection...");        
    Tb.log(MQTT_CLIENT_ID);
    #ifdef MQTTPASSWORD
      if (MQTT_Client.connect(MQTT_CLIENT_ID.c_str(), MQTT_USER, MQTT_PASS)) {
    #else
      if (MQTT_Client.connect(MQTT_CLIENT_ID.c_str())) { // Attempt to connect
    #endif // ifdef MQTTPASSWORD
      Tb.log_ln("MQTT connected");
      for (int i = 0; i < (sizeof topic_in / sizeof topic_in[0]); i++) {
        MQTT_Client.subscribe(topic_in[i].path.c_str());
        //Tb.log_ln(topic_in[i].name); 
        mqtt_flag[i] = 0;     
      }
    }
    else {
      Tb.log("MQTT connection failed, rc=");
      Tb.log(String(MQTT_Client.state()));
      Tb.log_ln(" try again in 5 seconds");
      delay(5000);  // Wait 5 seconds before retrying
    }      
  }
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {  
  DynamicJsonDocument doc_in(1024);
  deserializeJson(doc_in, (byte*)payload, length);   //parse MQTT message 
  for (int i = 0; i < (sizeof topic_in / sizeof topic_in[0]); i++) {     
    if (String(topic) == topic_in[i].path) {
      //Serial.print(topic_in[i].path);
      mqtt_data[i] = doc_in["value"];
      mqtt_flag[i] = 1;  
      /*Tb.log(String(i) + ": ");      
      Tb.log(topic_in[i].name + ": ");      
      Tb.log_ln(String(mqtt_data[i]));*/
    }
  }  
}  
