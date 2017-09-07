

#include "cstdio"
#include "cstdlib"
#include "string"
#include "iostream"
#include "string"
#include "vector"
#include "map"
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>

using namespace std;


/*!
 *  \class mdp_psim
 *  \brief Parallel SIMulator using C++.
 *  \author Massimo DiPierro
 *  \version 1.0
 *  \date    March 17, 2005
 *  \par Description of PSIM:
 *   This is a C++ class that allows a developer to create and debug parallel
 *   algorithms using the C++ language. PSIM allows the development,
 *   implementation and testing of parallel algorithms without dedicated
 *   hardware.
 */

class mdp_psim {
private:

  // Typed Constants
  const static int PROCESS_COUNT_MIN= 1;  //!< minimum number of processes
  const static int PROCESS_COUNT_MAX= 128;//!< maximum number of processes
  const static int CONN_LIST_IGNORE = -1; //!< connections that cannot occur
  const static int PROCESS_PARENT   = 0;  //!< The parent process ID number
  const static int COMM_RECV = 0;      //!< socket array indicator for reading
  const static int COMM_SEND= 1;  //!< socket array indicator for writing
  const static int COMM_TIMEOUT_DEFAULT = 86400;  //!< 1 day default

  // common enum values for logging routines
  enum enumBegEnd       { LOG_BEGIN, LOG_END };
  enum enumSendRecv     { LOG_SR_SEND, LOG_SR_RECV };
  enum enumSendRecvStep { LOG_SR_START, LOG_SR_FAIL, LOG_SR_SUCCESS };

  // Set this to true if you want verbose testing description

  // Class variables
  int     _verbatim;              //!< 0 for no output, 1 for some, 2 more
  int     _processCount;          //!< Holds the number of processes
  string  _logFileName;           //!< filename of the log file
  bool    _doLogging;             //!< do logging or not?
  FILE*   _logfileFD;             //!< file descriptor for the logging file
  int     _processID;             //!< process ID of "this" process

  int   (*_socketFD)[2];          //!< array to hold all of the sockets
  int _commTimeout;               //!< defaults to COMM_TIMEOUT_DEFAULT

  map< string, vector<char> >* _hash;  //!< Hash Map to hold out of sequence (send/receive) data


  /*!
       Used by the constructor ONLY.

       @param  processCount The number of processes to create in the simulation.
       @param  logFileName  The name of the file to be used for logging application progress.
       @param  verbatim level of output - 0 none, 1 some, 2 all
   */

  void psim_begin(int processCount, string logFileName, int verbatim) {
    _processCount=processCount;
    _logFileName=logFileName;
    _verbatim=verbatim;

    open_log();
    if ((processCount<PROCESS_COUNT_MIN) || (processCount<PROCESS_COUNT_MIN)) {
      log("PSIM ERROR: Invalid number of processes");
      throw string("PSIM ERROR: Invalid number of processes");
    }

    initialize();
    create_sockets();
    fork_processes();

    char buffer[256];
    sprintf(buffer, "process %i of %i created with pid=%i",
	    _processID, _processCount, getpid());
    log(buffer,1);

  }



  /*!
       Used by the destructor to close the sockets and cleanup all resources used by the class.
   */

  void psim_end() {
    for (int source=0; source<_processCount; source++) {
      for (int dest=0; dest<_processCount; dest++) {
	close(_socketFD[_processCount*source+dest][COMM_SEND]);
	close(_socketFD[_processCount*source+dest][COMM_RECV]);
      }
    }
    if (_socketFD != NULL)
      delete [] _socketFD;
    _socketFD = NULL;

    // only delete the _hash if it still exists
    if (_hash != NULL) {
      delete [] _hash;
      _hash = NULL;
    }

    char buffer[256];
    sprintf(buffer, "process %i terminating", _processID);
    log(buffer,1);

    close_log();
  }


  /*!
       Used by the constructor, this method creates hash table and the array
       that holds the socket descriptors.
   */

