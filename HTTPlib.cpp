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
/*HTTP library.
* This library has the purpose of drastically reducing work when you are in WEB 
* based programming activity. WEB communication is based on http protocol. 
* This protocol is, essentially based on a couple of messages : a request and 
* consequent response.
* This library comes with MWiFi library but can be easy adapted to other socket
* software. Just provide the new version of readData and write data functions, 
* try to use readLine function (or make another version) and define a buffer 
* linebuff present in MWiFi library.
* See HTTPlib.h for more details.
* 
* Version 2.4
* Author Daniele Denaro
*/

#include <HTTPlib.h>

//Text for standard http headers
	prog_char rnok[] PROGMEM="HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n"; 
	prog_char rerr[] PROGMEM="HTTP/1.1 500 ServerErr\r\nContent-Length: 0\r\n\r\n"; 
	prog_char rokempty[] PROGMEM="HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
  
  prog_char rnoaut[] PROGMEM=
  "HTTP/1.1 401 Unauthorized\r\n"
  "WWW-Authenticate: Basic realm=\"ArduinoServer autenticate\"\r\n"
  "Content-Length: 0\r\n\r\n";
  
   
  prog_char rlong[] PROGMEM=
	"HTTP/1.1 200 OK\r\n"
	"Server: Arduino-MWIFI/2.4\r\n"  
	"Content-Type: text/html\r\n"
	"Cache-Control: no-cache\r\n"
	"Connection: keep-alive\r\n"
//	"Connection: close\r\n"
 	"Transfer-Encoding: chunked\r\n\r\n"; 
 	
 	prog_char rshort[] PROGMEM=
	"HTTP/1.1 200 OK\r\n"
	"Server: Arduino-MWIFI/2.4\r\n"  
	"Content-Type: text/html\r\n"
	"Cache-Control: no-cache\r\n"
	"Connection: keep-alive\r\n";

   
  prog_char headerAuth[] PROGMEM="Authorization: Basic "; 
  prog_char headerLen[] PROGMEM="Content-Length: ";
  
  prog_char http[] PROGMEM=" HTTP/1.1\r\n"; 
   	
  
  char NOPAGE[]=RESPERR;      //text for error response
  
// 	char respmess[24]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
 	char respmess[4]={0,0,0,0};
 	
 	#define HTTPDEBUG 0

/*******************************************************************************/

/************** Server */

/* 
*  Manages request actviating correct call back function using rs[]
*  Or send a 404 Not Found message
*  Returns the resource name request.
*  N.B. Query string is provided to call back function as argument by this fun. 
*  Request can be GET or POST
*  First version without basic authentication.
*  Second version with basic autentication flag (set true for autentication))
*/
char* HTTP::getRequest(int socket,int nres,WEBRES rs[])
{getRequest(socket,nres,rs,NULL);}

char* HTTP::getRequest(int socket,int nres,WEBRES rs[],char *key)
{
	char* line=readLine(socket); 
	if (line==NULL) return NULL;
#if HTTPDEBUG
  Serial.println(line);
#endif  	
	if (memcmp(line,"GET",3)==0) {reqGET(socket,line,nres,rs,key);return Resource.name;}
	if (memcmp(line,"POST",4)==0) {reqPOST(socket,line,nres,rs,key);return Resource.name;}
	cleanBuff(socket);
	return NULL;
}

/*
*  Sends page stored as prog_char array in PROGMEM  
*/	
void HTTP::sendResponse(int sk,prog_char *page)
{
  if (page==NULL) {respERR(sk);return;}
  int len=strlen_P(page);
  int pos=0,lp=0;
  startLongResponse(sk);
  while(pos<len)
  {lp=len-pos;if (lp>MAXC) lp=MAXC;sendChunkResponse(sk,page+pos,lp,NULL);pos=pos+lp;}
	endLongResponse(sk);
}

/*
* Version for pages made by multiple modules (to optimize prog space)
* amodule: array of PROGMEM module; nm: its dimension
*/
void HTTP::sendResponse(int sk,prog_char* amodule[],uint8_t nm)
{
  if (amodule==NULL) {respERR(sk);return;}
  startLongResponse(sk);
  int k;
  for (k=0;k<nm;k++)
  {
   prog_char* page=amodule[k];
   if (page==NULL){endLongResponse(sk);return;} 
   int len=strlen_P(page);
   int pos=0,lp=0;
   while(pos<len)
   {lp=len-pos;if (lp>MAXC) lp=MAXC;sendChunkResponse(sk,page+pos,lp,NULL);pos=pos+lp;}
  }
	endLongResponse(sk);
}

