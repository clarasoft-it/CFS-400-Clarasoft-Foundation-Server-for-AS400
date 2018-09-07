# CFS-400-Clarasoft-Foundation-Server-for-AS400
TCP/IP Server and development framework for AS400

## What is CFS-400

CFS-400 is a TCP server platform for AS400. More specifically, CFS-400 is:

*A generic TCP server implementation that enables ILE RPG developpers to create and expose their own network services

*A software development kit for ILE RPG developpers who want to create their own TCP clients and services

*An implementation of the websocket protocole as well as an SDK to develop websocket clients and services in ILE RPG

## Building the TCP server and server handler

Copy the files on your AS400 in a source file named QCSRC (the CFSREG source needs to be copied in the QDDSSRC source file). Next, you will create the following modules (compilation commands follow):

* CFSAPI
* CLARAD
* CLARAH
* CSLIST
* CSSTR
* CSWSCK

From the AS400 command line, execute the following commands to create the above modules:

```bash
CRTCMOD MODULE(CFSAPI) SRCFILE(QCSRC) DBGVIEW(*ALL)  

CRTCMOD MODULE(CLARAD) SRCFILE(QCSRC) DBGVIEW(*ALL) 

CRTSQLCI OBJ(CLARAH) SRCFILE(QCSRC)
              SRCMBR(CLARAH) DBGVIEW(*SOURCE)   

CRTCMOD MODULE(CSLIST) SRCFILE(QCSRC) DBGVIEW(*ALL)     

CRTCMOD MODULE(CSSTR) SRCFILE(QCSRC) DBGVIEW(*ALL) 

CRTCMOD MODULE(CSWSCK) SRCFILE(QCSRC) DBGVIEW(*ALL)  
```

Next, you need to create a SRVPGM object that exports the CFS-400 toolkit functions (we will call it CFSAPI) by issuing the following command (you can use the binding source provided in the qbndsrc directory of this package if you wish rather than use EXPORT(*ALL)):

```bash
CRTSRVPGM SRVPGM(CFSAPI)
        MODULE(CFSAPI CSLIST  CSSTR CSWSCK) EXPORT(*ALL) 
```
        
Next, you must create the CFSCONF and CFSREG files as follow:

```bash
CRTPF FILE(CFSCONF) SRCMBR(CFSCONF)
```
```bash
CRTPF FILE(CFSREG) SRCMBR(CFSREG)
```

You will now create the the main TCP server (daemon) and the generic service handler (respectively CLARAD and CLARAH) by issuing the following command:

```bash
CRTPGM PGM(CLARAD) MODULE(CLARAD) BNDSRVPGM(CFSAPI)

CRTPGM PGM(CLARAH) MODULE(CLARAH) BNDSRVPGM(CFSAPI)
```

You now have the basic software components needed to create your own ILE RPG network services (and clients to those services); CFS-400 provides a sample ILE RPG network service that echoes data sent from a client. The next section shows you how to build and run this service using CFS-400. Next, we will show you how to test this ILE RPG echo service by building the sample ILE RPG echo client provided by CFS-400. This will get you started building you own services and clients.

## CFS-400 example: Creating your own ILE RPG network service

To show how an ILE RPG program can be used to run as a network service, copy the QRPGLESRC/ECHOH and QRPGLESRC/CFSAPIH source from this repository (under the qrpglesrc directory) to the QRPGLESRC source file on your system. You will then compile the ECHOH source into a module by issuing the following command:

```bash
CRTRPGMOD MODULE(ECHOH) SRCFILE(QRPGLESRC) SRCMBR(ECHOH) DBGVIEW(*ALL)  
```
Next, you will create a SRVPGM object from the above module (notice this uses the CFSAPI service program built above):

```bash
CRTSRVPGM SRVPGM(ECHOH)
        MODULE(ECHOH) BNDSRVPGM(CFSAPI) EXPORT(*ALL) 
```

For the CLARAH program to use the ILE RPG echo service, a record must be inserted into the CFSREG file: 

```bash
RGSRVNM: ECHOSERVICE           
RGLIBNM: LIBNAME        
RGPRCHD: ECHOH        
RGPRCNM: RUNECHO   
```

Where LIBNAME is the name of the library where the ECHOH service program resides. All other fields have to be as shown above (the RGPRCNM field holds the name of the sub-procedure exported by the service program and is the sub-procedure that will be called by the CLARAH handler when a client connects to the service). 

For the clarad daemon to execute CLARAH and have it bind to the ECHOH service program, the following record needs to be inserted intot the CFSCONF file (the CFPORT field specifies the TCP port on which the clarad daemon will listen for connections; use whichever port is suitable for your own use):

```bash
CFINST: ECHOSERVICE           
CFPORT: 11000        
CFMINHND: 3        
CFMAXHND: 10
CFHNDPTH: /QSYS.LIB/LIBNAME/CLARAH.PGM
CFBKLOG: 1024
```

To test this ILE RPG service, you can execute the CFS-400 daemon from the command line (in production, this command would be run in its own subsystem) and instruct it to use the ECHOH service by issuing the follwng command (note that the service library does not have to match the library where the daemon resides):

```bash
call CLARAD parm('ECHOSERVICE')                                                  
```

The above command will run three handler jobs and the main daemon job:


```bash
Opt  Job         User        Type     -----Status-----  Function      
     CLARAH      DUSER       BATCHI   ACTIVE            PGM-CLARAH    
     CLARAH      DUSER       BATCHI   ACTIVE            PGM-CLARAH    
     CLARAH      DUSER       BATCHI   ACTIVE            PGM-CLARAH    
     DUSER       DUSER       INTER    ACTIVE            PGM-CLARAD    
```
     
There are 3 handlers running waiting for clients to connect (this is specified in CFSCONF.CFMINHND); if all 3 handlers are busy servicing connections, then a fourth handler will be executed to handle an additional connexion and so on up to 10 executions of CLARAH (this is specified in CFSCONF.CFMAXHND). 

## CFS-400 example: Testing the ECHOH ILE RPG network service by running the ECHOC ILE RPG client

To test the handler, you can build the example ILE RPG client provided by this package (QRPGLESRC/ECHOC) by issuing the follwing commands (this assumes you have built the CFSAPI service program as shown above):

```bash
CRTRPGMOD MODULE(ECHOC) SRCFILE(QRPGLESRC) 
                   SRCMBR(ECHOC) DBGVIEW(*ALL)      
```

```bash
CRTPGM PGM(ECHOC) BNDSRVPGM((CFSAPI))  
```

Assuming you have started the clara-daemon and executed the ECHOH handler (as shown above), you can start debugging the program by setting a breakpoint at the call to the CFS_Connect function. Then, call the ECHOC program and have it connect to your AS400 by specifying its network name and using the port number specified in CFSCONF.CFPORT as parameters:

```bash
call ECHOC parm('myAS400hostName' '11000')   
```
Stepping through the program (assuming the connection succeeded), you will see that the string "Hello World!" will be converted to ASCII by calling various CSSTRCV functions and then sent over to the ECHOH handler. Stepping beyond the CFS_Read function that follows the CFS_Write function, the data received from the ECHOH handler will be converted from ASCII to EBCDIC (in the job CCSID). You can then check the content of the szMessage variable to see what the ECHOH handler returned. 

     
 
 









