# CFS-400-Clarasoft-Foundation-Server-for-AS400
TCP/IP Server and development framework for AS400

## What is CFS-400

CFS-400 is a TCP server platform for AS400. This platform enables ILE RPG developpers to create network services that can be used for modernizing user interfaces, enabling native web clients on existing ILE RPG applications and developping your own networking protocols.

## Building the TCP server and server handler

Copy the files on your AS400 in a source file named QCSRC (the CFSREG source needs to be copied in the QDDSSRC source file). Next, you will create the following modules (compilation commands follow):

* CFSAPI
* CLARAD
* CLARAH
* CLARAHS
* CSLIST
* CSSTR
* CSWSCK

From the AS400 command line, execute the following comamnds to create the above modules:

```bash
CRTCMOD MODULE(CFSAPI) SRCFILE(QCSRC) DBGVIEW(*ALL)  

CRTCMOD MODULE(CLARAD) SRCFILE(QCSRC) DBGVIEW(*ALL) 

CRTSQLCI OBJ(CLARAH) SRCFILE(QCSRC)
              SRCMBR(CLARAH) DBGVIEW(*SOURCE)   

CRTSQLCI OBJ(CLARAHS) SRCFILE(QCSRC)
              SRCMBR(CLARAHS) DBGVIEW(*SOURCE)   

CRTCMOD MODULE(CSLIST) SRCFILE(QCSRC) DBGVIEW(*ALL)     

CRTCMOD MODULE(CSSTR) SRCFILE(QCSRC) DBGVIEW(*ALL) 

CRTCMOD MODULE(CSWSCK) SRCFILE(QCSRC) DBGVIEW(*ALL)  
```

Next, you need to create a SRVPGM object that exports the CFS-400 toolkit functions (we will call it CFSAPI) by issuing the following command (alternatively, you can use the provided binding source cfsapi.bnd):

```bash
CRTSRVPGM SRVPGM(CFSAPI)
        MODULE(CFSAPI CSLIST  CSSTR CSWSCK) EXPORT(*ALL) 
```
        
Next, you must create the CFSREG file as follow:

```bash
CRTPF FILE(CFSREG) SRCMBR(CFSREG)
```

You will now create the the main TCP server (daemon) and the demonstration echo handler (respectively CLARAD and CLARAH) by issuing the following command:

```bash
CRTPGM PGM(CLARAD) MODULE(CLARAD) BNDSRVPGM(CFSAPI)

CRTPGM PGM(CLARAH) MODULE(CLARAH) BNDSRVPGM(CFSAPI)
```

## CFS-400 example: ILE RPG network service

To show how an ILE RPG program can be used to run as a network service, copy the ECHORPG.rpgle and CFSAPIH.rpgle source from this repository (under the ile_rpg_examples directory) to the QRPGLESRC source file on your system. You will then compile the ECHORPG source into a module by issuing the following command:

```bash
CRTRPGMOD MODULE(ECHORPG) SRCFILE(QRPGLESRC) SRCMBR(ECHORPG.RPGLE) DBGVIEW(*ALL)  
```
Next, you will create a SRVPGM object from the above module (notice this uses the CFSAPI service program built above):

```bash
CRTSRVPGM SRVPGM(ECHORPG)
        MODULE(ECHORPG) BNDSRVPGM(CFSAPI) EXPORT(*ALL) 
```

For the CLARAH program to use the ILE RPG echo service, a record must be inserted into the CFSREG file: 

```bash
RGSRVNM: ECHO           
RGLIBNM: LIBNAME        
RGPRCHD: ECHORPG        
RGPRCNM: ECHOHANDLER   
```

Where LIBNAME is the name of the library where the ECHORPG service program resides. All other fields have to be as shown above. Now, to test this ILE RPG service, you can execute the CFS-400 daemon from the command line (in production, this command would be run in its own subsystem) and instruct it to use the ECHO service by issuing the follwng command (this assumes that CLARAH is also in library LIBNAME although this is not mandatory: the service library does not have to match the library where the daemon resides):

```bash
call CLARAD                                                     
 parm('41101' '3' '4' '/QSYS.LIB/LIBANME.LIB/CLARAH.PGM' 'ECHO') 
```

The above command will run three handler jobs and the main daemon job:


```bash
Opt  Job         User        Type     -----Status-----  Function      
     CLARAH      DUSER       BATCHI   ACTIVE            PGM-CLARAH    
     CLARAH      DUSER       BATCHI   ACTIVE            PGM-CLARAH    
     CLARAH      DUSER       BATCHI   ACTIVE            PGM-CLARAH    
     DUSER       DUSER       INTER    ACTIVE            PGM-CLARAD    
```
     
There are 3 handlers running, waiting for client connections; if all 3 handlers are busy servicing connections, then a fourth handler will be executed to handle an additional connexion (that is the meaning of the '3' '4' parameters to the command). To test the handler, you can build one of the clients given as examples (C#, C or RPG)

     
 
 









