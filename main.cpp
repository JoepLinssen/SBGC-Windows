#include <iostream>
#include <SBGC.h>
#include <Windows.h>
#include <stdlib.h>
#include <ctime>

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
	}
	sbgc_parser.init(&com);

	uint16_t loopitr = 0;
	uint16_t loopmax = 200;
	clock_t t_start = clock();
	clock_t t_now;
	while(loopitr < loopmax) {
		//Request real time data
		SerialCommand cmd;
		cmd.init(SBGC_CMD_REALTIME_DATA);
		sbgc_parser.send_cmd(cmd, 0);
		//::Sleep(10);

		//Retrieve received data
		BYTE out[255];
		memset(&out, 0, sizeof(out));
		DWORD numRead = 0;
		if(!com.readMessage(out, sizeof(out), &numRead)) {
			std::cout << "Could not read from commport. Error " << GetLastError() << std::endl;
			return 0;
		}
		/*std::cout << "Read " << numRead << " bytes from commport" << std::endl;
		for (int i = 0; i < numRead; i++) {
			printf("%i ", out[i]);
		} printf("\n");*/

		//Parse received message
		if(!sbgc_parser.parse_message(out, sizeof(out))) {
			std::cout << "Failed to parse message." << std::endl;
			return 0;
		}

		//Unpack 
		SBGC_cmd_realtime_old_t rt_data;

		SerialCommand rec_cmd = sbgc_parser.in_cmd;
		switch(rec_cmd.id) {
		case SBGC_CMD_REALTIME_DATA: {
			uint8_t r = SBGC_cmd_realtime_old_unpack(rt_data, rec_cmd);
			if(r != 0) {
				std::cout << "Failed to unpack messge. Error: " << r << std::endl;
				return 0;
			}
			break;
		}
		default: {
			std::cout << "Failed to unpack message. Unknown message ID! (" << rec_cmd.id << ")" << std::endl;
			return 0; 
			break;
		}
		}

		system("CLS");
		t_now = clock();
		printf("t: %2.4f, r: %3.3f, p: %3.3f, y: %3.3f\n", (double)(t_now - t_start) / CLOCKS_PER_SEC,
														 SBGC_ANGLE_TO_DEGREE(rt_data.imu_angle[0])*5, 
														 SBGC_ANGLE_TO_DEGREE(rt_data.imu_angle[1])*5,
														 SBGC_ANGLE_TO_DEGREE(rt_data.imu_angle[2])*5);

		loopitr++;
	}

	std::cout << "Finished! Average computation time per loop: " << (double)(t_now - t_start) / CLOCKS_PER_SEC / (double)loopmax << std::endl;

}