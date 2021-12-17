#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Pangodream_18650_CL.h>

#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/Dialog_plain_112.h>

// select the display class (only one), matching the kind of display panel
#define GxEPD2_DISPLAY_CLASS GxEPD2_BW

// select the display driver class (only one) for your  panel
#define GxEPD2_DRIVER_CLASS GxEPD2_750_T7  // GDEW075T7   800x480

// somehow there should be an easier way to do this
#define GxEPD2_BW_IS_GxEPD2_BW true
#define GxEPD2_3C_IS_GxEPD2_3C true
#define GxEPD2_7C_IS_GxEPD2_7C true
#define GxEPD2_1248_IS_GxEPD2_1248 true
#define IS_GxEPD(c, x) (c##x)
#define IS_GxEPD2_BW(x) IS_GxEPD(GxEPD2_BW_IS_, x)
#define IS_GxEPD2_3C(x) IS_GxEPD(GxEPD2_3C_IS_, x)
#define IS_GxEPD2_7C(x) IS_GxEPD(GxEPD2_7C_IS_, x)
#define IS_GxEPD2_1248(x) IS_GxEPD(GxEPD2_1248_IS_, x)

#define svetaine_temp_topic "skaidiskes/meteo/svetaine/sensor/svetains_temperatra/state"
#define svetaine_dregme_topic "skaidiskes/meteo/svetaine/sensor/svetains_drgm/state"
#define laukas_temp_topic "skaidiskes/meteo/laukas/temp"
#define laukas_dregme_topic "skaidiskes/meteo/laukas/dregme"
#define laukas_slegis_topic "skaidiskes/meteo/laukas/slegis"
#define siltnamis_temp_topic "skaidiskes/meteo/siltnamis/temperatura"
#define baseinas_temp_topic "skaidiskes/meteo/baseinas/temp"
#define spyna_state_topic "skaidiskes/namai/spyna"
#define programos_testas "programos/testas"



#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  900        /* Time ESP32 will go to sleep (in seconds) */
#define Threshold 20

//const char* ssid = "Telia-F49C91-Greitas";
//const char* password = "11059C1715";

const char * ssid = "wifimesh";
const char* password = "EQR73445B5H";

const char * mqtt_server = "192.168.8.159";
const char * mqtt_user = "vilkas";
const char * mqtt_password = "labas123";
const int mqtt_port = 1883;
const char * willTopic = "$CONNECTED/CLIENT_ID";
const char * solar_web = "http://192.168.8.137/solar_api/v1/GetPowerFlowRealtimeData.fcgi";
const char * oruAPI = "http://api.openweathermap.org/data/2.5/weather?q=vilnius&units=metric&appid=35ac972ed30a9ed3e6ad8d09a895abf1";
const char * dienaAPI = "https://savaites-diena-api.herokuapp.com/savaitesdiena";
const char * menuoAPI = "https://savaites-diena-api.herokuapp.com/menuo";
const char * suo = "https://testas981.herokuapp.com/?pirmas=jonas&antras=petras";


WiFiClient espClient;
WiFiClientSecure clientA;
PubSubClient client(espClient);

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
HTTPClient http;
Pangodream_18650_CL BL;

#if defined(ESP32)
#define MAX_DISPLAY_BUFFER_SIZE 65536ul // e.g.
#if IS_GxEPD2_BW(GxEPD2_DISPLAY_CLASS)
#define MAX_HEIGHT(EPD) (EPD::HEIGHT <= MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8) ? EPD::HEIGHT : MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8))
#elif IS_GxEPD2_3C(GxEPD2_DISPLAY_CLASS)
#define MAX_HEIGHT(EPD) (EPD::HEIGHT <= (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 8) ? EPD::HEIGHT : (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 8))
#elif IS_GxEPD2_7C(GxEPD2_DISPLAY_CLASS)
#define MAX_HEIGHT(EPD) (EPD::HEIGHT <= (MAX_DISPLAY_BUFFER_SIZE) / (EPD::WIDTH / 2) ? EPD::HEIGHT : (MAX_DISPLAY_BUFFER_SIZE) / (EPD::WIDTH / 2))
#endif
GxEPD2_DISPLAY_CLASS<GxEPD2_DRIVER_CLASS, MAX_HEIGHT(GxEPD2_DRIVER_CLASS)> display(GxEPD2_DRIVER_CLASS(/*CS=*/ 15, /*DC=*/ 27, /*RST=*/ 26, /*BUSY=*/ 25));
#endif

#include "naujas_epaper_ekranas.h" // 7.5"  b/w

