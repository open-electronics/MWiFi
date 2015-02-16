/*
* This example demonstrates the use of socket as server to write and read to a client 
* after accepting connection .
* Just load program on Arduino end use telnet program from pc for commnication.
* on setup
* - startup
* - connect to access point
* - set as server listening on port . 
* on loop
* - test if someone asks for connection on port 
* - if server accepts connection start to manage incoming records
* - commands parsing end execution
*
* Use any kind of telnet program on pc to connect.
* Windows telnet is not very confortable (and in any case you have to unset local echo and set  record mode)
* You can use Putty or oter software. But in any case a simple Java program (socketClient.jar) is provided.
*
* Author: Daniele Denaro
*/


#include <MWiFi.h>             // include library

char nameAccP[]="D-Link-casa";  // change for local accessible access point
char passw[]="";                // empty if OPEN connection or password if WAP connection

char mac[18];                  // buffer for mac address of shield
char name[8];                  // char for shield name on net

char ip[16];                   // buffer for ip address
char remip[16];                   // buffer for ip address
char mask[16];                 // buffer for mask address
char gateway[16];              // buffer for gateway

int fc=0;                      // flag connection
int fs=0;                      // flag socket open
int ssocket;                   // server socket handle
int csocket;                   // client socket handle
unsigned int port=5000;        // port number for TCP connection

MWiFi WIFI;                    //instance of MWiFi library

void setup() 
{
  Serial.begin(9600);

  WIFI.begin();                // startup wifi shield
  
  WIFI.getConfig();            // reads default info from shield
  WIFI.getMAC(mac);            // gets string mac of shield
  WIFI.getName(name);          // gets name of shield on net 
  
  // print information on console
  Serial.print("MAC: ");Serial.println(mac);
  Serial.print("Name: ");Serial.println(name);
  Serial.print("MCW Version (hex)  : ");
  Serial.print(WIFI.MCWVERSION[1],HEX);Serial.print(" ");Serial.println(WIFI.MCWVERSION[0],HEX);
  Serial.print("Radio Version (hex): ");
  Serial.print(WIFI.RADIOVERSION[1],HEX);Serial.print(" ");Serial.println(WIFI.RADIOVERSION[0],HEX);
  
  WIFI.setNetMask("255.255.255.0");  //modify default
  WIFI.setGateway("0.0.0.0");        //modify default
  Serial.println("New mask: 255.255.255.0");
  Serial.println("New gateway: 0.0.0.0");
  
  if (passw[0]==0) {WIFI.ConnSetOpen(nameAccP);}      // if passw= empty string connect in open mode
  else             {WIFI.ConnSetWPA(nameAccP,passw);} // else connect in WAP mode
  
  fc=WIFI.Connect();             // try to connect
  
  if (!fc)                       // end if no connection 
  {Serial.print("Can't connect to ");Serial.print(nameAccP);Serial.println(". Do nothing");return;}
  
  WIFI.getIP(ip);                // get dynamic ip
  Serial.print("Connected with ");Serial.print(nameAccP);
  Serial.print(" local ip is: ");Serial.println(ip);
  
  ssocket=WIFI.openServerTCP(port);// open server socket. Listen for connection from remote (just one connection)
                                  // we must poll in loop routine to see if connection asked from remote
  
  if (ssocket==255)                //socket non valid. No server open. 
  {WIFI.Disconnect();fc=0;Serial.println("Socket problem. Disconnected! Do nothing");return;}
  
  Serial.print("Server active on port ");Serial.println(port);
  //end of server startup 
} 
  

void loop() 
{
  if(fc)                           //if server active
  {
    if (!fs)                       //if no yet connection request on port port
    {                              // verify if someone asks for connection on port por
      csocket=WIFI.pollingAccept(ssocket); if(csocket<255) fs=1; //csoccket is the client socket to communicate
      if (fs) {Serial.print("Server connected with : ");WIFI.getRemoteIP(remip);Serial.println(remip);}
    }
    else                           // connection established
    {
      char *line=WIFI.readDataLn(csocket);       //try to read a record (data termined by line feed \n
      if (line!=NULL)                            //if received
          {
            decodeCommand(line);                 //call routine to decode and execute command
          } 
    }
  }
}

/* Command format (token space separated): commandname          */
/*                                         commandname data     */
/*                                         commandname data data*/

