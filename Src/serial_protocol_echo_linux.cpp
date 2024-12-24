#include "serial_protocol_echo_linux.hpp"

using namespace std::chrono_literals;

SerialProtocolEchoSlaveHandle::SerialProtocolEchoSlaveHandle(std::string device_repo, int baudrate, std::string file_name) : serialPort(io, device_repo), \
                                                            msgDetectStage(0), ifNewMessage(0), curDatalogTask(DATALOG_TASK_NA), \
                                                            dataSlotLen(0), task(SERIALPROTOCOLECHO_TASK_FREE)
{
    SerialProtocolEchoSlaveHandle::curDatalogFilename = file_name;
    if (!SerialProtocolEchoSlaveHandle::serialPort.is_open())
    {
        try
        {
            SerialProtocolEchoSlaveHandle::serialPort.open(device_repo);
        }
        catch (...) // Problem: Cannot catch exception when USB is disconnected
        {
            std::cout << "Fail to connect to serial port." << std::endl;
        }
    }
    if (SerialProtocolEchoSlaveHandle::serialPort.is_open())
    {
        SerialProtocolEchoSlaveHandle::serialPort.set_option(boost::asio::serial_port_base::baud_rate(baudrate));
        SerialProtocolEchoSlaveHandle::serialPort.set_option(boost::asio::serial_port::flow_control());
        SerialProtocolEchoSlaveHandle::serialPort.set_option(boost::asio::serial_port::parity());
        SerialProtocolEchoSlaveHandle::serialPort.set_option(boost::asio::serial_port::stop_bits());
        SerialProtocolEchoSlaveHandle::serialPort.set_option(boost::asio::serial_port::character_size(8));
        std::cout << "successfully connect to serial port." << std::endl;
    }
    else
    {
        std::cout << "Fail to connect to serial port." << std::endl;
    }
}

void SerialProtocolEchoSlaveHandle::Host(void)
{
    ReceiveCargo();
    if (ifNewMessage)
    {
        if (ifNewMsgIsThisString("Serial Protocol Echo Master: State Free"))
        {
            if (task != SERIALPROTOCOLECHO_TASK_FREE)
                std::cout << "Entered task: Free" << std::endl;
            task = SERIALPROTOCOLECHO_TASK_FREE;
        }
        else if (ifNewMsgIsThisString("Datalog end request"))
        {
            std::cout << "Datalog end request received." << std::endl;
            SendText("Datalog end request received");
            curDatalogTask = DATALOG_TASK_END;
            return;
        }

        switch (task)
        {
            case SERIALPROTOCOLECHO_TASK_FREE:
            {
                if (ifNewMsgIsThisString("Datalog start request")) //Embedded system initiate a datalog start
                {
                    std::cout << "Entered task: Datalog" << std::endl;
                    task = SERIALPROTOCOLECHO_TASK_DATALOG;
                    curDatalogTask = DATALOG_TASK_RECEIVE_DATA_SLOT_LEN;
                }

                //Enter Datalog task
                if (curDatalogTask == DATALOG_TASK_START_SLAVE)
                    SendText("Datalog start");
                break;
            }
            case SERIALPROTOCOLECHO_TASK_DATALOG:
            {
                DatalogManager();
                break;
            }
        }
        ifNewMessage = 0;
    }
}


void SerialProtocolEchoSlaveHandle::ReceiveCargo(void)
{
    if (msgDetectStage < 3)
    {
        if (boost::asio::read(SerialProtocolEchoSlaveHandle::serialPort, boost::asio::buffer(SerialProtocolEchoSlaveHandle::tempRx, 1)))
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
        if (boost::asio::read(SerialProtocolEchoSlaveHandle::serialPort, boost::asio::buffer(SerialProtocolEchoSlaveHandle::tempRx, bytesToRead + 3)))
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
                msgDetectStage = 0;
                std::cout << "Invalid Serial Cargo: ";
                for (int i = 0; i <= bytesToRead + 2; i++)
                {
                    std::cout << std::hex << (int)tempRx[i] << " ";
                }
                std::cout << std::endl;
            }

            // std::cout << "CRC real result: " << std::hex << (unsigned int)crcFromMFEC << std::endl;
        }
    }
}

void SerialProtocolEchoSlaveHandle::TransmitCargo(uint8_t *data, uint8_t len)
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

    SerialProtocolEchoSlaveHandle::serialPort.write_some(boost::asio::buffer(txBuf, len + 6));
}

void SerialProtocolEchoSlaveHandle::SendText(std::string text)
{
    TransmitCargo((uint8_t *)text.data(), text.length());
}


bool SerialProtocolEchoSlaveHandle::ifNewMsgIsThisString(std::string str)
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

void SerialProtocolEchoSlaveHandle::DatalogManager(void)
{
    switch (curDatalogTask)
    {
        /////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////
        case DATALOG_TASK_START:
        {
            SendText("Datalog start");
            break;
        }
        /////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////
        case DATALOG_TASK_RECEIVE_DATA_SLOT_LEN:
        {
            if (ifNewMsgIsThisString("Datalog start request"))
                SendText("Datalog start request received");
            else
            {
                std::string msg((const char*)rxMessageCfrm, rxMessageLen);
                if (!strncmp((const char*)msg.data(), "len: ", 5) )
                {
                    SendText("Datalog length received");
                    std::string numStr(&msg.data()[5], 2);
                    dataSlotLen = atoi(numStr.data());
                    dataSlotLabellingCount = 1;
                    dataslotLabel.clear();
                    curDatalogTask = DATALOG_TASK_RECEIVE_DATA_SLOT_LABEL;
                    fileStream.open(curDatalogFilename);
                    fileStream << "Index," << "Time (ms),";
                    std::cout<<"Received Data Slot Length: "<<std::dec<<(unsigned int)dataSlotLen<<std::endl;
                }
            }
            break;
        }
        /////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////
        case DATALOG_TASK_RECEIVE_DATA_SLOT_LABEL:
        {
            std::string msg((const char*)rxMessageCfrm, rxMessageLen);
            if (!strncmp((const char*)msg.data(), "len: ", 5) )
                SendText("Datalog length received");
            /////////////////Receive labels///////////////
            if (rxMessageCfrm[0] == dataSlotLabellingCount && (dataslotLabel.size() + 1) == rxMessageCfrm[0])
            {
                std::string msg((const char*)(rxMessageCfrm + 1), rxMessageLen - 1);
                fileStream << msg.data() << ",";
                dataslotLabel.push_back(msg.data());
                std::cout<<"Received dataslot label " << std::dec << (int)dataSlotLabellingCount << ": " << msg.data() << std::endl;
                dataSlotLabellingCount++;
                SendText("Label received");
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
                curDatalogTask = DATALOG_TASK_LOGGING_DATA;
            }
            break;
        }
        /////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////
        case DATALOG_TASK_LOGGING_DATA:
        {
            // if(ifNewMsgIsThisString("Datalog end"))
            // {
            //     std::cout<<"Current datalog finished!"<<std::endl;
            //     curDatalogTask == DATALOG_TASK_END_PASSIVE;
            //     fileStream.close();
            // }

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
            break;
        }
        /////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////
        case DATALOG_TASK_END:
        {
            if (fileStream.is_open())
                fileStream.close();
            SendText("Datalog end");
            break;
        }
    }
}
