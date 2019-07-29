#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <EnvironmentCalculations.h>
#include <BME280I2C.h>
#include <Wire.h>


float lattitude, longitude;

  // (TX, RX)
SoftwareSerial gpsSerial(5,6);// gps module
SoftwareSerial gsmSerial(4,3); // gsm module
TinyGPSPlus gps;


String apn = "web.gprs.mtnnigeria.net";                       //APN
String apn_u = "ppp";                     //APN-Username
String apn_p = "ppp";                     //APN-Password
String url = "";  //URL for HTTP-POST-REQUEST


BME280I2C bme;    // Default : forced mode, standby time = 1000 ms
                  // Oversampling = pressure ×1, temperature ×1, humidity ×1, filter off,

// bmp parameters
char status;
float T(NAN), hum(NAN), P(NAN), altitude, dewPoint, seaLevel;

//air parameter
int air;

//solar parameter
int solar;

// string holding parameters

String data1, data2, data3, data4, data5, data6, data7, data8, data9, data10 = "";
void setup()
{
  Serial.begin(9600);
  gpsSerial.begin(9600);
  gsmSerial.begin(9600);
 while (!Serial);
  Serial.println("Starting Up");

   
  pinMode(A7, INPUT); // air quality
  
  pinMode(A6, INPUT); //solar radiation
 
  Wire.begin();

  while(!bme.begin())
  {
    Serial.println("Could not find BME280 sensor!");
    delay(1000);
  }

  // bme.chipID(); // Deprecated. See chipModel().
  switch(bme.chipModel())
  {
     case BME280::ChipModel_BME280:
       Serial.println("Found BME280 sensor! Success.");
       break;
     case BME280::ChipModel_BMP280:
       Serial.println("Found BMP280 sensor! No Humidity available.");
       break;
     default:
       Serial.println("Found UNKNOWN sensor! Error!");
  }
}

void loop(){
  bmp_loop(&Serial);
  airQuality_loop();
  solar_loop();
  gpsSerial.listen();
  gps_loop();

  data1 = altitude;
  data2 = T;
  data3 = P;
  data4 = air;
  data5 = solar;
  data6 = lattitude;
  data7 = longitude;
  data8 = dewPoint;
  data9 = hum;
  data10 = seaLevel;
  
  gsmSerial.listen();
  gsm_sendhttp();
}

void bmp_loop(Stream* client)
{
   BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
   BME280::PresUnit presUnit(BME280::PresUnit_Pa);

   bme.read(P, T, hum, tempUnit, presUnit);

   client->print("Temp: ");
   client->print(T);
   client->println("°"+ String(tempUnit == BME280::TempUnit_Celsius ? "C" :"F"));
   client->print("Humidity: ");
   client->print(hum);
   client->println("% RH");
   client->print("Pressure: ");
   client->print(P);
   client->println(" Pa");

   EnvironmentCalculations::AltitudeUnit envAltUnit  =  EnvironmentCalculations::AltitudeUnit_Meters;
   EnvironmentCalculations::TempUnit     envTempUnit =  EnvironmentCalculations::TempUnit_Celsius;

   float altitude = EnvironmentCalculations::Altitude(P, envAltUnit);
   float dewPoint = EnvironmentCalculations::DewPoint(T, hum, envTempUnit);
   float seaLevel = EnvironmentCalculations::EquivalentSeaLevelPressure(altitude, T, P);

   client->print("Altitude: ");
   client->print(altitude);
   client->println((envAltUnit == EnvironmentCalculations::AltitudeUnit_Meters ? "m" : "ft"));
   client->print("Dew point: ");
   client->print(dewPoint);
   client->println("°"+ String(envTempUnit == EnvironmentCalculations::TempUnit_Celsius ? "C" :"F"));
   client->print("Equivalent Sea Level Pressure: ");
   client->print(seaLevel);
   client->println(" Pa");

  delay(5000);  // Pause for 5 seconds.
}

void airQuality_loop()
{
  air = analogRead(A7);
  Serial.println("\nAir Quality: ");
  Serial.print(air);
  Serial.println();
}

void solar_loop()
{
  solar = analogRead(A1);
  Serial.println("\nSolar Reading: ");
  Serial.print(solar);
  Serial.println();
}

void gps_loop() {
  while(1)
  {
    while(gpsSerial.available() > 0)
    {
      gps.encode(gpsSerial.read());
    }
    if(gps.location.isUpdated())
    {
      lattitude = gps.location.lat();
      longitude = gps.location.lng();
      break;
    }
  }
  Serial.print(" Latitude = ");
  Serial.print(lattitude,6);
  Serial.print("° N");
  Serial.print(" Longitude = ");
  Serial.print(longitude,6);
  Serial.println("° E");
  delay(1000);
}

void gsm_sendhttp() {
   
  gsmSerial.println("AT");
  runsl();//Print GSM Status an the Serial Output;
  delay(1000);
    
  gsmSerial.println("AT+CGREG?");
   runsl();
  delay(10000);
  
  gsmSerial.println("AT+CGREG?");
   runsl();
  delay(200);
   
  gsmSerial.println("AT+CSCLK?");
   runsl();
  delay(50);

  
  gsmSerial.println("AT+SAPBR=3,1,Contype,GPRS");
  runsl();
  delay(100);
  gsmSerial.println("AT+SAPBR=3,1,APN," + apn);
  runsl();
  delay(100);
  gsmSerial.println("AT+SAPBR =1,1");
  runsl();
  delay(100);
  gsmSerial.println("AT+SAPBR=2,1");
  runsl();
  delay(2000);
  gsmSerial.println("AT+HTTPINIT");
  runsl();
  delay(100);
  gsmSerial.println("AT+HTTPPARA=CID,1");
  runsl();
  delay(100);
  gsmSerial.println("AT+HTTPPARA=URL," + url);
  runsl();
  delay(500);
  gsmSerial.println("AT+HTTPPARA=CONTENT,application/x-www-form-urlencoded");
  runsl();
  delay(100);
  gsmSerial.println("AT+HTTPDATA=192,10000");
  runsl();
  delay(100);
  gsmSerial.println("altitude=" + data1 + "&temperature=" + data2 + "&pressure=" + data3 + "&air=" + data5 + "&solar=" + data6 + "&lattitude=" + data7 + "&longitude=" + data8); 
  runsl();
  delay(10000);
  gsmSerial.println("AT+HTTPACTION=1");
  runsl();
  delay(5000);
  gsmSerial.println("AT+HTTPREAD");
  runsl();
  delay(100);
  gsmSerial.println("AT+HTTPTERM");
  runsl(); 
  delay(100);
}

//Print GSM Status
void runsl() {
  while (gsmSerial.available()) {
    Serial.write(gsmSerial.read());
  }
}
