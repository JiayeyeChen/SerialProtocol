#include <main.hpp>

void USB_RxCargo(SerialProtocolHandle* hserial)
{
    while(1)
    {
        hserial->ReceiveCargo();
    }
}

void Keyboard(SerialProtocolHandle* hserial)
{
    while(1)
    {
        std::string inputStr;
        std::cin >> inputStr;
        
        if (inputStr == "datalog-start")
        {
            std::cout << "User input: Datalog start!"<<std::endl;
            hserial->StartDataLogActive(hserial->curDatalogFilename);
        }
        else if (inputStr == "datalog-end")
        {
            std::cout << "User input: Datalog end!"<<std::endl;
            hserial->EndDataLogActive();
        }
        else if (!inputStr.compare(0,4,"file")) //Velocity control
        {
            hserial->curDatalogFilename.clear();
            std::cin >> hserial->curDatalogFilename;
            std::cout << "File name is: " << hserial->curDatalogFilename << std::endl;
        }
    }
}
//argv[1]: USB device address. argv[2]: Serial port baudrate. argv[3]: Datalog filename.
int main(int argc, char** argv)
{
    SerialProtocolHandle hSerial(argv[1], atoi(argv[2]), argv[3]);
    std::thread Thread_USB_RxCargo(USB_RxCargo, &hSerial);
    std::thread Thread_KeyboardInput(Keyboard, &hSerial);

    hSerial.curDatalogFilename = argv[3];
    while(1)
    {
        hSerial.DataLogManager();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return 0;
}
