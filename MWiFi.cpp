/*
  Copyright (c) 2014 Daniele Denaro.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
*/


/*************************************************************************************************/
/*
*                Library for Arduino WIFI shield by "Futura Elettronica"
*
*    This library has 3 functional parts:
*    1 - WIFI principal functions. To connect net and to create and connect 
*        socket and write or read.
*    2 - Setting functions. These functions are dedicated to set proprieties of 
*        shield and network.
*    3 - Basic functions (private) to talk with MCW1001A component using its 
*        protocol. These functions are called by major functions
*
*    Every command waits for answer (ACK or specialized answer) with time-out.
*    Asynchronous message are waiting too, when expected. But, in any case, it are eventually read 
*    before each command sending. (Real asynchronous management is not scheduled) 
*
*    Led 0 on when started up.
*    Led 1 on when connected
*    
*    Author: Daniele Denaro
*    Version: 2.4
*/
/*************************************************************************************************/

#include <MWiFi.h>

//static uint8_t SHORTMESS[7]={0x55,0xAA,0x00,0x00,0x00,0x00,0x45};
static uint8_t PREAMBLE[6]={0x55,0xAA,0x00,0x00,0x00,0x00};
static uint8_t GPIOMESS[9]={0x55,0xAA,0xAC,0x00,0x02,0x00,0x00,0x00,0x45}; 

static int next=0;
static int frb=0;

SoftwareSerialWIFI WIFISerial(RXPIN,TXPIN); //RX,TX


/************************************* Main functions ********************************************/

/*
* Setup MCW1001A (first function to issue).
* Hardware reset (pin 7).
* Read startup message at 115200 from MCW and then change serial speed to WIFISPEED define.
* Then switch on led 0.
*/
void MWiFi::begin()
{
         #if WIFIDEBUG  
         Serial.begin(9600);Serial.println("Debug on!"); 
         #endif
         wdt_disable();
         WIFISerial.begin(115200);
         pinMode(7,OUTPUT);digitalWrite(7,HIGH);delay(1);digitalWrite(7,LOW);
         receiveMessWait(10000);
         WIFISerial.begin(WIFISPEED);
         setLed(0,1);
         receiveMess(); // just to clean buffer
         delay(100);
         setSockSize();
         setARP(0);
         errorHandle=NULL;
}

/*
* It gets values from MCW.
* MAC address, IP, Net Mask , Gataway and status of net.
* Every value can be retrieved by specialized function.
*/
void MWiFi::getConfig()
{
          sendShortMess(48);            //command 48: net info
          receiveMessWait(8000);            //response 48 to cmd 48
          if (rxcode==48)
          {
            memcpy(&MAC,&rxmbuff[1],6);
            memcpy(&IP,&rxmbuff[7],4);
            memcpy(&NETMASK,&rxmbuff[23],4);
            memcpy(&GATEWAY,&rxmbuff[39],4);
            NETSTATUS=rxmbuff[55];
          }
}

/*
* Power down function. It switches off radio and clears every setting.
* Shield can be switched on, only by reset (or begin()).
*/
void MWiFi::setPowerOff()
{
          uint8_t mess[4]={1,0,0,0};
          sendLongMess(102,mess,4);
          receiveMessWait(1000); //ACK
          setLed(0,0);
}

/*
* Power save mode (PSPOLL). If millis <0 interval sugested by access point 
* Interval as multiple of 100ms
*/
void MWiFi::setPowerSave(int millis)
{
          uint8_t mess[4]={1,0,0,0};
          if (millis<0){mess[0]=2;}
          else {uint16_t time=millis/100;mess[0]=3;memcpy(&mess[2],&time,2);}
          sendLongMess(102,mess,4);
          receiveMessWait(1000); //ACK         
}

/*
* Exit from save mode
*/
void MWiFi::setFullPower()
{
          uint8_t mess[4]={4,0,0,0};
          sendLongMess(102,mess,4);
          receiveMessWait(1000); //ACK         
}

/*
* Network scanning.
* Returns number of visible networks .
*/
uint8_t  MWiFi::scanNets()
{
           uint8_t mess[2]={0xFF,0};
           sendLongMess(80,mess,2);
           receiveMessWait(1000); //ACK
           uint8_t nn=0;
           if (getAsyncWait(30000)==9) nn=NNETS ;
           return nn;
}