void decodeCommand(char *line)
{
   char command[15]="";
   char data1[15]="";
   char data2[15]="";
   sscanf(line,"%14s %14s %14s",command,data1,data2);                  //read line and tokenize
   if ((strlen(command)==0)|(strcmp(command,"?")==0)) {help();return;} //if no command or ? help
   if (strcmp(command,"RAnalog")==0)  {ranalog(data1);return;}         //Comand routines
   if (strcmp(command,"RDigital")==0) {rdigital(data1);return;}
   if (strcmp(command,"WAnalog")==0)  {wanalog(data1,data2);return;}
   if (strcmp(command,"WDigital")==0) {wdigital(data1,data2);return;}
   if (strcmp(command,"SetPin")==0)   {pinset(data1,data2);return;}
   if (strcmp(command,"CloseConn")==0){WIFI.closeSock(csocket);fs=false;return;}
}


/************************ command functions ******************************/
char pinErr[]="Pin number incorrect! Ana(0-5);Dig(4-13)(no7);Pwm(5,6,9,10,11)\r";
char answer[50];
  
void ranalog(char *data)
{
   int fok;
   int pin;
   fok=sscanf(data,"%d",&pin); if (fok==0) {WIFI.writeDataLn(csocket,pinErr);return;} //string to int
   if ((pin<0)|(pin>5)) {WIFI.writeDataLn(csocket,pinErr);return;}
   sprintf(answer,"Analog pin %d : %d\r",pin,analogRead(pin));
   WIFI.writeDataLn(csocket,answer);                                                  //answer
}

void rdigital(char *data)
{
   int fok;
   int pin;
   fok=sscanf(data,"%d",&pin); if (fok<=0) {WIFI.writeDataLn(csocket,pinErr);return;} 
   if ((pin<4)|(pin>13)) {WIFI.writeDataLn(csocket,pinErr);return;}
   sprintf(answer,"Digital pin %d : %d\r",pin,digitalRead(pin));
   WIFI.writeDataLn(csocket,answer);
}

void wdigital(char *data1, char *data2)
{
   int fok;
   int pin;
   int val;
   fok=sscanf(data1,"%d",&pin); if (fok<=0) {WIFI.writeDataLn(csocket,pinErr);return;} 
   if ((pin<4)|(pin>13)|(pin==7)) {WIFI.writeDataLn(csocket,pinErr);return;}
   fok=sscanf(data2,"%d",&val); if (fok<=0) {WIFI.writeDataLn(csocket,"No value!\r");return;} 
   if (val<0) val=0;if (val>1) val=1;
   digitalWrite(pin,val);
   sprintf(answer,"Digital pin %d set to %d \r",pin,val);
   WIFI.writeDataLn(csocket,answer);
}

void wanalog(char *data1, char *data2)
{
   int fok;
   int pin;
   int val;
   fok=sscanf(data1,"%d",&pin); if (fok<=0) {WIFI.writeDataLn(csocket,pinErr);return;} 
   if ((pin!=5)&(pin!=6)&(pin!=9)&(pin!=10)&(pin!=11)) {WIFI.writeDataLn(csocket,pinErr);return;}
   fok=sscanf(data2,"%d",&val); if (fok<=0) {WIFI.writeDataLn(csocket,"No value!\r");return;} 
   if (val<0) val=0;if (val>1024) val=1024;
   analogWrite(pin,val);
   sprintf(answer,"Pwm pin %d set to %d \r",pin,val);
   WIFI.writeDataLn(csocket,answer);
}

void pinset(char *data1, char *data2)
{ 
   int fok;
   int pin;
   int val;
   fok=sscanf(data1,"%d",&pin); if (fok<=0) {WIFI.writeDataLn(csocket,pinErr);return;} 
   if ((pin<4)|(pin>13)) {WIFI.writeDataLn(csocket,pinErr);return;}
   val=-1;
   if (strcmp(data2,"INPUT")==0) val=0;
   if (strcmp(data2,"OUTPUT")==0) val=1;
   if (strcmp(data2,"INPUT_PULLUP")==0) val=2;
   if (val<0) {WIFI.writeDataLn(csocket,"No valid option!\r");return;} 
   pinMode(pin,val);
   sprintf(answer,"Digital pin %d set to %s \r",pin,data2);
   WIFI.writeDataLn(csocket,answer);  
}

void help()
{
   WIFI.writeDataLn(csocket,"* Network commands program\r");
   WIFI.writeDataLn(csocket,"* Use: send line with command and data\r");
   WIFI.writeDataLn(csocket,"* Commands availlable:\r");
   WIFI.writeDataLn(csocket,"*   RAnalog  pin  Es.: RAnalog 3\r");
   WIFI.writeDataLn(csocket,"*   RDigital pin\r");
   WIFI.writeDataLn(csocket,"*   WAnalog  pin  val\r");
   WIFI.writeDataLn(csocket,"*   WDigital pin  val\r");
   WIFI.writeDataLn(csocket,"*   SetPin   pin  mode(INPUT/OUTPUT/INPUT_PULLUP)\r");
   WIFI.writeDataLn(csocket,"*   CloseConn\r\n******\r");
}

