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

/******************************************************************************/
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
*    Asynchronous message are waiting too, when expected. But, in any case, 
*    it are eventually read before each command sending. 
*    (Real asyncronous management is not scheduled) 
*
*    Led 0 on when started up.
*    Led 1 on when connected
*    
*    Arduino pin utilized: RXPIN,TXPIN (see define),D7 fixed (for shield reset)
*    
*    Author: Daniele Denaro
*    Version: 2.4
*/
/******************************************************************************/
/*********************** Included in library **********************************/
#ifndef MWiFi_h
#define MWiFi_h

#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <Arduino.h>
#include <utility/SoftwareSerialWIFI.h>
#include <utility/EEPROM.h>

#define WIFIDEBUG 0        //for debug use only (use 1 for debugging)
#define ERRLOG 1           //serial output of error code(set 1 if you like it) 


#define LINEBUFFLEN 84     //buffer length for reading data received as record
                           //and used also by HTTPlib 
                           //change it if you have particular requirements 

#define CONNTOUT  60       //Time out for connection trying (in sec)


// Definitions for  SoftSerial internal communication
#define RXPIN 2            //Pin used by SoftSerial(not available for other use)
#define TXPIN 3            //Pin used by SoftSerial(not available for other use)
#define WIFISPEED 57600    //SoftSerial bauds (internal communication with MCW) 



class MWiFi
{

public:

/*** data from async event ***/

    uint8_t CONNSTATUS;     // connection status from conn event
    uint8_t CONNEVDATA;     // connection data  from conn event
    
    uint16_t ERRORTYPE;     // error type from error event

    uint8_t STARTUPBITS;    // startup bits flags from startup (begin)
    
    uint8_t MCWVERSION[2];  // Version of firmware from startup
    uint8_t RADIOVERSION[2];// Version of radio module from startup

    uint16_t PINGTIME;      // feedback time for ping command
    
    uint8_t NNETS;          // number of nets scanned
    