/*
* Returns a string of num net (< number returned by scanNets).
* String contains : 
* Net: SSID security(OPEN/WPA/WPA2/WEP) type(AccesPoint/AdHoc)  quality(RSSI value)
*/
char* MWiFi::getNetScanned(uint8_t num)
{
           uint8_t mess[2];
           mess[0]=num;
           mess[1]=0;
           sendLongMess(81,mess,2);
           receiveMessWait(3000);
           if (rxcode==22)
           {
              uint8_t pos=0;
//              int i;for(i=0;i<56;i++){Serial.print(i);Serial.print("|");Serial.print(rxmbuff[i],HEX);Serial.print(" ");}
//              Serial.println();
              free(netscn);free(ssidscn);
              uint8_t lssid=rxmbuff[6];
              netscn=(char*)malloc(lssid+22);
              ssidscn=(char*)malloc(lssid+1);
              memcpy(&netscn[pos],"Net: ",5);
              pos=5;
              memcpy(&netscn[pos],&rxmbuff[7],lssid);
              memcpy(ssidscn,&rxmbuff[7],lssid);ssidscn[lssid]='\0';
              pos=pos+lssid;
               if ((rxmbuff[39]&16)>0)
               { if ((rxmbuff[39]&64)>0)  {memcpy(&netscn[pos]," WPA  ",6);secure=1;}
                 if ((rxmbuff[39]&128)>0) {memcpy(&netscn[pos]," WPA2 ",6);secure=2;}
                 if ((rxmbuff[39]&192)==0){memcpy(&netscn[pos]," WEP  ",6);secure=3;}
               }
               else {memcpy(&netscn[pos]," OPEN ",6);secure=0;}
              pos=pos+6;
              if (rxmbuff[55]==1) {memcpy (&netscn[11+lssid],"Acc.P",5);access=0;}
              else {memcpy (&netscn[11+lssid],"AdHoc",5);access=1;}
              pos=pos+5;
              RSSI=rxmbuff[52];sprintf(&netscn[pos]," %3d ",RSSI);
              netscn[lssid+22]='\0';
              return netscn;
           }
           else return NULL;
}

/*
* Returns a string of net identified by name or null if it doesn't exist.
* String contains : 
* Net: SSID security(OPEN/WPA/WPA2/WEP) type(AccesPoint/AdHoc)  quality(RSSI value)
*/
char* MWiFi::existSSID(char *name)
{
           int i;
           char *SSID;
           int n=scanNets();
           for(i=0;i<n;i++) 
           {SSID=getNetScanned(i);if (strncmp(name,&SSID[5],strlen(name))==0) return SSID;}
           return NULL; 
}

/*
* Returns SSID string of best of totnets scanned.
* But only if OPEN and Access Point type. 
*/
char* MWiFi::getSSIDBestOpen(uint8_t totnets)
{
            int i;char* s;int q=0;int index=0;
            for (i=0;i<totnets;i++)
            {
              s=getNetScanned(i);
              if (s==NULL) {break;}
              if ((secure==0)&(access==0)) {if (RSSI>q) {q=RSSI;index=i;}}
            }
            if (q>0) {s=getNetScanned(index);return ssidscn;} 
            else return NULL;
}

/*
* Sets access mode : 
*         1=infrastructure (access point) (default)
*         2=ad hoc
*/
void MWiFi::setNetMode(uint8_t mode)
{
           uint8_t lmess=2;
           uint8_t mess[lmess];
           mess[0]=1;
           mess[1]=mode;
           sendLongMess(55,mess,lmess);      //cmd 55: to set mode          
           receiveMessWait(3000); //ACK
}

/*
* Prepare a connection to a open network (not secure net) with this SSID.
* Params: SSID and its length.
* Or SSID as null terminated string.
*/
void MWiFi::ConnSetOpen(char *ssid, uint8_t lssid)
{
          setSsid(ssid,lssid);
          uint8_t lmess=2;
          uint8_t mess[lmess];
          mess[0]=1;
          mess[1]=0;
          sendLongMess(65,mess,lmess);      //cmd 65: to set non protected connection          
          receiveMessWait(3000); //ACK
}
void MWiFi::ConnSetOpen(char *ssid)
{
           ConnSetOpen(ssid,strlen(ssid));
}

/*
* Prepare a connection to a sicure network (WPA or WPA2). 
* Params: SSID and its length; password and its length.
* Or SSID and password as null terminated strings.
*/
void MWiFi::ConnSetWPA(char *ssid, uint8_t lssid, char *psw, uint8_t lpsw)
{
          setSsid(ssid,lssid);
          uint8_t lmess=4+lpsw;
          uint8_t mess[lmess];
          mess[0]=1;
          mess[1]=8;
          mess[2]=0;
          mess[3]=lpsw;
          memcpy(&mess[4],psw,lpsw);
          sendLongMess(68,mess,lmess);      //cmd 68: setting WPA or WPA2 connection
          receiveMessWait(3000);  //ACK
}
void MWiFi::ConnSetWPA(char *ssid, char *psw)
{
          ConnSetWPA(ssid,strlen(ssid),psw,strlen(psw)); 
}

/*
* Prepare and connect (convenience function) open access point ssid
* Returns 1 if connected or 0 otherwise.
*/
uint8_t MWiFi::ConnectOpen(char *ssid)
{
          ConnSetOpen(ssid);
          return Connect();
}

/*
* Prepare and connect (convenience function) with WPA password
* Returns 1 if connected or 0 otherwise.
*/
uint8_t MWiFi::ConnectWPAwithPsw(char *ssid, char *psw)
{
          ConnSetWPA(ssid,psw);
          return Connect();
}

/*
* Prepares and connects with WPA password and returns calculated key for next 
* faster connection.
* Buffer key of 32 bytes can be used to connect ssid next time.
* Returns 1 if connected or 0 otherwise and key not availlable.
*/
uint8_t MWiFi::ConnectWPAandGetKey(char *ssid,char *psw, uint8_t key[32])
{
          ConnSetWPA(ssid,psw);
          uint8_t fc=Connect();
          if (!fc) return fc;
          uint8_t mess[2]={1,0};
          sendLongMess(71,mess,2);
          receiveMessWait(1000); //key back
          if (rxcode==49)
          { memcpy(key,&rxmbuff[0],32); }
          return fc;
}

