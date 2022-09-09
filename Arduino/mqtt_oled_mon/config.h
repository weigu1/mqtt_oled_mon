/****** WiFi SSID and PASSWORD ******/
const char *MY_WIFI_SSID = "your_ssid";
const char *MY_WIFI_PASSWORD = "your_password";

/****** WiFi and network settings ******/
// UDP logging settings if enabled in setup(); Port used for UDP logging
const word UDP_LOG_PORT = 6464;
// IP address of the computer receiving UDP log messages
const byte UDP_LOG_PC_IP_BYTES[4] = {192, 168, 178, 100};
// optional (access with UDP_logger.local)
const char *NET_MDNSNAME = "MQTT_OLED_MON";
// optional hostname
const char *NET_HOSTNAME = "MQTT_OLED_MON";
// only if you use a static address (uncomment //#define STATIC in ino file)
const byte NET_LOCAL_IP_BYTES[4] = {192, 168, 178, 155};
const byte NET_GATEWAY_BYTES[4] = {192, 168, 178, 1};
const byte NET_MASK_BYTES[4] = {255,255,255,0};
const byte NET_DNS_BYTES[4] = {8,8,8,8}; //  second dns (first = gateway), 8.8.8.8 = google
// only if you use OTA (uncomment //#define OTA in ino file)
const char *MY_OTA_NAME = "mqtt_oled_mon"; // optional (access with mqtt_oled_mon.local)
// Linux Create Hasgh with: echo -n 'P@ssword1' | md5sum
const char *MY_OTA_PASS_HASH = "myHash";     // Hash for password

/****** MQTT settings ******/
const char *MQTT_SERVER = "192.168.178.222";
const long PUBLISH_TIME = 10000; //Publishes every in milliseconds
const int MQTT_MAXIMUM_PACKET_SIZE = 1024; // look in setup()
String MQTT_CLIENT_ID = "mqtt_oled_mon_1"; // this must be unique!!!
const short MY_MQTT_PORT = 1883; // or 8883
// only if you use MQTTPASSWORD (uncomment //#define MQTTPASSWORD in ino file)
const char *MY_MQTT_USER = "me";
const char *MY_MQTT_PASS = "meagain";

struct MQTT_TOPIC_IN {
  const byte number;
  const String name;
  String path;
};

