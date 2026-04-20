#pragma once

#include <stdio.h>

#include "dpn_peripheral_interface_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DPNDDK_HMD_BUF_LEN  104
//#define DPNDDK_HMD_BUF_LEN  80
#define DPNDDK_HC_BUF_LEN   72
#define DPNDDK_DATA_BUF_CNT  16

#define DPNDDK_OUT_BUF_LEN  128
#define DPNDDK_OUT_BUF_CNT  32

#define DPNDDK_MAGIC_NUMBER  0xf0a5

    DPN_API void Dpnddk_WriteHmdBuf(void *pBuf);
    DPN_API void Dpnddk_WriteLhcBuf(void *pBuf);
    DPN_API void Dpnddk_WriteRhcBuf(void *pBuf);
    DPN_API char *Dpnddk_ReadHmdBuf();
    DPN_API char *Dpnddk_ReadLhcBuf();
    DPN_API char *Dpnddk_ReadRhcBuf();
    DPN_API void Dpnddk_InitBuffers();

    DPN_API void Dpnddk_EnableDataExchange(sockaddr_in *clientAddr);

    enum DPNDDK_PACKET_TYPE_T {
        DPNDDK_PACKET_TYPE_HMD = 1,
        DPNDDK_PACKET_TYPE_LHC,
        DPNDDK_PACKET_TYPE_RHC,
    };

    struct Dpnddk_header_t {
        unsigned short uhMagicNumber;
        unsigned short uhPacketType;
        unsigned short uhPacketLength;  // length in byte excluding header
        unsigned short uhReserved;
    };

    void Dpnddk_DataExchangeStep();

    void Dpnddk_InitServer();
    void Dpnddk_ServerStep();
    void Dpnddk_UninitServer();

    int Dpnddk_GetMode();
    BOOL Dpnddk_ReceiveData(int deviceType, void *buf, int len);

    void Dpnddk_DbgPrint(const TCHAR * fmt, ...);

    

#ifdef __cplusplus
}
#endif

class CDpvrDdkLog {
public:
    enum {
        IDX_BUF_SIZE = 16,
        MAX_DEVICE = 16,
        MAX_LOG_LENGTH = 128,
        MAGIC_NUMBER = 0xfabf,
        FILE_FLAG_STREAM_END = 1,
        FILE_FLAG_STREAM_START = 2
    };
    enum {                
        DPRDDK_PACKET_HMD_RAW = 1,
        DPRDDK_PACKET_HMD_FUSION = 2,
        DPRDDK_PACKET_HMD_POSITION = 3,
        DPRDDK_PACKET_HMD_OFFSET = 4,
        DPRDDK_PACKET_HMD_SN = 5,
        DPRDDK_PACKET_HMD_USB_STATUS = 6,
        DPRDDK_PACKET_DEVICE_RAW = 8,
        DPRDDK_PACKET_DEVICE_POSITION=9,
        DPRDDK_PACKET_BASE_INFO = 16, // baseIndex(uint8_t)+ddk_version(uint32_t)+tagDpraBaseStationInfo, using multiple sub buffer
        DPRDDK_PACKET_FILE_FLAG = 32 // timeb structure+sample time(double)+uint32_t flag. FILE_FLAG_STREAM_END set means end of file
    };

    enum {
        LOG_MASK_HMD_RAW = 0x1,
        LOG_MASK_HMD_FUSION = 0x2,
        LOG_MASK_HMD_POSITION = 0x4,
        LOG_MASK_DEVICE_RAW=0x8,
        LOG_MASK_DEVICE_POSITION=0x10
    };

    enum {
        DPRDDK_DEVICE_TYPE_HMD = 0,
        DPRDDK_DEVICE_TYPE_LHC,
        DPRDDK_DEVICE_TYPE_RHC,
        DPRDDK_DEVICE_TYPE_MR
    };
#pragma warning(disable: 4200)  
    struct tagDpvrDDKDataHeader{
        unsigned short magic_number; // 0xfabf
        unsigned char data_len; // size in byte for data member
        unsigned char packet_type;
        unsigned char packet_sub_type; // for merging multiple packet
        unsigned char ReservedByte; // must be zero
        unsigned short reserved; // must be zero for now
        unsigned char data[0];
    };
#pragma warning(default: 4200)
private:
    unsigned char m_outBuf[DPNDDK_OUT_BUF_LEN];
    char     m_strLogFileName[MAX_LOG_LENGTH];// max 
    unsigned char *m_buf[MAX_DEVICE];
    unsigned char m_wrptr[MAX_DEVICE];
    unsigned char m_rdptr[MAX_DEVICE];
    unsigned int m_logMask;
    unsigned int m_nDeviceCount;
    FILE *logFile;
public:

    CDpvrDdkLog();
    ~CDpvrDdkLog();
    void CreateBuffers(int deviceCnt);
    unsigned char* GetBuffer(int deviceIdx);
    void FillHeader(unsigned char *buf, unsigned char len, unsigned char packet_type, unsigned char packet_sub_type);
    void ForwardWritePtr(int deviceIdx);
    void DoLog();
    void SetMask(unsigned int mask);
    unsigned int GetMask();
    void SetLogFilePathName(char *filePathName);
};

extern CDpvrDdkLog g_DdkLog;