/*
* Connects ssid using numeric key (previously calculated on password base)
* Much faster than password connection.
* Returns 1 if connected or 0 otherwise.
*/
uint8_t MWiFi::ConnectWPAwithKey(char *ssid,uint8_t key[32])
{
          uint8_t lssid=strlen(ssid);
          setSsid(ssid,lssid);
          uint8_t lmess=4+32;
          uint8_t mess[lmess];
          mess[0]=1;
          mess[1]=7;                    //using key instead of password
          mess[2]=0;
          mess[3]=32;
          memcpy(&mess[4],key,32);
          sendLongMess(68,mess,lmess);  //cmd 68: setting WPA or WPA2 connection
          receiveMessWait(3000);  //ACK
          return Connect();
}

/*
* Version EEPROM based of connection functions with key.
* This kind of functions let you use connection key without 32 RAM bytes binding 
* Please note that 33 bytes are used on EEPROM : 1 flag + 32 key
* EEadd can be from 0 to max EEPROM-33
* EEPROM bytes are written only if differ from previous value.
* Key from connection function is written only if connection is ok.
*/
uint8_t MWiFi::ConnectWPAandGetKeyEE(char *ssid,char *psw, int EEadd)
{
          ConnSetWPA(ssid,psw);
          uint8_t fc=Connect();
          if (!fc) return fc;
          uint8_t mess[2]={1,0};
          sendLongMess(71,mess,2);
          receiveMessWait(1000); //key back
          if (rxcode==49)
          { uint8_t i;uint8_t ee;
            for(i=0;i<32;i++) 
            {
             ee=EEPROM.read(EEadd+1+i);
             if (ee==rxmbuff[i]) continue;
             EEPROM.write(EEadd+1+i,rxmbuff[i]);
            }
            if (EEPROM.read(EEadd)!=1) EEPROM.write(EEadd,1);//flag key in EEPROM
          }
          return fc;
}

uint8_t MWiFi::ConnectWPAwithKeyEE(char *ssid,int EEadd)
{
          uint8_t lssid=strlen(ssid);
          setSsid(ssid,lssid);
          uint8_t lmess=4+32;
          uint8_t mess[lmess];
          mess[0]=1;
          mess[1]=7;                    //using key instead of password
          mess[2]=0;
          mess[3]=32;
          uint8_t i;for (i=0;i<32;i++)mess[4+i]=EEPROM.read(EEadd+1+i);
          sendLongMess(68,mess,lmess);  //cmd 68: setting WPA or WPA2 connection
          receiveMessWait(3000);  //ACK
          return Connect();
}

void MWiFi::storeKeyOnEEPROM(uint8_t key[32],int EEadd)
{
          int i;
          for (i=0;i<32;i++)
          {if (key[i]==EEPROM.read(EEadd+i+1)) continue;
           EEPROM.write(EEadd+i+1,key[i]);}
          if (EEPROM.read(EEadd)==1) return;   //flag key in EEPROM
          else EEPROM.write(EEadd,1); 
}

bool MWiFi::isKeyInEEPROM(int EEadd)
{
          if (EEPROM.read(EEadd)==1) return true;
          else return false;
}


/*
* Connect step (after setting connection).
* It can take until 30 sec if secure connection (ascii password transforming)
* Return: 1 = Connection done ; 0 = no connection
* If connected switch on led 1
*/
uint8_t MWiFi::Connect()
{
          uint8_t cf=0;
          uint8_t lmess=2;
          uint8_t mess[lmess];
          mess[0]=1;
          mess[1]=0;
          sendLongMess(90,mess,lmess);      //cmd 90: connect
          receiveMessWait(1000);              // ACK
          CONNSTATUS=0;
          IP[0]=0;IP[1]=0;IP[2]=0;IP[3]=0;
          uint8_t i;
          for (i=0;i<CONNTOUT;i++){if (CONNSTATUS==0) getAsyncWait(1000);else break;}
          if (CONNSTATUS==1) for (i=0;i<CONNTOUT;i++){if (IP[0]==0) getAsyncWait(1000);else break;}
          if (CONNSTATUS==1) {setLed(1,1); cf=1;} else {setLed(1,0);cf=0;}
          return cf;
}

/*
* Disconnect. Switch off led 1
*/
uint8_t MWiFi::Disconnect()
{
          uint8_t cf=1;
          uint8_t ev=0;
          if (CONNSTATUS!=1) {CONNSTATUS=0; setLed(1,0);cf=0;return cf;}
          sendShortMess(91);                //cmd 91: disconnect
          receiveMessWait(1000); //ACK
          uint8_t i;
          // async for Connection status changed
          for (i=0;i<CONNTOUT;i++) {if(ev!=16) ev=getAsyncWait(1000);else break;}   
          if (ev==16) {CONNSTATUS=0; setLed(1,0);cf=0;} 
          return cf;
}

