#include <Arduino.h>

/*
 * Rui Santos 
 * Complete Project Details http://randomnerdtutorials.com
 */

// Load required libraries
#include <WiFi.h>
#include <SD.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>

// Replace with your network credentials
const char *ssid = "your ssid";
const char *password = "your pass";

// uncomment one of the lines below for whatever DHT sensor type you're using
#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
//#define DHTTYPE DHT22 // DHT 22  (AM2302), AM2321

// GPIO the DHT is connected to
const int DHTPin = 15;
//intialize DHT sensor
DHT dht(DHTPin, DHTTYPE);

// create a bmp object
Adafruit_BMP085 bmp;

// Web page file stored on the SD card
File webFile;

// Set potentiometer GPIO
const int potPin = 32;

// IMPORTANT: At the moment, GPIO 4 doesn't work as an ADC when using the Wi-Fi library
// This is a limitation of this shield, but you can use another GPIO to get the LDR readings
const int LDRPin = 34;

// variables to store temperature and humidity
float tempC;
float tempF;
float humi;

// Variable to store the HTTP request
String header;

// Set web server port number to 80
WiFiServer server(80);

void setup()
{
    // initialize serial port
    Serial.begin(115200);

    // initialize DHT sensor
    dht.begin();

    // initialize BMP180 sensor
   // if (!bmp.begin())
  //  {
  //      Serial.println("Could not find BMP180 or BMP085 sensor");
       // while (1)
       // {
       // }
   // }

    // initialize SD card
  if (!SD.begin())
    {
        Serial.println("Card Mount Failed");
        return;
    }
    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE)
    {
        Serial.println("No SD card attached");
        return;
    }
    // initialize SD card
    Serial.println("Initializing SD card...");
    if (!SD.begin())
    {
        Serial.println("ERROR - SD card initialization failed!");
        return; // init failed
    }

    // Connect to Wi-Fi network with SSID and password
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    // Print local IP address and start web server
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    server.begin();
}


void readDHT()
{
    Serial.println("Reading DHT sensor");
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    humi = dht.readHumidity();
    // Read temperature as Celsius (the default)
    tempC = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    tempF = dht.readTemperature(true);

    // Check if any reads failed and exit early (to try again).
    if (isnan(humi) || isnan(tempC) || isnan(tempF))
    {
        Serial.println("Failed to read from DHT sensor!");
        return;
    }

    Serial.print("Humidity: ");
  Serial.print(humi);
  Serial.print(" %\t Temperature: ");
  Serial.print(tempC);
  Serial.print(" *C ");
  Serial.print(tempF);
  Serial.println(" *F");
}

// Send XML file with the latest sensor readings
void sendXMLFile(WiFiClient cl)
{
    tempC=99;
    tempF=99;
    humi=45;
    Serial.println("Sending data readings to web page via file");
    //cl.println("");
Serial.println(tempC);
    // Read DHT sensor and update variables
    readDHT();
Serial.println("DHT readings read");
    // Prepare XML file
    cl.print("<?xml version = \"1.0\" ?>");
    cl.print("<inputs>");

    cl.print("<reading>");
    cl.print(tempC);
    cl.println("</reading>");

    cl.print("<reading>");
    cl.print(tempF);
    cl.println("</reading>");

    cl.print("<reading>");
    cl.print(humi);
    cl.println("</reading>");

    //float currentTemperatureC = bmp.readTemperature();
    cl.print("<reading>");
    //cl.print(currentTemperatureC);
    cl.print(tempF);
    cl.println("</reading>");
    float currentTemperatureF = (9.0 / 5.0) * tempC + 32.0;
    // float currentTemperatureF = (9.0 / 5.0) * currentTemperatureC + 32.0;
    cl.print("<reading>");
    cl.print(currentTemperatureF);
    cl.println("</reading>");

    cl.print("<reading>");
    //cl.print(bmp.readPressure());
    cl.print(tempF);
    cl.println("</reading>");

    cl.print("<reading>");
    cl.print(analogRead(potPin));
    //cl.print(tempF);
    cl.println("</reading>");

    // IMPORTANT: Read the note about GPIO 4 at the pin assignment
    cl.print("<reading>");
    cl.print(analogRead(LDRPin));
    //cl.print(tempF);
    cl.println("</reading>");

    cl.print("</inputs>");
}


void loop()
{
    WiFiClient client = server.available(); // Listen for incoming clients
// this does work ----- Serial.println("Checking client server available");
    if (client)
    { // if new client connects
        boolean currentLineIsBlank = true;
        while (client.connected())
        {
            if (client.available())
            {                           // client data available to read
                char c = client.read(); // read 1 byte (character) from client
                header += c;
                // if the current line is blank, you got two newline characters in a row.
                // that's the end of the client HTTP request, so send a response:
                if (c == '\n' && currentLineIsBlank)
                {
                    Serial.println("sending header file");
                    // send a standard http response header
                    client.println("HTTP/1.1 200 OK");
                    // Send XML file or Web page
                    // If client already on the web page, browser requests with AJAX the latest
                    // sensor readings (ESP32 sends the XML file)
                    if (header.indexOf("update_readings") >= 0)
                    {
                        // send rest of HTTP header
                        client.println("Content-Type: text/xml");
                        client.println("Connection: keep-alive");
                        client.println();
                        // Send XML file with sensor readings
                        sendXMLFile(client);
                        Serial.println("Sending XML file.............");
                    }
                    // When the client connects for the first time, send it the index.html file
                    // stored in the microSD card
                    else
                    {
                        // send rest of HTTP header
                        client.println("Content-Type: text/html");
                        client.println("Connection: keep-alive");
                        client.println();
                        // send web page stored in microSD card
                        Serial.println("Opening SD card");
                        webFile = SD.open("/index.html");
                        if (webFile)
                        {
                            while (webFile.available())
                            {
                                // send web page to client
                                Serial.println("Reading SD card");
                                client.write(webFile.read());
                            }
                            webFile.close();
                        }
                    }
                    break;
                }
                // every line of text received from the client ends with \r\n
                if (c == '\n')
                {
                    // last character on line of received text
                    // starting new line with next character read
                    currentLineIsBlank = true;
                }
                else if (c != '\r')
                {
                    // a text character was received from client
                    currentLineIsBlank = false;
                }
            } // end if (client.available())
        }     // end while (client.connected())
        // Clear the header variable
        header = "";
        // Close the connection
        client.stop();
        Serial.println("Client disconnected.");
    } // end if (client)
}




