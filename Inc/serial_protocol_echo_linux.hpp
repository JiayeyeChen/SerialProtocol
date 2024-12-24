#ifndef __SERIAL_PROTOCOL_ECHO_LINUX_HPP
#define __SERIAL_PROTOCOL_ECHO_LINUX_HPP

#include <boost/asio.hpp>
#include "crc16_modbus.hpp"
#include <iostream>
#include <string>
#include <fstream>
#include <vector>

enum SERIALPROTOCOLECHO_Task
{
    SERIALPROTOCOLECHO_TASK_FREE,
    SERIALPROTOCOLECHO_TASK_DATALOG
};

enum DataLogTask
{
    DATALOG_TASK_NA,
    DATALOG_TASK_START_SLAVE,
    DATALOG_TASK_START,
    DATALOG_TASK_RECEIVE_DATA_SLOT_LEN,
    DATALOG_TASK_RECEIVE_DATA_SLOT_LABEL,
    DATALOG_TASK_LOGGING_DATA,
    DATALOG_TASK_END
};

class SerialProtocolEchoSlaveHandle
{
    public:
            boost::asio::io_service io;
            boost::asio::serial_port serialPort;
            SerialProtocolEchoSlaveHandle(std::string device_repo, int baudrate, std::string file_name);
            void ReceiveCargo(void);
            /* Rx message */
            uint8_t ifNewMessage;
            uint8_t tempRx[264];
            uint8_t msgDetectStage;
            uint8_t bytesToRead;
            uint8_t rxMessageCfrm[256];
            uint8_t rxMessageLen;
            /* Tx message */
            void TransmitCargo(uint8_t *data, uint8_t len);
            void SendText(std::string text);
            /* Datalog control */
            enum DataLogTask curDatalogTask;
            std::string curDatalogFilename;
            uint8_t dataSlotLen;
            uint8_t dataSlotLabellingCount;
            std::vector<std::string> dataslotLabel;
            bool ifNewMsgIsThisString(std::string str);
            std::ofstream fileStream;
            uint32_t systemTime, index;
            void DatalogManager(void);
            /* Task scheduler */
            enum SERIALPROTOCOLECHO_Task task;
            void Host(void);
            
            
            // void DataLogManager(void);
            // void DataLogReceiveManager(void);
            // void DataLogTransmitManager(void);
            // std::vector<std::string> dataslotLabel;
            // void StartDataLogActive(std::string filename);
            // void StartDataLogPassive(std::string filename);
            // void EndDataLogActive(void);
            // void TurnOffDataLog(void);
            // bool ifNewMsgIsThisString(std::string str);
            // /*Received MCU Values*/
            // 
    private:

};



#endif
