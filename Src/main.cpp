#include <main.hpp>

void Keyboard(SerialProtocolEchoSlaveHandle* hserial)
{
    while(1)
    {
        std::string inputStr;
        std::cin >> inputStr;
        
        if (inputStr == "ds")
        {
            std::cout << "User input: Datalog start!"<<std::endl;
            hserial->curDatalogTask = DATALOG_TASK_START_SLAVE;
        }
        else if (inputStr == "de")
        {
            std::cout << "User input: Datalog end!"<<std::endl;
            hserial->curDatalogTask = DATALOG_TASK_END;
        }
        else if (!inputStr.compare(0,4,"file"))
        {
            hserial->curDatalogFilename.clear();
            std::cin >> hserial->curDatalogFilename;
            std::cout << "File name is: " << hserial->curDatalogFilename << std::endl;
        }
        else if (!inputStr.compare(0,3,"say"))
        {
            std::string words;
            std::cin >> words;
            hserial->SendText(words);
        }
    }
}
//argv[1]: USB device address. argv[2]: Serial port baudrate. argv[3]: Datalog filename.
int main(int argc, char** argv)
{
    // SerialProtocolHandle hSerial(argv[1], atoi(argv[2]), argv[3]);
    SerialProtocolEchoSlaveHandle hSerial(argv[1], atoi(argv[2]), argv[3]);
    std::thread Thread_KeyboardInput(Keyboard, &hSerial);
    hSerial.curDatalogFilename = argv[3];
    while(1)
    {
        hSerial.Host();
        // std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return 0;
}
