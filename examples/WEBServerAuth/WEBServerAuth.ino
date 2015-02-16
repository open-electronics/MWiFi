/*
* This example makes a  WEB server with authentication.
* Using WPA cryptography authentication is quite secure inside lan even if is 
* in Basic format.
* There is a menu as home page.
* Home page can be reached using /index url or just reaching the node.
* Server is addressed by its ip on net (usually dynamic) and port 
* For example http//192.168.1.5/index 
* You don't need to use port if decide to set port 80 (default for browsers)
* From home page menu you can reach these pages:
*  - Analogical page to read analogical value
*  - Digital reading page to read digital pin D4,D5,D12 (D2,D3,D7 occupied by WIFI shield)
*  - Digital setting page to switch on/off pin D6,D8,D9
*  - PWM page to power up/down pwm pin D10,D11
*
* Pages are stored in program-mem and a dynamic response compilation is used for values reading.
* Each parameater is represented by a tag @. This tag is substituded with a string found in a prepared 
* array of strings. (Substitution takes place by sequential order)
*
* The getRequest() function automatically activates the function corresponding to a demanded page (resource) 
* For this purpose you have to populate an array of WEBRES type defined.
* Each WEBRES is a struct of 2 element:
*  - URL name : just last name with a slash prefixed (ex. /index)
*  - function name : the function that has to manage request and send response
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
#define PASSWORD     ""          // password if WAP
#define PORT         80                  // server listening port (if you prefere set as 80)

#define FAUTH        true
#define WUSER        "arduserver"
#define WPASSW       "test"

char mac[18];                  // buffer for mac address of shield
char name[8];                  // char for shield name on net

char ip[16];                   // buffer for (dynamic) ip address as string
char remip[16];                // buffer for remote ip asking link wit us
boolean fc=0;                  // flag connection
boolean fs=0;                  // flag socket open
int ssocket;                   // server socket handle
int csocket;                   // client socket handle

byte key[32];                  //key buffer on ram 
boolean fkey=false;            //buffer of EEPROM(0) flag . It means key in memory
char *Wkey;                    //buffer for coded authorization

HTTP WIFI;                     //instance of MWiFi library

#define VERBOSE 1              // if 1 some information will be printed on console
#define ERRLOG 1

int d6=0;                      // output default values
int d8=0;
int d9=0;
int d10=0;                     // pwm default values
int d11=0;

#define GREY "#666666"         // off color
#define RED  "#FF0000"         // on color
#define ON   "ON"              // on text
#define OFF  "OFF"             // off text

/**************** HTML pages *****************/
prog_char pageindex[] PROGMEM=
"<html><head>"
"<title>Index</title>"
"<style type='text/css'>"
"body,td,th {color: #FFF;}"
"body {background-color: #066;}"
"a:link {color: #FF0;}"
"a:visited {color: #FF0;}"
"</style>"
"</head>"
"<body>"
"<h1>Welcome to Arduino Server</h1>"
"<p>You can :</p>"
"<h2><ul>"
"<li><a href='/Analog'>read analogic pins</a></li>"
"<li><a href='/RDigital'>read digital pins</a></li>"
"<li><a href='/WDigital'>set digital pins on/off</a></li>"
"<li><a href='/Pwm'>power up/down PWM pins</a></li>"
"<li><a href='/End'>End session </a></li>"
"</ul></h2>"
"</body></html>";

prog_char pageanalog[] PROGMEM=
"<title>Analogic Reading</title>"
"<style type='text/css'>"
"body,td,th {color: #FFF;}"
"body {background-color: #066;}"
"a:link {color: #FF0;}"
"a:visited {color: #FF0;}"
"</style></head>"
"<body>"
"<h1>Analogic reading (0 to 1024)</h1>"
"<h2>"
"<table width='400' border='1'>"
"<tr><td width='232'>A0</td><td width='152'>@</td></tr>"
"<tr><td>A1</td><td>@</td></tr>"
"<tr><td>A2</td><td>@</td></tr>"
"<tr><td>A3</td><td>@</td></tr>"
"<tr><td>A4</td><td>@</td></tr>"
"<tr><td>A5</td><td>@</td></tr>"
"</table>"
"<p><a href='/index'>Home</a></p>"
"</h2>"
"</body></html>";

