/* 
 * File:   main.cpp
 * Author: Thor
 *
 * Created on 24. marts 2015, 16:00
 */

#include <stdint.h> //For uint  
#include <sstream>  //String Streams for convertion 
#include <iostream> //Input/output 
#include <vector>   //For string vectors (string arry)
#include <cstdlib>  //For accesses to c++ std namespace 
#include <fstream>  //For files 
#include <time.h>   //For sytem time 
#include <unistd.h> //For nanoSleep 
#include <pthread.h>    //For multithreading 

//For network 
#include <arpa/inet.h>  //For 'inet_ntoa' to get readable ip 
#include <cstring>      //For memset'ing the socket structs 
#include <sys/socket.h> //For sockets :-D
#include <netdb.h>      //For some socket stuff...Dono
#include <errno.h>      //For error msg's

#include "ProgArg_s.h"  //For system arguments 

#define PATH_ARG 1//From OS
#define EXPECTED_NO_OF_ARGS 2  +PATH_ARG//From the user/OS
#define NO_FLAGS 0
#define EXPECTED_CLIENTS 10
#define CMD_AMOUNT 10
#define RX_BUFFER_SIZE 1400
#define SAMPLE_RESOLUTION_ms 200

//Will send IP and thread No back to client if defined 
#define DEBUG

using namespace std;

//How many sensor's conected to this board
static const uint8_t SENSOR_AMOUNT = 5;
static const string SENSOR_AMOUNT_STR = ("5");

//Can't return character for some strange reson?! Use this instead of std::endl;
static const string ENDL = ("\n");

//The usage for this program
static const string USAGE = ("Usage: -port xxxx");
static const string REMOTE_USAGE = (
        "Remote Usage --> Send a 'TM20' cmd followed by: \n\r"
        "                 'GSA' =  GET_SONSOR_AMOUNT\n\r"
        "                 'GET_S' = READ_SENSOR_NO followed by the sensor No\n\r"
        "                 'KILL_C' = CLOSE_CONNECTION from server side\n\r"
        "                 'ECHO' = MAKE_UPPERCASE followed by a str to send back in uppercase \n\r"
        "                 'SEN_SET' = SET_SAMPLERATE followed by sensor No followed by sample rate No\n\r"
        "                 'STOP_S' = STOP_SENSOR followed by sensor No\n\r"
        "                 'START_S' = START_SENSOR same  \n\r"
        "                 'STATUS' = GET_BOARD_STATUS\n\r"
        "You must send atleast one cmd but you can send as many you want in one go \n\r");

//Strings, each corresponding to a cmd from the client user. See above ^
static const string TM20_CMD = ("TM20");
static const string GET_SONSOR_AMOUNT = ("GSA");
static const string READ_SENSOR_NO = ("GET_S");
static const string CLOSE_CONNECTION = ("KILL_C");
static const string MAKE_UPPERCASE = ("ECHO");
static const string SET_SAMPLERATE = ("SEN_SET");
static const string STOP_SENSOR = ("STOP_S");
static const string START_SENSOR = ("START_S");
static const string GET_BOARD_STATUS = ("STATUS");

//For the tokenizer
const char DELIMITER = ' ';

//Argument expected for this program 
//ProgArg_s ipAddresForThisServer(1, "-ip", STRING, 14);
ProgArg_s portNrForThisServer(2, "-port", NUMBER);

//For running through the arguments you just made. 
//REMEMBER TO CHANGE THE VECTOR SIZE TO FIT YOUR ARGS :-D 
std::vector<ProgArg_s*> args_v(1);

//Mutex variable to make code sections thread safe (synchronize between threads)
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
uint16_t threadCount = 0;
bool freeThreadSlots[EXPECTED_CLIENTS];

/**
 * Sensor struct with all the info for the sensors
 */
struct Sensors_s {
    uint32_t smapleRate;
    uint8_t sensorNo;
    string sensorNo_str;
    string sensorName;
    uint64_t sensorValue;
    string timeStamp;
    uint32_t counter;
    bool isActive;
};
Sensors_s sensorList[SENSOR_AMOUNT];//List of sensors

/**
 * Data needed for the client threads.
 * Mostly used to send debug responses.
 */
struct ClientData_s {
    int socketDescripter;//The socket used to communicate with the client
    uint8_t threadId;
    struct sockaddr_in clientAddr;

};