  void initialize() {
    _processID = PROCESS_PARENT;
    _commTimeout = COMM_TIMEOUT_DEFAULT;

    _hash = new map< string, vector<char> >[_processCount];
    if (_hash == NULL) {
      log("PSIM ERROR: failure to allocate hash");
      throw string("PSIM ERROR: failure to allocate hash");
    }
    _socketFD = new int[_processCount*_processCount][2];
    if (_socketFD == NULL) {
      log("PSIM ERROR: failed to create socket array");
      throw string("PSIM ERROR: failed to create socket array");
    }
  }


  /*!
       Opens all of the sockets necessary for communication and
       inserts them into an array.
   */

  void create_sockets() {
    for (int source=0; source<_processCount; source++) {
      for (int dest=0; dest<_processCount; dest++) {
	if (socketpair(AF_LOCAL, SOCK_STREAM, 0,
		       _socketFD[_processCount*source+dest]) < 0) {
	  log("PSIM ERROR: socketpair");
	  throw string("PSIM ERROR: socketpair");
	}
      }
    }
    char buffer[256];
    for (int source=0; source<_processCount; source++)
      for (int dest=0; dest<_processCount; dest++) {
	sprintf(buffer,"_socketFD[%i*%i+%i]={%i,%i}",
		source,_processCount,dest,
		_socketFD[_processCount*source+dest][COMM_SEND],
		_socketFD[_processCount*source+dest][COMM_RECV]);
	log(buffer);
      }
  }


  /*!
       Forks all of the processes creating the simulated parallel environment.
   */

  void fork_processes() {
    _processID=0;
    for (int i=1; i<_processCount; i++) {
      int pid = fork();

      if (pid == -1) {
	log("PSIM ERROR: fork");
	throw("PSIM ERROR: fork");
      } else if (pid == 0) { // Child Process
	_processID = i;
	break;
      }
    }
  }


  /*!
       Varifies that the destination process ID is valid. This is done before data is sent or received.
       @param  processID process id number to validate
   */

  void check_process_id(int processID) {

    if ((processID == _processID) ||
	(processID < PROCESS_PARENT) ||
	(processID >= _processCount)) {

      char buffer[256];
      sprintf(buffer, "PSIM ERROR: process %i referencing %i.",
	      _processID, processID);
      log(buffer);
      throw string( buffer );
    }
  }


  /*!
       This method initializes the process log and sets it up for appending messages.
   */

  void open_log() {
    _doLogging = false;
    if (_logFileName.length()==0) {
      return;
    }

    // open and reset file
    if ((_logfileFD = fopen(_logFileName.c_str(), "w")) == NULL) {
      log("PSIM ERROR: unable to create logfile");
      throw string("PSIM ERROR: unable to create logfile");
    }
    // close the log file
    close_log();

    // reopen the log file in append mode
    if ((_logfileFD = fopen(_logFileName.c_str(), "a")) == NULL) {
      log("PSIM ERROR: unable to open logfile");
      throw string("PSIM ERROR: unable to open logfile");
    }
    _doLogging=true;

  }


  /*!
       Closes the log file.
   */

  void close_log() {
    if (_doLogging)
      fclose(_logfileFD);
  }


  /*!
       Centralizes the repetitive task of logging the steps during send and receive.
       @param sourcedestProcessID the process id to log
       @param tag a string that helps to identify the data being sent
       @param method an enumerated value identifying the the method being logged
       @param step an enumerated value identifying 'start', 'fail' or 'success'
   */

  void logSendRecv(int sourcedestProcessID,
		   string tag,
		   enumSendRecv method,
		   enumSendRecvStep step){

    char buffer[256];
    const char cmdSendRecv[2][8] = { "send", "recv" };
    const char stepSendRecv[3][12] = { "starting...", "failed!", "success." };

    sprintf(buffer, "%i %s(%i,%s) %s",
	    _processID, cmdSendRecv[method],
	    sourcedestProcessID, tag.c_str(), stepSendRecv[step]);
    log(buffer);
  }


  /*!
       Handles the sending of binary data.
       @param destProcessID the destination process id to send data to
       @param pdataToSend pointer to data that is to be sent
       @param dataSize size of the data to be sent in bytes
   */

