/* ===========================================================================
  Clarasoft Foundation Server Repository
  cfsrepo.c

  CRTSQLCI OBJ(CFSREPO) SRCFILE(QCSRCX)
              OUTPUT(*PRINT) DBGVIEW(*SOURCE)

  Distributed under the MIT license

  Copyright (c) 2020 Clarasoft I.T. Solutions Inc.

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files
  (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify,
  merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
  ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH
  THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

---------------------------------------------------------------------------
  Change history
---------------------------------------------------------------------------

  2020-07-12
  Frederic Soucie
  creation

=========================================================================== */

EXEC SQL SET OPTION CLOSQLCSR=*ENDMOD;

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include "qcsrc/cslib.h"

EXEC SQL INCLUDE SQLCA;

#define CFSRPS_SECMODE_NONE          "*NONE"
#define CFSRPS_SECMODE_TLS           "*TLS"
#define CFSRPS_SECMODE_SSL           "*TLS"

#define TLSCFG_LVL_ENVIRON            1
#define TLSCFG_LVL_SESSION            2

#define TLSCFG_PARAMTYPE_STRING       1
#define TLSCFG_PARAMTYPE_NUMERIC      2
#define TLSCFG_PARAMTYPE_ENUM         3

#define TLSCFG_PARAMINFO_FMT_100      100

typedef struct tagCFSRPS {

  CSMAP* config;
  char szDomain[33];
  char szSubDomain[33];
  char szName[33];
  char szPath[33];

} CFSRPS;

typedef struct tagCFSRPS_CONFIGINFO {

  char szDomain[33];
  char szSubDomain[33];
  char szName[33];
  char szPath[100];
  char szDesc[129];

} CFSRPS_CONFIGINFO;

typedef struct tagTLSCFG_CONFIGINFO {

  char szName[65];
  char szDesc[129];

} TLSCFG_CONFIGINFO;

typedef struct tagCFSRPS_PARAMINFO {

  int  fmt;
  char szParam[33];
  int  type;
  char szValue[256];

} CFSRPS_PARAMINFO;

typedef struct tagTLSCFG_PARAMINFO {

  int  fmt;
  char szName[65];
  int  param;
  int  type;
  int  availLevel;
  int  level;
  char szValue[256];

} TLSCFG_PARAMINFO;

CFSRPS*
  CFSRPS_Constructor
    (void) {

  CFSRPS* Instance;

  Instance = (CFSRPS*)malloc(sizeof(CFSRPS));

  Instance->config = CSMAP_Constructor();

  return Instance;
}

void
  CFSRPS_Destructor
    (CFSRPS** This) {

  CSMAP_Destructor(&((*This)->config));
  free(*This);
}

CSRESULT
  CFSRPS_LoadConfig
    (CFSRPS* This,
     CFSRPS_CONFIGINFO* configInfo) {

EXEC SQL BEGIN DECLARE SECTION;

  char CFSRPS_LoadConfig_szCfgDomain[33];
  char CFSRPS_LoadConfig_szCfgSubDomain[33];
  char CFSRPS_LoadConfig_szCfgName[33];
  char CFSRPS_LoadConfig_szCfgPath[100];
  char CFSRPS_LoadConfig_szDesc[129];
  char CFSRPS_LoadConfig_szParam[33];
  char CFSRPS_LoadConfig_szValue[256];

EXEC SQL END DECLARE SECTION;

  CFSRPS_PARAMINFO pi;

  CSMAP_Clear(This->config);

  strcpy(CFSRPS_LoadConfig_szCfgPath, configInfo->szPath);

  EXEC SQL DECLARE C_3 CURSOR FOR
    SELECT
      param,
      value
    FROM RPSCFP
      Where path = :CFSRPS_LoadConfig_szCfgPath;

  if (SQLCODE == 0) {

     EXEC SQL OPEN C_3;

     if (SQLCODE == 0) {

       EXEC SQL FETCH C_3 Into
           :CFSRPS_LoadConfig_szParam,
           :CFSRPS_LoadConfig_szValue;

       while (SQLCODE == 0) {

          strcpy(pi.szParam, CFSRPS_LoadConfig_szParam);
          strcpy(pi.szValue, CFSRPS_LoadConfig_szValue);

          CSMAP_Insert(This->config,
                       CFSRPS_LoadConfig_szParam,
                       (void*)(&pi),
                       sizeof(CFSRPS_PARAMINFO));

          EXEC SQL FETCH C_3 Into
             :CFSRPS_LoadConfig_szParam,
             :CFSRPS_LoadConfig_szValue;
       }

       EXEC SQL CLOSE C_3;
     }
     else {
        //EXEC SQL ROLLBACK;
        return CS_FAILURE;
     }
  }
  else {
    //EXEC SQL ROLLBACK;
    return CS_FAILURE;
  }

  //EXEC SQL COMMIT;
  return CS_SUCCESS;
}

