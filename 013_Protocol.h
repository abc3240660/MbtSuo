#ifndef __MOBIT_PROTOCOL_H
#define __MOBIT_PROTOCOL_H

#include "003_BG96.h"

#define PROTOCOL_HEAD   "#MOBIT"
#define DEV_TAG         "ECAR"
#define SW_VERSION      "V201909181450"
#define HW_VERSION      "V1.0"

#define CMD_DEV_ACK     "Re"// DEV ACK

// F407 Send to Server automatically
#define CMD_DEV_REGISTER    "REG"// DEV Host
#define CMD_HEART_BEAT      "HB"// DEV Host
#define CMD_DOOR_LOCKED     "LC"// DEV Host
#define CMD_DOOR_UNLOCKED   "OL"// DEV Host
#define CMD_CALYPSO_UPLOAD  "RC"// DEV Host
#define CMD_INVALID_MOVE    "AL"// DEV Host
#define CMD_REPORT_GPS      "GEO"// DEV Host
#define CMD_IAP_SUCCESS     "UP"// DEV Host
#define CMD_CHARGE_STARTED  "CG"// DEV Host
#define CMD_CHARGE_STOPED   "CGE"// DEV Host
#define CMD_FINISH_ADDNFC   "ADDCR"// DEV Host
#define CMD_FINISH_RST      "RSOK"// DEV Host
#define CMD_RISK_REPORT     "FA"// DEV Host

// F407 Recv from Server and Action / ACK
#define CMD_QUERY_PARAMS    "QG"// DEV ACK
#define CMD_RING_ALARM      "DD"// DEV ACK
#define CMD_UNLOCK_DOOR     "OL"// DEV ACK
#define CMD_FACTORY_RST     "RS"// DEV ACK
#define CMD_ENTER_SLEEP     "SLEEP"// DEV ACK
#define CMD_QUERY_GPS       "GGEO"// DEV ACK
#define CMD_IAP_UPGRADE     "UP"// DEV ACK
#define CMD_CHANGE_APN      "UG"// DEV ACK
#define CMD_QUERY_NFC       "QCL"// DEV ACK
#define CMD_EXIT_SLEEP      "WU"// DEV ACK
#define CMD_DELETE_NFC      "RMC"// DEV ACK
#define CMD_ADD_NFC         "ADDC"// DEV ACK
#define CMD_QUERY_ALARM     "ALC"// DEV ACK
#define CMD_MODIFY_ALARM    "MALC"// DEV ACK
#define CMD_QUERY_ICCID     "ICCID"// DEV ACK

#define LEN_NET_TCP    32

#define LEN_SYS_TIME    32
#define LEN_IMEI_NO     32
#define LEN_BAT_VOL     32
#define LEN_RSSI_VAL    32
#define LEN_MAX_SEND    512
#define LEN_DW_MD5      32
#define LEN_DW_URL      128
#define LEN_FILE_NAME   128

enum CMD_TYPE {
    // DEV Auto CMDs
    DEV_REGISTER = 0,
    HEART_BEAT,
    DOOR_LOCKED,
    DOOR_UNLOCKED,
    CALYPSO_UPLOAD,
    INVALID_MOVE,
    REPORT_GPS,
    IAP_SUCCESS,
    CHARGE_STARTED,
    CHARGE_STOPED,
    FINISH_ADDNFC,
    FINISH_RST,
    EXIT_SLEEP,
    RISK_REPORT,

    // SVR Auto CMDs
    QUERY_PARAMS,
    RING_ALARM,
    UNLOCK_DOOR,
    FACTORY_RST,
    ENTER_SLEEP,
    QUERY_GPS,
    IAP_UPGRADE,
    CHANGE_APN,
    QUERY_NFC,
    DELETE_NFC,
    ADD_NFC,
    QUERY_ALARM,
    MODIFY_ALARM,
    QUERY_ICCID,
    UNKNOWN_CMD
};

void CalcRegisterKey(void);
void ParseMobitMsg(char* msg);
void DequeueTcpRequest(void);

bool TcpHeartBeat(void);
bool TcpDeviceRegister(void);
bool TcpFinishFactoryReset(void);
bool TcpExitCarriageSleep(void);
bool TcpReportGPS(void);
bool TcpInvalidMovingAlarm(void);
bool TcpRiskAlarm(void);
bool TcpFinishIAP(void);
bool TcpFinishAddNFCCard(void);
bool TcpReadedOneCard(u8* card_id, u8* serial_nr);
bool TcpLockerLocked(void);
bool TcpLockerUnlocked(void);
bool TcpChargeStarted(void);
bool TcpChargeStoped(void);
void ProcessIapRequest(void);
void ProcessTcpRequest(void);

bool IsIapRequested(void);

bool TcpReNormalAck(u8* cmd_str, u8* sta);
bool TcpReQueryParams(void);
bool TcpReQueryGPS(void);
bool TcpReQueryNFCs(void);
bool TcpReQueryAlarm(void);
bool TcpReQueryIccid(void);
bool TcpReDeleteNFCs(void);

bool DoUnLockTheLockerFast(void);
bool DoRingAlarmFast(void);
bool DoFactoryResetFast(void);
bool DoEnterSleepFast(void);
bool DoEnterSleepFast(void);
bool DoQueryGPSFast(void);
bool DoQueryNFCFast(void);
bool DoAddNFCFast(void);
bool DoHttpIAP(void);

void ReportFinishAddNFC(u8 gs_bind_cards[][LEN_BYTE_SZ64], u8* index_array);
void ReportLockerUnlocked(void);
u16 GetHeartBeatGap(void);
u8 IsDuringBind(void);

u8 IsApnChangeWait(void);
void ResetApnChange(void);
void ProtocolParamsInit(void);

u8 IsDuringShip(void);
void ClearShipStatus(void);
void PowerOnMainSupply(void);
void StartCommunication(void);
bool ManualIapRequested(void);

#endif