  void send_buffer(int destProcessID,
		   const void* pdataToSend, long dataSize) {

    int destIndex = _processCount*_processID+destProcessID;
    if (write(_socketFD[destIndex][COMM_SEND], pdataToSend, dataSize)
	!= dataSize) {
      log("PSIM ERROR: failure to write to socket");
      throw string("PSIM ERROR: failure to write to socket");
    }
  }


  /*!
       Sends a data tag and a vector of chars (as binary data).
       @param destProcessID the destination process id to send data to
       @param tag string identifier of the data being sent
       @param data a character vector holding all of the data to be sent
   */

  void send_binary(int destProcessID,
		   const string& tag,
		   const vector<char>& data) {

    int tagSize = tag.size();
    int dataSize = data.size();
    send_buffer(destProcessID, &tagSize, sizeof(tagSize));
    send_buffer(destProcessID, tag.c_str(), tagSize);
    send_buffer(destProcessID, &dataSize, sizeof(dataSize));
    send_buffer(destProcessID, &data[0], dataSize);
  }


   /*!
       Handles the receiving of binary data through the sockets.
       @param sourceProcessID the source process id to receive data from
       @param pdataToReceive pointer to the buffer that will hold the data received
       @param dataSize the size of the receive buffer
   */

  void recv_buffer(int sourceProcessID,
		   void* pdataToReceive, long dataSize) {
    long bytes = 0;
    long t0=time(NULL);
    int sourceIndex = _processCount*sourceProcessID+_processID;

    // set up blocking read
    do{
      bytes+=read(_socketFD[sourceIndex][COMM_RECV],
		  (char*) pdataToReceive+bytes,dataSize-bytes);
      if(time(NULL)-t0>_commTimeout) {
	log("PSIM ERROR: timeout error in readin from socket");
	throw string("PSIM ERROR: timeout error in readin from socket");
      }
    } while (bytes<dataSize);
  }


  /*!
       Receives data utilizing a data tag to make sure that the data coming in is what was expected.
       @param sourceProcessID the source process id to receive data from
       @param tag string identifier of the data being sent
       @param data a character vector for receiving the data to be sent
   */

  void recv_binary(int sourceProcessID,
		   const string& tag,
		   vector<char>& data ) {

    static vector<char> dataTmp;
    static string tagReceived;
    int size;

    map< string, vector<char> >::iterator itr;

    while (true) {
      itr = _hash[sourceProcessID].find(tag);

      if (itr != _hash[sourceProcessID].end()) { // Found?
	data = _hash[sourceProcessID][tag];
	_hash[sourceProcessID].erase(itr);
	break;
      } else {
	recv_buffer(sourceProcessID, &size, sizeof(size));
	char* buffer= new char[size+1];
	recv_buffer(sourceProcessID, buffer, size);
	buffer[size] = 0;
	tagReceived = buffer;
	delete buffer;

	if (tagReceived == tag) {
	  recv_buffer(sourceProcessID, &size, sizeof(size));
	  data.resize(size);
	  recv_buffer(sourceProcessID, &data[0], size);
	  break;
	} else {
	  recv_buffer(sourceProcessID, &size, sizeof(size));
	  dataTmp.resize(size);
	  recv_buffer(sourceProcessID, &dataTmp[0], size);
	  _hash[sourceProcessID][tagReceived] = dataTmp;
	}
      }
    }
  }


  /*!
       Centralized log method used by some of the public methods to send
       a standardized message to the log at the beginning and the end of
       the routine.
       @param method the name of the method logging the message
       @param begEnd enumerated value - beginning or end of the method
   */

  void doBegEndLog(string method, enumBegEnd begEnd) {
    char buffer[256];
    char* be;

    if (begEnd == LOG_BEGIN) be = "BEGIN";
    else  be = "END";

    sprintf(buffer, "%i %s [%s]", _processID, be, method.c_str());
    log(buffer);
  }

public:

  // *******************************************************************
  // *******************************************************************
  // ***                                                             ***
  // ***                P U B L I C   M E T H O D S                  ***
  // ***                                                             ***
  // *******************************************************************
  // *******************************************************************