//______________ From the Internet!!!! ________________
//http://coliru.stacked-crooked.com/a/652f29c0500cf195) 
//http://stackoverflow.com/questions/53849/how-do-i-tokenize-a-string-in-c

void tokenize(std::string str, std::vector<string> &token_v) {
    size_t start = str.find_first_not_of(DELIMITER), end = start;

    while (start != std::string::npos) {
        // Find next occurence of delimiter
        end = str.find(DELIMITER, start);
        // Push back the token found into vector
        token_v.push_back(str.substr(start, end - start));
        // Skip all occurences of the delimiter to find new start
        start = str.find_first_not_of(DELIMITER, end);
    }
}
//========== FROM INTERNET END =================

/**
 * !!!NOT IMPLIMENTED!!! 
 * @TODO Make it. 
 * @Remember Lock mutex for thread safety!
 * @param log
 */
void writeLogToFile(stringstream log) {
    pthread_mutex_lock(&lock);
    //    
    //    ifstream logfile;
    //    
    //    
    //    

    pthread_mutex_unlock(&lock);
}

/**
 * Simulating a sensor read. 
 * Returns a random number.
 * @param uint8_t sensorNo
 * @return int64_t random No
 */
int64_t readSensor(uint8_t sensorNo) {
    return random();
}

/**
 * Set the sample rate for a sensor 
 * @ThreadSafe Yes 
 * @param uint8_t sensorNo
 * @param uint32_t sempleRate
 */
void setSensorSampleRate(uint8_t sensorNo, uint32_t sempleRate) {
    pthread_mutex_lock(&lock);
    sensorList[sensorNo].smapleRate = sempleRate;
    pthread_mutex_unlock(&lock);
}

/**
 * From the internet!
 * Get the system time as a string. 
 * @return time as a string 
 */
const string getTime() {
    //___________  FROM INTERNET!!!! ____________ 
    //http://stackoverflow.com/questions/997946/how-to-get-current-time-and-date-in-c

    // Get current date/time
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    // Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
    // for more information about date/time format
    strftime(buf, sizeof (buf), "%X. %d-%m-%Y", &tstruct);

    return buf;
    //========== FROM INTERNET END =================
}

/**
 * This is to be able to handle more than one client at the time.
 * 
 * 
 * This is the thread a clients gets when she is connected to this server. 
 * It handles the commands described in the "REMOTE_USAGE". 
 * It will keep the connection up until client disconnect or get a cmd to shut 
 * down by the client user. 
 * It will check the inputs from the client user and act accordingly. 
 * @param void *clientSocket struct (info about this client)
 * @return void * (nothing)
 */
