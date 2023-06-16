#include "serial_protocol_linux.hpp"

using namespace std::chrono_literals;

SerialProtocolHandle::SerialProtocolHandle(std::string device_repo, int baudrate, std::string file_name) : serialPort(io, device_repo), \
                                                            msgDetectStage(0), ifNewMessage(0), curDatalogTask(DATALOG_TASK_FREE), \
                                                            dataSlotLen(0)
{
    SerialProtocolHandle::curDatalogFilename = file_name;
    if (!SerialProtocolHandle::serialPort.is_open())
    {
        try
        {
            SerialProtocolHandle::serialPort.open(device_repo);
        }
        catch (...) // Problem: Cannot catch exception when USB is disconnected
        {
            std::cout << "Fail to connect to Exoskeleton serial port." << std::endl;
        }
    }
    if (SerialProtocolHandle::serialPort.is_open())
    {
        SerialProtocolHandle::serialPort.set_option(boost::asio::serial_port_base::baud_rate(baudrate));
        SerialProtocolHandle::serialPort.set_option(boost::asio::serial_port::flow_control());
        SerialProtocolHandle::serialPort.set_option(boost::asio::serial_port::parity());
        SerialProtocolHandle::serialPort.set_option(boost::asio::serial_port::stop_bits());
        SerialProtocolHandle::serialPort.set_option(boost::asio::serial_port::character_size(8));
        std::cout << "successfully connected to Exoskeleton serial port." << std::endl;
    }
    else
    {
        std::cout << "Fail to connect to Exoskeleton serial port." << std::endl;
    }
}

void SerialProtocolHandle::ReceiveCargo(void)
{
    if (msgDetectStage < 3)
    {
        if (boost::asio::read(SerialProtocolHandle::serialPort, boost::asio::buffer(SerialProtocolHandle::tempRx, 1)))
        {
            if (msgDetectStage == 0)
            {
                if (tempRx[0] == 0xAA)
                    msgDetectStage = 1;
            }
            else if (msgDetectStage == 1)
            {
                if (tempRx[0] == 0xCC)
                    msgDetectStage = 2;
                else
                    msgDetectStage = 0;
            }
            else if (msgDetectStage == 2)
            {
                bytesToRead = tempRx[0];
                msgDetectStage = 3;
            }
        }
    }
    else if (msgDetectStage == 3)
    {
        if (boost::asio::read(SerialProtocolHandle::serialPort, boost::asio::buffer(SerialProtocolHandle::tempRx, bytesToRead + 3)))
        {
            // // std::cout << "Reached crc check stage." << std::endl;
            // uint32_t crcCalculate[bytesToRead + 1];
            // crcCalculate[0] = (uint32_t)bytesToRead;
            // // std::cout << std::hex << (int)crcCalculate[0] << std::endl;////////////////////
            // for (uint8_t i = 0; i <= bytesToRead - 1; i++)
            // {
            //     crcCalculate[i + 1] = (uint32_t)tempRx[i];
            //     // std::cout << std::hex << (int)crcCalculate[i + 1] << std::endl;////////////////////
            // }
            // uint32_t crcResult = CRC32_32BitsInput(crcCalculate, bytesToRead + 1);
            // // std::cout << "CRC calculated result: " << std::hex << (unsigned int)crcResult << std::endl;
            
            uint8_t trueSize = bytesToRead + 3;
            // std::cout << "True size: "  << (unsigned int)trueSize << std::endl;
            uint8_t crcCalBuf[trueSize];
            memset(crcCalBuf, 0xFF, sizeof(crcCalBuf));
            crcCalBuf[0] = 0xAA;
            crcCalBuf[1] = 0xCC;
            crcCalBuf[2] = bytesToRead;
            memcpy(&crcCalBuf[3], tempRx, bytesToRead);
            // std::cout << "CRC calculated buff: " << std::endl;
            // for (uint8_t i = 0; i <= sizeof(crcCalBuf)-1; i++)
            // {
            //     std::cout << std::hex << (unsigned int)crcCalBuf[i] << std::endl;
            // }
            uint16_t crcResult = CRC16_Modbus(crcCalBuf, sizeof(crcCalBuf));
            // std::cout << "CRC calculated result: " << std::hex << (unsigned int)crcResult << std::endl;
            uint32_t crcReveiced = tempRx[bytesToRead] | tempRx[bytesToRead + 1] << 8;
            // std::cout << "CRC Received result: " << std::hex << (unsigned int)crcReveiced<<std::endl;


            if (tempRx[bytesToRead + 2] == 0x55 && crcResult == crcReveiced)
            {
                msgDetectStage = 0;

                memcpy(rxMessageCfrm, tempRx, bytesToRead);
                rxMessageLen = bytesToRead;
                // if (ifNewMessage)
                    // std::cout << "Unprocessed Message Detected!" << std::endl;
                ifNewMessage = 1;
                DataLogReceiveManager();
                
                // std::cout<<"Cargo Received!"<<std::endl;
                // std::cout<<"Message is :" << std::endl;
                // for (uint8_t i = 0; i <= bytesToRead - 1; i++)
                // {
                //     std::cout.fill('0');
                //     std::cout.width(2);
                //     std::cout << std::hex<< (int)rxMessageCfrm[i] << " ";
                // }
                // std::cout << std::endl;

            }
            else
            {
                std::cout << "Invalid Cargo!" << std::endl;
            }

            // std::cout << "CRC real result: " << std::hex << (unsigned int)crcFromMFEC << std::endl;
        }
    }
}