    void (*errorHandle)(int);// default : errorHandle = NULL ; 
                             // if errorHandle = any customer function
                             // this function is called instead of
                             // standard reset.
                             // N.B. assign after begin function call
                             // because begin function assign to NULL

/********************************** Main functions ****************************/

/*
* Setup MCW1001A (first function to issue).
* Hardware reset (pin 7).
* Read startup message at 115200 from MCW and then change serial speed to 
* WIFISPEED define.
* Then switch on led 0.
*/
    void begin();

/*
* Gets values from MCW.
* MAC address, IP, Net Mask , Gateway and status of net.
* Every value can be returned by specialized function.
*/
    void getConfig();
    
/*
* Power down function. It switches off radio and clears every setting.
* Shield can be switched on, only by reset (or begin()).
*/
    void setPowerOff();
    
/*
* Power save mode (PSPOLL). If millis <0 interval sugested by access point 
* Interval as multiple of 100ms
*/
    void setPowerSave(int millis);
    
/*
* Exit from save mode
*/
    void setFullPower();
    
/*
* Network scanning.
* Returns number of visible networks .
*/
    uint8_t  scanNets();

/*
* Returns a string of num net (< number returned by scanNets).
* String contains : 
* SSID security(OPEN/WPA/WPA2/WEP) type(AccesPoint/AdHoc)  quality(RSSI value)
*/
    char* getNetScanned(uint8_t num);

/*
* Return a string of net identified by name or null if it doesn't exist.
* String contains : 
* SSID security(OPEN/WPA/WPA2/WEP) type(AccesPoint/AdHoc)  quality(RSSI value)
*/
    char* existSSID(char *name);

/*
* Return SSID string of best (in terms of RSSI) of totnets scanned.
* But only if OPEN and Access Point type. 
*/
    char* getSSIDBestOpen(uint8_t totnets);

/*
* Sets access mode : 
*         1=infrastructure (access point) (default)
*         2=ad hoc
*/
    void setNetMode(uint8_t mode);


/*
* Prepare and connect open access point ssid
* Returns 1 if connected or 0 otherwise.
*/
    uint8_t ConnectOpen(char *ssid);

/*
* Prepare and connect with WPA password
* Returns 1 if connected or 0 otherwise.
*/
    uint8_t ConnectWPAwithPsw(char *ssid, char *psw);
    
/*
* Prepare and connect with WPA password and returns calculated key for next 
* faster connection.
* Buffer key of 32 bytes can be used to connect ssid next time.
* Returns 1 if connected or 0 otherwise and key not availlable.
*/    
    uint8_t ConnectWPAandGetKey(char *ssid,char *psw, uint8_t key[32]);
    
/*
* Connect ssid using numeric key (previously calculated on password base)
* Mutch faster than password connection.
* Returns 1 if connected or 0 otherwise.
*/    
    uint8_t ConnectWPAwithKey(char *ssid,uint8_t key[32]);   

/*
* Version EEPROM based of connection functions with key.
* This kind of functions let you use connection key without 32 RAM bytes binding 
* Please note that 33 bytes are used on EEPROM : 1 flag + 32 key
* EEadd can be from 0 to max EEPROM-33
* EEPROM bytes are written only if differ from previous value.
* Key from connection function is written only if connection is ok.
*/
    uint8_t ConnectWPAandGetKeyEE(char *ssid,char *psw, int EEadd);
    uint8_t ConnectWPAwithKeyEE(char *ssid,int EEadd);
    void storeKeyOnEEPROM(uint8_t key[32],int EEadd);    
    bool isKeyInEEPROM(int EEadd);

/**** two times connection (obsolete). Use previous direct functions **/
/*
* Prepare a connection to a open network (not secure net) with this SSID.
* Params: SSID and its length.
* Or SSID as null terminated string.
*/
    void ConnSetOpen(char *ssid, uint8_t lssid);
    void ConnSetOpen(char *ssid);
    
/*
* Prepare a connection to a sicure network (WPA or WPA2). 
* Params: SSID and its length; password and its length.
* Or SSID and password as null terminated strings.
*/
    void ConnSetWPA(char *ssid, uint8_t lssid, char *psw, uint8_t lpsw);
    void ConnSetWPA(char *ssid, char *psw);

/*
* Real connect step.
* It can take until 30 sec if secure connection (ascii password transforming)
* Return: 1 = Connection done ; 0 = no connection
* If connected switch on led 1
*/
    uint8_t Connect();
/**********************************************************************/

/*
* Disconnect. Switch off led 1
*/
    uint8_t Disconnect();

/*
* Ping to a remote IP.
* String ip (null terminated) format: n.n.n.n (where n : decimal number 0-255) 
* Return 1 if echo received; 0 if timeout (4 sec by MCW)
*/
    uint8_t ping(char *ipremote);

/*
* Open a TCP Socket to a remote IP and Port port
* IP as a string n.n.n.n (where n= decimal number 0-255)
* Returns socket if connection ok; 0xFF if it can't open socket.
*/
    uint8_t openSockTCP(char *ipremote,uint16_t port);

/*
* Open a TCP Server listening on Port port
* Returns socket. (0xFF if can't)
*/
    uint8_t openServerTCP(uint16_t port);

/*
* Used when in server mode. 
* Polling for Accept command (i.e. accept is non blocking).
* If remote ip ask for connection it returns client socket ; else  FF
* Client socket must be used to read and write.
* Remote ip asking for connection can be retrieved by function getRemoteIP()
*/
    uint8_t pollingAccept(uint8_t sock);

/*
* Write data on socket sk.
* Buffer contains data and lbuff is its length.
* Or buffer contains string (null terminated).
* Or buffer string will be padded with line feed
* Plus version to send data read from ProgMem
* Return: bytes number really sent. 
*/
    uint16_t writeData(uint8_t sk,uint8_t *buffer,uint16_t lbuff);
    uint16_t writeData(uint8_t sk, char *buffer);
    uint16_t writeDataLn(uint8_t sk, char *buffer);
    uint16_t writeDataPM(uint8_t sk,prog_char *buffer,uint16_t lbuff);
    uint16_t writeDataPM(uint8_t sk, prog_char *buffer);
    uint16_t writeDataLnPM(uint8_t sk, prog_char *buffer);

/*
* Read data from socket sk. (It is not blocking)
* Buffer is the container and lbuff its length.
* If no bytes are available it returns 0.
* Else returns number of bytes actually read.
* Or gets data until line feed and returns string substituting ln with '\0' 
* (end string)  
*/
    uint16_t readData(uint8_t sk,uint8_t *buffer,uint16_t lbuff);
    char*    readDataLn(uint8_t sk);
/* 
* Version with timeout (milliseconds)
*/
    char* readDataLn(int sk, int millis);
    
/*
* Close socket
*/
    void closeSock(uint8_t sk);


/*********************************  Setting Functions *************************/

/*
* Gets MAC address of shield. Call after getConfig() function
* Needs a buffer 18 bytes long. String format (hex) xx.xx.xx.xx.xx.xx
*/
    void getMAC(char mac[18]);

/*
* Gets name issued by shield to the network when connected. 
* Made using first 3 field of MAC address.
* Needs a buffer 8 bytes long.
*/
    void getName(char name[8]);

/*
* Sets IP and set config as not dynamic IP (use only if you want not dynamic IP)
* (Default IP is dynamic)
* String ip (null terminated) format: n.n.n.n (where n : decimal number 0-255) 
*/
    void setIP(char *ip);

/*
* Gets string IP address(use after connection to obtain dynamic IP)
* Needs a buffer 16 bytes long.
* String IP format: n.n.n.n (where n : decimal number 0-255) 
*/ 
    void getIP(char ip[16]);   
    
/*
* It makes IP dynamic (reset if it was not dynamic)
*/
    void setIPdhcp();


/*
* Sets the Net Mask
* String mask (null terminated) format: n.n.n.n (where n : decimal number 0-255) 
*/
    void setNetMask(char *mask);
    
/*
* Gets string Net Mask address
* Needs a buffer 16 bytes long.
* String mask format: n.n.n.n (where n : decimal number 0-255) 
*/ 
    void getNetMask(char mask[16]);   

/*
* Sets the Gateway address
* String gatw (null terminated) format: n.n.n.n (where n : decimal number 0-255) 
*/
    void setGateway(char *gatw);

/*
* Gets string Gateway address
* Needs a buffer 16 bytes long.
* String gatw format: n.n.n.n (where n : decimal number 0-255) 
*/ 
    void getGateway(char gatw[16]); 
    
/*
* Gets string IP of remote address(ask for connection on server
* Use when in server mode.
* Needs a buffer 16 bytes long.
* String IP format: n.n.n.n (where n : decimal number 0-255) 
*/ 
    void getRemoteIP(char remoteIP[16]);    


/*
* Reset command should stimulate an async reset response (some problem)
*/
    void resetMCW();

/*
* Specialized function to switch on/off led on WIFI shield.
* led (0 to 3); state 1(on) 0(off)
*/
    void setLed(int led,int state);