prog_char pagewdigital[] PROGMEM=
"<html>"
"<head>"
"<meta http-equiv='Content-Type' content='text/html; charset=utf-8' />"
"<title>Untitled Document</title>"
"<style type='text/css'>"
"body,td,th {color: #FFF;}"
"body {background-color: #066;}"
"a:link {color: #FF0;}"
"a:visited {color: #FF0;}"
"</style>"
"</head>"
"<body>"
"<h1>Set Digital ON/OFF</h1>"
"<p>&nbsp;</p>"
"<table width='400' border='1'>"
"<tr><td width='%33'><div align='center'>D6</div></td><td width='%33'>"
"<div align='center'>D8</div></td><td width='%33'><div align='center'>D9</div>"
"</td></tr>"
"<tr><td bgcolor='@'>&nbsp;</td><td bgcolor='@'></td><td bgcolor='@'></td></tr>"
"<tr><td><form id='form6' name='form6' method='get' action='/wdig'>"
"<div align='center'>"
"<input type='submit' name='D6ON' id='D6ON' value='ON ' />&nbsp;&nbsp;"
"<input type='submit' name='D6OFF' id='D6OFF' value='OFF' />"
"</div></form></td>"
"<td><form id='form8' name='form8' method='get' action='/wdig'>"
"<div align='center'>"
"<input type='submit' name='D8ON' id='D8ON' value='ON ' />&nbsp;&nbsp;"
"<input type='submit' name='D8OFF' id='D8OFF' value='OFF' />"
"</div></form></td>"
"<td><form id='form9' name='form9' method='get' action='/wdig'>"
"<div align='center'>"
"<input type='submit' name='D9ON' id='D9ON' value='ON ' />&nbsp;&nbsp;"
"<input type='submit' name='D9OFF' id='D9OFF' value='OFF' />"
"</div></form></td></tr>"
"</table>"
"<p>&nbsp;</p>"
"<h2><p><a href='/index'>Home</a></p></h2></body></html>";

prog_char pagerdigital[] PROGMEM= 
"<html><head>"
"<meta http-equiv='Content-Type' content='text/html; charset=utf-8' />"
"<title>Untitled Document</title>"
"<style type='text/css'>"
"body,td,th {color: #FFF;}body {background-color: #066;}"
"a:link {color: #FF0;}a:visited {color: #FF0;}"
"</style></head>"
"<body>"
"<h1>Read Digital Input</h1>"
"<p>Pins are set as INPUT_PULLUP</p>"
"<p>Use refresh for updating values</p>"
"<table width='400' border='1'>"
"<tr>"
"<td width='%33'><div align='center'>D4</div></td>"
"<td width='%33'><div align='center'>D5</div></td>"
"<td width='%33'><div align='center'>D12</div></td></tr>"
"<tr>"
"<td><div align='center'>@</div></td>"
"<td><div align='center'>@</div></td>"
"<td><div align='center'>@</div></td></tr>"
"</table>"
"<p>&nbsp;</p>"
"<h2><p><a href='/index'>Home</a></p></h2></body></html>";

prog_char pagepwm[] PROGMEM=
"<html><head>"
"<title>PWM page</title>"
"<style type='text/css'>body,td,th {color: #FFF;}"
"body {background-color: #066;}a:link {color: #FF0;}"
"a:visited {color: #FF0;}"
"</style></head>"
"<body>"
"<h1>Set Digital ON/OFF</h1>"
"<h3>"
"<p>Insert values from 0 to 255</p>"
"<form id='FPWM10' name='form1' method='get' action='PwmSet'>"
"<label for='PWM10'>PWM on pin 10 >>> &nbsp;</label>"
"<input name='PWM10' type='text' id='PWM10' value='@' size='10'/>"
"<input type='submit' name='D10' id='D10' value='Set PWM 10' />"
"</form>"
"</br>"
"<form id='FPWM11' name='form2' method='get' action='PwmSet'>"
"<label for='PWM11'>PWM on pin 11 >>> &nbsp;</label>"
"<input name='PWM11' type='text' id='PWM11' value='@' size='10'/>"
"<input type='submit' name='D11' id='D11' value='Set PWM 11' />"
"</form>"
"</h3>"
"<p>&nbsp;</p>"
"<h2><p><a href='/index'>Home</a></p></h2>"
"</body></html>";