void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.println("setup");
  display.init(115200); // uses standard SPI pins, e.g. SCK(18), MISO(19), MOSI(23), SS(5)
  // *** special handling for Waveshare ESP32 Driver board *** //
  // ********************************************************* //
  SPI.end(); // release standard SPI pins, e.g. SCK(18), MISO(19), MOSI(23), SS(5)
  //SPI: void begin(int8_t sck=-1, int8_t miso=-1, int8_t mosi=-1, int8_t ss=-1);
  SPI.begin(13, 12, 14, 15); // map and init SPI pins SCK(13), MISO(12), MOSI(14), SS(15)
  // *** end of special handling for Waveshare ESP32 Driver board *** //
  // **************************************************************** //
  // first update should be full refresh

  delay(1000);

  setup_wifi();
  //  clientA.setCACert(root_ca);
  Serial.println("setup");
  delay(100);

  timeClient.begin();
  delay(10);
  timeClient.setTimeOffset(7200); //turi buti 7200

  delay(100);


  paveiksliukas();


  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  delay(100);

  reconnect();

  delay(100);
  // Initialize a NTPClient to get time

  int var = 0;
  while (var < 10000) {
    client.loop();
    var ++;
  }

  delay(100);
  solar_http();
  delay(100);
  ntp();
  ntp_time();
  grazintiMenesioSkaiciu();
  grazintiMenesioPavadinima();
  gautiOruPrognoze();
  dienosPavadinimas();

  display.powerOff();

  Serial.println("setup done");
}

void loop()
{
}

void paveiksliukas() {

  display.setFullWindow(); // Set full window mode, meaning is going to update the entire screen
  display.firstPage(); // Tell the graphics class to use paged drawing mode
  do {
    // Put everything you want to print in this screen:

    // Print image:
    display.fillScreen(GxEPD_WHITE); // Clear previous graphics to start over to print new things.
    // Format: (POSITION_X, POSITION_Y, IMAGE_NAME, IMAGE_WIDTH, IMAGE_HEIGHT, COLOR)
    // Color options are GxEPD_BLACK, GxEPD_WHITE, GxEPD_RED
    display.drawBitmap(0, 0, gImage_naujas_epaper_ekranas, 800, 480, GxEPD_BLACK);

    float Body_Data_Site_E_Day = solar_http();
    String dayStamp = ntp();
    String timeStamp = ntp_time();
    String menesioDiena = grazintiMenesioSkaiciu();
    String menesioPavadinimas = grazintiMenesioPavadinima();
    float laipsniai = gautiOruPrognoze();
    int laipsniaiInt = (int) laipsniai;
    String laipsniaiString = String (laipsniaiInt);
    String savaitesDiena = dienosPavadinimas();

    String belekas = "1";

    Serial.println("===============");
    Serial.println(laipsniaiInt);
    Serial.println("=================");


    float calculated = Body_Data_Site_E_Day / 1000;

    display.setTextColor(GxEPD_BLACK); // Set color for text
    display.setFont( & Dialog_plain_112); // Set font
    display.setCursor(460, 300); // saules kwh pozicija (x,y)
    display.print(calculated); // calculated

    //        display.setCursor(220, 300);  // svetaines temp pozicija (x,y)
    //        display.print("-88");  // Print some text

  int16_t x = 280; //220
  int16_t y = 140;  //140
  int16_t x1 = 278; //218
  int16_t y1 = 138;
  uint16_t width = 800;  //175
  uint16_t height = 105;
     
    display.getTextBounds(laipsniaiString, x, y, &x1, &y1, &width, &height);
    display.setCursor(x - width /3, y);  // lauko temp pozicija (x,y)
    display.print(laipsniaiString);  // Print some text

    //       display.setCursor(415, 140);  // siltnamio temp pozicija (x,y)
    //       display.print("-88");  // Print some text

    //       display.setCursor(635, 140);  // baseino temp pozicija (x,y)
    //       display.print("88");  // Print some text

    display.setCursor(40, 160);  // savaites dienos numeris pozicija (x,y)
    display.print(menesioDiena);  // Print some text

    display.setFont( & FreeMonoBold12pt7b);
    display.setCursor(60, 60);  // savaites dienos pavadinimas pozicija (x,y)
    display.print(menesioPavadinimas);  // Print some text

    display.setCursor(30, 190);  // savaites dienos pavadinimas pozicija (x,y)
    display.print(savaitesDiena);  // Print some text

//    display.setCursor(30, 240);  // baseino temp pozicija (x,y)
//    display.print("SKAIDISKES");  // Print some text
//
//    display.setCursor(30, 280);  // baseino temp pozicija (x,y)
//    display.print(laipsniaiInt);  // Print some text

    display.setFont( & FreeMono9pt7b);
    display.setCursor(80, 470); // atnaujinta (x,y)
    display.print(dayStamp); // Print some text (dayStamp)

    display.setCursor(192, 470); // atnaujinta (x,y)
    display.print("--"); // Print some text

    display.setCursor(220, 470); // atnaujinta (x,y)
    display.print(timeStamp); // Print some text (timeStamp)

    display.setCursor(380, 470); // baterija (x,y)
    display.print(BL.getBatteryChargeLevel()); // Print some text (BL.getBatteryChargeLevel());

    

  }
  while (display.nextPage()); // Print everything we set previously
  // End of screen 2
}