void *clientHandlerThread(void *clientSocket) {

    bool keepAlive = true;
    stringstream log; //Not used 
    stringstream converter; //For converting strings to numbers and vice versa
    vector<string> rxStrings_v; //Hold the cmd's after they have bean tokenized
    string rxString; //For converting the char buf to string so it can be tokenized (easer than going through the buf byte by byte :-D)
    string txMsg; //Msg to send
    char rxBuffer [RX_BUFFER_SIZE]; //For holding the incoming bytes 

    struct ClientData_s *thisClientData; //To get the clientSocket out from the void*

    thisClientData = (struct ClientData_s *) clientSocket; //Get the data from the void*

    while (keepAlive) {
        //Clear all containers 
        converter.str(std::string());
        converter.clear();
        txMsg.clear();
        rxString.clear();
        rxStrings_v.clear();
        memset(rxBuffer, 0, sizeof (rxBuffer));

        //Wait for incoming bytes from the client user 
        ssize_t bytesRx = recv(thisClientData->socketDescripter, rxBuffer, RX_BUFFER_SIZE, NO_FLAGS);

        // ________ Error checking _________
        //This should go to a log stream and then to a file. (In the future)
        if (bytesRx == 0) {
            cout << "Connection from: " << inet_ntoa(thisClientData->clientAddr.sin_addr) << " is lost. Closing thread and conection" << ENDL;
            keepAlive = false;
            continue;
        }

        if (bytesRx == -1) {
            cout << "Rx error!" << strerror(errno) << ENDL;
            keepAlive = false;
            continue;
        }
        // ________ Error checking END _________

#ifdef DEBUG //Sending info back to the user client
        int threadId = thisClientData->threadId;
        converter << threadId;
        txMsg += "Hello '";
        txMsg += inet_ntoa(thisClientData->clientAddr.sin_addr); //Get readable IP address
        txMsg += "' you have client thread:" + converter.str() + ENDL;
#endif

        rxString.append(rxBuffer); //Convert char buf to string for tokenizer
        tokenize(rxString, rxStrings_v); //Tokenize the incoming data string

        //This section will go through the incoming data and act according to 
        //any cmd's send by the client user. It will also check for validity 
        //and respond if something is a miss. There is quite a lot of checks to 
        //make absolutely sure the user can't break this program. It has bean 
        //tested thoroughly and has not failed yet, but never say never! It is 
        //not beautiful with that big for-loop, but because it's inside a thread 
        //its easy than making many thread safe functions. 
        if (rxStrings_v.at(0) != TM20_CMD) {//Check for TM20 cmd
            txMsg += "ERROR: No 'TM20' cmd received" + ENDL;
            txMsg += REMOTE_USAGE;
        } else {
            for (uint8_t i = 1; i < rxStrings_v.size(); i++) {//Go through all incoming cmds/sub-strings

                if (rxStrings_v[i] == GET_SONSOR_AMOUNT) {

                    txMsg += SENSOR_AMOUNT_STR + " Sensors are attached to this board" + ENDL;
                    txMsg += "Sensors are:" + ENDL;
                    for (uint8_t i = 0; i < SENSOR_AMOUNT; i++) {//Get all info about all sensors
                        txMsg += "Name: " + sensorList[i].sensorName;
                        txMsg += " is No: " + sensorList[i].sensorNo_str;
                        txMsg += ENDL;
                    }
                } else if (rxStrings_v[i] == READ_SENSOR_NO) {
                    i++;//Jump to next string 
                    if (i < rxStrings_v.size()) {// Make sure the string exists. Safety first !!
                        stringstream converter(rxStrings_v.at(i));//For converting string to number 
                        int sensorNo;//The number (USE INTS!!! UINT_T DONT WORK. IMPORTANT!!!)
                        if (converter >> sensorNo) {//Try to convert
                            stringstream tempConv;//New converter for sensor value. Number to string
                            tempConv << sensorList[sensorNo].sensorValue;//We know its a number no need to check :-)
                            txMsg += "Sensor name: " + sensorList[sensorNo].sensorName;
                            txMsg += "Sensor No: " + sensorList[sensorNo].sensorNo_str;
                            txMsg += " Reads: ";
                            txMsg += tempConv.str();//The sensor value 
                            txMsg += " Sampled: " + sensorList[sensorNo].timeStamp;
                            txMsg += ENDL;
                        } else {//Make Error massage
                            txMsg += "ERROR: Read sensor '" + rxStrings_v[i] +
                                    "' is not a number! Pull yourself together!" + ENDL + ENDL;
                            txMsg += REMOTE_USAGE;
                            break;
                        }
                    } else {//Make Error massage
                        txMsg += "ERROR: Need a sensor number to read! What is wrong with you!" + ENDL + ENDL;
                        txMsg += REMOTE_USAGE;
                        break;
                    }
                } else if (rxStrings_v[i] == CLOSE_CONNECTION) {
                    txMsg += "Closing connection from server side. "
                            "Thank you for participating. See you next time! :-D" + ENDL;
                    keepAlive = false;//Close connection --> thread will close aswell 
                } else if (rxStrings_v[i] == MAKE_UPPERCASE) {
                    ++i; //make next string to upper case 
                    if (i < rxStrings_v.size()) {//Safety first !!
                        txMsg += "ECHO '" + rxStrings_v[i] + "' To upper: ";
                        for (uint8_t ch = 0; ch < rxStrings_v[i].length(); ch++) {//Convert string to upper case
                            txMsg += toupper(rxStrings_v[i].at(ch));
                        }
                    } else {
                        txMsg += "ERROR: Need a string to echo! What do you expect?! Miracles?" + ENDL + ENDL;
                        txMsg += REMOTE_USAGE;
                        break;
                    }
                    txMsg += ENDL;
                } else if (rxStrings_v[i] == SET_SAMPLERATE) {
                    ++i;
                    if (i < rxStrings_v.size()) {//Safety first !!
                        converter.str(rxStrings_v[i]);
                        int sensorNo;
                        if (converter >> sensorNo) {
                            ++i;
                            if (i < rxStrings_v.size()) {//Safety first !!{
                                converter.clear();
                                converter.str(rxStrings_v[i]);
                                int sampleRate;
                                if (converter >> sampleRate) {
                                    setSensorSampleRate(sensorNo, sampleRate);//This is a thread safe function
                                    txMsg += "Sensor name: " + sensorList[sensorNo].sensorName + ENDL;
                                    txMsg += "Sensor No: " + sensorList[sensorNo].sensorNo_str + ENDL;
                                    txMsg += "now has sample rate  ";
                                    //No need to convert number again, we know it's OK at this point. Cheating hehe :-D
                                    txMsg += rxStrings_v[i] + ENDL;

                                } else {
                                    txMsg += "ERROR: Set sample rate to '" + rxStrings_v[i] +
                                            "' is not a number! Pull yourself together!" + ENDL + ENDL;
                                    txMsg += REMOTE_USAGE;
                                    break;
                                }
                            } else {
                                txMsg += "ERROR: Need a sample rate to set! What is wrong with you!" + ENDL + ENDL;
                                txMsg += REMOTE_USAGE;
                                break;
                            }
                        } else {
                            txMsg += "ERROR: Set sample rate of sensor '" + rxStrings_v[i] +
                                    "' is not a number! Pull yourself together!" + ENDL + ENDL;
                            txMsg += REMOTE_USAGE;
                            break;
                        }
                    } else {
                        txMsg += "ERROR: Need a sensor number to set a sample rate to! What is wrong with you!" + ENDL + ENDL;
                        txMsg += REMOTE_USAGE;
                        break;
                    }
                } else if (rxStrings_v[i] == STOP_SENSOR) {
                    i++;
                    if (i < rxStrings_v.size()) {//Safety first !!
                        converter.str(rxStrings_v[i]);
                        int sensorNo;
                        if (converter >> sensorNo) {
                            sensorList[sensorNo].isActive = false;
                            txMsg += "Sensor name: " + sensorList[sensorNo].sensorName + ENDL;
                            txMsg += "Sensor No: " + sensorList[sensorNo].sensorNo_str + ENDL;
                            txMsg += "Is now INACTIVE" + ENDL;
                        } else {
                            txMsg += "ERROR: Stop sensor '" + rxStrings_v[i - 1] +//Tell them whats wrong
                                    "' is not a number! Pull yourself together!" + ENDL + ENDL;
                            txMsg += REMOTE_USAGE;
                            break;
                        }
                    } else {
                        txMsg += "ERROR: Need a sensor number to stop! What is wrong with you!" + ENDL + ENDL;
                        txMsg += REMOTE_USAGE;
                        break;
                    }
                } else if (rxStrings_v[i] == START_SENSOR) {
                    i++;
                    if (i < rxStrings_v.size()) {//Safety first !!
                        converter.str(rxStrings_v[i]);
                        int sensorNo;
                        if (converter >> sensorNo) {
                            sensorList[sensorNo].isActive = true;
                            txMsg += "Sensor name: " + sensorList[sensorNo].sensorName + ENDL;
                            txMsg += "Sensor No: " + sensorList[sensorNo].sensorNo_str + ENDL;
                            txMsg += "Is now ACTIVE" + ENDL;
                        } else {
                            txMsg += "ERROR: Start sensor '" + rxStrings_v[i - 1] +
                                    "' is not a number! Pull yourself together!" + ENDL + ENDL;
                            txMsg += REMOTE_USAGE;
                            break;
                        }
                    } else {
                        txMsg += "ERROR: Need a sensor number to start! What is wrong with you!" + ENDL + ENDL;
                        txMsg += REMOTE_USAGE;
                        break;
                    }
                } else if (rxStrings_v[i] == GET_BOARD_STATUS) {//TODO: make this a bit more interesting
                    txMsg += getTime();
                } else {
                    txMsg += "ERROR: Wrong cmd received" + ENDL + ENDL;
                    txMsg += REMOTE_USAGE;
                    break;
                }
            }
        }
        ssize_t bytesSent = send(thisClientData->socketDescripter, txMsg.c_str(), txMsg.length(), NO_FLAGS);
        if (bytesSent != txMsg.length())std::cout << "Send error. Bytes lost";
    }

    close(thisClientData->socketDescripter);//Close this socket 
    //Lock for updating the thread list
    pthread_mutex_lock(&lock);
    cout << "Closing from: " << inet_ntoa(thisClientData->clientAddr.sin_addr) << ENDL;
    freeThreadSlots[thisClientData->threadId] = true;//This slot is now open 
    threadCount--;//One les thread is now running 
    pthread_mutex_unlock(&lock);
    pthread_exit(NULL);//Close down this thread
}

