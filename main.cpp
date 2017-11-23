#include <iostream>
#include <SBGC.h>
#include <Windows.h>

class WindowsComObj : public SBGC_ComObj {
	HANDLE hComm; //Serial port handle
public:
	int init() { //TODO fix variable port number]
		//Open port
		hComm = CreateFile(TEXT("\\\\.\\COM15"),
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING,
			NULL);

		if(hComm == INVALID_HANDLE_VALUE) {
			return 0;
		} 

		// Use Device Control Block to set serial port properties
		DCB dcb = {0};
		dcb.DCBlength = sizeof(DCB);

		if(!::GetCommState (hComm, &dcb))
			return 0;
		dcb.BaudRate = CBR_115200;
		dcb.ByteSize = 8;
		dcb.Parity	 = NOPARITY; 
		dcb.StopBits = ONESTOPBIT;
		
		if(!::SetCommState(hComm, &dcb))
			return 0;
		
		// Set up event monitoring
		SetCommMask(hComm, EV_RXCHAR);

		// Set up timeouts
		COMMTIMEOUTS timeouts = { 0 };
		timeouts.ReadIntervalTimeout         = 50;
		timeouts.ReadTotalTimeoutConstant    = 50;
		timeouts.ReadTotalTimeoutMultiplier  = 10;
		timeouts.WriteTotalTimeoutConstant   = 50;
		timeouts.WriteTotalTimeoutMultiplier = 10;

		SetCommTimeouts(hComm, &timeouts);


		return 1;
	}

	// Return the number of bytes received in input buffer
	virtual uint16_t getBytesAvailable() {
		DWORD dwEventMask;
		return WaitCommEvent(hComm, &dwEventMask, NULL);
	}
	
	// Read byte from the input stream
	virtual uint8_t readByte() {
		uint8_t out;
		ReadFile(hComm, &out, sizeof(out), NULL, NULL);
		return out;
	}

	// Read message from input system
	uint8_t readMessage(BYTE *buf, DWORD szBuf, DWORD *numRead) {
		return ::ReadFile(hComm, buf, szBuf, numRead, NULL);
	}

	// Write byte to the output stream
	virtual void writeByte(uint8_t b) {
		DWORD numWritten = 0;
		WriteFile(hComm, &b, sizeof(b), &numWritten, NULL);
	}

	// Return the space available in the output buffer. Return 0xFFFF if unknown.
	virtual uint16_t getOutEmptySpace() {
		return 0xFFFF;
	}
};


int main() {
	
	SBGC_Parser sbgc_parser;
	WindowsComObj com;

	//Initialize connection
	if(!com.init()) {
		DWORD dw = GetLastError();
		std::cout << "Communication initialization failed. Error " << dw << std::endl;
		return 0;
	} else {
		std::cout << "Succesfully initialized communication!" << std::endl;
	}

	sbgc_parser.init(&com);

	//Request real time data
	SerialCommand cmd;
	cmd.init(SBGC_CMD_REALTIME_DATA);
	sbgc_parser.send_cmd(cmd, 0);
	std::cout << "Data request written to port!" << std::endl;
	::Sleep(20);

	//Retrieve received data
	BYTE out[255];
	memset(&out, 0, sizeof(out));
	DWORD numRead = 0;
	if(!com.readMessage(out, sizeof(out), &numRead)) {
		std::cout << "Could not read from commport. Error " << GetLastError() << std::endl;
		return 0;
	}
	std::cout << "Read " << numRead << " bytes from commport" << std::endl;
	for (int i = 0; i < numRead; i++) {
		printf("%i ", out[i]);
	} printf("\n");

	//Parse received message
	std::cout << "Parsing message... ";
	if(!sbgc_parser.parse_message(out, sizeof(out))) {
		std::cout << "Failed." << std::endl;
		return 0;
	}
	std::cout << "Success!" << std::endl;

	//Unpack 
	SBGC_cmd_realtime_old_t rt_data;

	std::cout << "Unpacking... ";
	SerialCommand rec_cmd = sbgc_parser.in_cmd;
	switch(rec_cmd.id) {
	case SBGC_CMD_REALTIME_DATA: {
		std::cout << "ID: REALTIME_DATA... ";
		uint8_t r = SBGC_cmd_realtime_old_unpack(rt_data, rec_cmd);
		if(r == 0) {
			std::cout << "Unpacked!!" << std::endl;
		} else {
			std::cout << "Error. Code: " << r << std::endl;
			return 0;
		}
		break;
	}
	default: {
		std::cout << "Unknown message ID! (" << rec_cmd.id << ")" << std::endl;
		return 0; 
		break;
	}
	}

	std::cout << "Camera euler angles! r: " << SBGC_DEGREE_ANGLE_SCALE*rt_data.imu_angle[0] 
								  << " p: " << SBGC_DEGREE_ANGLE_SCALE*rt_data.imu_angle[1] 
								  << " y: " << SBGC_DEGREE_ANGLE_SCALE*rt_data.imu_angle[2] << std::endl;

}