// ============================================================

void setup_wifi() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  //WiFi.config(local_IP, gateway, subnet, dns);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

}

/********************************** START CALLBACK*****************************************/
void callback(char * topic, byte * payload, unsigned int length) {
  payload[length] = '\0';
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);

  Serial.print("Message: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char) payload[i]);
  }

  Serial.println();
  Serial.println("-----------------------");

  mqtt_siltnamis(topic, payload);  //perduodu pointerius is funkcijos callback? Ar perduodu cia char masyvus?
  mqtt_baseinas(topic, payload);
  //mqtt_laukas_t(topic, payload);
  mqtt_svetaine_t(topic, payload);
  mqtt_lauko_durys(topic, payload);
}

void mqtt_siltnamis(char *topic, byte *payload) {

  String stringPranesimas = String((char*) payload);
  int skaiciai = stringPranesimas.toInt();
  String strTopic = String((char * ) topic);

  if (strTopic == siltnamis_temp_topic) {

    Serial.print("mqtt_proceso pranesimas yra: ");
    Serial.println(skaiciai);

    display.setPartialWindow(425, 50, 180, 105);
    display.setTextColor(GxEPD_BLACK); // Set color for text
    display.setFont( & Dialog_plain_112); // Set font
    display.firstPage();
    do
    {
      display.setCursor(420, 140);
      display.print(skaiciai);
    }
    while (display.nextPage());

    
  }
}

void mqtt_baseinas(char * topic, byte * payload) {

  String stringPranesimas = String((char*) payload);
  int skaiciai = stringPranesimas.toInt();
  String strTopic = String((char * ) topic);

  if (strTopic == baseinas_temp_topic) {

    Serial.print("mqtt_proceso pranesimas yra: ");
    Serial.println(skaiciai);

    display.setPartialWindow(645, 50, 175, 105);
    display.setTextColor(GxEPD_BLACK); // Set color for text
    display.setFont( & Dialog_plain_112); // Set font
    display.firstPage();
    do
    {
      display.setCursor(635, 140);
      display.print(skaiciai);
    }
    while (display.nextPage());
  }
}
void mqtt_laukas_t(char * topic, byte * payload) {

  String stringPranesimas = String((char*) payload);
  int skaiciai = stringPranesimas.toInt();
  String strTopic = String((char * ) topic);

  if (strTopic == laukas_temp_topic) {

    Serial.print("mqtt_proceso pranesimas yra: ");
    Serial.println(skaiciai);

    display.setPartialWindow(230, 50, 175, 105);
    display.setTextColor(GxEPD_BLACK); // Set color for text
    display.setFont( & Dialog_plain_112); // Set font
    display.firstPage();
    do
    {
      display.setCursor(220, 140);
      display.print(skaiciai);
    }
    while (display.nextPage());

  }
}
void mqtt_svetaine_t(char * topic, byte * payload) {

  String stringPranesimas = String((char*) payload);
  int skaiciai = stringPranesimas.toInt();
  String strTopic = String((char * ) topic);

  if (strTopic == svetaine_temp_topic) {

    Serial.print("mqtt_proceso pranesimas yra: ");
    Serial.println(skaiciai);

    display.setPartialWindow(230, 200, 175, 105);
    display.setTextColor(GxEPD_BLACK); // Set color for text
    display.setFont( & Dialog_plain_112); // Set font
    display.firstPage();
    do
    {
      display.setCursor(250, 300);
      display.print(skaiciai);
    }
    while (display.nextPage());
  }
}
void mqtt_lauko_durys(char * topic, byte * payload) {

  String msg;
  String strTopic;

  strTopic = String((char * ) topic);
  if (strTopic == spyna_state_topic) {

    msg = String((char * ) payload);
    Serial.print("mqtt_proceso pranesimas yra: ");
    Serial.println(msg);

    display.setPartialWindow(630, 455, 200, 50);
    display.setTextColor(GxEPD_BLACK); // Set color for text
    display.setFont(&FreeMonoBold12pt7b); // Set font display.setFont(&FreeMonoBold12pt7b);
    display.firstPage();
    do
    {
      display.setCursor(630, 470);
      display.print(msg);
    }
    while (display.nextPage());

  }
}