/*
*  Sends dynamically assembled page substituting tag @ with strings found in
*  array param[] (array of string: array of char array i.e. array of pointer to
*  char array)
*  npar is the dimension of this array 
*/	
void HTTP::sendDynResponse(int sk,prog_char page[],int npar,char *param[])
{
	int len=strlen_P(page);
  int i,lp=0,np=0,pos=0;
	char c;
	if ((param==NULL)|(npar<=0)) {sendResponse(sk,page);return;}
	startLongResponse(sk);
	while(pos<len)
	{
		 lp=0;
		 for(i=pos;i<len;i++)
     {
			 lp++;c=pgm_read_byte(page+i);
       if (c=='@')
			 {
				 if ((np<npar)&(param[np]!=NULL))
           {sendChunkResponse(sk,page+pos,lp,param[np]);np++;break;}
				 else np++;
			 }
   		 if ((lp>=MAXC)&(c!='@')) {sendChunkResponse(sk,page+pos,lp,NULL);break;}
			 if (i==len-1)sendChunkResponse(sk,page+pos,lp,NULL); 
		 }
		 pos=pos+lp;
  }
	endLongResponse(sk);
}

/*
* Version for pages made by multiple modules (to optimize prog space)
* amodule: array of PROGMEM module; nm: its dimension
*/
void HTTP::sendDynResponse(int sk,prog_char* amodule[],uint8_t nm,int npar,char *param[])
{
  if (amodule==NULL) {respERR(sk);return;}
  if ((param==NULL)|(npar<=0)) {sendResponse(sk,amodule,nm);return;}
  startLongResponse(sk);
  int k;
  for (k=0;k<nm;k++)
  {
    prog_char* page=amodule[k];
    if (page==NULL){endLongResponse(sk);return;} 
    int len=strlen_P(page);
    int i,lp=0,np=0,pos=0;
	  char c;
	  if ((param==NULL)|(npar<=0)) {sendResponse(sk,page);return;}
  	while(pos<len)
	  {
		 lp=0;
		 for(i=pos;i<len;i++)
     {
			 lp++;c=pgm_read_byte(page+i);
       if (c=='@')
			 {
				 if ((np<npar)&(param[np]!=NULL))
           {sendChunkResponse(sk,page+pos,lp,param[np]);np++;break;}
				 else np++;
			 }
   		 if ((lp>=MAXC)&(c!='@')) {sendChunkResponse(sk,page+pos,lp,NULL);break;}
			 if (i==len-1)sendChunkResponse(sk,page+pos,lp,NULL); 
		 }
		 pos=pos+lp;
    }
  }
  endLongResponse(sk);
}

/*
* Sends short data as response. Typically used for forms or ajax answering.
*/
void HTTP::sendShortResponse(int sk,char *data)
{
  if (data==NULL) {respERR(sk);return;}
  int len=strlen(data);
  char clen[10];snprintf(clen,10,"%d\r\n\r\n",len);
  writeDataPM(sk,rshort);
  writeDataPM(sk,headerLen);
  writeData(sk,clen);
  writeData(sk,data);
}

/*
*  Sets the authentication key for controlled access.
*  The key is the link of usename+':'+password, and is encoded using base64 format.
*  So, the key about 1.3 longer than the original. Don't use username and password 
*  too long because the Arduino ram lack.
*  It's possible to define up to NAUTHKEY keys.(see HTTPlin.h)
*/
char* HTTP::codeWebKey(char user[],char psw[])
{
   int len=strlen(user)+strlen(psw);
   char bapp[len+1];
   sprintf(bapp,"%s:%s",user,psw);
   return B64.base64_encode(bapp);
}


/************ Client */

/*
*  Sends a request of a resource(URI) using GET method
*  Resource can be : resourcename+querystring Ex: "/index?A1=122&A2=34"
*  Resource can by prepared using addParameter function (see later)
*/
void HTTP::sendRequestGET(int sk,char* resource)
{
  char host[16];getRemoteIP(host);
  char* headers[1];headers[0]="Host: ";strcat(headers[0],host);
  sendRequestGET(sk,headers,1,resource);
}