void SerialProtocolHandle::TransmitCargo(uint8_t *data, uint8_t len)
{
    uint8_t txBuf[len + 6];
    txBuf[0] = 0xAA;
    txBuf[1] = 0xCC;
    txBuf[2] = len;
    memcpy(&txBuf[3], data, len);
    uint8_t crcCalculate[len + 3];
    crcCalculate[0] = 0xAA;
    crcCalculate[1] = 0xCC;
    crcCalculate[2] = len;
    memcpy(&crcCalculate[3], data, len);
    uint16_t crc = CRC16_Modbus(crcCalculate, len + 3);
    // std::cout<<"tx crc is: "<<std::hex<<(unsigned int)crc<<std::endl;
    txBuf[len + 3] = (uint8_t)(crc & 0x000000FF);
    txBuf[len + 4] = (uint8_t)(crc >> 8 & 0x000000FF);
    txBuf[len + 5] = 0x55;

    // for (uint8_t i = 0; i <= sizeof(txBuf) - 1; i++)
    // {
    //     std::cout<<std::hex<<(unsigned int)txBuf[i]<<" ";
    // }
    // std::cout<<std::endl;

    SerialProtocolHandle::serialPort.write_some(boost::asio::buffer(txBuf, len + 6));
}

void SerialProtocolHandle::SendText(std::string text)
{
    TransmitCargo((uint8_t *)text.data(), text.length());
}

void SerialProtocolHandle::StartDataLogActive(std::string filename)
{
    curDatalogTask = DATALOG_TASK_START_ACTIVE;
    fileStream.open(filename.data());
    fileStream << "Index," << "Time (ms),";
}

bool SerialProtocolHandle::ifNewMsgIsThisString(std::string str)
{
    if(ifNewMessage)
    {
        std::string msg((const char*)rxMessageCfrm, rxMessageLen);
        if (!msg.compare(str.data()))
        {
            ifNewMessage = 0;
            return true;
        }
        return false;
    }
    return false;
}