    void setARP(uint16_t sec);

protected:

    uint8_t MAC[6];
    uint8_t IP[4];
    uint8_t NETMASK[4];
    uint8_t GATEWAY[4];
    uint8_t NETSTATUS;
    uint8_t REMOTEIP[4];
    uint8_t PINGOK;
    
    uint8_t *txbuff;

    uint8_t rxcode;
    uint16_t rxlmbuff;
    uint8_t *rxmbuff; 
    int pbyte;
    unsigned long stimer; //Used by timer
 
    char* netscn;
    char* ssidscn;
    uint8_t secure;
    uint8_t access;
    uint8_t RSSI;
    
    char linebuff[LINEBUFFLEN]; //circular buffer for readLine
    
/*Reads a line (string terminated by line feed) at a time, from data received. 
* It uses a buffer long LINEBUFFLEN (default 81).
* So, max string length is LINEBUFFLEN-1 (for null terminator) (default 80) 
* If line das not contains line feed it is terminated at 80 char anyway.
* That is,it returns a null terminated string corresponding to one record, 
* or max 80 char (null terminated)
* When end of data it returns NULL
*/
    char* readLine(int sk);
    

#if WIFIDEBUG
    void printHEX(unsigned char *buff,int len);
    void printDEC(unsigned char *buff,int len);
    void printAsync(int ev);
#endif

void setSockSize();

/*
* Create a stream with headers and trailer, containing a specialized message 
* with its code, length and command data. 
* Send command stream on WIFISerial 
*/
    void sendLongMess(uint8_t code, uint8_t  mess[], uint16_t lenmess);

/*
* Create a stream of minimum standard length (7) with code command and 
* no command data.
*/
    void sendShortMess(uint8_t code);

/*
* Specialized stream for GPIO command.
*/
    void sendGPMessage(int gp,int s);


/*
* Set millisec waiting timer (for receiveMessageWait function time out)
*/
    void setTimer(unsigned long ms);

/*
* Check if time out
*/
    int getTimeout();

/*
* Receive function from WIFISerial with timeout.
* It waits until first byte is available on WIFISerial, or time out. 
* Then it starts to receive answer stream, until trailer or fixed time out.
*/
    int receiveMessWait(unsigned long ms);

/*
* Called by receiveMessWait function when first byte is available.
* But it has 1 sec for complete message receiving.
* Then it detects code and data length. With this length it creates buffer to 
* receive data.
* The buffer is rxmbuff and its length rxlmbuff.
* The trailer is detected for message validating.
* It returns the code. But it is also available as rxcode variable.
*/
    int receiveMess();

/*
* Called by receiveMess()
* Single byte reading and decoding. Based on pointer starting with 0x55.
*/
    void readMess(uint8_t b,int *e);

/*
* It decodes possible asynchronous message.
* It returns event code or 0 (if no async.).
*/
    uint8_t getAsync();

    uint8_t decodeAsync();

/*
* As getAsync() but it waits until message arrives. Useful when command needs 
* an answer but it is provided by async. transmission. 
*/
    uint8_t getAsyncWait(unsigned long ms);


/*
* It sets SSID (hot spot name)
*/
    void setSsid(char *ssid, uint8_t lssid);

/*
* Create a TCP socket. 
* It returns the socket number (handle) and retains it in global variable socket 
*/
    uint8_t createSockTCP();
    
    uint16_t sendFromMem(uint8_t sk,uint8_t *buffer,uint16_t lbuff,uint8_t ln);    
    uint16_t sendFromProgMem(uint8_t sk,prog_char *pgbuffer,uint16_t lbuff,uint8_t ln);
    
    void cleanBuff(int sk);
    void resetBuff();

 /*
 * Routines for event management
 */
    void connectionLost();
    void errorRoutine();

};
    
#endif