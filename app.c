//
// app.c - This program reads text from a
// 	serial port and prints it to the console.
//	Also saves them to a CSV file, 
//	and sends to remote server http://193.136.120.133/~sad/ 
// 	using HTTP POST
// 
//
// To compile with MinGW:
//
//      gcc -o app.exe app.c -lws2_32
//
// set baudrate = 9600
//
//
// To stop the program, press Ctrl-C
//


#include <stdlib.h>
#include <stdio.h>

// AppPost
#include<winsock2.h>
#pragma comment(lib,"ws2_32.lib") //Winsock Library

// CSV file
#include<time.h>

// readCom 
//#define WINVER 0x0500

#include <windows.h>
#define MESSAGE_LENGTH 100

// Declare variables and structures
HANDLE hSerial = INVALID_HANDLE_VALUE;
DCB dcbSerialParams = { 0 };
COMMTIMEOUTS timeouts = { 0 };
DWORD dwBytesWritten = 0;
char dev_name[MAX_PATH] = "";
int dev_number = -1;
int baudrate = 9600;
int scan_max = 30;
int scan_min = 1;
int simulate_keystrokes = 0;
int debug = 1; // print some info by default
int id = -1;

void CloseSerialPort()
{
    if (hSerial != INVALID_HANDLE_VALUE)
    {
        // Close serial port
        fprintf(stderr, "\nClosing serial port...");
        if (CloseHandle(hSerial) == 0)
            fprintf(stderr, "Error\n");
        else fprintf(stderr, "OK\n");
    }
}

void exit_message(const char* error_message, int error)
{
    // Print an error message
    fprintf(stderr, error_message);
    fprintf(stderr, "\n");

    // Exit the program
    exit(error);
}

void simulate_keystroke(char c)
{
    // This structure will be used to create the keyboard
    // input event.
    INPUT ip;

    // Set up a generic keyboard event.
    ip.type = INPUT_KEYBOARD;
    ip.ki.wScan = 0; // hardware scan code for key
    ip.ki.time = 0;
    ip.ki.dwExtraInfo = 0;

    // Press the key
    // Currently only alphabet lettes, spaces, commas and full stops.
    if (c >= 0x61 && c <= 0x7A) c -= 0x20;
    if (c >= 0x30 && c <= 0x39) ip.ki.wVk = c;
    else if (c >= 0x41 && c <= 0x5A) ip.ki.wVk = c;
    else if (c == ' ') ip.ki.wVk = VK_SPACE;
    else if (c == ',') ip.ki.wVk = VK_OEM_COMMA;
    else if (c == '.') ip.ki.wVk = VK_OEM_PERIOD;
    else if (c == '\b') ip.ki.wVk = VK_BACK;
    else if (c == '\t') ip.ki.wVk = VK_TAB;
    else if (c == '\n') ip.ki.wVk = VK_RETURN;
    else return;

    ip.ki.dwFlags = 0; // 0 for key press
    SendInput(1, &ip, sizeof(INPUT));

    // Release the key
    ip.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
    SendInput(1, &ip, sizeof(INPUT));
}

void writeCSV(int mode, int * str_com_values){
	
	time_t timer;
    char buffer[26];
    struct tm* tm_info;

    timer = time(NULL);
    tm_info = localtime(&timer);

    strftime(buffer, 26, "%d-%m-%Y %H:%M:%S", tm_info);

	FILE *fpt;

	fpt = fopen("appCSV.csv", "a");
	if (mode == 1){
		fprintf(fpt,"%s,temperature,%d\n", buffer, str_com_values[0]);
	}
	else if (mode == 2){
		fprintf(fpt,"%s,light,%d\n", buffer, str_com_values[1]);
	}
	else if (mode == 3){
		fprintf(fpt,"%s,temperature,%d\n", buffer, str_com_values[0]);
		fprintf(fpt,"%s,light,%d\n", buffer, str_com_values[1]);
	}	
	
	fclose(fpt);
	
}