  /*!
       Provide the number of processes to create and the name of
       the logfile if desired and "" if no logfile is needed.
       @param processCount the number of processes to create
       @param logFileName name of the logfile
       @param verbatim level of output - 0 none, 1 some, 2 all
   */

  mdp_psim(int processCount, string logFileName=".psim.log", int verbatim=0) {
    psim_begin(processCount, logFileName, verbatim);
  }

  /*!
       Construct the processes using command line arguments
       @param argc command line argument count
       @param argv command line arguments
   */

  mdp_psim(int argc, char** argv) {
    int processCount=parse_argv_nprocs(argc,argv);
    string logFileName=parse_argv_logfile(argc,argv);
    int verbatim=parse_argv_verbatim(argc,argv);
    psim_begin(processCount, logFileName, verbatim);
  }


  /*!
       Deallocates space that was created within the process, releases
       sockets, closes the log, etc.
       @param argc command line argument count
       @param argv command line arguments
   */

  virtual ~mdp_psim() {
    psim_end();
  }


  /*!
       Accepts a string and appends the message to the common log file.
       @param message information to send to the log
       @param level amount of data to log
   */

  void log(string message,int level=2) {
    if (_doLogging) {
      int fd=fileno(_logfileFD);
      flock(fd,LOCK_EX);
      fwrite("PSIM LOG: ", 10, 1, _logfileFD);
      fwrite(message.c_str(), message.length(), 1, _logfileFD);
      fwrite("\n", 1, 1, _logfileFD);
      // Clear out the file buffer
      fflush(_logfileFD);
      flock(fd,LOCK_UN);
    }
    if(_verbatim>=level) {
      cout << "PSIM LOG: " << message << endl;
      cout.flush();
    }
  }


  /*!
       Returns an integer identifying which process is currently executing.
       @return current process ID
   */

  int id() {
    return _processID;
  }


  /*!
       Returns an integer identifying the current number of active processes.
       @return number of processes
   */

  int nprocs() {
    return _processCount;
  }


  /*!
       Sets the number of seconds that a process will wait to
       receive data from another process before throwing an
       exception.
       @param commTimeout number of seconds to wait for a timeout during
       data transfer.
   */

  void setCommTimeout(unsigned int commTimeout) {
    _commTimeout = commTimeout;
  }


  /*!
       This aynchronous method sends the data referenced by
       "dataToSend" to "destProcessID".  The size of the data
       is obtained by looking at the type "T".
       @param destProcessID destination process id
       @param dataTag identifier of the data being sent
       @param dataToSend data being sent
   */

  template<class T>
  void send(int destProcessID, string dataTag, T &dataToSend) {
    logSendRecv(destProcessID, dataTag,LOG_SR_SEND, LOG_SR_START);
    vector<char> data(sizeof(T));
    for(unsigned long k=0; k<sizeof(T); k++)
      data[k]=((char*)&dataToSend)[k];
    send_binary(destProcessID, dataTag, data);
    // cout << _processID << "->" << destProcessID << " " << dataTag << " " << dataToSend << endl;
    logSendRecv(destProcessID, dataTag, LOG_SR_SEND, LOG_SR_SUCCESS);
  }


  /*!
       This aynchronous method sends the data at location
       "pdataToSend" to "destProcessID".  The size of the data
       being sent is provided in the integer: "dataSize".
       @param destProcessID destination process id
       @param dataTag identifier of the data being sent
       @param pdataToSend pointer to data being sent
       @param dataSize byte count of data being sent
   */

  template<class T>
  void send(int destProcessID, string dataTag,
	    T *pdataToSend, long dataSize) {
    logSendRecv(destProcessID, dataTag, LOG_SR_SEND, LOG_SR_START);
    vector<char> data(sizeof(T)*dataSize);
    for(long k=0; k<data.size(); k++)
      data[k]=((char*)pdataToSend)[k];
    send_binary(destProcessID, dataTag, data);
    logSendRecv(destProcessID, dataTag,LOG_SR_SEND, LOG_SR_SUCCESS);
  }