/********************************** START RECONNECT*****************************************/
void reconnect() {
  // Loop until we're reconnected

  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");

    if (client.connect("ESP32Client", mqtt_user, mqtt_password)) {

      Serial.println("connected");

    } else {

      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);

    }
  }
  Serial.println("Pradedame prenumeruoti topikus: ");

  client.subscribe(svetaine_temp_topic);
  client.subscribe(svetaine_dregme_topic);
  client.subscribe(laukas_temp_topic);
  client.subscribe(laukas_dregme_topic);
  client.subscribe(laukas_slegis_topic);
  client.subscribe(siltnamis_temp_topic);
  client.subscribe(baseinas_temp_topic);
  client.subscribe(spyna_state_topic);
  client.subscribe(programos_testas);
}

String ntp() {
  // Variables to save date and time

  String formattedDate;
  String dayStamp;
  String timeStamp;

  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }
  // The formattedDate comes with the following format:
  // 2018-05-28T16:00:13Z
  // We need to extract date and time
  formattedDate = timeClient.getFormattedDate();
  //Serial.println(formattedDate);

  // Extract date
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  Serial.print("ATNAUJINTA: ");
  Serial.print(dayStamp);

  return dayStamp;

}

String ntp_time() {
  // Variables to save date and time

  String formattedDate;
  String timeStamp;
  String dayStamp;

  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }
  // The formattedDate comes with the following format:
  // 2018-05-28T16:00:13Z
  // We need to extract date and time
  formattedDate = timeClient.getFormattedDate();
  //Serial.println(formattedDate);
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  // Extract time
  timeStamp = formattedDate.substring(splitT + 1, formattedDate.length() - 1);
  Serial.print("--");
  Serial.println(timeStamp);
  return timeStamp;

  delay(100);

}

String grazintiMenesioSkaiciu() {
  // Variables to save date and time

  String formattedDate;
  String dayStamp;

  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }
  formattedDate = timeClient.getFormattedDate();

  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  Serial.println();

  String dienos = dayStamp.substring(8);
  Serial.print("MENESIO DIENA: ");
  Serial.print(dienos);
  Serial.println();

  return dienos;

}

String grazintiMenesioPavadinima() {
  // Variables to save date and time

  String formattedDate;
  String dayStamp;
  char * menesiuPavadinimai[] = {"NULL", "SAUSIS", "VASARIS", "KOVAS", "BALANDIS", "GEGUŽĖ", "BIRŽELIS", "LIEPA", "RUGPJŪTIS", "RUGSĖJIS", "SPALIS", "LAPKRITIS", "GRUODIS"};

  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }
  formattedDate = timeClient.getFormattedDate();

  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);

  String menesis = dayStamp.substring(5, 7);

  int menesisSK = menesis.toInt();
  //  Serial.println("DABARTINIO MENESIO PAVADINIMAS YRA: ");
  //  Serial.println(menesisSK);
  //  Serial.println("----****--------");
  //  Serial.println(menesiuPavadinimai[menesisSK]);
  //  Serial.println("----------------");

  return menesiuPavadinimai[menesisSK];

}

float solar_http() {

  http.begin(espClient, solar_web); //Specify the URL
  int httpCode = http.GET(); //Make the request

  if (httpCode > 0) { //Check for the returning code

    String json = http.getString();
    Serial.println(httpCode);
    //    Serial.println(json); // reikalingas tik parodyti uzklausos turini, testavimui

    const size_t capacity = JSON_OBJECT_SIZE(0) + 2 * JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(2) + 3 * JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(11) + 490;
    DynamicJsonBuffer jsonBuffer(capacity);

    JsonObject & root = jsonBuffer.parseObject(json);

    JsonObject & Body_Data = root["Body"]["Data"];

    JsonObject & Body_Data_Site = Body_Data["Site"];
    float Body_Data_Site_E_Day = Body_Data_Site["E_Day"];

    Serial.print("Šios dienos duomenys yra: ");
    Serial.println(Body_Data_Site_E_Day);

    return Body_Data_Site_E_Day;

  } else {
    Serial.println("Error on HTTP request");
  }

  http.end(); //Free the resources
}

