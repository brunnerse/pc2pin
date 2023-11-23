/*
 * Connects to Smartphone Hotspot per Wifi, then sets up a server the app ESPController can connect to.
 * 
 * LED D0 (Lower LED):  Is On when the server is waiting for a new connection
 * LED D4 (Upper LED): Is On when the server sets up the Wifi Connection and the Server and when the engine is on
 * 
 */
#include <WiFi.h>
#include <string.h>
#include "config.h" 

#define NUM_PINS 25

int mode[NUM_PINS];
int outVals[NUM_PINS];

enum _state {
	CONNECTED, DISCONNECTED
} state;


#define ACK '.' //acknowledge byte, sent every second so the client knows the server didnt time out

#define PIN_LED 2
#define LED_BLINK_RATE 3 

void sendf(WiFiClient& client, const char* fstr, int arg1=0, int arg2=0, int arg3=0) {
  char str[100];
  int len = snprintf(str, 100, fstr, arg1, arg2, arg3);
  Serial.printf("Sending data %s\n", str);
  client.write(str, len);
}

class LEDManager {
	int pin;
	bool isOn;
	int lastCycleSwitched;

	LEDManager(int pin) :
		pin(pin), isOn(false), lastCycleSwitched(0) 
  {
		pinMode(pin, OUTPUT);
	}

	void on() {
		isOn = true;
		digitalWrite(this->pin, 1);
	}
	void off() {
		isOn = true;
		digitalWrite(this->pin, 1);
	}

	void toggle() {
		if (isOn)
			off();
		else
			on();
	}

	void blink(int cycleNr, int cycleRate) {
		// TODO
	}
};



void flush(WiFiClient client);
void findClient();


bool isLedOn = false;
WiFiServer server(SERVER_PORT, 1);
WiFiClient client;
int loopsSinceACK = 0;

void setup() {
	// init pins
	pinMode(PIN_LED, OUTPUT);
	//TODO use LEDManager instead
  	for (int i = 0; i < NUM_PINS; i++)
    	mode[i] = -1;

	//delay(10);
#ifdef DEBUG  
    Serial.begin(19200);
#endif

  #if ACCESSPOINT
      IPAddress local_IP(192, 168, 0, 1);
      IPAddress gateway(192, 168, 9, 4); //TODO what is the gateway?
      IPAddress subnet(255, 255, 255, 0);

      #ifdef DEBUG
        Serial.print("Setting soft-AP configuration ... ");
      #endif
      bool ret = WiFi.softAPConfig(local_IP, gateway, subnet);
      #ifdef DEBUG
        Serial.println(ret ? "Ready" : "Failed!");
        Serial.print("Setting soft-AP.. ");
      #endif
      //delay(20);
      ret = WiFi.softAP(SSID, PASSWORD, 1, false);
    
      #ifdef DEBUG
        Serial.println(ret ? "Ready" : "Failed!");
        Serial.print("Soft-AP IP address = ");
        Serial.println(WiFi.softAPIP());
      #endif
  #else
        #ifdef DEBUG
            Serial.printf("Connecting to %s...\n", SSID);
        #endif
        
        WiFi.begin(SSID, PASSWORD);
        delay(20);
        while (WiFi.status() != WL_CONNECTED) {
            delay(200);
            #ifdef DEBUG
                Serial.print(".");
            #endif
        }
        #ifdef DEBUG
            Serial.println("\nWiFi connected");
        #endif
        #ifdef DEBUG
            Serial.print("IP address: ");
            Serial.println(WiFi.localIP());
        #endif
    #endif
    delay(20);
    // Start the server
    #ifdef DEBUG
        Serial.println("Starting server...");
    #endif
    server.begin();
    server.setNoDelay(true);
  
    while (!server)
      delay(50);
    #ifdef DEBUG
      Serial.printf("no delay: %d\n", server.getNoDelay());
      Serial.println("Server started");
    #endif

    //digitalWrite(PIN_LED, HIGH);

	state = DISCONNECTED;
}