prog_char pageend[] PROGMEM=
"<html><head>"
"<title>ENDe</title>"
"<style type='text/css'>body,td,th {color: #FFF;}"
"body {background-color: #066;}a:link {color: #FF0;}"
"a:visited {color: #FF0;}"
"</style></head>"
"<body>"
"<h1>Session END!</h1>"
"<h2>Server ready for another client</h2>"
"<h2><p><a href='/index'>Back to new session</a></p><h2>"
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
  
  if (EEPROM.read(0)==1)             // If EEPROM key flag is OK then reads key stored
  {fkey=true; int i;for(i=0;i<32;i++) key[i]=EEPROM.read(i+1);}
  else fkey=false;                   // else uses password
  
  pinMode(6,OUTPUT);                 // pin 6 as output
  pinMode(8,OUTPUT);                 // pin 8 as output
  pinMode(9,OUTPUT);                 // pin 9 as output
  pinMode(4,INPUT_PULLUP);           // pin 4 as input with pullup (1 if not closed to GND)
  pinMode(5,INPUT_PULLUP);           // pin 5 as input with pullup
  pinMode(12,INPUT_PULLUP);          // pin 12 as input with pullup
  pinMode(10,OUTPUT);                // pin 10 as output for pwm
  pinMode(11,OUTPUT);                // pin 11 as output for pwm
  
  Wkey=WIFI.codeWebKey(WUSER,WPASSW);
} 
/************************ end setup ***************************/

int netConnect()
{
  fc=0;
  if (PASSWORD=="") {fc=WIFI.ConnectOpen(ACCESSPOINT);}      // if passw= empty string connect in open mode
  else                                                       // else connect in WAP mode
   {
      if (!fkey)                                             // with password and compute new key
      {
        fc=WIFI.ConnectWPAandGetKeyEE(ACCESSPOINT,PASSWORD,0);
      }
      else  {fc=WIFI.ConnectWPAwithKeyEE(ACCESSPOINT,0);}     // with key if already calculated
      if (fc==1) {fkey=true;}
   } 
   return fc; 
}