/*
*  As previous function. But with possibility of setting headers of http packet.
*  For example to set Host propriety.Useful for sites with one address and 
*  multiple hosts. Heach header is a null terminated string like: "xxxx: xxxxxxx"
*  nh is the headers[] dimension (sizeoff)
*/
void HTTP::sendRequestGET(int sk,char* headers[],int nh,char* resource)
{
  int lres=strlen(resource);
	int len=16+lres;
	writeData(sk,"GET ");
	writeData(sk,resource);
	writeDataPM(sk,http);
	int i=0;
	for (i=0;i<nh;i++)
  {writeData(sk,headers[i]);writeData(sk,"\r\n");}
  writeData(sk,"\r\n\r\n");	
}

/*
*  Sends a request of a resource(URI) using POST method
*  Resource doesn't contain query string (just the URI) 
*  data can contain any string. In this case values can by formatted as any 
*  kind of sequence (XML,JSON, sequence of values etc.). Obviously you can
*  also use query string format. Web application can retrieve data using input
*  stream (not getParameters). 
*/
void HTTP::sendRequestPOST(int sk,char* resource,char* data)
{
  char host[16];getRemoteIP(host);
  char* headers[1];headers[0]="Host: ";strcat(headers[0],host);
  sendRequestPOST(sk,headers,1,resource,data);
}

/*
*  As previous function. But with possibility of setting headers of http packet.
*  For example to set Host propriety.Useful for sites with one address and 
*  multiple hosts. Heach header is a null terminated string like: "xxxx: xxxxxxx"
*  nh is the headers[] dimension (sizeoff)
*/
void HTTP::sendRequestPOST(int sk,char* headers[],int nh,char* resource,char* data)
{
	int lres=strlen(resource);
	int len=16+lres;
	writeData(sk,"POST ");
	writeData(sk,resource);
	writeDataPM(sk,http);
	writeDataPM(sk,headerLen);
	char dlen[10];sprintf(dlen,"%d\r\n",strlen(data));
	writeData(sk,dlen);
	int i=0;
	for (i=0;i<nh;i++)
  {writeData(sk,headers[i]);writeData(sk,"\r\n");}
	writeData(sk,"\r\n");
	writeData(sk,data);
	writeData(sk,"\r\n\r\n");
}

/*
*  Sends a request of a resource(URI) using PUT method:
*  Like POST method. 
*/
void HTTP::sendRequestPUT(int sk,char* resource,char* data)
{
  char host[16];getRemoteIP(host);
  char* headers[1];headers[0]="Host: ";strcat(headers[0],host);
  sendRequestPUT(sk,headers,1,resource,data);
}

/*
*  With headers setting possibility.
*/
void HTTP::sendRequestPUT(int sk,char* headers[],int nh,char* resource,char* data)
{
	int lres=strlen(resource);
	int len=16+lres;
	writeData(sk,"PUT ");
	writeData(sk,resource);
	writeDataPM(sk,http);
	writeDataPM(sk,headerLen);
	char dlen[10];sprintf(dlen,"%d\r\n",strlen(data));
	writeData(sk,dlen);
	int i=0;
	for (i=0;i<nh;i++)
  {writeData(sk,headers[i]);writeData(sk,"\r\n");}
	writeData(sk,"\r\n");
	writeData(sk,data);
	writeData(sk,"\r\n\r\n");
//	writeData(sk,"\r\n");
}

/*
*  Sends a request of a resource(URI) using DELETE method
*/
void HTTP::sendRequestDELETE(int sk,char* resource)
{
  char host[16];getRemoteIP(host);
  char* headers[1];headers[0]="Host: ";strcat(headers[0],host);
  sendRequestGET(sk,headers,1,resource);
}

/*
*  As previous function. But with possibility of setting headers of http packet.
*  For example to set Host propriety.Useful for sites with one address and 
*  multiple hosts. Heach header is a null terminated string like: "xxxx: xxxxxxx"
*  nh is the headers[] dimension (sizeoff)
*/
void HTTP::sendRequestDELETE(int sk,char* headers[],int nh,char* resource)
{
  int lres=strlen(resource);
	int len=16+lres;
	writeData(sk,"DELETE ");
	writeData(sk,resource);
	writeDataPM(sk,http);
	int i=0;
	for (i=0;i<nh;i++)
  {writeData(sk,headers[i]);writeData(sk,"\r\n");}
  writeData(sk,"\r\n\r\n");	
}