float gautiOruPrognoze() {

  http.begin(oruAPI); //Specify the URL
  int httpCode = http.GET(); //Make the request

  if (httpCode > 0) { //Check for the returning code

    String json = http.getString();
    Serial.println(httpCode);
    //    Serial.println(json); // reikalingas tik parodyti uzklausos turini, testavimui

    const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(1) + 2 * JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(6) + JSON_OBJECT_SIZE(13) + 410;
    DynamicJsonBuffer jsonBuffer(capacity);

    //const char* json = "{\"coord\":{\"lon\":25.2798,\"lat\":54.6892},\"weather\":[{\"id\":701,\"main\":\"Mist\",\"description\":\"mist\",\"icon\":\"50d\"}],\"base\":\"stations\",\"main\":{\"temp\":-2.83,\"feels_like\":-2.83,\"temp_min\":-2.89,\"temp_max\":-2.38,\"pressure\":1025,\"humidity\":100},\"visibility\":3000,\"wind\":{\"speed\":0.51,\"deg\":0},\"clouds\":{\"all\":90},\"dt\":1639387402,\"sys\":{\"type\":2,\"id\":2001071,\"country\":\"LT\",\"sunrise\":1639377240,\"sunset\":1639403541},\"timezone\":7200,\"id\":593116,\"name\":\"Vilnius\",\"cod\":200}";



    JsonObject& root = jsonBuffer.parseObject(json);

    float coord_lon = root["coord"]["lon"]; // 25.2798
    float coord_lat = root["coord"]["lat"]; // 54.6892

    JsonObject& weather_0 = root["weather"][0];
    int weather_0_id = weather_0["id"]; // 701
    const char* weather_0_main = weather_0["main"]; // "Mist"
    const char* weather_0_description = weather_0["description"]; // "mist"
    const char* weather_0_icon = weather_0["icon"]; // "50d"

    const char* base = root["base"]; // "stations"

    JsonObject& main = root["main"];
    float main_temp = main["temp"]; // -2.83
    float main_feels_like = main["feels_like"]; // -2.83
    float main_temp_min = main["temp_min"]; // -2.89
    float main_temp_max = main["temp_max"]; // -2.38
    int main_pressure = main["pressure"]; // 1025
    int main_humidity = main["humidity"]; // 100

    int visibility = root["visibility"]; // 3000

    float wind_speed = root["wind"]["speed"]; // 0.51
    int wind_deg = root["wind"]["deg"]; // 0

    int clouds_all = root["clouds"]["all"]; // 90

    long dt = root["dt"]; // 1639387402

    JsonObject& sys = root["sys"];
    int sys_type = sys["type"]; // 2
    long sys_id = sys["id"]; // 2001071
    const char* sys_country = sys["country"]; // "LT"
    long sys_sunrise = sys["sunrise"]; // 1639377240
    long sys_sunset = sys["sunset"]; // 1639403541

    int timezone = root["timezone"]; // 7200
    long id = root["id"]; // 593116
    const char* name = root["name"]; // "Vilnius"
    int cod = root["cod"]; // 200


    Serial.println("-----------------");
    Serial.println("ORO TEMPERATURA");
    Serial.println(main_temp);
    Serial.println(sys_country);
    Serial.println("-----------------");

    //float prognoze[] = {main_temp,main_feels_like,main_humidity,main_temp_max,main_temp_min};
    return main_temp;
  }
  else {
    Serial.println("NEGAUTA JOKIO ATSAKYMO IS API");
  }
  http.end(); //Free the resources
}

String dienosPavadinimas() {

  http.begin(clientA, dienaAPI); //Specify the URL
  int httpCode = http.GET(); //Make the request

  if (httpCode > 0) { //Check for the returning code

    String json = http.getString();
//    Serial.println("kodas ===========>>>>");
//    Serial.println(httpCode);
//    Serial.println("payload ===========>>>>");
//    Serial.println(json); // reikalingas tik parodyti uzklausos turini, testavimui

const size_t capacity = JSON_OBJECT_SIZE(1) + 40;
DynamicJsonBuffer jsonBuffer(capacity);

//const char* json = "{\"savaitesdiena\":\"KETVIRTADIENIS\"}";

JsonObject& root = jsonBuffer.parseObject(json);

const char* savaitesdiena = root["savaitesdiena"]; // "KETVIRTADIENIS"

    return savaitesdiena;
  }
  else {
    Serial.println("NEGAUTA JOKIO ATSAKYMO IS API");
  }
  http.end(); //Free the resources
}
