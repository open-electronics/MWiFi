/*
* This example makes a simple WEB server. 
* Two Analogical values A1 and A2 and two Digital values D4 and D5 are shown at each refesh.
* The server show only one page.
* Page is stored in program-memory and a dynamic response compilation is used for values reading.
* Each parameater is represented by a tag @. This tag is substituded with a string found in a prepared 
* array of strings. (Substitution takes place by sequential order)
*
* The getRequest() function automatically activates the function corresponding to a demanded page (resource) 
* For this purpose you have to populate an array of WEBRES type defined.
* Each WEBRES is a struct of 2 element:
*  - URL name : just last name with a slash prefixed (ex. /index)
*  - function name : the function that has to manage request and send response
* In this case WEBRES array has just only one element (the index page).
*
* Console print is done just for controll (you can delete)
*
* Library error management have reset policy at present.
* So, socket problem or net connection lost produces automatic reset.
*
* Author: Daniele Denaro
*/

#include <HTTPlib.h>             // include library (HTTP library is a derivate class of WiFi)

#define ACCESSPOINT  "D-Link-casa"       // access point name
#define PASSWORD     ""                  // password if WAP
#define PORT         80                // server listening port 

char mac[18];                  // buffer for mac address of shield
char name[8];                  // char for shield name on net

char ip[16];                   // buffer for (dynamic) ip address as string
char remip[16];                // buffer for remote ip asking link wit us
boolean fc=0;                  // flag connection
boolean fs=0;                  // flag socket open
int ssocket;                   // server socket handle
int csocket;                   // client socket handle

HTTP WIFI;                     //instance of MWiFi library

#define VERBOSE 1              // if 1 some information will be printed on console

/**************** HTML pages *****************/
prog_char pageServer[] PROGMEM=
"<html><head>"
"<title>Arduino Server</title>"
"<style type='text/css'>"
"body,td,th {color: #FFF;}"
"body {background-color: #066;}"
"a:link {color: #FF0;}"
"a:visited {color: #FF0;}"
"</style></head>"
"<body>"
"<h1>Welcome to Arduino Server</h1>"
"<h3>Analogical and Digital input</h3>"
"<table width='300px' border='1' cellspacing='10' cellpadding='2'>"
"<tr><td width='200px'>Analog A1</td>"
"<td width='100px'>@</td></tr>"                    //@ tag for A1 value
"<tr><td>Analog A2</td><td>@</td></tr>"            //@ tag for A2 value
"<tr><td>Digital D4</td><td>@</td></tr>"           //@ tag for D4 value
"<tr><td>Digital D5</td><td>@</td></tr>"           //@ tag for D5 value
"</table>"
"</body></html>";

/******************** end HTML Pages *********************/

void setup() 
{

  WIFI.begin();                // startup wifi shield

#if VERBOSE     
  Serial.begin(9600);
  WIFI.getConfig();            // reads default info from shield
  WIFI.getMAC(mac);            // gets string mac of shield
  WIFI.getName(name);          // gets name of shield on net 
  // print information on console
  Serial.print("MAC: ");Serial.println(mac);
  Serial.print("Name: ");Serial.println(name);
#endif

  WIFI.setNetMask("255.255.255.0");  //modify default
  WIFI.setGateway("0.0.0.0");        //modify default
  
  netConnection();
  
  pinMode(4,INPUT_PULLUP);           // pin 4 as input with pullup (1 if not closed to GND)
  pinMode(5,INPUT_PULLUP);           // pin 5 as input with pullup
} 
/************************ end setup ***************************/

void netConnection()                                           // connection to network
{
#if VERBOSE    
    Serial.println("Waiting for connection...");Serial.println(ip);
#endif 
    // if WPA connection: using password it is ossible to waith 1m
    // using key just ms
    if (PASSWORD=="") {fc=WIFI.ConnectOpen(ACCESSPOINT);}         // if passw= empty string connect in open mode
    else              {fc=WIFI.ConnectWPAwithPsw(ACCESSPOINT,PASSWORD);} // else connect in WAP mode
    if (!fc) {Serial.println("No connection!");return;}
    WIFI.getIP(ip);                                            // get dynamic ip
#if VERBOSE    
    Serial.print("Net Connected as ");Serial.println(ip);
#endif    
    ssocket=WIFI.openServerTCP(PORT);// open server socket. Listen for connection from remote (just one connection)
                                     // we must poll in loop routine to see if connection asked from remote
    if (ssocket==255)                //socket non valid. No server open. 
    {
      WIFI.Disconnect();fc=0;
#if VERBOSE      
      Serial.println("Socket problem. Disconnected!");
#endif
      wdt_enable(WDTO_500MS);           // reset
      delay (1000);
    }
#if VERBOSE    
      Serial.print("Server active on port ");Serial.println(PORT);
#endif
}

void socketConnection()                                          // link with remote client 
{
      // verify if someone asks for connection on port PORT
      csocket=WIFI.pollingAccept(ssocket); if(csocket<255) fs=1; //csoccket is the client socket to communicate
#if VERBOSE
      if (fs) {Serial.print("Server connected with : ");WIFI.getRemoteIP(remip);Serial.println(remip);}
#endif      
}

  
/*
* Make an array of WEBRES elements. Heach of them are made by a couple of URLname-routinename.
*/
WEBRES rs[]={{"/index",pindex}};  //only one page

/*************************** loop *******************************/

void loop() 
{
  if(!fc) {delay(10000); netConnection();}   // if not connection yet, make it
  if(fc)                                     // if server active (connection established)
  {
    if (!fs)  socketConnection();            // if no one is connected listen for any connection try 
    if (fs)  WIFI.getRequest(csocket,8,rs);  // if someone is connected listen for any http request 
                                             // array of WEBRES elements is provided, with its dimension
  }                                          // correspondig function is automatically called
                                             // if no risource corresponds, "not found" response is sent

  if (fs){ WIFI.closeSock(csocket);fs=false;}// better to close socket after sent response.
}

/********************* Page Functions ***************************/ 
void pindex(char *query)                     // index page
{
#if VERBOSE
  Serial.println("Index...");
#endif 
  char *val[4];       //array of values (as string)
     // dynamic response: a set of values string populates an array of 4 positions
     // corresponding to 4 tags @ in pageanalog
  char val0[5];sprintf(val0,"%d",analogRead(1));val[0]=val0;  //read analog A1
  char val1[5];sprintf(val1,"%d",analogRead(2));val[1]=val1;  //read analog A2
  if(digitalRead(4)) val[2]="ON";else val[2]="OFF";           //read digital D4
  if(digitalRead(5)) val[3]="ON";else val[3]="OFF";           //read digital D5
  
     //all values as string in val array
 
   WIFI.sendDynResponse(csocket,pageServer,4,val); // send dynamic response 4 tags substitutions 
}

/********************** End page functions ************************/