/******** Response

/*
*  Returns pointer to predefined buffer linebuff (see MWiFi) containing 
*  data sent. If data sents exceed LINEBUFFLEN data are cutted. (linebuff is 
*  always a null terminated string)
*  Returns NULL if response not arrived or NOPAGE if response error
*/
char* HTTP::getResponse(int sk){return getResponse(sk,0);}

/*
*   Receive response waiting timeout millisec
*/
char* HTTP::getResponse(int sk,int timeout)
{
	int len;
	char* line;
  int i;
  for(i=-1;i<timeout;i=i+10){line=readLine(sk);if(line!=NULL) break;delay(10);}
	if (line==NULL) return NULL;
	if (memcmp(line,"HTTP/1.1",8)!=0) 
        {strcpy(respmess,"ERR");cleanBuff(sk);return "Err!";}
	strncpy(respmess,(line+9),3);
	if (memcmp(line,"HTTP/1.1 200",12)!=0)  {cleanBuff(sk);return NOPAGE;} 
	while (line!=NULL)
	{
		line=readLine(sk);
		if (line!=NULL) 
		{
		  char *param=checkHeader(line,headerLen); 
      if (param!=NULL) sscanf(param,"%i",&len);  
			if (strlen(line)<2) {break;}
		}
	}
	readLine(sk);
	char* p=strchr(linebuff,'\0');*p='\n';
	if (len>LINEBUFFLEN) len=LINEBUFFLEN;
  linebuff[len]='\0';
  resetBuff();
  uint8_t tmp[8];while (readData(sk,tmp,8)>0);
	return linebuff;
}

/*
*   This function lets you receive long response.
*   Returns -1 if response not arrived or -2 if response error.
*   Else returns number of bytes loaded into the rbuff.
*   If this number is equal to rbufflen it means there are more bytes to read
*   and you have to use getNextResponseBuffer() function until it returns 0.
*   Be carefull! You must read all bytes to clean MCW buffer.
*/
unsigned int HTTP::getResponse(int sk,uint8_t rbuff[],int rbufflen)
{return getResponse(sk,rbuff,rbufflen,0);}

/*
* With timeout (millisec)
*/
unsigned int HTTP::getResponse(int sk,uint8_t rbuff[],int rbufflen,int timeout)
{
	int len;
	char* line;
  int i;
  for(i=-1;i<timeout;i=i+10){line=readLine(sk);if(line!=NULL) break;delay(10);}
	if (line==NULL) return -1;
	if (memcmp(line,"HTTP/1.1",8)!=0) 
        {strcpy(respmess,"ERR");cleanBuff(sk);return -2;}
	strncpy(respmess,(line+9),3);	
	if (line==NULL) return -1;
	if (memcmp(line,"HTTP/1.1 200",12)!=0) {cleanBuff(sk);return -2;} 
	while (line!=NULL)
	{
		line=readLine(sk);
		if (line!=NULL) 
		{
		  char *param=checkHeader(line,headerLen); 
      if (param!=NULL) sscanf(param,"%i",&len);  
			if (strlen(line)<2) {break;}
		}
	}
  resetBuff();
  if (len==0) return 0;
  uint16_t nb=readData(sk,rbuff,rbufflen);
	return nb;
}

/*
*   Function for further bytes reading (and cleaning MCW receive buffer).
*   Returns number of bytes loaded into the rbuff; or 0 if no more bytes
*   available:
*/
unsigned int HTTP::getNextResponseBuffer(int sk,uint8_t rbuff[],int rbufflen)
{
   uint16_t nb=readData(sk,rbuff,rbufflen); 
   return nb;
}

/*
*   Get response message code. It can be called after  getResponse function.
*/
char* HTTP::getResponseMessage()
{
    return respmess;
}

/********************************************************************************/

