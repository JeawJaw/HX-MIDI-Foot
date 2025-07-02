#include <SPIFFS.h>
#include <Arduino.h>
//#include <MIDI.h>
#include <PCF8574.h>
#include <Wire.h>
#include <Preferences.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include "page_string.h"
#include <AxeFxControl.h>

#define SwitchChip_Addr 0x38  //PCF8574AT
#define LEDChip_Addr 0x39  //PCF8574AT

#define ESP32_INTERRUPTED_PIN 23
#define ESP32_LED_STATUS_PIN 2
#define ESP32_LED_ACTIVITY_PIN 15

//MIDI_CREATE_INSTANCE(HardwareSerial, Serial2, midiA);

Preferences switch_data;

AxeSystem Axe;

void keyPressedOnPCF8574();

PCF8574 switchChip_input(SwitchChip_Addr, ESP32_INTERRUPTED_PIN, keyPressedOnPCF8574);
PCF8574 ledChip_output(LEDChip_Addr);

bool keyPressed = false;
bool connection_status = false;
unsigned long timeElapsed;

uint8_t led_data = 0x00;

uint8_t timeout_count = 0;

static byte midi_channel = 0;

byte switch_state = 0x00;

int current_pin_pressed =0;

const char* ssid = "PIGMan";
const char* password = "12345678";

/* Put IP Address details */
IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

const char* host = "pigman";

AsyncWebServer server(80);

void notFound(AsyncWebServerRequest *request) 
{
  request->send(404, "text/plain", "Not found");
}

void fw_update(AsyncWebServerRequest *request)
{
  Serial.println("--------- Update Firmware Page Response ----------------");

  AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", update_html);
  //response->addHeader("Content-Encoding", "gzip");
  request->send(response);

  //request->send_P(200, "text/html", update_html);
}

void update_req(AsyncWebServerRequest *request)
{
    //request->send_P(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    //ESP.restart();
    AsyncWebServerResponse *response = request->beginResponse((Update.hasError())?500:200, "text/plain", (Update.hasError())?"FAIL":"OK");
    response->addHeader("Connection", "close");
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
    ESP.restart();
}

void update_upload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
    if (!index) 
    {
      int cmd = U_FLASH;       
      if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) // Start with max available size
      { 
        Update.printError(Serial);
        return request->send(400, "text/plain", "OTA could not begin");
      }
    } 

    if (len) 
    {
      // แฟลช(เบิร์นโปรแกรม)ลง ESP32
      if (Update.write(data, len) != len) 
      {
        Update.printError(Serial);
        return request->send(400, "text/plain", "OTA could not write");
      }
    } 

    if (final) 
    {
      if (Update.end(true)) 
      { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", len);
      } 
      else 
      {
        Update.printError(Serial);
        return request->send(400, "text/plain", "Could not end OTA");
      }
    }
  
}

void WIFI_Setup()
{
  WiFi.mode(WIFI_AP);
  //WiFi.begin(ssid, password);
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);

  if (!MDNS.begin(host)) 
  { //http://pigman.local
    Serial.println("Error setting up MDNS responder!");
    while (1) 
    {
      delay(1000);
    }
  }
  else 
  {
    Serial.println("mDNS responder started");  
  }
}

void WebServer_Setup()
{
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) 
  {
    //request->send(200, "text/plain", "Hi! I am ESP32.");
    request->redirect("/update");
  });

  // server.on("/src/bootstrap.bundle.min.js", HTTP_GET, [](AsyncWebServerRequest *request)
  // {
  //   request->send(SPIFFS, "/src/bootstrap.bundle.min.js", "text/javascript");
  // });
 
  // server.on("/src/jquery-3.7.1.min.js", HTTP_GET, [](AsyncWebServerRequest *request)
  // {
  //   request->send(SPIFFS, "/src/jquery-3.7.1.min.js", "text/javascript");
  // });
 
  // server.on("/src/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest *request)
  // {
  //   request->send(SPIFFS, "/src/bootstrap.min.css", "text/css");
  // });

  // server.on("/src/PIGManLogo.png", HTTP_GET, [](AsyncWebServerRequest *request)
  // {
  //   request->send(SPIFFS, "/src/PIGManLogo.png", "image/png");
  // });

  // server.on("/css/fontawesome.css", HTTP_GET, [](AsyncWebServerRequest *request)
  // {
  //   request->send(SPIFFS, "/css/fontawesome.css", "text/css");
  // });

  server.on("/update", HTTP_GET, fw_update);
  server.onNotFound(notFound);
  
  server.on("/update_submit", HTTP_POST, update_req, update_upload);

  server.begin();
}