/**
 * A thread for polling the sensors at the sample-rate interval specified  for ech sensor.
 * Set the resolution for the polling at the "SAMPLE_RESOLUTION_ms" define
 * @param void* for pthread
 * @return void* for pthread 
 */
void *sensorHandler(void *arg) {
    for (;;) {
        //Set the sleep time in nano sec. This needs a funky struct to work, 
        //thats why its a bit strange
        nanosleep((struct timespec[]) {
            {0, 10000 * SAMPLE_RESOLUTION_ms}
        }, NULL);

        //Go trough sensor list to see if its time to sample the specific sensor
        for (uint8_t i = 0; i < SENSOR_AMOUNT; i++) {
            if (sensorList[i].isActive && sensorList[i].counter++ >= sensorList[i].smapleRate) {
                sensorList[i].counter = 0; //No need to lock this 
                pthread_mutex_lock(&lock); //Lock so none is reading while we update the values
                sensorList[i].timeStamp = getTime();
                //Call with struct No. to be sure values is consistent (i and number can be different)
                sensorList[i].sensorValue = readSensor(sensorList[i].sensorNo);
                pthread_mutex_unlock(&lock);
            }
        }
    }
}

/**
 * Initialize the sensors
 */
void initSensors() {
    sensorList[0].sensorName = "Temp-Sensor";
    sensorList[0].sensorNo = 0; //For internal function calls
    sensorList[0].sensorNo_str = "0"; //Both str and int val to ease the TCP responds
    sensorList[0].sensorValue = 0;
    sensorList[0].smapleRate = 1000; //Times the "SAMPLE_RESOLUTION_ms" 
    sensorList[0].counter = 0;
    sensorList[0].isActive = true;

    sensorList[1].sensorName = "ADC";
    sensorList[1].sensorNo = 1;
    sensorList[1].sensorNo_str = "1";
    sensorList[1].sensorValue = 0;
    sensorList[1].smapleRate = 2000;
    sensorList[1].counter = 0;
    sensorList[1].isActive = true;

    sensorList[2].sensorName = "Humidity-Sensor";
    sensorList[2].sensorNo = 2;
    sensorList[2].sensorNo_str = "2";
    sensorList[2].sensorValue = 0;
    sensorList[2].smapleRate = 1000;
    sensorList[2].counter = 0;
    sensorList[2].isActive = true;

    sensorList[3].sensorName = "Light-Sensor";
    sensorList[3].sensorNo = 3;
    sensorList[3].sensorNo_str = "3";
    sensorList[3].sensorValue = 0;
    sensorList[3].smapleRate = 5000;
    sensorList[3].counter = 0;
    sensorList[3].isActive = true;

    sensorList[4].sensorName = "Magnet-Sensor";
    sensorList[4].sensorNo = 4;
    sensorList[4].sensorNo_str = "4";
    sensorList[4].sensorValue = 0;
    sensorList[4].smapleRate = 1000;
    sensorList[4].counter = 0;
    sensorList[4].isActive = true;
}

