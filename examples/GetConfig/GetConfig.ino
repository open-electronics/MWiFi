/*
* This example demonstrates the use of some basic commands for MWiFi shield
* - startup
* - get default configuration
* - set some configuration value
* - scan wifi visible networks
* - select the best open (without password) accesspoint in terms of radio signal (RSSI) (if any)
* - try to connect to this best accesspoint (if any)
* - get the dynamic IP provided by the accesspoint
* - close connection
*
* Author: Daniele Denaro
*/


#include <MWiFi.h>             // include library

char mac[18];                  // buffer for mac address of shield
char name[8];                  // char for shield name on net

char ip[16];                   // buffer for ip address
char mask[16];                 // buffer for mask address
char gateway[16];              // buffer for gateway

MWiFi WIFI;                    //instance of MWiFi library

void setup() 
{
  Serial.begin(9600);

  WIFI.begin();                // startup wifi shield
  
  WIFI.getConfig();            // reads default info from shield
  WIFI.getMAC(mac);            // gets string mac of shield
  WIFI.getName(name);          // gets name of shield on net 
  WIFI.getNetMask(mask);       // gets dafault mask
  WIFI.getGateway(gateway);    // gets default gateway
  
  // print information on console
  Serial.print("MAC: ");Serial.println(mac);
  Serial.print("Name: ");Serial.println(name);
  Serial.print("MASK: ");Serial.println(mask);
  Serial.print("GATEWAY: ");Serial.println(gateway);
  
  WIFI.setNetMask("255.255.255.0");  //modify default
  WIFI.setGateway("0.0.0.0");        //modify default
  Serial.println("New mask: 255.255.255.0");
  Serial.println("New gateway: 0.0.0.0");
  
  int nn=WIFI.scanNets();        // scans visible networks. nn is the number of nets found
  
  Serial.print("Found ");Serial.print(nn);Serial.println(" nets");
  
  int i;
  for (i=0;i<nn;i++)             // displays values for each net (name,security type,access type,RSSI)
  { char* net=WIFI.getNetScanned(i); if (net!=NULL); Serial.println(net);}
  
  Serial.println("Try to get the best open access point");
  
  char *nameAccP=NULL;           // access point (SSID) name initialising
  
  nameAccP=WIFI.getSSIDBestOpen(nn); //selects the best open access point (if any) 
  
  if(nameAccP!=NULL)
  {Serial.print("Best open access point is : ");Serial.println(nameAccP);}
  else
  {Serial.println("Open access point not found!");}
  
  int fc=0;                      // flag connection. Connected when = 1
  
  if(nameAccP!=NULL)
  {
    WIFI.ConnSetOpen(nameAccP);  // prepare connection
    fc=WIFI.Connect();           // connect command (if OK fc=1)
    if (fc)
    {
      WIFI.getIP(ip);            // if connected load ip buffer with new IP from net (dynamic IP)
      Serial.print("Connected with ");
      Serial.print(nameAccP);
      Serial.print(" new IP is : ");
      Serial.println(ip);
    }
  } 
  
  if (fc) {delay(5000); fc=WIFI.Disconnect();} // just to demonstrate connection process
  if (fc==0) {Serial.println();Serial.println("Disconnected !");}
}

 

void loop() 
{
}