void Chip_Setup()
{
  switchChip_input.pinMode(P0, INPUT_PULLUP);
	switchChip_input.pinMode(P1, INPUT_PULLUP);
  switchChip_input.pinMode(P2, INPUT_PULLUP);
  switchChip_input.pinMode(P3, INPUT_PULLUP);
  switchChip_input.pinMode(P4, INPUT_PULLUP);
  switchChip_input.pinMode(P5, INPUT_PULLUP);
  switchChip_input.pinMode(P6, INPUT_PULLUP);
  switchChip_input.pinMode(P7, INPUT_PULLUP);

  ledChip_output.pinMode(P0, OUTPUT, LOW);
  ledChip_output.pinMode(P1, OUTPUT, LOW);
  ledChip_output.pinMode(P2, OUTPUT, LOW);
  ledChip_output.pinMode(P3, OUTPUT, LOW);
  ledChip_output.pinMode(P4, OUTPUT, LOW);
  ledChip_output.pinMode(P5, OUTPUT, LOW);
  ledChip_output.pinMode(P6, OUTPUT, LOW);
  ledChip_output.pinMode(P7, OUTPUT, LOW);

	Serial.print(">>>> Init Switch Chip (PCF8574AT:0x38)...");
	if (switchChip_input.begin())
  {
		Serial.println("Switch Chip OK");
	}
  else
  {
		Serial.println("Switch Chip Fail !");
	}
  //delay(100);

  Serial.print(">>>> Init LED Chip (PCF8574AT:0x39)...");
  if (ledChip_output.begin())
  {
		Serial.println("LED Chip OK");
	}
  else
  {
		Serial.println("LED Chip Fail !");
	}
  ledChip_output.digitalWriteAll(led_data);
}

void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(115200);

  Serial.println("System : Setup Init");

  Chip_Setup();
	
  pinMode(ESP32_LED_STATUS_PIN, OUTPUT);
  pinMode(ESP32_LED_ACTIVITY_PIN, OUTPUT);
  digitalWrite(ESP32_LED_STATUS_PIN, LOW);
  digitalWrite(ESP32_LED_ACTIVITY_PIN, LOW);  

  Serial.println("System : Setup Exit");
  //delay(500);

  //switch_data.begin("Switch_Data", false);

  //for(int i=0;i<8;i++)
  //{    
    //switch_data.putUInt("switch"+i,i);
  //}
  
  Serial.println("System : System Start");
  digitalWrite(ESP32_LED_STATUS_PIN, HIGH);

  //midiA.begin(midi_channel);

  Axe.begin(Serial2);
  

  if(!SPIFFS.begin())
  {
     Serial.println("An Error has occurred while mounting SPIFFS");
     return;
  }
  else
  {
    Serial.println("- SPIFF Begin");
  }


  WIFI_Setup();
  WebServer_Setup();
  //AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  
}

void loop() 
{
  
  // put your main code here, to run repeatedly:
  if (keyPressed)
  { 
    //PCF8574::DigitalInput sw_val = swtichChip_input.digitalReadAll();
    byte sw_val_byte = switchChip_input.digitalReadAll();
    //Serial.println("READ VALUE FROM PCF ");
    //Serial.print(sw_val, BIN);
    keyPressed= false;

    if((sw_val_byte & bit(0)) == LOW)
    {      
      switchEngage(P0);
    }
    else if((sw_val_byte & bit(1)) == LOW)
    {     
      switchEngage(P1);      
    }
    else if((sw_val_byte & bit(2)) == LOW)
    {      
      switchEngage(P2);      
    } 
    else if((sw_val_byte & bit(3)) == LOW)
    {     
      switchEngage(P3);      
    }
    else if((sw_val_byte & bit(4)) == LOW)
    {     
      switchEngage(P4);     
    }
    else if((sw_val_byte & bit(5)) == LOW)
    {      
      switchEngage(P5);     
    }
    else if((sw_val_byte & bit(6)) == LOW)
    {      
      switchEngage(P6);     
    }
    else if((sw_val_byte & bit(7)) == LOW)
    {     
      switchEngage(P7);      
    } 
	}  
}