void netConnection()                                           // connection to network
{
    int i;for(i=0;i<5;i++) {fc=netConnect(); if(fc==1) break;}  // try to connect for 5 times
    if (fc==0)     {delay(60000);return;}                        // wait for new try 
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
      wdt_enable(WDTO_1S);           // reset
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
WEBRES rs[]={{"/index",pindex},{"/Analog",panalog},{"/RDigital",rdigital},
             {"/WDigital",wdigital},{"/wdig",wdig},{"/Pwm",pwmpage},{"/PwmSet",pwmset},{"/End",sessend}};

/*************************** loop *******************************/

void loop() 
{
  if(!fc) netConnection();                   // if not connection yet, make it
  if(fc)                                     // if server active (connection established)
  {
    if (!fs)  socketConnection();            // if no one is connected listen for any connection try 
    if (fs)                                  // if someone is connected listen for any http request 
    {if(FAUTH) WIFI.getRequest(csocket,8,rs,Wkey);  // with authentication 
     else WIFI.getRequest(csocket,8,rs); }                    // no authentication
                                             // array of WEBRES elements is provided, with its dimension
  }                                          // correspondig function is automatically called
                                             // if no risource corresponds, "not found" response is sent
  if (fs){ WIFI.closeSock(csocket);fs=false;}
}

/********************* Page Functions ***************************/ 
void pindex(char *query)                     // index page
{
#if VERBOSE
  Serial.println("Index...");
#endif  
   WIFI.sendResponse(csocket,pageindex);     // send response: pageindex
}

void panalog(char *query)                    // analog page
{
#if VERBOSE  
  Serial.println("Analog...");
#endif  
  char *val[6];
                      // dynamic response: a set of values string populates an array of 6 positions
                      // corresponding to 6 tags @ in pageanalog
  char val0[5];snprintf(val0,5,"%d",analogRead(0));val[0]=val0;
  char val1[5];snprintf(val1,5,"%d",analogRead(1));val[1]=val1;
  char val2[5];snprintf(val2,5,"%d",analogRead(2));val[2]=val2;
  char val3[5];snprintf(val3,5,"%d",analogRead(3));val[3]=val3;
  char val4[5];snprintf(val4,5,"%d",analogRead(4));val[4]=val4;
  char val5[5];snprintf(val5,5,"%d",analogRead(5));val[5]=val5;
  
  WIFI.sendDynResponse(csocket,pageanalog,6,val); // send dynamic response
}

void rdigital(char *query)                    // digital readyng page
{
#if VERBOSE  
  Serial.println("RDigital...");
#endif  
  char *val[3];
                      // as previous function but for just 3 tags
  if(digitalRead(4)) val[0]=ON;else val[0]=OFF;
  if(digitalRead(5)) val[1]=ON;else val[1]=OFF;
  if(digitalRead(12))val[2]=ON;else val[2]=OFF;
  WIFI.sendDynResponse(csocket,pagerdigital,3,val); // send dynamic response
}

void wdigital(char *query)                    // digital writing page (has form and buttons)
{
#if VERBOSE  
  Serial.println("WDigital...");
#endif 
  char *val[3];
  if (d6) val[0]=RED; else val[0]=GREY;
  if (d8) val[1]=RED; else val[1]=GREY;
  if (d9) val[2]=RED; else val[2]=GREY;
  WIFI.sendDynResponse(csocket,pagewdigital,3,val);         // send dyn. response: pagewdigital
}

void wdig(char *query)                            // function called by form in pagewdigital
{
#if VERBOSE  
  Serial.println("Setting...");
#endif  
                                                  // request has parameters 
                                                  // these parameters activate pins
  if (WIFI.getParameter(query,strlen(query),"D6ON")!=NULL) {digitalWrite(6,1);d6=1;}
  if (WIFI.getParameter(query,strlen(query),"D6OFF")!=NULL){digitalWrite(6,0);d6=0;}
  if (WIFI.getParameter(query,strlen(query),"D8ON")!=NULL) {digitalWrite(8,1);d8=1;}
  if (WIFI.getParameter(query,strlen(query),"D8OFF")!=NULL){digitalWrite(8,0);d8=0;}
  if (WIFI.getParameter(query,strlen(query),"D9ON")!=NULL) {digitalWrite(9,1);d9=1;}
  if (WIFI.getParameter(query,strlen(query),"D9OFF")!=NULL){digitalWrite(9,0);d9=0;}
  
  wdigital(query);                                 // refresh page with new values
}

void pwmpage(char *query)                          // pwm page (has form and buttons)
{
#if VERBOSE  
  Serial.println("Pwm...");
#endif 
  char *val[2];
  char val10[5];sprintf(val10,"%d",d10);val[0]=val10;
  char val11[5];sprintf(val11,"%d",d11);val[1]=val11;
  WIFI.sendDynResponse(csocket,pagepwm,2,val);      // send response: pagepwm 
}

void pwmset(char *query)                            // as wdig this function is called by form in pagepwm
{
#if VERBOSE  
  Serial.println("Pwm Setting...");
#endif  
  char *pwmval;                                      
                                                    // reads parameter and set pins
  pwmval=WIFI.getParameter(query,strlen(query),"PWM10"); 
  if (pwmval!=NULL) {int pv;sscanf(pwmval,"%d",&pv);analogWrite(10,pv);d10=pv;}
  pwmval=WIFI.getParameter(query,strlen(query),"PWM11"); 
  if (pwmval!=NULL) {int pv;sscanf(pwmval,"%d",&pv);analogWrite(11,pv);d11=pv;}
  
   pwmpage(query);                                  // refresh page with new values
}

void sessend(char *query)                          // page to close session
{
  if (FAUTH) WIFI.respNoAuth(csocket);
  else WIFI.sendResponse(csocket,pageend);
  WIFI.closeSock(csocket);
  fs=false;
}

/********************** End page functions ************************/

