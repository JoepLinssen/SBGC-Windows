#include <iostream>
#include <SBGC.h>
#include <Windows.h>

int main() {
	//Open port
	HANDLE hComm = CreateFile(TEXT("\\\\.\\COM15"),
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING,
		NULL);

	if(hComm == INVALID_HANDLE_VALUE) {
		std::cout << "Opening port. Error " << GetLastError() << std::endl;
		return 0;
	} else {
		std::cout << "Succesfully opened port!" << std::endl;
	}

	// Use Device Control Block to set serial port properties
	DCB dcb = {0};
	dcb.DCBlength = sizeof(DCB);

	if(!::GetCommState (hComm, &dcb)) {
		std::cout << "Could not retrieve commport state. Error " << GetLastError() << std::endl;
		return 0;
	}
	dcb.BaudRate = CBR_115200;
	dcb.ByteSize = 8;
	dcb.Parity	 = NOPARITY; 
	dcb.StopBits = ONESTOPBIT;
		
	if(!::SetCommState(hComm, &dcb)) {
		std::cout << "Could not set commport state. Error " << GetLastError() << std::endl;
		return 0;
	}

	// Set up timeouts
	COMMTIMEOUTS timeouts = { 0 };
	timeouts.ReadIntervalTimeout         = 50;
	timeouts.ReadTotalTimeoutConstant    = 50;
	timeouts.ReadTotalTimeoutMultiplier  = 10;
	timeouts.WriteTotalTimeoutConstant   = 50;
	timeouts.WriteTotalTimeoutMultiplier = 10;

	if(!::SetCommTimeouts(hComm, &timeouts)) {
		std::cout << "Could set commport timeouts. Error " << GetLastError() << std::endl;
		return 0;
	}

	//Request profile 2 info
	BYTE b[6] = {0x3E, 0x44, 0x01, 0x45, 0x01, 0x01};
	DWORD numWritten = 0;
	if(!::WriteFile(hComm, b, sizeof(b), &numWritten, NULL)) {
		std::cout << "Could not write to commport. Error " << GetLastError() << std::endl;
		return 0;
	} else {
		std::cout << "Data request written to port!  ";
		std::cout << "numWritten: " << numWritten << ", length(b): " << sizeof(b)/sizeof(*b) << std::endl;
	}
	::Sleep(20); //sleep thread for 20 milliseconds

	//Retrieve received data
	BYTE out[255];
	memset(&out, 0, sizeof(out));
	DWORD numRead = 0;
	if(!::ReadFile(hComm, &out, sizeof(out), &numRead, NULL)) {
		std::cout << "Could not read from commport. Error " << GetLastError() << std::endl;
		return 0;
	} 
	std::cout << "Read bytes from port!  ";
	std::cout << "numRead: " << numRead << std::endl;
	for (int i = 0; i < numRead; i++) {
		printf("%i ", out[i]);
	} printf("\n");



}