void HTTP::reqGET(int socket,char *buff,int nres,WEBRES rs[],char *key)
{
	char* query=NULL;
  char* endfield=strchr(&buff[4],'?'); {if (endfield!=NULL) query=endfield+1;}
	if (endfield==NULL) {endfield=strchr(&buff[4],' ');}
	if (endfield==NULL) {respNOK(socket);return;}
	*endfield='\0';
  strlcpy(Resource.name,&buff[4],URILEN);
	int lquery=0;
	if (query!=NULL) 
	{char* endq=strchr(query,' ');if (endq!=NULL) {*endq='\0';} else *query='\0';}
	if (query!=NULL) {strlcpy(Resource.query,query,QUERYLEN);}
	bool fauth=true;
	if (key!=NULL)
	{
	  char *param;
	  fauth=false;
	  while (buff!=NULL)
	  {
		  buff=readLine(socket);
 #if HTTPDEBUG
  if (buff!=NULL) Serial.println(buff);
 #endif		  
		  if (strlen(buff)==0) {query=++buff;buff=NULL;}
		  if (buff!=NULL) 
		  {param=checkHeader(buff,headerAuth);
       if (param!=NULL) fauth=checkUserPsw(param,key);}
	  }	  
  }
	cleanBuff(socket);
 #if HTTPDEBUG
  Serial.println(Resource.name);
  Serial.println(Resource.query);
 #endif	
  if (fauth) activateRes(socket,nres,rs);
  else respNoAuth(socket);
}

void HTTP::reqPOST(int socket,char *buff,int nres,WEBRES rs[],char *key)
{
  char* query=NULL;
  char* endfield=strchr(&buff[5],' ');
	if (endfield==NULL) {respNOK(socket);return;}
	*endfield='\0';
	strlcpy(Resource.name,&buff[5],URILEN);	
	char* res=strdup(&buff[5]);
	int len=0;
	bool fauth;if (key!=NULL) fauth=false; else fauth=true;
	char *param;
	while (buff!=NULL)
	{
		buff=readLine(socket);
 #if HTTPDEBUG		
   Serial.println(buff);
 #endif   
		if (strcspn(buff,"\r\n")==0) {query=buff+2;buff=NULL;}
		if (buff!=NULL) 
		{
		  if (key!=NULL) 
       {param=checkHeader(buff,headerAuth);
        if (param!=NULL) fauth=checkUserPsw(param,key);}
      param=checkHeader(buff,headerLen); 
      if (param!=NULL) sscanf(param,"%i",&len);  
		}
	}
	if (len>QUERYLEN-1) len=QUERYLEN-1;
 	if (query!=NULL)strlcpy(Resource.query,query,len+1);
 #if HTTPDEBUG
  Serial.println(Resource.name);
  Serial.println(Resource.query);
 #endif
  if (fauth) activateRes(socket,nres,rs);
  else respNoAuth(socket);
}

char* HTTP::checkHeader(char *buff, prog_char *header)
{
   int i;
   for (i=0;i<strlen_P(header);i++) 
      {if (buff[i]!=pgm_read_byte(&header[i])) return NULL;}
   return &buff[i];
}

bool HTTP::checkUserPsw(char *param,char *userpsw)
{
   if (strncmp(param,userpsw,strlen(userpsw))==0) return true;
   else return false;
}


void HTTP::respNOK(int sk)
{
    writeDataPM(sk,rnok);
}

void HTTP::respERR(int sk)
{
	  writeDataPM(sk,rerr);
}

void HTTP::respNoAuth(int sk)
{
    writeDataPM(sk,rnoaut);
}

void HTTP::respOKempty(int sk)
{
    writeDataPM(sk,rokempty);
}


void HTTP::startLongResponse(int sk)
{
  writeDataPM(sk,rlong);
}

void HTTP::sendChunkResponse(int sk,prog_char data[],int ldata,char* spar)
{
	int lpar;
	if (spar==NULL) lpar=0;
	else {lpar=strlen(spar);ldata--;}
  int tdata=ldata+lpar;
  char slen[5];sprintf(slen,"%02x\r\n",tdata);
  writeData(sk,slen);
  writeDataPM(sk,data,ldata);
	if (lpar>0) {writeData(sk,(uint8_t*)spar,lpar);}
  writeData(sk,"\r\n");
}

void HTTP::endLongResponse(int sk)
{
	writeData(sk,"0\r\n\r\n");
}


void HTTP::activateRes(int sk,int nres,WEBRES rs[])
{
	int i;
  if (strcmp(Resource.name,"/")==0) strcpy(Resource.name,"/index");
	for (i=0;i<nres;i++)
	{
		if (strcmp(Resource.name,rs[i].name)==0) 
           {Resource.qlen=strlen(Resource.query);rs[i].fun(Resource.query);return;}
	}
	respNOK(sk);
}