void SerialProtocolHandle::DataLogReceiveManager(void)
{
    if (ifNewMsgIsThisString("Datalog start"))
    {
        std::cout << "Datalog started passively!" << std::endl;
        curDatalogTask = DATALOG_TASK_START_PASSIVE;
        fileStream.open(curDatalogFilename);
        fileStream << "Index," << "Time (ms),";
        return;
    }
    if (ifNewMsgIsThisString("Datalog end"))
    {
        std::cout << "Datalog ending passively!" << std::endl;
        curDatalogTask = DATALOG_TASK_END_PASSIVE;
        return;
    }
 ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if (curDatalogTask == DATALOG_TASK_FREE)
    {}
    else if (curDatalogTask == DATALOG_TASK_START_PASSIVE || curDatalogTask == DATALOG_TASK_START_ACTIVE)
    {
       if(ifNewMessage)
       {
           std::string msg((const char*)rxMessageCfrm, rxMessageLen);
           if (!strncmp((const char*)msg.data(), "len: ", 5) )
           {
               std::string numStr(&msg.data()[5], 2);
               dataSlotLen = atoi(numStr.data());
               dataSlotLabellingCount = 1;
               curDatalogTask = DATALOG_TASK_RECEIVE_DATA_SLOT_LEN;
               std::cout<<"Received Data Slot Length: "<<std::dec<<(unsigned int)dataSlotLen<<std::endl;
               ifNewMessage = 0;
           }
       }
    }
    else if (curDatalogTask == DATALOG_TASK_RECEIVE_DATA_SLOT_LEN)
    {
        if (ifNewMsgIsThisString("Datalog label"))
        {
            curDatalogTask = DATALOG_TASK_RECEIVE_DATA_SLOT_MSG;
            dataslotLabel.clear();
        }
    }
    else if (curDatalogTask == DATALOG_TASK_RECEIVE_DATA_SLOT_MSG)
    {
        if (ifNewMsgIsThisString("Datalog label"))
            return;
        if (ifNewMessage)
        {
            if (rxMessageCfrm[0] == dataSlotLabellingCount && (dataslotLabel.size() + 1) == rxMessageCfrm[0])
            {
                std::string msg((const char*)(rxMessageCfrm + 1), rxMessageLen - 1);
                fileStream << msg.data() << ",";
                dataslotLabel.push_back(msg.data());
                std::cout<<"Received dataslot label " << std::dec << (int)dataSlotLabellingCount << ": " << msg.data() << std::endl;
                dataSlotLabellingCount++;
                ifNewMessage = 0;
            }
            else if (dataslotLabel.size() == dataSlotLen && (rxMessageLen == dataSlotLen * 4 + 8))
            {
                std::cout << "Dataslot labels are: ";
                for(int i=0;i<dataslotLabel.size();i++)
                {
                    std::cout << dataslotLabel[i] << "  ";
                }
                std::cout << std::endl;
                
                std::cout << "Datalog ongoing......" << std::endl;
                fileStream << std::endl;
                curDatalogTask = DATALOG_TASK_DATALOG;
                ifNewMessage = 0;
            }
            else
            {
                std::cout << "Received Data label error. Exit current datalog sequence!" << std::endl;
                curDatalogTask == DATALOG_TASK_FREE;
                fileStream.close();
                std::cout << "file stream closed!"<<std::endl;
                ifNewMessage = 0;
            }
        }
    }
    else if (curDatalogTask == DATALOG_TASK_DATALOG)
    {
        if(ifNewMsgIsThisString("Datalog end"))
        {
            std::cout<<"Current datalog finished!"<<std::endl;
            curDatalogTask == DATALOG_TASK_END_PASSIVE;
            fileStream.close();
        }

        if (ifNewMessage)
        {
            memcpy(&index, rxMessageCfrm, 4);
            memcpy(&systemTime, rxMessageCfrm + 4, 4);
            fileStream << index << "," << systemTime << ",";
            std::cout << "Index: " << std::dec << (int)index << " Time: " << (int)systemTime << " ";
            for (uint8_t i = 1 ; i <= dataSlotLen; i++)
            {
                float data = 0.0f;
                memcpy(&data, rxMessageCfrm + 4 * (i + 1), 4);
                fileStream << data << ",";
                std::cout << dataslotLabel[i - 1]<< ": " << data<< "   ";
            }
            std::cout << std::endl;
            fileStream << std::endl;
            ifNewMessage = 0;
        }
        
    }
    else if (curDatalogTask == DATALOG_TASK_END_PASSIVE || curDatalogTask == DATALOG_TASK_END_ACTIVE)
    {
        if(ifNewMsgIsThisString("Datalog Ready to End!"))
        {
            std::cout << "Datalog ended!" << std::endl;
            curDatalogTask = DATALOG_TASK_FREE;
        }
    }
}

void SerialProtocolHandle::DataLogTransmitManager(void)
{
    if (curDatalogTask == DATALOG_TASK_START_ACTIVE)
    {
        SendText("Datalog start");
    }
    else if (curDatalogTask == DATALOG_TASK_START_PASSIVE)
    {
        SendText("Datalog start request confirmed!");
    }
    else if (curDatalogTask == DATALOG_TASK_RECEIVE_DATA_SLOT_LEN)
    {
        SendText("Data slot length received!");
    }
    else if (curDatalogTask == DATALOG_TASK_RECEIVE_DATA_SLOT_MSG)
    {
        if (dataSlotLabellingCount >= 1 && dataSlotLabellingCount <= dataSlotLen + 1)
        {
            TransmitCargo((uint8_t*)&dataSlotLabellingCount, 1);
        }
    }
    else if (curDatalogTask == DATALOG_TASK_DATALOG)
    {}
    else if (curDatalogTask == DATALOG_TASK_END_ACTIVE)
    {
        SendText("Datalog end");
    }
    else if (curDatalogTask == DATALOG_TASK_END_PASSIVE)
    {
        SendText("Datalog end request received!");
    }
    else if (curDatalogTask == DATALOG_TASK_FREE)
    {
        SendText("Datalog free");
    }
}

void SerialProtocolHandle::DataLogManager(void)
{
    DataLogTransmitManager();
}

void SerialProtocolHandle::EndDataLogActive(void)
{
    curDatalogTask = DATALOG_TASK_END_ACTIVE;
    fileStream.close();
}