  /*!
       This synchronous "blocking" method retrieves the data sent
       to "destProcessID".  The size of the data being sent is
       provided in the integer: "dataSize".
       @param sourceProcessID source process id
       @param dataTag identifier of the data being received
       @param dataToReceive variable of type T for receiving data
   */

  template<class T>
  void recv(int sourceProcessID, string dataTag, T &dataToReceive) {
    logSendRecv(sourceProcessID, dataTag,LOG_SR_RECV, LOG_SR_START);
    vector<char> data;
    recv_binary(sourceProcessID, dataTag, data);
    if(data.size()!=sizeof(T)) {
      log("PSIM ERROR: recv invalid data)");
      throw string("PSIM ERROR: recv invalid data)");
    };
    for(unsigned long k=0; k<sizeof(T); k++)
      ((char*)&dataToReceive)[k]=data[k];
    // cout << _processID << "<-" << sourceProcessID << " " << dataTag << " " << dataToReceive << endl;
    logSendRecv(sourceProcessID, dataTag, LOG_SR_RECV, LOG_SR_SUCCESS);
  }


  /*!
       This synchronous "blocking" method retrieves the data sent
       to "destProcessID".  "dataSize" bytes are copied to
       location "pdataToReceive".
       @param sourceProcessID source process id
       @param dataTag identifier of the data being received
       @param pdataToReceive pointer to location to receive data
       @param dataSize size of receive buffer
   */

  template<class T>
  void recv(int sourceProcessID, string dataTag,
	    T *pdataToReceive, long dataSize) {
    logSendRecv(sourceProcessID, dataTag, LOG_SR_RECV, LOG_SR_START);
    vector<char> data;
    recv_binary(sourceProcessID, dataTag, data);
    if(data.size()!=sizeof(T)*dataSize) {
      log("PSIM ERROR: recv invalid data size");
      throw string("PSIM ERROR: recv invalid data size");
    }
    for(long k=0; k<data.size(); k++)
      ((char*)pdataToReceive)[k]=data[k];
    logSendRecv(sourceProcessID, dataTag, LOG_SR_RECV, LOG_SR_SUCCESS);
  }


  /*!
       Allows broadcasting data to all processes.
       sourceProcessID sends data (data) to all of the other
       processes who receive the data through the same variable).
       @param sourceProcessID source process id
       @param data data to send to all processes
   */

  template<class T>
  void broadcast(int sourceProcessID, T &data) {
    static string tag = "BROADCAST:0";
    doBegEndLog(tag, LOG_BEGIN);
    if (_processID == sourceProcessID) {
      for (int i=0; i<_processCount; i++) {
	if (i != sourceProcessID)
	  send(i, tag, data);
      }
    } else {
      recv(sourceProcessID, tag, data);
    }
    if(tag=="BROADCAST:0") tag="BROADCAST:1"; else tag="BROADCAST:0";
    doBegEndLog(tag, LOG_END);
  }


  /*!
       Allows broadcasting data to all processes.
       sourceProcessID sends data (data) to all of the other
       processes who receive the data through the same variable.
       @param sourceProcessID source process id
       @param data data to send to all processes
       @param dataSize size of the data to send
   */

  template<class T>
    void broadcast(int sourceProcessID, T *data, int dataSize) {
    static string tag = "BROADCASTV:0";
    doBegEndLog(tag, LOG_BEGIN);
    if (_processID == sourceProcessID) {
      for (int i=0; i<_processCount; i++) {
	if (i != sourceProcessID)
	  send(i, tag, data, dataSize);
      }
    } else {
      recv(sourceProcessID, tag, data, dataSize);
    }
    if(tag=="BROADCASTV:0") tag="BROADCASTV:1"; else tag="BROADCASTV:0";
    doBegEndLog(tag, LOG_END);
  }


  /*!
       All parallel processes construct a list of the data passed
       by each process - the list is broadcasted and returned by
       each processor - this method is used to implement global
       sum and some other global operations.
       @param dest destination process id
       @param data data to retrieve
       @return vector containing the list of data collected
   */