/*
* Ping to a remote IP.
* String ip (null terminated) format: n.n.n.n (where n : decimal number 0-255) 
* Return 1 if echo received; 0 if timeout (4 sec by MCW)
*/
uint8_t MWiFi::ping(char *ipremote)
{
          PINGOK=255;
          sscanf(ipremote,"%3d.%3d.%3d.%3d",&REMOTEIP[0],&REMOTEIP[1],&REMOTEIP[2],&REMOTEIP[3]);
          uint8_t mess[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
          memcpy(mess,REMOTEIP,4);
          sendLongMess(121,mess,16);        //cmd 121: ping
          receiveMessWait(2000);              //ACK
          getAsyncWait(10000);                //async for ping answer
          if (PINGOK==0) return 1;else return 0;
}



/*
* Open a TCP Socket to a remote IP and Port port
* IP as a string n.n.n.n (where n= decimal number 0-255)
* Returns socket if connection ok; 0xFF if it can't open socket.
*/
uint8_t MWiFi::openSockTCP(char *ipremote,uint16_t port)
{
          sscanf(ipremote,"%3d.%3d.%3d.%3d",&REMOTEIP[0],&REMOTEIP[1],&REMOTEIP[2],&REMOTEIP[3]);
          uint8_t ok=0xFE;
          uint8_t socket=createSockTCP();        //allocate socket (cmd 110) 
          if (socket>=0xFE) return 0xFF;
          uint8_t mess2[20];memset(mess2,0,20);
          mess2[0]=socket;
          memcpy(&mess2[2],&port,2);
          memcpy(&mess2[4],&REMOTEIP[0],4);
          uint8_t i;
          for(i=0;i<250;i++)
          {
           sendLongMess(113,mess2,20);          //command 113: connect to IP and remote Port
           receiveMessWait(1000);               //response 25: response to cmd 113
           if (rxcode==25)
            {ok=rxmbuff[0];if (ok!=0xFE) break;}// 0xFE : connection in progress
           delay(100);
          }
          if ((ok==0xFF)|(ok==0xFE)) {closeSock(socket);return 0xFF;} //0xFF : no connection
          return socket;
}

/*
* Open a TCP Server listening on Port port
* Returns socket. (0xFF if can't)
*/
uint8_t MWiFi::openServerTCP(uint16_t port)
{
          uint8_t ok=0;
          uint8_t socket=createSockTCP();                       //create socket
          if (socket>=0xFE) return 0xFF;
          uint8_t mess[4];
          memcpy(&mess[0],&port,2);
          mess[2]=socket;mess[3]=0;
          sendLongMess(112,mess,4);           //cmd 112: bind socket to port
          receiveMessWait(1000);                //response 24: response to cmd 112
          if (rxcode==24) if (rxmbuff[2]>0) return 0XFF;
          uint8_t mess2[2];
          mess2[0]=socket;
          mess2[1]=1;
          sendLongMess(114,mess2,2);          //cmd 114: on listening status
          uint8_t i;
          for(i=0;i<250;i++)
          {
           receiveMessWait(1000);                //response 26: response to cmd 114
           if (rxcode==26) {ok=rxmbuff[0];        // 0xFE : connection in progress
           if (ok!=0xFE) break;}
           delay(100);
          }
          if ((ok==0xFF)|(ok==0xFE)) {closeSock(socket);return 0xFF;} //0xFF : no connection
          return socket;          
}

/*
* Used when in server mode. 
* Polling for Accept command (i.e. accept is non blocking).
* If remote ip ask for connection it returns client socket ; else  FF
* Client socket must be used to read and write
* Remote ip asking for connection can be retrieved by function getRemoteIP()
*/
uint8_t MWiFi::pollingAccept(uint8_t sock)
{
          uint8_t sk;
          uint8_t mess[2];
          mess[0]=sock;mess[1]=0;
          sendLongMess(115,mess,2);          //cmd 115: accept mode command
          receiveMessWait(1000);               //response 27: response to cmd 115
          if (rxcode==27) 
          {
            if (rxmbuff[0]>=0xFE) return 0xFF;
 //           if (rxmbuff[0]==255) return 0;
            sk=rxmbuff[0];
            memcpy(&REMOTEIP[0],&rxmbuff[4],4);
            return sk;
          }
          return 0xFF;
}

/*
* Write data on socket sk.
* Buffer contains data and lbuff is its length.
* Or buffer contains string (null terminated).
* Or buffer string will be padded with line feed
* Plus version to send data from progmem.
* Return: bytes number really sent.  
*/
uint16_t MWiFi::writeData(uint8_t sk,uint8_t *buffer,uint16_t lbuff)
{
          return sendFromMem(sk,buffer,lbuff,0);
}
uint16_t MWiFi::writeData(uint8_t sk,char *buffer)
{
          uint16_t len=strlen(buffer);
          return sendFromMem(sk,(uint8_t*)buffer,len,0);
}
uint16_t MWiFi::writeDataLn(uint8_t sk,char *buffer)
{
          uint8_t len=strlen(buffer);
          return sendFromMem(sk,(uint8_t*)buffer,len,1);
}
uint16_t MWiFi::writeDataPM(uint8_t sk,prog_char *buffer,uint16_t lbuff)
{
          return sendFromProgMem(sk,buffer,lbuff,0);
}
uint16_t MWiFi::writeDataPM(uint8_t sk,prog_char *buffer)
{
          uint16_t len=strlen_P(buffer);
          return sendFromProgMem(sk,buffer,len,0);
}
uint16_t MWiFi::writeDataLnPM(uint8_t sk,prog_char *buffer)
{
          uint8_t len=strlen_P(buffer);
          return sendFromProgMem(sk,buffer,len,1);
}


/*
* Read data from socket sk. (It is not blocking)
* Buffer is the container and lbuff its length.
* If no bytes are available it returns 0.
* Else returns number of bytes actually read.
* Or gets data until line feed and returns string substituding ln with '\0' 
* (end string)  
*/
uint16_t MWiFi::readData(uint8_t sk,uint8_t *buffer,uint16_t lbuff)
{
          uint16_t bread=0;
          uint16_t lb=lbuff;
          if (buffer==NULL) lb=0;
          uint8_t mess[4];
          mess[0]=sk;
          mess[1]=0;
          memcpy(&mess[2],&lb,2);
          sendLongMess(117,mess,4);          //cmd 117: receive data
          receiveMessWait(10000);              //response 29: response to cmd 117
          if (rxcode==29) {memcpy(&bread,&rxmbuff[2],2);}
          if (buffer==NULL) return bread;
          else {memcpy(buffer,&rxmbuff[4],bread);return bread;}
}
char* MWiFi::readDataLn(uint8_t sk)
{
          return readLine(sk);          
}

/* 
* Version with timeout (milliseconds)
*/
char* MWiFi::readDataLn(int sk, int millis)
{
          int n=millis/10;
          char *rec=NULL;
          int i;
          for (i=0;i<n;i++)
                {rec=readLine(sk);if (rec!=NULL) break;else delay(10);}
          return rec;
}


/*
* Close socket
*/
void MWiFi::closeSock(uint8_t sk)
{
 //         cleanBuff(sk);
          uint8_t mess[2]={0,0};
          mess[0]=sk;
          sendLongMess(111,mess,2);        //cmd 111: close socket
          receiveMessWait(2000);            //ACK
}


/*********************************  Setting Functions ********************************************/

/*
* Gets MAC address of shield. Call after getConfig() function
* Needs a buffer 18 bytes long. String format (hex) xx.xx.xx.xx.xx.xx
*/
void MWiFi::getMAC(char mac[18])
{
          snprintf(mac,18,"%02X.%02X.%02X.%02X.%02X.%02X",MAC[0],MAC[1],MAC[2],MAC[3],MAC[4],MAC[5]);
}

/*
* Gets name issued by shield to the network when connected. 
* Made using first 3 field of MAC address.
* Needs a buffer 8 bytes long.
*/
void MWiFi::getName(char name[8])
{
           sprintf(name,"W%02X%02X%02X",MAC[0],MAC[1],MAC[2]);
}

/*
* Sets IP (if it is not dynamic)
* String IP format: n.n.n.n (where n : decimal number 0-255) 
*/
void MWiFi::setIP(char *IPstring)
{
          sscanf(IPstring,"%3d.%3d.%3d.%3d",&IP[0],&IP[1],&IP[2],&IP[3]);
          uint8_t mess[18]={0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
          memcpy(&mess[2],IP,4);
          sendLongMess(41,mess,18);
          receiveMessWait(2000);  //ACK              
}

/*
* Gets string IP address(use after connection to obtain dynamic IP)
* Needs a buffer 16 bytes long.
* String ip (null terminated) format: n.n.n.n (where n : decimal number 0-255) 
*/ 
void MWiFi::getIP(char ip[16])
{
           sprintf(ip,"%d.%d.%d.%d",IP[0],IP[1],IP[2],IP[3]);
}

/*
* It makes IP dynamic (default) (only for reset after not dynamic use))
*/
void MWiFi::setIPdhcp()
{
          uint8_t mess[18]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
          sendLongMess(41,mess,18);
          receiveMessWait(2000);  //ACK   
}


/*
* Set the Net Mask 
* String mask (null terminated) format: n.n.n.n (where n : decimal number 0-255) 
*/
void MWiFi::setNetMask(char *MASKstring)
{
          sscanf(MASKstring,"%3d.%3d.%3d.%3d",&NETMASK[0],&NETMASK[1],&NETMASK[2],&NETMASK[3]);
          uint8_t mess[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
          memcpy(&mess[0],NETMASK,4);
          sendLongMess(42,mess,16);
          receiveMessWait(2000);  //ACK
}

/*
* Gets string Net Mask address
* Needs a buffer 16 bytes long.
* String mask format: n.n.n.n (where n : decimal number 0-255) 
*/ 
void MWiFi::getNetMask(char mask[16])
{
          sprintf(mask,"%d.%d.%d.%d",NETMASK[0],NETMASK[1],NETMASK[2],NETMASK[3]);
}


/*
* Sets the Gateway address
* String gatw (null terminated) format: n.n.n.n (where n : decimal number 0-255) 
*/
void MWiFi::setGateway(char *GATWstring)
{
          sscanf(GATWstring,"%3d.%3d.%3d.%3d",&GATEWAY[0],&GATEWAY[1],&GATEWAY[2],&GATEWAY[3]);
          uint8_t mess[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
          memcpy(&mess[0],GATEWAY,4);
          sendLongMess(44,mess,16);
          receiveMessWait(2000);  //ACK
}

/*
* Gets string Gateway address
* Needs a buffer 16 bytes long.
* String gatw format: n.n.n.n (where n : decimal number 0-255) 
*/ 
void MWiFi::getGateway(char gatw[16])
{
          sprintf(gatw,"%d.%d.%d.%d",GATEWAY[0],GATEWAY[1],GATEWAY[2],GATEWAY[3]);
}

/*
* Gets string IP of remote address(ask for connection on server
* Use when in server mode.
* Needs a buffer 16 bytes long.
* String IP format: n.n.n.n (where n : decimal number 0-255) 
*/ 
void MWiFi::getRemoteIP(char remoteIP[16])    
{
          sprintf(remoteIP,"%d.%d.%d.%d",REMOTEIP[0],REMOTEIP[1],REMOTEIP[2],REMOTEIP[3]);
}


/*
* Reset command should stimulate an async reset response (some problem)
*/
void MWiFi::resetMCW()
{
         setPowerOff();
         digitalWrite(7,HIGH);delay(100);
         begin();

/*
         STARTUPBITS=0;
         sendShortMess(170);
         receiveMessWait(2000);          //ACK
         while(STARTUPBITS==0) getAsyncWait(10000);  //Async reset data
         if (STARTUPBITS==0) wdt_enable(WDTO_1S);
*/         
}

/*
* Specialized function to switch on/off led on WIFI shield.
* led (0 to 3); state 1(on) 0(off)
*/
void MWiFi::setLed(int led,int state)
{
        sendGPMessage(led,state); 
        receiveMessWait(2000);
}


void MWiFi::setARP(uint16_t sec)
{
          sendLongMess(173,(uint8_t*)&sec,2);
 //     printHEX(txbuff,9);    
          receiveMessWait(2000);
}




/*********************** Utilities (subfunctions called by principal functions) ******************/
#if WIFIDEBUG
void MWiFi::printHEX(unsigned char *buff,int len)
{
	int i;
	for (i=0;i<len;i++) {Serial.print(buff[i],HEX);Serial.print(" ");}
	Serial.println();
}


void MWiFi::printDEC(unsigned char *buff,int len)
{
	int i;
	for (i=0;i<len;i++) {Serial.print(buff[i],DEC);Serial.print(" ");}
	Serial.println();
}


void MWiFi::printAsync(int ev)
{
        switch (ev)
        {
          case 8: Serial.print("Event connection: ");
                  Serial.print(CONNSTATUS);Serial.print(" , ");Serial.println(CONNEVDATA);break;
          case 16: Serial.print("Event IP: ");printDEC(IP,4);break;
          case 255: Serial.print("ERROR : ");Serial.println(ERRORTYPE);break;       
        }
}
#endif

void MWiFi::setSockSize()
{
          uint8_t mess[10];
          mess[0]=1;                          //1 server socket
          mess[1]=2;                          //2 client socket
          uint16_t v20=2000;
          uint16_t v10=1000;
          memcpy(&mess[2],&v20,2);            // server rx buff
          memcpy(&mess[4],&v20,2);            // server tx buff
          memcpy(&mess[6],&v10,2);            // client rx buff
          memcpy(&mess[8],&v10,2);            // client tx buff
          sendLongMess(122,mess,10);        //cmd 122: set sockets
          receiveMessWait(2000); 
}  

/*
* Create a stream of minimum standard length (7) with code command and no command data.
*/
void MWiFi::sendShortMess(uint8_t code)
{
	      PREAMBLE[2]=code;
	      memset(&PREAMBLE[3],0,3);
	      int i;for (i=0;i<6;i++) WIFISerial.write(PREAMBLE[i]);
	      WIFISerial.write(0x45);
	      uint8_t ev=getAsync();     //eventually read async mess (simulate polling)
        #if WIFIDEBUG
        if(ev!=0) printAsync(ev);
        Serial.print(" > ");
        for (i=0;i<6;i++)   {Serial.print(PREAMBLE[i],HEX);Serial.print(" ");}
                             Serial.print(0x45,HEX);Serial.println();        
        #endif

}

/*
* Specialized stream for GPIO command.
*/
void MWiFi::sendGPMessage(int gp,int s)
{
        GPIOMESS[6]=(uint8_t)gp;
        GPIOMESS[7]=(uint8_t)s;
	      int i;for (i=0;i<9;i++) WIFISerial.write(GPIOMESS[i]);
        uint8_t ev=getAsync();     //eventually read async mess (simulate polling)
        #if WIFIDEBUG
        if(ev!=0) printAsync(ev);
        Serial.print(" > ");
        for (i=0;i<6;i++)   {Serial.print(GPIOMESS[i],HEX);Serial.print(" ");}
        #endif
	      
}

/*
* Create a stream with headers and trailer, containing a specialized message with its code, 
* length and command data. 
* Send command stream on WIFISerial.
* And clean receive buffer reading possible async event.
*/
void MWiFi::sendLongMess(uint8_t code,uint8_t *buff,uint16_t len)
{
       	PREAMBLE[2]=code;PREAMBLE[3]=0;
        memcpy(&PREAMBLE[4],&len,2);
        int i;
        for (i=0;i<6;i++)   WIFISerial.write(PREAMBLE[i]);
        for (i=0;i<len;i++) WIFISerial.write(buff[i]);
                            WIFISerial.write(0x45);
        uint8_t ev=getAsync();     //eventually read async mess (simulate polling)
        #if WIFIDEBUG
        if(ev!=0) printAsync(ev);
        Serial.print(" > ");
        for (i=0;i<6;i++)   {Serial.print(PREAMBLE[i],HEX);Serial.print(" ");}
        for (i=0;i<len;i++) {Serial.print(buff[i],HEX);Serial.print(" ");}
                             Serial.print(0x45,HEX);Serial.println();        
        #endif
}


/*
* Set millisec waiting timer (for receiveMessageWait function time out)
*/
void MWiFi::setTimer(unsigned long ms)
{
        stimer=millis()+ms;
}

/*
* Check if time out
*/
int MWiFi::getTimeout()
{
        if (stimer<millis())return 1;
        else return 0;
}

/*
* Receive function from WIFISerial with timeout.
* It waits until first byte is available on WIFISerial, or time out. 
* Then it starts to receive answer stream, until trailer or fixed time out.
*/
int MWiFi::receiveMessWait(unsigned long ms)
{
        setTimer(ms);
        rxcode=0xFD;
        while(WIFISerial.available()==0)
          {delay(10);if(getTimeout()) {return rxcode;}}
        return receiveMess();
}

/*
* Called by receiveMessWait function when first byte is available.
* But it has 1 sec for complete message receiving.
* Then it detects code and data length. With this length it creates buffer to receive data.
* The buffer is rxmbuff and its length rxlmbuff.
* The trailer is detected for message validating.
* It returns the code. But it is also available as rxcode variable.
*/
int MWiFi::receiveMess()
{
        uint8_t b=0;
        rxcode=0xFD;
        rxlmbuff=0 ;
        free(rxmbuff);
        rxmbuff=NULL;
        pbyte=0;
        int e=0;
        setTimer(4000);
        while(WIFISerial.available()>0)
        {
          delayMicroseconds(100);
          b=WIFISerial.read();
          readMess(b,&e);
          #if WIFIDEBUG
          Serial.print(b,HEX);Serial.print(" ");
          #endif
          if (e<0) break;
          if (getTimeout()) 
           {rxcode=0xFE;rxlmbuff=0;rxmbuff=NULL;
           #if WIFIDEBUG
           Serial.print("Code: ");Serial.println(rxcode,HEX);
           #endif  
           return rxcode;}
        }
        if (rxcode==1) decodeAsync();
        #if WIFIDEBUG 
        if (rxcode==1){Serial.print("Event: ");Serial.println(rxmbuff[0],HEX);}
        if (rxcode==0){Serial.println("Code: ACK");}
        else if ((rxcode!=0xFD)&(rxcode!=1)) {Serial.print("Code: ");Serial.println(rxcode,HEX);} 
        #endif        
        return rxcode;
}

/*
* Called by receiveMess()
* Single byte reading and decoding. Based on pointer starting with 0x55.
*/
void MWiFi::readMess(uint8_t b,int *e)
{
        switch (pbyte)
        {
          case 0: if (b==0x55) pbyte=1;break;
          case 1: if (b==0xAA) pbyte=2;break;
          case 2: rxcode=b;pbyte=3;break;
          case 3: if (b==0x80) {rxcode=0;};pbyte=4;break;
          case 4: memcpy(&rxlmbuff,&b,1);pbyte=5;break;
          case 5: memcpy(&rxlmbuff+1,&b,1);pbyte=6;
                  if (rxlmbuff>0) {rxmbuff=(uint8_t*)malloc(rxlmbuff);};break;
          case 6: if (b==0x45) {*e=-1;rxmbuff=NULL;break;}
          default:if (pbyte<rxlmbuff+6) {rxmbuff[pbyte-6]=b;pbyte++;break;} 
                  else 
                  {if (b==0x45) 
                    {*e=-1;break;}else {*e=-1;rxcode=0xFF;rxlmbuff=0;rxmbuff=NULL;break;}}
        } 
}

/*
* It decodes possible asynchronous message.
* It returns event code or 0 (if no async.).
*/
uint8_t MWiFi::getAsync()
{        
         uint8_t cd=receiveMess();
         if (cd!=1) return 0;
         return decodeAsync();
}

uint8_t MWiFi::decodeAsync()
{
         uint8_t ev=rxmbuff[0];
         switch (ev)
         {
           case 8: CONNSTATUS=rxmbuff[1];CONNEVDATA=rxmbuff[2];
                   if (CONNSTATUS==5) connectionLost();break;
           case 9: NNETS=rxmbuff[1];break;
           case 16: memcpy(&IP[0],&rxmbuff[2],4);break;
           case 26: PINGOK=rxmbuff[1];memcpy(&PINGTIME,&rxmbuff[2],2);break;
           case 27: STARTUPBITS=rxmbuff[1];memcpy(&MCWVERSION[0],&rxmbuff[2],2);
                    memcpy(&RADIOVERSION[0],&rxmbuff[4],2);break;
           case 255: memcpy(&ERRORTYPE,&rxmbuff[2],2);errorRoutine();break;
         }
         #if WIFIDEBUG
         printAsync(ev);
         #endif
         return ev;
}

/*
* As getAsync() but it waits until message arrives. 
* Useful when command needs an answer but it is provided by async. transmission. 
*/
uint8_t MWiFi::getAsyncWait(unsigned long ms)
{
         setTimer(ms);
         uint8_t cd=0;
         while(WIFISerial.available()==0)
          {delay(10);if(getTimeout()) {return cd;}}
         return getAsync();
}


/*
* It sets SSID (hot spot name)
*/
void MWiFi::setSsid(char *ssid, uint8_t lssid)
{
          uint8_t lmess=2+lssid;
          uint8_t mess[lmess];
          mess[0]=1;
          mess[1]=lssid;
          memcpy(&mess[2],ssid,lssid);
          sendLongMess(57,mess,lmess);
          receiveMessWait(2000); //ACK
}

/*
* Create a TCP socket. 
* It returns the socket number (handle) and retained it in global variable socket. 
*/
uint8_t MWiFi::createSockTCP()
{
          uint8_t socket=0xFE;
          uint8_t mess[2]={1,0};
          sendLongMess(110,mess,2);      //cmd 110: allocate socket
          receiveMessWait(2000);          //ACK
          if (rxcode==23) socket=rxmbuff[0];
          return socket;
}

uint16_t MWiFi::sendFromMem(uint8_t sk,uint8_t *buffer,uint16_t lbuff,uint8_t ln)
{
          uint16_t bsent=0;
          int len=lbuff+4;
          if (ln) len++;
       	  PREAMBLE[2]=116;PREAMBLE[3]=0;      //cmd 116: send data
          memcpy(&PREAMBLE[4],&len,2);
          int i,tbuff=lbuff;
          uint8_t info[4];
          info[0]=sk;info[1]=0;
          if (ln) tbuff++; 
          memcpy(&info[2],&tbuff,2);
          for (i=0;i<6;i++)     WIFISerial.write(PREAMBLE[i]);
          for (i=0;i<4;i++)     WIFISerial.write(info[i]);
          for (i=0;i<lbuff;i++) WIFISerial.write(buffer[i]);
          if (ln)               WIFISerial.write('\n');
                                WIFISerial.write(0x45);
          receiveMessWait(30000);                //response 28: response to cmd 116
          if (rxcode==28) memcpy(&bsent,rxmbuff,2);
          return bsent;
}

uint16_t MWiFi::sendFromProgMem(uint8_t sk,prog_char *pgbuffer,uint16_t lbuff,uint8_t ln)
{
          uint16_t bsent=0;
          int len=lbuff+4;
          if (ln) len++;
       	  PREAMBLE[2]=116;PREAMBLE[3]=0;      //cmd 116: send data
          memcpy(&PREAMBLE[4],&len,2);
          int i,tbuff=lbuff;;
          uint8_t info[4];
          info[0]=sk;info[1]=0;
          if (ln) tbuff++;
          memcpy(&info[2],&tbuff,2);
          for (i=0;i<6;i++)     WIFISerial.write(PREAMBLE[i]);
          for (i=0;i<4;i++)     WIFISerial.write(info[i]);
          for (i=0;i<lbuff;i++) WIFISerial.write(pgm_read_byte(pgbuffer+i));
          if (ln)               WIFISerial.write('\n');
                                WIFISerial.write(0x45);
          receiveMessWait(30000);                //response 28: response to cmd 116
          if (rxcode==28) memcpy(&bsent,rxmbuff,2);
          return bsent;
}


/*Reads a line (string terminated by line feed) at a time, from data received. 
* It uses a buffer long LINEBUFFLEN (default 81).
* So, max string length is LINEBUFFLEN-1 (for null terminator) (default 80) 
* If line das not contains line feed it is terminated at 80 char anyway.
* That is,it returns a null terminated string corresponding to one record, 
* or max 80 char (null terminated)
* When end of data it returns NULL
*/

char* MWiFi::readLine(int sk)
{
	if (next>0) 
	{
	  int i=0;for (i=0;i<frb;i++)
	  {if ((i+next)>=frb) linebuff[i]='\0';	else linebuff[i]=linebuff[i+next];}
	  frb=frb-next;
	}
	int len=LINEBUFFLEN-frb-1;
	int cb=readData(sk,(uint8_t*)&linebuff[frb],len);
	frb=frb+cb;
	if (frb==0){next=0;return NULL;}
	int plf;
	char* lf=(char*)memchr(linebuff,'\n',frb-1);
	if (lf!=NULL) {plf=lf-linebuff;next=plf+1;}
	else {plf=frb; next=plf;}
	linebuff[plf]='\0';
  return linebuff;
}



void MWiFi::cleanBuff(int sk)
{
   int n=1;
   while (n>0)
   {
    n=readData(sk,(uint8_t*)linebuff,LINEBUFFLEN-1);
    if (n>0) continue; 
    else {delay(10);n=readData(sk,(uint8_t*)linebuff,LINEBUFFLEN-1);}
   }
   next=0;frb=0;
}

void MWiFi::resetBuff()
{
   next=0;frb=0;
}

void MWiFi::connectionLost()
{
      if (ERRLOG) 
      {
       char pbuff[30];
       snprintf(pbuff,30,"Connection status: %d details: %d",CONNSTATUS,CONNEVDATA);
       Serial.println(pbuff);
      }
      wdt_enable(WDTO_15MS);
      delay(20); 
}

void MWiFi::errorRoutine()
{
      if (errorHandle!=NULL){errorHandle(ERRORTYPE);return;}
      if (ERRLOG) 
      {
        char pbuff[20];
        if (ERRORTYPE>72) snprintf(pbuff,20,"WFWrn: %2d ",ERRORTYPE);
        else snprintf(pbuff,20,"WFErr: %2d ",ERRORTYPE);
        Serial.println(pbuff);
      }
      if (ERRORTYPE<73){wdt_enable(WDTO_15MS);delay(20);}
}


/***************************************** END LIBRARY *******************************************/