void debouce_switch(uint8_t p_pin)
{
  //uint8_t pin_val = swtichChip_input.digitalRead(p_pin, true);
  
  int counter = 0;       // how many times we have seen new value
  int reading;           // the current value read from the input pin
  int current_state = LOW;    // the debounced input value
  // the following variable is a long because the time, measured in milliseconds,
  // will quickly become a bigger number than can be stored in an int.
  long time = 0;         // the last time the output pin was sampled
  int debounce_count = 60; // number of millis/samples to consider before declaring a debounced input

  while(1)
  {
    if(millis() != time)
    {
        reading = switchChip_input.digitalRead(p_pin, true);
        if(reading == LOW && counter > 0)
        {
            counter--;
        }
        else if(reading == HIGH)
        {
          counter++;
        }

        if(counter >= debounce_count)
        {
          counter = 0;
          break;
        }
        time = millis();
    }   
  }
}

void ledEngage()
{
  //led_data = (1<<p_pin);
  ledChip_output.digitalWriteAll(led_data);
  //ledChip_output.digitalWrite(p_pin, led_data);
}

void switchEngage(uint8_t p_pin)
{
  Serial.println("Switch # " + String(p_pin+1) + " >> Pressed"); 

  byte cc_num = 0x00;
  byte cc_value = 0x00;
  byte ch = midi_channel;

  byte pc_num = 0x00;
  
  switch(p_pin)
  {
    case 0:
      cc_num = 82;

      switch_state = switch_state ^ (1<<p_pin); // inverse bit 
      cc_value = ((switch_state & (1<<p_pin)) >> p_pin) ? 127:0;
      led_data = switch_state;
      //ledEngage();
      Serial.printf("switch_state : %b\n", switch_state);
      //Serial.printf("led data : %b\n", led_data);
      //byte ch = midi_channel;
      //midiA.sendControlChange(cc_num, cc_value, ch);
      Axe.sendControlChange(cc_num, cc_value, midi_channel);
      
      break;
    case 1:
      cc_num = 83;
      switch_state = switch_state ^ (1<<p_pin); // inverse bit 
      cc_value = ((switch_state & (1<<p_pin)) >> p_pin) ? 127:0;
      led_data = switch_state;
      //ledEngage();

      Serial.printf("switch_state : %b\n", switch_state);
      //midiA.sendControlChange(cc_num, cc_value, ch);
      Axe.sendControlChange(cc_num, cc_value, midi_channel);
      break;
    case 2:
      cc_num = 84;
      switch_state = switch_state ^ (1<<p_pin); // inverse bit 
      cc_value = ((switch_state & (1<<p_pin)) >> p_pin) ? 127:0;
      led_data = switch_state;
      
      //ledEngage();

      Serial.printf("switch_state : %b\n", switch_state);
      //midiA.sendControlChange(cc_num, cc_value, ch);
      Axe.sendControlChange(cc_num, cc_value, midi_channel);
      break;
    case 3:
      cc_num = 85;
      switch_state = switch_state ^ (1<<p_pin); // inverse bit 
      cc_value = ((switch_state & (1<<p_pin)) >> p_pin) ? 127:0;
      led_data = switch_state;
      
      //ledEngage();

      Serial.printf("switch_state : %b\n", switch_state);
      //midiA.sendControlChange(cc_num, cc_value, ch);
      Axe.sendControlChange(cc_num, cc_value, midi_channel);
      break;
    case 4:
      pc_num = 0x00;
      //midiA.sendProgramChange(pc_num, ch);
      Axe.sendProgramChange(pc_num, midi_channel);
      switch_state = (switch_state & 0x0F) | (1<<p_pin);
      led_data = switch_state;
      break;
    case 5:
      pc_num = 0x01;
      //midiA.sendProgramChange(pc_num, ch);
      Axe.sendProgramChange(pc_num, midi_channel);
      switch_state = (switch_state & 0x0F) | (1<<p_pin);
      led_data = switch_state;
      break;
    case 6:
      pc_num = 0x02;
      //midiA.sendProgramChange(pc_num, ch);
      Axe.sendProgramChange(pc_num, midi_channel);
      switch_state = (switch_state & 0x0F) | (1<<p_pin);
      led_data = switch_state;
      break;
    case 7:
      pc_num = 0x03;
      //midiA.sendProgramChange(pc_num, ch);
      Axe.sendProgramChange(pc_num, midi_channel);
      switch_state = (switch_state & 0x0F) | (1<<p_pin);
      led_data = switch_state;
      break;
    default : break;
  }
  ledEngage();  
  debouce_switch(p_pin);
  current_pin_pressed = p_pin;
  Serial.println("Switch # " + String(p_pin+1) + " >> Released");
}

void keyPressedOnPCF8574()
{
	// Interrupt called (No Serial no read no wire in this function, and DEBUG disabled on PCF library)
	 keyPressed = true;

}