  template<class T>
  vector<T> collect(int dest, T &data) {
    static string tag="COLLECT";
    vector<T> dataList;
    T dataToReceive;
    dataList.resize(_processCount);
    doBegEndLog(tag, LOG_BEGIN);

    if (_processID != dest) {
      send(dest, tag, data);
    } else {
      dataList[dest]=data;

      for (int i=0; i<_processCount; i++) {
	if(i!=dest) {
	  recv(i, tag, dataToReceive);
	  dataList[i]=dataToReceive;
	}
      }
    }
    if(tag=="COLLECT:0") tag="COLLECT:1"; else tag="COLLECT:0";
    doBegEndLog(tag, LOG_END);
    return dataList;
  }


  /*!
       All parallel processes construct a list of the data passed
       by each process - the list is broadcasted and returned by
       each processor - this method is used to implement global
       sum and some other global operations.
       @param data data to process
       @return vector containing the list of data combined
   */

  template<class T>
  vector<T> combine(T &data) {
    vector<T> dataList=collect(PROCESS_PARENT,data);
    cout << id() << " size=" << dataList.size() << endl;

    broadcast(PROCESS_PARENT, &dataList[0], dataList.size());
    cout << id() << " list=" << dataList[0] << dataList[1]<< dataList[2]<< endl;
    return dataList;
  }


  /*!
       Initiates a blocking point so that the processes pause
       until ALL processes have reached the barrier.
   */

  void barrier() {
    int dummy;
    broadcast(PROCESS_PARENT,dummy);
    collect(PROCESS_PARENT,dummy);
  }


  /*!
       All parallel processes sum their data in parallel.  The sum
       is returned.
       @param item data to add to the list
       @return summed data
   */

  template<class T>
  T add(T &item) {
    vector<T> dataList;
    T total=0;
    dataList=collect(PROCESS_PARENT,item);
    if(_processID==PROCESS_PARENT)
      for (int i=0; i<dataList.size(); i++) {
	total += dataList[i];
      }
    broadcast(PROCESS_PARENT,total);
    return total;
  }


  /*!
       parses command line arguments for number of processes to create
       \verbatim
       Examples:
		  a.out -PSIM_NPROCS=2           (parallel processes)
		  a.out -PSIM_LOGFILE=./test.log (log into test.log)
		  a.out -PSIM_VERBATIM=1         (show all communications)
       \endverbatim
       @param argc command line argument count
       @param argv command line arguments
       @return number of processes parsed from the command line
   */

  static int parse_argv_nprocs(int argc, char** argv) {
    int n=1;
    for(int i=1; i<argc; i++)
      if(strncmp(argv[i],"-PSIM_NPROCS=",13)==0) {
	sscanf(argv[i]+13,"%i",&n);
	break;
      }
    return n;
  }


  /*!
       parses command line arguments for log file name
       \verbatim
       Examples:
		  a.out -PSIM_NPROCS=2           (parallel processes)
		  a.out -PSIM_LOGFILE=./test.log (log into test.log)
		  a.out -PSIM_VERBATIM=1         (show all communications)
       \endverbatim
       @param argc command line argument count
       @param argv command line arguments
       @return name of logfile parsed from the command line
   */

  static string parse_argv_logfile(int argc, char** argv) {
    for(int i=1; i<argc; i++)
      if(strncmp(argv[i],"-PSIM_LOGFILE=",14)==0) {
	return string(argv[i]+14);
      }
    return string("");
  }


  /*!
       parses command line arguments for logging level
       \verbatim
       Examples:
		  a.out -PSIM_NPROCS=2           (parallel processes)
		  a.out -PSIM_LOGFILE=./test.log (log into test.log)
		  a.out -PSIM_VERBATIM=1         (show all communications)
       \endverbatim
       @param argc command line argument count
       @param argv command line arguments
       @return log level parsed from the command line
   */

  static int parse_argv_verbatim(int argc, char** argv) {
    int n=1;
    for(int i=1; i<argc; i++)
      if(strncmp(argv[i],"-PSIM_VERBATIM=",15)==0) {
	sscanf(argv[i]+15,"%i",&n);
	break;
      }
    return n;
  }
};


