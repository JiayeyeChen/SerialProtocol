#ifndef __SERIAL_PROTOCOL_LINUX
#define __SERIAL_PROTOCOL_LINUX

#include <boost/asio.hpp>
#include "crc16_modbus.hpp"
#include <iostream>
#include <string>
#include <fstream>
#include "main.hpp"
#include <vector>

enum DataLogTask
{
    DATALOG_TASK_OFF,
    DATALOG_TASK_FREE,
    DATALOG_TASK_START_ACTIVE,
    DATALOG_TASK_START_PASSIVE,
    DATALOG_TASK_RECEIVE_DATA_SLOT_LEN,
    DATALOG_TASK_RECEIVE_DATA_SLOT_MSG,
    DATALOG_TASK_DATALOG,
    DATALOG_TASK_END_ACTIVE,
    DATALOG_TASK_END_PASSIVE
};


class SerialProtocolHandle
{
    public:
            boost::asio::io_service io;
            boost::asio::serial_port serialPort;
            SerialProtocolHandle(std::string device_repo, int baudrate, std::string file_name);
            void ReceiveCargo(void);
            /*Rx message*/
            uint8_t ifNewMessage;
            uint8_t tempRx[264];
            uint8_t msgDetectStage;
            uint8_t bytesToRead;
            uint8_t rxMessageCfrm[256];
            uint8_t rxMessageLen;
            /*Tx message*/
            void TransmitCargo(uint8_t *data, uint8_t len);
            void SendText(std::string text);
            /*Datalog control*/
            enum DataLogTask curDatalogTask;
            void DataLogManager(void);
            void DataLogReceiveManager(void);
            void DataLogTransmitManager(void);
            uint8_t dataSlotLen;
            uint8_t dataSlotLabellingCount;
            std::vector<std::string> dataslotLabel;
            std::ofstream fileStream;
            std::string curDatalogFilename;
            void StartDataLogActive(std::string filename);
            void EndDataLogActive(void);
            bool ifNewMsgIsThisString(std::string str);
            void TurnOnDatalog(void);
            void TurnOffDatalog(void);
            /*Received MCU Values*/
            uint32_t systemTime, index;
    private:

};


#endif