/**
 * Main 
 * @param argc
 * @param argv
 * @return 
 */
int main(int argc, char** argv) {

    initSensors();
    
    //Collection of client data. Each thread need one for a safe location in mem
    ClientData_s clientData[EXPECTED_CLIENTS] = {0};
    
    //Threads used in this program
    pthread_t threads[EXPECTED_CLIENTS];
    pthread_t sensorPollingThread;

    int32_t errorCode = 0;
    int32_t socketId = 0;

    //  args_v[0] = &ipAddresForThisServer;
    args_v[0] = &portNrForThisServer; //Argument from user 

    //For holding and filling socket struct's 
    struct addrinfo hostInfo;
    struct addrinfo* hostInfoList;

    //Clean 
    memset(&hostInfo, 0, sizeof (hostInfo));
    memset(&freeThreadSlots, true, sizeof (freeThreadSlots));


    //================== Argument Checks ===================
    //See ProgArg_s files for the args used here 
    if (argc < EXPECTED_NO_OF_ARGS || argc > EXPECTED_NO_OF_ARGS) {
        cout << "Wrong number of arguments" << ENDL;
        cout << USAGE << ENDL;
        return false;
    }

    // O(n^2) 
    for (uint8_t i = 0; i < args_v.size(); i++) {//Get arguments  
        uint8_t j;
        for (j = 1; j < argc; j++) {//0.arg is always path. Don't check that 
            if (args_v[i]->equals(argv[j])) {
                if (!args_v[i]->isValid(argv[j + 1])) {
                    args_v[i]->printError();
                    cout << USAGE << ENDL;
                    return false;
                }
                break;
            }
        }
        if (!args_v[i]->hasValue) {
            args_v[i]->printError();
            cout << USAGE << ENDL;
        }
    }
    //=============== Argument Checks finished ==================

    //For Socket initialization see 
    //http://codebase.eu/tutorial/linux-socket-programming-c/ 

    //Address info : address = Address field unspecifed (both IPv4 & 6)
    hostInfo.ai_addr = AF_UNSPEC;

    // Address info : socket type. (Use SOCK_STREAM for TCP or SOCK_DGRAM for UDP.)
    hostInfo.ai_socktype = SOCK_STREAM;

    hostInfo.ai_flags = AI_PASSIVE; //From .h file: "Socket address is intended for `bind'." 

    //getaddrinfo is used to get info for the socket.
    //Null: use local host. Port no is set by user args (5555)
    errorCode = getaddrinfo(NULL, portNrForThisServer.getParamVal().c_str(), &hostInfo, &hostInfoList);
    if (errorCode != 0)
        cout << "getaddrinfo error" << gai_strerror(errorCode);

    //Make a socket and returns a socket descriptor. 
    //All info comes from 'getaddrinfo' --> into the struct 'hostInfoList'
    socketId = socket(hostInfoList->ai_family, hostInfoList->ai_socktype, hostInfoList->ai_protocol);
    if (socketId == -1)
        cout << "Socket error" << strerror(errno);

    //Socket options is set to reuse the add: http://pubs.opengroup.org/onlinepubs/7908799/xns/setsockopt.html
    //This is to make sure the port is not in use by a previous call by this code. 
    int optionValue_yes = 1;
    errorCode = setsockopt(socketId, SOL_SOCKET, SO_REUSEADDR, &optionValue_yes, sizeof optionValue_yes);
    //Bind socket to local port.
    errorCode = bind(socketId, hostInfoList->ai_addr, hostInfoList->ai_addrlen);
    if (errorCode == -1)
        cout << "Bind error" << strerror(errno);

    errorCode = listen(socketId, EXPECTED_CLIENTS);
    if (errorCode == -1)
        cout << "Listen error" << strerror(errno);

    freeaddrinfo(hostInfoList); //Not needed anymore

    //Make thread for polling the sensors
    pthread_create(&sensorPollingThread, NULL, sensorHandler, NULL);
    
    for (;;) {
        if (threadCount < EXPECTED_CLIENTS) {
            int newIncommingSocket = 0; //New socket ID
            struct sockaddr_in incommingAddr; //New socket address info
            socklen_t addrSize = sizeof (incommingAddr); //The size of the address
            //Wait for a client to connect. PROGRAM BLOCKS HERE UNTIL CLIENTS CONNECT 
            newIncommingSocket = accept(socketId, (struct sockaddr*) &incommingAddr, &addrSize);
            if (newIncommingSocket == -1) std::cout << "Accept error" << strerror(errno); //Check for errors
            else {
                //Show the incoming IP in readable txt
                cout << "Connection accepted. From : " << inet_ntoa(incommingAddr.sin_addr) << ENDL;
                //Find a free slot for this client
                for (uint8_t freeSlot = 0; freeSlot < EXPECTED_CLIENTS; freeSlot++) {
                    if (freeThreadSlots[freeSlot]) {
                        //Free slot found. Lock for thread safe updating of the thread list
                        pthread_mutex_lock(&lock);
                        freeThreadSlots[freeSlot] = false; //This slot is now taken
                        threadCount++;
                        pthread_mutex_unlock(&lock);
                        //Copy info to safe location before parsing to new thread
                        //These structs holds info about the client for the thread to service
                        clientData[freeSlot].threadId = freeSlot; //What slot does this thread have
                        clientData[freeSlot].clientAddr = incommingAddr; //This cleint address
                        clientData[freeSlot].socketDescripter = newIncommingSocket; //This clients socket
                        //Make the new thread and parse the info struct as void pointer
                        pthread_create(&threads[freeSlot], NULL, clientHandlerThread, static_cast<void*> (&clientData[freeSlot]));
                        break;
                    }
                }
            }
        }

    }
    return 0;
}