MQTT_TOPIC_IN topic_in[] = {
  {0,"Bat_Soc","myhouse/victron_cerbo_N/ed3a43se2345/system/0/Dc/Battery/Soc"},
  {1,"Bat_Power","myhouse/victron_cerbo_N/ed3a43se2345/system/0/Dc/Battery/Power"},
  {2,"Bat_Voltage","myhouse/victron_cerbo_N/ed3a43se2345/system/0/Dc/Battery/Voltage"},
  {3,"Bat_Current","myhouse/victron_cerbo_N/ed3a43se2345/system/0/Dc/Battery/Current"},
  {4,"PV_Power","myhouse/victron_cerbo_N/ed3a43se2345/system/0/Dc/Pv/Power"},
  {5,"PV_Voltage","myhouse/victron_cerbo_N/ed3a43se2345/solarcharger/1/Pv/0/V"},
  {6,"PV_Current","myhouse/victron_cerbo_N/ed3a43se2345/system/0/Dc/Pv/Current"},
  {7,"PV_Yield","myhouse/victron_cerbo_N/ed3a43se2345/solarcharger/1/History/Daily/0/Yield"},
  {8,"Load_Power_L1","myhouse/victron_cerbo_N/ed3a43se2345/vebus/276/Ac/Out/L1/P"},
  {9,"Load_Power_L2","myhouse/victron_cerbo_N/ed3a43se2345/vebus/276/Ac/Out/L2/P"},
  {10,"Load_Power_L3","myhouse/victron_cerbo_N/ed3a43se2345/vebus/276/Ac/Out/L3/P"},
  {11,"Load_Voltage_L1","myhouse/victron_cerbo_N/ed3a43se2345/vebus/276/Ac/Out/L1/V"},
  {12,"Load_Voltage_L2","myhouse/victron_cerbo_N/ed3a43se2345/vebus/276/Ac/Out/L2/V"},
  {13,"Load_Voltage_L3","myhouse/victron_cerbo_N/ed3a43se2345/vebus/276/Ac/Out/L3/V"},
  {14,"Load_Current_L1","myhouse/victron_cerbo_N/ed3a43se2345/vebus/276/Ac/Out/L1/I"},
  {15,"Load_Current_L2","myhouse/victron_cerbo_N/ed3a43se2345/vebus/276/Ac/Out/L2/I"},
  {16,"Load_Current_L3","myhouse/victron_cerbo_N/ed3a43se2345/vebus/276/Ac/Out/L3/I"},
  {17,"Load_Freq_L1","myhouse/victron_cerbo_N/ed3a43se2345/vebus/276/Ac/Out/L1/F"},
  {18,"Net_Power_L1","myhouse/victron_cerbo_N/ed3a43se2345/vebus/276/Ac/ActiveIn/L1/P"},
  {19,"Net_Power_L2","myhouse/victron_cerbo_N/ed3a43se2345/vebus/276/Ac/ActiveIn/L2/P"},
  {20,"Net_Power_L3","myhouse/victron_cerbo_N/ed3a43se2345/vebus/276/Ac/ActiveIn/L3/P"},
  {21,"Net_Voltage_L1","myhouse/victron_cerbo_N/ed3a43se2345/vebus/276/Ac/ActiveIn/L1/V"},
  {22,"Net_Voltage_L2","myhouse/victron_cerbo_N/ed3a43se2345/vebus/276/Ac/ActiveIn/L2/V"},
  {23,"Net_Voltage_L3","myhouse/victron_cerbo_N/ed3a43se2345/vebus/276/Ac/ActiveIn/L3/V"},
  {24,"Net_Current_L1","myhouse/victron_cerbo_N/ed3a43se2345/vebus/276/Ac/ActiveIn/L1/I"},
  {25,"Net_Current_L2","myhouse/victron_cerbo_N/ed3a43se2345/vebus/276/Ac/ActiveIn/L2/I"},
  {26,"Net_Current_L3","myhouse/victron_cerbo_N/ed3a43se2345/vebus/276/Ac/ActiveIn/L3/I"},
  {27,"DC_Power","myhouse/victron_cerbo_N/ed3a43se2345/system/0/Dc/System/Power"},
  {28,"LEDs_Inverter","myhouse/victron_cerbo_N/ed3a43se2345/vebus/276/Leds/Inverter"},
  {29,"Bat_State","myhouse/victron_cerbo_N/ed3a43se2345/settings/0/Settings/CGwacs/BatteryLife/State"},
};


/******* Oleds ******/
SSD1322_I2C oled1(7, 6, 5, 4, 3,0x20); // rst, bl, en, rw, di, i2caddr
SSD1322_I2C oled2(7, 6, 5, 4, 3,0x21); // rst, bl, en, rw, di, i2caddr
SSD1322_I2C oled3(7, 6, 5, 4, 3,0x22); // rst, bl, en, rw, di, i2caddr
SSD1322_I2C oled4(7, 6, 5, 4, 3,0x23); // rst, bl, en, rw, di, i2caddr

/******* global vars ******/
double power_net_sum = 0; //to calculate mean power
double power_load_sum = 0;
double power_pv_sum = 0;
double soc_sum = 0;
double graph_array[4];
byte power_array_counter = 0;
byte power_array_nr_values = 72;
byte array_net[121];
byte array_load[121];
byte array_pv[121];
byte array_soc[121];
bool flag_night = false;
byte hour_sleep = 22;
byte hour_awake = 6;
byte vertical_line_x = 0;