CSRESULT
  CFSRPS_LookupParam
    (CFSRPS* This,
     char* szParamName,
     CFSRPS_PARAMINFO* param) {

  CFSRPS_PARAMINFO* ppi;
  long size;

  size = sizeof(CFSRPS_PARAMINFO);
  if (CS_SUCCEED(CSMAP_Lookup(This->config, szParamName, (void**)&ppi, &size))) {
    strcpy(param->szParam, ppi->szParam);
    strcpy(param->szValue, ppi->szValue);
    return CS_SUCCESS;
  }

  return CS_FAILURE;
}

CSRESULT
  TLSCFG_LsParam
    (char*  szName,
     long   filter,
     int    fmt,
     CSLIST Listing) {

EXEC SQL BEGIN DECLARE SECTION;

  char TLSCFG_LsParam_szCfgName[65];
  char TLSCFG_LsParam_szStatement[1025];
  char TLSCFG_LsParam_szParamName[129];
  char TLSCFG_LsParam_szValue[129];

  long TLSCFG_LsParam_param;
  long TLSCFG_LsParam_type;
  long TLSCFG_LsParam_level;
  long TLSCFG_LsParam_availLevel;

  TLSCFG_PARAMINFO paramInfo;

EXEC SQL END DECLARE SECTION;

  TLSCFG_PARAMINFO* ppi;

  strcpy(TLSCFG_LsParam_szCfgName, szName);

  CSLIST_Clear(Listing);

  switch(filter) {

    case TLSCFG_LVL_ENVIRON:
    case TLSCFG_LVL_SESSION:

      sprintf(TLSCFG_LsParam_szStatement,
              "Select "
              "a.param, a.type, a.level, a.value "
              "From TLSCFP a "
              "Where a.name = '%s' "
              "And a.level = %ld ",
              TLSCFG_LsParam_szCfgName,
              filter);

      break;

    default:

      sprintf(TLSCFG_LsParam_szStatement,
              "Select "
              "a.param, a.type, a.level, a.value "
              "From TLSCFP a "
              "Where a.name = '%s' ",
              TLSCFG_LsParam_szCfgName);

      break;
  }

  EXEC SQL PREPARE STMT_1 FROM :TLSCFG_LsParam_szStatement;

  EXEC SQL DECLARE C_1 CURSOR FOR STMT_1;

  if (SQLCODE == 0) {

    EXEC SQL OPEN C_1;

    if (SQLCODE == 0) {

      EXEC SQL FETCH C_1 Into
          :TLSCFG_LsParam_param,
          :TLSCFG_LsParam_type,
          :TLSCFG_LsParam_level,
          :TLSCFG_LsParam_szValue;

      while (SQLCODE == 0) {

        switch(fmt) {

          default:

            paramInfo.fmt = TLSCFG_PARAMINFO_FMT_100;
            paramInfo.param = TLSCFG_LsParam_param;
            strcpy(paramInfo.szValue, TLSCFG_LsParam_szValue);
            paramInfo.type = TLSCFG_LsParam_type;
            paramInfo.level = TLSCFG_LsParam_level;

            break;
        }

        CSLIST_Insert(Listing,
                   (void*)(&paramInfo),
                   sizeof(TLSCFG_PARAMINFO),
                   CSLIST_BOTTOM);

        EXEC SQL FETCH C_1 Into
            :TLSCFG_LsParam_param,
            :TLSCFG_LsParam_type,
            :TLSCFG_LsParam_level,
            :TLSCFG_LsParam_szValue;
      }

      EXEC SQL CLOSE C_1;
    }
    else {
      //EXEC SQL ROLLBACK;
      return CS_FAILURE;
    }
  }
  else {
    //EXEC SQL ROLLBACK;
    return CS_FAILURE;
  }

  //EXEC SQL COMMIT;
  return CS_SUCCESS;
}
 