/*********************************************************************/

/* 
* Returns value (string),giving name and query_string received (with its length); 
* or NULL if name not found.
* Function actually tokenize query string, so doesn't allocate space
* Query_string can start with ? or not.
*/
char* HTTP::getParameter(char *query,char *name)  
{
   int i;
   char *ini,*end, *val=NULL;
   char *start=strchr(query,'?');
	 if (start==NULL) start=query;else start++;
	 end=start;
	 
	 int qlen=Resource.qlen;
	 if (qlen>0) 
   {for (i=0;i<qlen;i++){if(query[i]=='&') query[i]=0;} Resource.qlen=-qlen;}
	 else qlen=-qlen;
	 
	 for (i=0;i<=qlen;i++)
	 {
		 if (*start==0) 
       {ini=end;end=start;val=getVal(name,ini,end);if (val!=NULL) return val;}
		 start++;
	 }
   return NULL;
}

/*
* Returns encoded resource+query string 
* if value==NULL name is considered as resource name and initialize query string 
* Any following function call add (coded) parameter to query string
* Be careful! querylen is the sizeof buffer query (not strlen)
*/
void HTTP::addParameter(char *query,int qlen,char *name,char *value)
{
	if (value==NULL) {query[0]='\0';strlcpy(query,name,qlen);}
	else 
	{
		char *newpar=setParameter(name,value);
		if (strchr(query,'?')==NULL) strcat(query,"?");
		else strcat(query,"&");
		strlcat(query,newpar,qlen);
		free(newpar);
	}
}


char* HTTP::getVal(char* name,char* ini,char* end)
{
	 char* c;
	 int len,lenv;
	 if (*ini==0) ini++;
	 if (*end==0) end--;
	 c=ini;
	 while(c<=end)
	 {
		 if(*c=='=') 
		 {
			 len=c-ini;
			 if (strlen(name)!=len) return NULL;
			 if(memcmp(name,ini,len)!=0) return NULL;
			 lenv=end-c;
			 return unencode(++c,lenv);
		 }
         c++;
	 }
	 return NULL;
}

char* HTTP::setParameter(char* name,char* value)
{
	int len=strlen(value)*3+1;
	char* codevalue = (char*)malloc(len*sizeof(char));
    encode(value,codevalue,strlen(value));
	len=strlen(name)+strlen(codevalue)+2;
	char* parameter=(char*)malloc(len*sizeof(char));
	parameter[0]=0;
	strcat(parameter,name);
	strcat(parameter,"=");
	strcat(parameter,codevalue);
	free(codevalue);
	return parameter;
}

char* HTTP::unencode(char *src, int len)  
{
     int i=0,code,ldecoded=0;
     char *sval; sval=src;
     while(i<len) {if (src[i]=='%'){i+=2;};i++;ldecoded++;}
	   char* decoded=(char*)malloc(ldecoded+1);
     for (i=0; i<ldecoded; i++, src++)  
	 {
          if (*src=='+')  decoded[i]=' ';
          else 
		  if (*src=='%')  
		  {
               if (sscanf(src+1, "%2x", &code) != 1) code='?';
               decoded[i] = (char) code;
               src += 2;
          }else  decoded[i]=*src;
	 }
 
     decoded[ldecoded]='\0';
	 strncpy(sval,decoded,len);
	 free(decoded);
	 return sval;     
}

char* HTTP::encode(char *src, char *dest, int len)  
{
     int i,code;
     
     for (i=0; i<len; i++, src++, dest++)  
	 {
		 if (*src==' ')  {*dest='+';continue;}
		 if (*src<0x30)  {sprintf(dest,"%%%2x",*src);dest+=2;continue;}
		 if ((*src>0x39)&(*src<0x41))  {sprintf(dest,"%%%2x",*src);dest+=2;continue;}
		 if ((*src>0x5A)&(*src<0x61))  {sprintf(dest,"%%%2x",*src);dest+=2;continue;}
		 if (*src>0x7A)  {sprintf(dest,"%%%2x",*src);dest+=2;continue;}
         *dest=*src;
     }
	 *dest='\0';
     return dest;
}