int main(int argc, char* argv[])
{	
	// Welcome message
    if (debug)
    {
        fprintf(stderr, "\napp.exe\n");
        fprintf(stderr, "Version: 10-6-2012\n\n");
    }
	
	// app Post section
	WSADATA wsa;
	SOCKET s;
	struct sockaddr_in server;

	printf("\nInitialising Winsock...");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Failed. Error Code : %d", WSAGetLastError());
		return 1;
	}

	printf("Initialised.\n");

	//Create a socket
	if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		printf("Could not create socket : %d", WSAGetLastError());
	}

	printf("Socket created.\n");

	server.sin_addr.s_addr = inet_addr("193.136.120.133");
	server.sin_family = AF_INET;
	server.sin_port = htons(80);

	//Connect to remote server
	if (connect(s, (struct sockaddr*)&server, sizeof(server)) < 0)
	{
		puts("connect error");
		return 1;
	}

	puts("Connected");
	
	// Post vars
	char content [150];
	char msgRequest[150];
	char ipAux [] = "193.136.120.133";
	
    // Parse command line arguments. Available options:
    //
    // /devnum DEVICE_NUMBER
    // /baudrate BITS_PER_SECOND
    // /id ROBOT_ID_NUMBER
    // /keystrokes
    // /debug
    // /quiet
    //
    int n = 1;
    while (n < argc)
    {
        // Process next command line argument
        if (strcmp(argv[n], "/devnum") == 0)
        {
            if (++n >= argc) exit_message("Error: no device number specified", 1);

            // Set device number to specified value
            dev_number = atoi(argv[n]);
        }
        else if (strcmp(argv[n], "/baudrate") == 0)
        {
            if (++n >= argc) exit_message("Error: no baudrate value specified", 1);

            // Set baudrate to specified value
            baudrate = atoi(argv[n]);
        }
        else if (strcmp(argv[n], "/id") == 0)
        {
            if (++n >= argc) exit_message("Error: no id number specified", 1);

            // Set id to specified value
            id = atoi(argv[n]);
        }
        else if (strcmp(argv[n], "/keystrokes") == 0)
        {
            simulate_keystrokes = 1;
        }
        else if (strcmp(argv[n], "/debug") == 0)
        {
            debug = 2;
        }
        else if (strcmp(argv[n], "/quiet") == 0)
        {
            debug = 0;
        }
        else
        {
            // Unknown command line argument
            if (debug) fprintf(stderr, "Unrecognised option: %s\n", argv[n]);
        }

        n++; // Increment command line argument counter
    }

    // Debug messages
    if (debug > 1) fprintf(stderr, "dev_number = %d\n", dev_number);
    if (debug > 1) fprintf(stderr, "baudrate = %d\n\n", baudrate);

    // Register function to close serial port at exit time
    atexit(CloseSerialPort);

    if (dev_number != -1)
    {
        // Limit scan range to specified COM port number
        scan_max = dev_number;
        scan_min = dev_number;
    }

    // Scan for an available COM port in _descending_ order
    for (n = scan_max; n >= scan_min; --n)
    {
        // Try next device
        sprintf(dev_name, "\\\\.\\COM%d", n);
        if (debug > 1) fprintf(stderr, "Trying %s...", dev_name);
        hSerial = CreateFile(dev_name, GENERIC_READ | GENERIC_WRITE, 0, 0,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (hSerial != INVALID_HANDLE_VALUE)
        {
            if (debug > 1) fprintf(stderr, "OK\n");
            dev_number = n;
            break; // stop scanning COM ports
        }
        else if (debug > 1) fprintf(stderr, "FAILED\n");
    }

    // Check in case no serial port could be opened
    if (hSerial == INVALID_HANDLE_VALUE)
        exit_message("Error: could not open serial port", 1);

    // If we get this far, a serial port was successfully opened
    if (debug) fprintf(stderr, "Opening COM%d at %d baud\n\n", dev_number, baudrate);

    // Set device parameters:
    //  baudrate (default 9600), 1 start bit, 1 stop bit, no parity
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (GetCommState(hSerial, &dcbSerialParams) == 0)
        exit_message("Error getting device state", 1);
    dcbSerialParams.BaudRate = baudrate;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    if (SetCommState(hSerial, &dcbSerialParams) == 0)
        exit_message("Error setting device parameters", 1);

    // Set COM port timeout settings
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    if (SetCommTimeouts(hSerial, &timeouts) == 0)
        exit_message("Error setting timeouts", 1);

    // Read text and print to console (and maybe simulate keystrokes)
    int state = 1;
    //int i;
    char c;
    char message_buffer[MESSAGE_LENGTH] = {'\0'};
    DWORD bytes_read;

    // Depending on whether a robot id has been specified, either
    // print all incoming characters to the console or filter by
    // the specified id number
	int need_clean = 1;
	int found_start = 0;
	int mode = 0;
	int temp = 0;
	int light = 0;
	int values [2];// 0 - temp, 1 - light 
  
	while (1)
	{
			
		ReadFile(hSerial, &c, 1, &bytes_read, NULL);
	
		if (bytes_read == 1)
		{
			if (need_clean == 1){
				strcpy(message_buffer, "");
				need_clean = 0;
			}
			
			strncat(message_buffer, &c, 1);
			printf("%c", c);
			if(found_start == 0){
				if(strstr(message_buffer,"<msg>") != NULL){
					
					strcpy(message_buffer, "<msg>");
					found_start = 1;
				}	
			}
			else{	
				if (strstr(message_buffer,"<msg>") != NULL && strstr(message_buffer,"</msg>") != NULL){ 
					if (strstr(message_buffer,"<m>1</m>") != NULL){
						sscanf(message_buffer, "<msg><m>%d</m><t>%d</t></msg>", &mode, &values[0]);
						
						// write to CSV file
						writeCSV(mode, values);
						
						// write to server
						strcpy(content, message_buffer);
						sprintf(msgRequest,"POST /~sad/ HTTP/1.1\r\nHost: %s\r\nContent-Type: application/xml\r\nContent-Length: %d\r\n\r\n%s",ipAux, strlen(content), content);
						if (send(s, msgRequest, strlen(msgRequest), 0) < 0)
						{
							return 1;
						}
					}
					else if (strstr(message_buffer,"<m>2</m>") != NULL){
						sscanf(message_buffer, "<msg><m>%d</m><l>%d</l></msg>", &mode, &values[1]);
						
						//write to CSV file
						writeCSV(mode, values);
						
						// write to server
						strcpy(content, message_buffer);
						sprintf(msgRequest,"POST /~sad/ HTTP/1.1\r\nHost: %s\r\nContent-Type: application/xml\r\nContent-Length: %d\r\n\r\n%s",ipAux, strlen(content), content);
						if (send(s, msgRequest, strlen(msgRequest), 0) < 0)
						{
							return 1;
						}
					}
					else if (strstr(message_buffer,"<m>3</m>") != NULL){
						sscanf(message_buffer, "<msg><m>%d</m><l>%d</l><t>%d</t></msg>", &mode, &values[1], &values[0]);
						
						//write to CSV file
						writeCSV(mode, values);
						
						// write to server
						strcpy(content, message_buffer);
						sprintf(msgRequest,"POST /~sad/ HTTP/1.1\r\nHost: %s\r\nContent-Type: application/xml\r\nContent-Length: %d\r\n\r\n%s",ipAux, strlen(content), content);
						if (send(s, msgRequest, strlen(msgRequest), 0) < 0)
						{
							return 1;
						}
					}	
					need_clean =1;
					found_start = 0;
				}	
				if (simulate_keystrokes == 1) simulate_keystroke(c);
			}	
		}     
    }
    

    // We should never get to this point because when the user
    // presses Ctrl-C, the atexit function will be called instead.
    return 0;
}