void loop() {

  // manage wifi clients
	switch (state) {
		case DISCONNECTED:
			checkClientAvailable();
			if (client) {
				state = CONNECTED;
				#ifdef DEBUG
					Serial.print("Connected to Client at ");
					Serial.println(client.remoteIP());
				#endif
				// TODO make LED on
			}
			//TODO make LED blink
			return;
		case CONNECTED:
			if (!client) {
				state = DISCONNECTED;
				#ifdef DEBUG
					Serial.println("Client disconnected.");
				#endif
				// TODO make all outputs off?
				return;
			}
      // TODO check if client timed out 
    	loopsSinceACK++;
			if (loopsSinceACK == -1) { //client timed out
				#ifdef DEBUG
					Serial.println("Client timed out.");
				#endif
				client.stop();
				client = WiFiClient();
				return;
			}
			// TODO check if another client wants to connect (server.available)
			// and reject the client; or connect, send error message and disconnect
    		// as the program allows only one Client to be connected at the same time
			//    WiFiClient::stopAllExcept(&client);
			// TODO send ack periodically
    		//client.write(ACK);
			break;
	}

  delay(10);

	// process client messages
	if (state == CONNECTED && client.available()) {           
		char action = client.read(); //first char says left or right motor
    Serial.printf("Received %c\n", action);

		//Test if it was just an ACKnowledge byte sent to confirm the connection didnt timeout
		// TODO just ignore ACK and instead count cycles until client.available?
		// this would mean client/server can avoid ACK if another piece of data is sent
		if (action == ACK) {
			loopsSinceACK = 0;
		} else {
			// TODO find right functions
			int pin = client.parseInt();
      client.readStringUntil(' ');
			String value = client.readStringUntil(' ');
      int repeat = client.parseInt();
      int delayVal = client.parseInt();

      client.readStringUntil('\n');


			value.trim();
			value.toLowerCase();

      
    bool error = false;

      for (int i = 0; i < repeat && !error; i++) {
        switch (action) {
          case 'r':
            if (mode[pin] == 'r' || mode[pin] == 'p') {
              sendf(client, "IN [Pin %2d]: %d\n", pin, digitalRead(pin));
            } else {
              sendf(client, "[ERROR] Pin %2d is not set to input\n", pin);
              error = true;
            }
            break;
          case 'w':
            if (mode[pin] == 'w') {
              if (value == "n"){
                sendf(client, "OUT [Pin %2d]: %d\n", pin, outVals[pin]);
                break;
              }
              unsigned int outVal = (value == "t") ? !outVals[pin] : atoi(value.c_str());
              if (outVal > 1) {
                sendf(client, "[ERROR] Value %d too high; use mode A for digital writing\n", outVal);
                error = true;
              } else {
                digitalWrite(pin, outVal);
                sendf(client, "OUT [Pin %2d]: %d\n", pin, outVal);
                outVals[pin] = outVal;
              }
            } else {
              sendf(client, "[ERROR] Pin %d is not set to output\n", pin);
              error = true;
            }
            break;
          case 'a':
            if (mode[pin] != 'w') {
              Serial.printf("Analog val of %d: %d\n", pin, analogRead(pin));
              sendf(client, "IN A [Pin %2d]: %4d\n", pin, analogRead(pin));
            } else {
              sendf(client, "[ERROR] Cannot read analog: Pin %2d is set to output\n", pin);
              error = true;
            }
            break;
          case 'A':
            if (mode[pin] == 'w') {
              unsigned int outVal = atoi(value.c_str());
              if (outVal > 255) {
                sendf(client, "[ERROR] Value %d too high; analog value must be in range 0-255\n", outVal);
                error = true;
              } else {
                analogWrite(pin, outVal);
                sendf(client, "OUT A [Pin %2d]: %3d\n", pin, outVal);
                outVals[pin] = outVal;
              }
            } else {
              sendf(client, "[ERROR] Pin %d is not set to output\n", pin);
              error = true;
            }
            break;
          case 'p':
            if (value.endsWith("pullup")) {
              pinMode(pin, INPUT_PULLUP);
              sendf(client, "PINMODE [Pin %d]: input_pullup\n", pin);
              mode[pin] = 'p';
            }
            else if (value.startsWith("in")) {
              pinMode(pin, INPUT);
              sendf(client, "PINMODE [Pin %d]: input\n", pin);
              mode[pin] = 'r';
            } else if (value.startsWith("out")) {
              pinMode(pin, OUTPUT);
              sendf(client, "PINMODE [Pin %d]: output\n", pin);
              mode[pin] = 'w';
            } else {
              sendf(client, "[ERROR] Pin mode %s invalid\n", (int)value.c_str());
              error = true;
            }
            break;
          default:
            sendf(client, "[ERROR] Command invalid\n");
            error = true;
        }
        delay(delayVal);
      }
    }
  }
  delay(10);
}


bool checkClientAvailable() {
	client = server.available();
	if (client) {
    	sendf(client, "ESP_SERVER %s\n", (int)VERSION);
		bool clientApproved = false;
		for (int i = 0; i < 200; ++i) { //Wait 1 Second for Message By client
			delay(5);
			if (client.available()) {
				String response = client.readStringUntil('\n');
				Serial.println(response);
				if (response.equals(String("ESP_CLIENT ") + VERSION)) {
					clientApproved = true;
				}
				break;
			}
		}
		if (!clientApproved) {
			sendf(client, "Client does not match server, requires ESP_CLIENT %s\n", (int)VERSION);
			// reset client
			client.stop();
			client = WiFiClient();
		} else {
			return true;
		}
	}
	return false;
}


void flush(WiFiClient client) {
    char c;
    do {
        c = client.read();
    } while (c != '\n' && c != -1);
}
