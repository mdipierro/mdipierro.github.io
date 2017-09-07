#include "mdp_all.h"


class PSIM {

  private:

    // Typed Constants
    const static int PROCESS_COUNT_MIN    = 1;      // minimum number of processes
    const static int PROCESS_COUNT_MAX    = 128;    // maximum number of processes
    const static int CONN_LIST_IGNORE     = -1;     // indicators for connections that cannot occur
    const static int PROCESS_PARENT       = 0;      // The parent process ID number
    const static int COMM_RECEIVE         = 0;      // pipe array indicator for reading
    const static int COMM_SEND            = 1;      // pipe array indicator for writing
    const static int COMM_TIMEOUT_DEFAULT = 86400;  // 1 day default - can be modified via method call

    // common enum values for logging routines
    enum enumBegEnd       { LOG_BEGIN, LOG_END };
    enum enumSendRecv     { LOG_SR_SEND, LOG_SR_RECV };
    enum enumSendRecvStep { LOG_SR_START, LOG_SR_FAIL, LOG_SR_SUCCESS };

    // Set this to true if you want verbose testing description
    const static bool DISPLAY_TEST_VALS = false;

    // Class variables
    int     _processCount;          // Holds the number of processes in the simulation
    string  _logFileName;           // filename of the log file
    bool    _doLogging;             // do logging or not?
    FILE*   _logfileFD;             // file descriptor for the logging file
    int     _processID;             // process ID of "this" process

    int   (*_pipeFD)[2];            // array to hold all of the pipes
    int   (*_pipeConnectionList);   // array to hold the pipe connection pointers
    unsigned int _commTimeout;      // defaults to COMM_TIMEOUT_DEFAULT but can be modified

    static map< string, vector<char> >* _hash;  // Hash Map to hold out of sequence (send/receive) data

    /* ******************************************************** */
    /* ***               setupConnectionList()              *** */
    /* ***                                                  *** */
    /* ***  Sets up the array that identifies the pipe to   *** */
    /* ***  be used when one process talks to another.      *** */
    /* ***                                                  *** */
    /* ***  A fully connected graph is created to identify  *** */
    /* ***  which pipes are used for direct communication   *** */
    /* ***  from one process to another.                    *** */
    /* ***                                                  *** */
    /* ******************************************************** */
    void setupConnectionList() {

      int k = 0;

      // Populate the connection list array that identifies the process to process pipe
      for (int i=PROCESS_PARENT; i<_processCount; i++) {
        for (int j=PROCESS_PARENT; j<_processCount; j++) {
          if (i == j)
            _pipeConnectionList[i*_processCount+j] = CONN_LIST_IGNORE;
          else {
            if (j >= i)
              _pipeConnectionList[i*_processCount+j] = k++;
            else
              _pipeConnectionList[i*_processCount+j] =
                      _pipeConnectionList[j*_processCount+i];
          }
        }
      }

      if (DISPLAY_TEST_VALS) {  // Testing purposes only...
        for (int i=PROCESS_PARENT; i<_processCount; i++) {
          for (int j=PROCESS_PARENT; j<_processCount; j++) {
            cout << _pipeConnectionList[i*_processCount+j] << " ";
          }
          cout << endl;
        }
      }
    }


    /* ******************************************************************* */
    /* ***         Private Method: createPipes                         *** */
    /* ***                                                             *** */
    /* ***  Opens all of the pipes necessary for communication and     *** */
    /* ***  inserts them into an array.                                *** */
    /* ***                                                             *** */
    /* ******************************************************************* */

    void createPipes() {

      int pipeCount = (_processCount * (_processCount - 1)) / 2;

      for (int i=0; i<pipeCount; i++) {
        if ( pipe( _pipeFD[i] ) < 0) {
          throw (string("Error creating pipes"));
        }
      }

      if (DISPLAY_TEST_VALS) {  // Testing purposes only...
        for (int i=0; i<pipeCount; i++) {
          cout << i << ") 0: " << _pipeFD[i][COMM_RECEIVE];
          cout << " - 1: " << _pipeFD[i][COMM_SEND] << endl;
        }
      }
    }


    /* ******************************************************************* */
    /* ***         Private Method: getDestIndex                        *** */
    /* ***                                                             *** */
    /* ***  Retrieves the appropriate index for the pipe that          *** */
    /* ***  communicates from the current processID to destProcessID.  *** */
    /* ***                                                             *** */
    /* ******************************************************************* */

    int getDestIndex(int destProcessID) {
      int index = _pipeConnectionList[_processID * _processCount + destProcessID];

      if (index == CONN_LIST_IGNORE)
        throw string("Attempting to send and receive from myself!");

      return index;
    }


    /* ******************************************************************* */
    /* ***         Private Method: checkProcessID                      *** */
    /* ***                                                             *** */
    /* ***  Varifies that the destination process ID is valid. This    *** */
    /* ***  is done before data is sent or received.                   *** */
    /* ***                                                             *** */
    /* ******************************************************************* */

    void checkProcessID(int destProcessID, enumSendRecv methodCall) {
       static const char cmdSendRecv[2][8] = { "send", "receive" };

      if ((destProcessID == _processID) || (destProcessID < PROCESS_PARENT) ||
                (destProcessID >= _processCount)) {

        char buffer[256];
        sprintf(buffer, "Error in %s: Invalid Argument Exception, source: %i - destination: %i.",
                cmdSendRecv[methodCall], _processID, destProcessID);

        throw string( buffer );
      }
    }


    /* ******************************************************************* */
    /* ***         Private Method: deallocateResources                 *** */
    /* ***                                                             *** */
    /* ***  Used by the destructor, this is the method that reclaims   *** */
    /* ***  used resources.                                            *** */
    /* ***                                                             *** */
    /* ******************************************************************* */

    void deallocateResources() {

      int pipeCount = (_processCount * (_processCount - 1)) / 2;

      for (int i=0; i<pipeCount; i++) {
        for (int j=0; j<2; j++) {
          if ( _pipeFD[i][j] > 0 )
            std::close( _pipeFD[i][j] );
        }
      }

      closeLog();

      // only delete the _hash if it still exists
      if (_hash != NULL) {
        delete [] _hash;
        _hash = NULL;
      }

      if (_pipeFD != NULL)
        delete [] _pipeFD;
      _pipeFD = NULL;

      if (_pipeConnectionList != NULL)
        delete [] _pipeConnectionList;
      _pipeConnectionList = NULL;
    }


    /* ******************************************************************* */
    /* ***         Private Method: initialize                          *** */
    /* ***                                                             *** */
    /* ***  Used by the constructor, this method sets up values and    *** */
    /* ***  some of the needed resources.                              *** */
    /* ***                                                             *** */
    /* ******************************************************************* */

    void initialize() {

      int pipeCount;
      int connectionCount;

      _pipeFD = NULL;
      _pipeConnectionList = NULL;

      _commTimeout = COMM_TIMEOUT_DEFAULT;

      if (_hash == NULL)
        _hash = new map< string, vector<char> >[_processCount];

      // calculate the number of pipes necessary for full communication
      // count = (n(n-1))/2 but there needs to be 2 file descriptors per pipe
      pipeCount = _processCount * (_processCount - 1);

      _pipeFD = new int[pipeCount/2][2];

      if (_pipeFD == NULL)
        throw string("Error in initialize: Failed to create pipe array.");

      connectionCount = _processCount * _processCount;

      _pipeConnectionList = new int[connectionCount];

      if (_pipeConnectionList == NULL)
        throw string("Error in initialize: Failed to create pipe connection array.");

      setupConnectionList();

      createPipes();
    }


    /* ******************************************************************* */
    /* ***         Private Method: openLog                             *** */
    /* ***                                                             *** */
    /* ***  This method initializes the process log and sets it up for *** */
    /* ***  appending messages.                                        *** */
    /* ***                                                             *** */
    /* ******************************************************************* */

    void openLog(string logFileName) {

      if (logFileName.length() > 0)
        _logFileName = logFileName;

      _doLogging = true;

      // open and reset file
      if ((_logfileFD = fopen(_logFileName.c_str(), "w")) == NULL)
        throw string("Error Creating Logfile - initialize");

      char buffer[128];
      sprintf(buffer, "START: created %i parallel processes %i", _processCount, _processID);
      log(buffer);

      sprintf(buffer, "Creating process %i - PID: %i.", _processID, getpid());
      log(buffer);

      // close the log file
      closeLog();

      // reopen the log file in append mode
      if ((_logfileFD = fopen(_logFileName.c_str(), "a")) == NULL)
        throw string("Error Creating Logfile - open(append mode)");

      if (DISPLAY_TEST_VALS)
        cout << "Opening logfile: " << _logFileName << " - Process: " << _processID << endl;

    }


    /* ******************************************************************* */
    /* ***         Private Method: closeLog                            *** */
    /* ***                                                             *** */
    /* ***  Closes the log file.                                       *** */
    /* ***                                                             *** */
    /* ******************************************************************* */

    void closeLog() {
      if (_doLogging)
        fclose(_logfileFD);

      if (DISPLAY_TEST_VALS)
        cout << "Log file closed.\n";
    }


    /* ******************************************************************* */
    /* ***         Private Method: logSendRecv                         *** */
    /* ***                                                             *** */
    /* ***  Centralizes the repetitive task of logging the steps       *** */
    /* ***  during send and receive.                                   *** */
    /* ***                                                             *** */
    /* ******************************************************************* */

    void logSendRecv(int destProcessID, enumSendRecv method, enumSendRecvStep step){
      char buffer[256];
      const char cmdSendRecv[2][8] = { "send", "receive" };
      const char stepSendRecv[3][12] = { "starting...", "failed!", "success." };

      sprintf(buffer, "process %i: %s(%i...) %s", _processID, cmdSendRecv[method],
                      destProcessID, stepSendRecv[step]);

      log(buffer);
    }


    /* ******************************************************************* */
    /* ***         Private Method: doSend                              *** */
    /* ***                                                             *** */
    /* ***  Handles the sending of binary data.                        *** */
    /* ***                                                             *** */
    /* ******************************************************************* */

    template<class T>
    void doSend(int destProcessID, T *pdataToSend, size_t dataSize) {

      int destIndex = 0;

      destIndex = getDestIndex(destProcessID);

      if (std::write(_pipeFD[destIndex][COMM_SEND], pdataToSend, dataSize) != dataSize) {
        logSendRecv(destProcessID, LOG_SR_SEND, LOG_SR_FAIL);
        throw string("Error in send: Writing through the pipe failed. errno = " + errno);
      }
    }


    /* ******************************************************************* */
    /* ***         Private Method: binary_send                         *** */
    /* ***                                                             *** */
    /* ***  Sends a data tag and a vector of chars (as binary data).   *** */
    /* ***                                                             *** */
    /* ******************************************************************* */

    void binary_send(int destProcessID, const string& tag, const vector<char>& data) {

      int tagSize = tag.size();
      char charData;
      int charSize = sizeof(char);
      int destIndex = getDestIndex(destProcessID);
      int dataLength = data.size();

      doSend(destProcessID, &tagSize, sizeof(tagSize));

      doSend(destProcessID, tag.c_str(), tag.size());

      doSend(destProcessID, &dataLength, sizeof(dataLength));

      for (int i=0; i<dataLength; i++) {
        charData = data[i];
        doSend(destProcessID, (char*) &charData, charSize);
      }
    }


    /* ******************************************************************* */
    /* ***         Private Method: doReceive                           *** */
    /* ***                                                             *** */
    /* ***  Handles the receiving of binary data through the pipes.    *** */
    /* ***                                                             *** */
    /* ******************************************************************* */

    template<class T>
    void doReceive(int sourceProcessID, T *pdataToReceive, size_t dataSize) {
      int sourceIndex = 0;
      unsigned int readCount = 0;

      sourceIndex = getDestIndex(sourceProcessID);

      // set up blocking read
      do{
        if (std::read(_pipeFD[sourceIndex][COMM_RECEIVE], pdataToReceive, dataSize) == dataSize)
          break;
        sleep(1);
      } while (readCount++ < _commTimeout);

      if (readCount >= _commTimeout) {
        logSendRecv(sourceProcessID, LOG_SR_RECV, LOG_SR_FAIL);
        throw string("Error in receive: Reading the pipe failed. errno = " + errno);
      }
    }


    /* ******************************************************************* */
    /* ***         Private Method: binary_recv                         *** */
    /* ***                                                             *** */
    /* ***  Receives data utilizing a data tag to make sure that the   *** */
    /* ***  data coming in is what was expected.                       *** */
    /* ***                                                             *** */
    /* ******************************************************************* */

    void binary_recv(int sourceProcessID, const string& tag, vector<char>& data ) {

      static vector<char> dataTmp;
      static string tagReceived;
      int size;
      char cdataIn;

      map< string, vector<char> >::iterator itr;

      while (true) {
        itr = _hash[_processID].find(tag);

        if (itr != _hash[_processID].end()) { // Found?
          data = _hash[_processID][tag];
          _hash[_processID].erase(itr);
          break;
        } else {
          doReceive(sourceProcessID, (int*) &size, sizeof(size));
          char buffer[size+1];
          doReceive(sourceProcessID, (char*) &buffer, size);
          buffer[size] = 0;
          tagReceived = buffer;

          if (tagReceived == tag) {
            doReceive(sourceProcessID, (int*) &size, sizeof(size));
            data.resize(size);

            for (int i=0; i<size; i++) {
              doReceive(sourceProcessID, (char*) &cdataIn, sizeof(char));
              data[i] = cdataIn;
            }
            break;

          } else {
            doReceive(sourceProcessID, (int*) &size, sizeof(size));
            dataTmp.resize(size);

            for (int i=0; i<size; i++) {
              doReceive(sourceProcessID, (char*) &cdataIn, sizeof(char));
              dataTmp[i] = cdataIn;
            }

            _hash[_processID][tagReceived] = dataTmp;
          }
        }
      }
    }


    /* ******************************************************************* */
    /* ***         Private Method: doBegEndLog                         *** */
    /* ***                                                             *** */
    /* ***  Centralized log method used by some of the public methods  *** */
    /* ***  to send a standardized message to the log at the beginning *** */
    /* ***  and the end of the routine.                                *** */
    /* ***                                                             *** */
    /* ******************************************************************* */

    void doBegEndLog(string method, enumBegEnd begEnd) {
      char buffer[256];
      char* be;

      if (begEnd == LOG_BEGIN) be = "BEGIN";
        else  be = "END";

      sprintf(buffer, "process %i: %s %s(...)", _processID, be, method.c_str());
      log(buffer);
    }


  public:

    /* ******************************************************************* */
    /* ******************************************************************* */
    /* ***                                                             *** */
    /* ***                P U B L I C   M E T H O D S                  *** */
    /* ***                                                             *** */
    /* ******************************************************************* */
    /* ******************************************************************* */


    /* ******************************************************************* */
    /* ***               Constructor: PSIM                             *** */
    /* ***                                                             *** */
    /* ***  Provide the number of processes to create and the name of  *** */
    /* ***  the logfile if desired and "" if no logfile is needed.     *** */
    /* ***                                                             *** */
    /* ******************************************************************* */

    PSIM(int processCount, string logFileName) {

      if (DISPLAY_TEST_VALS)
        cout << "PSIM Constructor Called\n";

      if ( (processCount < PROCESS_COUNT_MIN) || (processCount < PROCESS_COUNT_MIN) )
        throw string("PSIM: Error initializing - invalid process count.");

      _processCount = processCount;

      _processID = PROCESS_PARENT;

      _doLogging = false;

      openLog(logFileName);

      initialize();

      for (int i=1; i<_processCount; i++) {
        int pid = fork();

        if (pid == -1) // Error
          throw("Error Constructing PSIM: Unable to create all child processes.");
        else if (pid == 0) { // Child Process
          _processID = i;

          char buffer[256];
          sprintf(buffer, "Creating process %i - PID: %i.", _processID, getpid());
          log(buffer);

          if (DISPLAY_TEST_VALS)
            cout << "Child forked - pid: " << getpid() << "  -  _processID: " << _processID << endl;
          break;
        }
      }

      if (_processID == PROCESS_PARENT)
        log("START: done.");
    }



    /* ******************************************************************* */
    /* ***               Destructor: ~PSIM                             *** */
    /* ***                                                             *** */
    /* ***  Deallocates space that was created within the process,     *** */
    /* ***  releases pipes, closes the log, etc.                       *** */
    /* ***                                                             *** */
    /* ******************************************************************* */

    virtual ~PSIM() {
      deallocateResources();

      if (DISPLAY_TEST_VALS)
        cout << "PSIM Destructor Invoked\n";
    }


    /* ******************************************************************* */
    /* ***         Public Method: log                                  *** */
    /* ***                                                             *** */
    /* ***  Accepts a string and appends the message to the common     *** */
    /* ***  log file.  Note: locking is not necessary because of the   *** */
    /* ***  deffinition of append.  It does not matter how many        *** */
    /* ***  processes share file pointers, writing will always occur   *** */
    /* ***  at the end of the file.                                    *** */
    /* ***                                                             *** */
    /* ******************************************************************* */

    void log(string message) {

      int retVal;
      int lockTryCount = 0;

      if (DISPLAY_TEST_VALS)
        cout << "Trying to log message: " << message << " - Process: " << _processID << endl;

      if (_doLogging) {
        fwrite(message.c_str(), message.length(), 1, _logfileFD);
        fwrite("\r\n", 2, 1, _logfileFD);
        // Clear out the file buffer
        fflush(_logfileFD);
      }
    }


    /* ******************************************************************* */
    /* ***         Public Method: getProcessID                         *** */
    /* ***                                                             *** */
    /* ***  Returns an integer identifying which process is currently  *** */
    /* ***  executing.                                                 *** */
    /* ***                                                             *** */
    /* ******************************************************************* */

    int getProcessID() {
      return _processID;
    }


    /* ******************************************************************* */
    /* ***         Public Method: close                                *** */
    /* ***                                                             *** */
    /* ***  Posts the END message to the log.                          *** */
    /* ***                                                             *** */
    /* ******************************************************************* */

    void close() {
      char buffer[256];
      sprintf(buffer, "END: process %i terminating - PID: %i!", _processID, getpid());
      log(buffer);
    }


    /* ******************************************************************* */
    /* ***         Public Method: getNumberOfProcesses                 *** */
    /* ***                                                             *** */
    /* ***  Returns an integer identifying the current number of       *** */
    /* ***  active processes.                                          *** */
    /* ***                                                             *** */
    /* ******************************************************************* */

    int getNumberOfProcesses() {
      return _processCount;
    }


    /* ******************************************************************* */
    /* ***         Public Method: setCommTimeout                       *** */
    /* ***                                                             *** */
    /* ***  Sets the number of seconds that a process will wait to     *** */
    /* ***  receive data from another process before throwing an       *** */
    /* ***  exception.                                                 *** */
    /* ***                                                             *** */
    /* ******************************************************************* */

    void setCommTimeout(unsigned int commTimeout) {
      _commTimeout = commTimeout;
    }


    /* ******************************************************************* */
    /* ***         Public Method: send                                 *** */
    /* ***                                                             *** */
    /* ***  This aynchronous method sends the data referenced bu       *** */
    /* ***  "dataToSend" to "destProcessID".  The size of the data     *** */
    /* ***  is obtained by looking at the type "T".                    *** */
    /* ***                                                             *** */
    /* ******************************************************************* */

    template<class T>
    void send(int destProcessID, string dataTag, vector<char> &dataToSend) {

      try {
        checkProcessID(destProcessID, LOG_SR_SEND);
      } catch (string e) {
        logSendRecv(destProcessID, LOG_SR_SEND, LOG_SR_FAIL);
        throw e;
      }

      logSendRecv(destProcessID, LOG_SR_SEND, LOG_SR_START);

      binary_send(destProcessID, dataTag, dataToSend);

      logSendRecv(destProcessID, LOG_SR_SEND, LOG_SR_SUCCESS);
    }



    /* ******************************************************************* */
    /* ***         Public Method: send                                 *** */
    /* ***                                                             *** */
    /* ***  This aynchronous method sends the data referenced bu       *** */
    /* ***  "dataToSend" to "destProcessID".  The size of the data     *** */
    /* ***  is obtained by looking at the type "T".                    *** */
    /* ***                                                             *** */
    /* ******************************************************************* */

    template<class T>
    void send(int destProcessID, T &dataToSend) {

      int destIndex = 0;
      int dataSize = sizeof(T);

      try {
        checkProcessID(destProcessID, LOG_SR_SEND);
      } catch (string e) {
        logSendRecv(destProcessID, LOG_SR_SEND, LOG_SR_FAIL);
        throw e;
      }

      logSendRecv(destProcessID, LOG_SR_SEND, LOG_SR_START);

      destIndex = getDestIndex(destProcessID);

      if (std::write(_pipeFD[destIndex][COMM_SEND], &dataToSend, dataSize) != dataSize){
        logSendRecv(destProcessID, LOG_SR_SEND, LOG_SR_FAIL);
        throw string("Error in send: Writing through the pipe failed. errno = " + errno);
      }

      logSendRecv(destProcessID, LOG_SR_SEND, LOG_SR_SUCCESS);
    }


    /* ******************************************************************* */
    /* ***         Public Method: send                                 *** */
    /* ***                                                             *** */
    /* ***  This aynchronous method sends the data at location         *** */
    /* ***  "pdataToSend" to "destProcessID".  The size of the data    *** */
    /* ***  being sent is provided in the integer: "dataSize".         *** */
    /* ***                                                             *** */
    /* ******************************************************************* */

    template<class T>
    void send(int destProcessID, T *pdataToSend, size_t dataSize) {

      int destIndex = 0;

      try {
        checkProcessID(destProcessID, LOG_SR_SEND);
      } catch (string e) {
        logSendRecv(destProcessID, LOG_SR_SEND, LOG_SR_FAIL);
        throw e;
      }

      logSendRecv(destProcessID, LOG_SR_SEND, LOG_SR_START);

      destIndex = getDestIndex(destProcessID);

      if (std::write(_pipeFD[destIndex][COMM_SEND], pdataToSend, dataSize) != dataSize) {
        logSendRecv(destProcessID, LOG_SR_SEND, LOG_SR_FAIL);
        throw string("Error in send: Writing through the pipe failed. errno = " + errno);
      }

      logSendRecv(destProcessID, LOG_SR_SEND, LOG_SR_SUCCESS);
    }


    /* ******************************************************************* */
    /* ***         Public Method: receive                              *** */
    /* ***                                                             *** */
    /* ***  This synchronous "blocking" method retrieves the data sent *** */
    /* ***  to "destProcessID".  The size of the data being sent is    *** */
    /* ***  provided in the integer: "dataSize".                       *** */
    /* ***                                                             *** */
    /* ******************************************************************* */

    template<class T>
    void receive(int sourceProcessID, string dataTag, vector<char> &dataToReceive) {

      try {
        checkProcessID(sourceProcessID, LOG_SR_RECV);
      } catch (string e) {
        logSendRecv(sourceProcessID, LOG_SR_RECV, LOG_SR_FAIL);
        throw e;
      }

      logSendRecv(sourceProcessID, LOG_SR_RECV, LOG_SR_START);

      binary_recv(sourceProcessID, dataTag, dataToReceive );

      logSendRecv(sourceProcessID, LOG_SR_RECV, LOG_SR_SUCCESS);
    }


    /* ******************************************************************* */
    /* ***         Public Method: receive                              *** */
    /* ***                                                             *** */
    /* ***  This synchronous "blocking" method retrieves the data sent *** */
    /* ***  to "destProcessID".  The size of the data being sent is    *** */
    /* ***  provided in the integer: "dataSize".                       *** */
    /* ***                                                             *** */
    /* ******************************************************************* */

    template<class T>
    void receive(int sourceProcessID, T &dataToReceive) {

      int sourceIndex = 0;
      int dataSize = sizeof(T);
      unsigned int readCount = 0;

      try {
        checkProcessID(sourceProcessID, LOG_SR_RECV);
      } catch (string e) {
        logSendRecv(sourceProcessID, LOG_SR_RECV, LOG_SR_FAIL);
        throw e;
      }

      logSendRecv(sourceProcessID, LOG_SR_RECV, LOG_SR_START);

      sourceIndex = getDestIndex(sourceProcessID);

      // set up blocking read
      do{
        if (std::read(_pipeFD[sourceIndex][COMM_RECEIVE], &dataToReceive, dataSize) == dataSize)
          break;
        sleep(1);
      } while (readCount++ < _commTimeout);

      if (readCount >= _commTimeout) {
        logSendRecv(sourceProcessID, LOG_SR_RECV, LOG_SR_FAIL);
        throw string("Error in receive (Timeout): Reading the pipe failed. errno = " + errno);
      }

      logSendRecv(sourceProcessID, LOG_SR_RECV, LOG_SR_SUCCESS);
    }


    /* ******************************************************************* */
    /* ***         Public Method: receive                              *** */
    /* ***                                                             *** */
    /* ***  This synchronous "blocking" method retrieves the data sent *** */
    /* ***  to "destProcessID".  "dataSize" bytes are copied to        *** */
    /* ***  location "pdataToReceive".                                 *** */
    /* ***                                                             *** */
    /* ******************************************************************* */

    template<class T>
    void receive(int sourceProcessID, T *pdataToReceive, size_t dataSize) {

      int sourceIndex = 0;
      unsigned int readCount = 0;

      try {
        checkProcessID(sourceProcessID, LOG_SR_RECV);
      } catch (string e) {
        logSendRecv(sourceProcessID, LOG_SR_RECV, LOG_SR_FAIL);
        throw e;
      }

      logSendRecv(sourceProcessID, LOG_SR_RECV, LOG_SR_START);

      sourceIndex = getDestIndex(sourceProcessID);

      // set up blocking read
      do{
        if (std::read(_pipeFD[sourceIndex][COMM_RECEIVE], pdataToReceive, dataSize) == dataSize)
          break;
        sleep(1);
      } while (readCount++ < _commTimeout);

      if (readCount >= _commTimeout) {
        logSendRecv(sourceProcessID, LOG_SR_RECV, LOG_SR_FAIL);
        throw string("Error in receive: Reading the pipe failed. errno = " + errno);
      }

      logSendRecv(sourceProcessID, LOG_SR_RECV, LOG_SR_SUCCESS);
    }


    /* ******************************************************************* */
    /* ***         Public Method: barrier                              *** */
    /* ***                                                             *** */
    /* ***  Initiates a blocking point so that the processes pause     *** */
    /* ***  until ALL processes have reached the barrier.              *** */
    /* ***                                                             *** */
    /* ******************************************************************* */

    void barrier() {
      char* method = "barrier";
      int lenBarrier = strlen(method);
      char buffer[lenBarrier+1];

      doBegEndLog(method, LOG_BEGIN);

      if (_processID != PROCESS_PARENT)
        send(0, method, lenBarrier);
      else {
        for (int i=1; i<_processCount; i++) {
          receive(i, (char*) buffer, lenBarrier);
          buffer[lenBarrier] = 0;
          if (strcmp(method, buffer) != 0)
            throw string("Error in barrier: invalid data received.");
        }
      }

      doBegEndLog(method, LOG_END);
    }


    /* ******************************************************************* */
    /* ***         Public Method: broadcast                            *** */
    /* ***                                                             *** */
    /* ***  Allows broadcasting data to all processes.                 *** */
    /* ***  sourceProcessID sends data (data) to all of the other      *** */
    /* ***  processes who receive the data through the same variable). *** */
    /* ***                                                             *** */
    /* ******************************************************************* */

    template<class T>
    void broadcast(int sourceProcessID, T &data) {

      char* method = "broadcast";
      doBegEndLog(method, LOG_BEGIN);

      if (_processID == sourceProcessID) {
        for (int i=0; i<_processCount; i++) {
          if (i != sourceProcessID)
            send(i, data);
        }
      } else {
        receive(sourceProcessID, data);
      }

      doBegEndLog(method, LOG_END);
    }


    /* ******************************************************************* */
    /* ***         Public Method: list                                 *** */
    /* ***                                                             *** */
    /* *** All parallel processes construct a list of the data passed  *** */
    /* *** by each process.  The list is broadcasted and returned by   *** */
    /* *** each processor.  This method is used to implement global    *** */
    /* *** sum and some other global operations.                       *** */
    /* ***                                                             *** */
    /* ******************************************************************* */

    template<class T>
    vector<T> list(int sourceProcessID, T &data) {

      vector<T> dataList;
      T dataToReceive;

      char* method = "list";
      doBegEndLog(method, LOG_BEGIN);

      if (_processID != PROCESS_PARENT) {
        send(PROCESS_PARENT, data);
      } else {
        dataList.push_back(data);

        for (int i=1; i<_processCount; i++) {
          receive(i, dataToReceive);
          dataList.push_back(dataToReceive);
        }
      }

      broadcast(PROCESS_PARENT, dataList);

      doBegEndLog(method, LOG_END);

      return dataList;
    }


    /* ******************************************************************* */
    /* ***         Public Method: sum                                  *** */
    /* ***                                                             *** */
    /* *** All parallel processes sum their data in parallel.  The sum *** */
    /* *** is returned.                                                *** */
    /* ***                                                             *** */
    /* ******************************************************************* */

    template<class T>
    int sum(int sourceProcessID, vector<T> &dataList) {

      int total = 0;
      char* method = "sum";
      doBegEndLog(method, LOG_BEGIN);

      for (int i=0; i<dataList.size(); i++)
        total += dataList[i];

      doBegEndLog(method, LOG_END);

      return total;
    }

};

/* ******************************************************************* */
/* ***                                                             *** */
/* ***                 Initialize Static Variables                 *** */
/* ***                                                             *** */
/* ******************************************************************* */

map< string, vector<char> >* PSIM::_hash = NULL;

/* ******************************************************************* */
/* ***                                                             *** */
/* ***                         End of PSIM                         *** */
/* ***                                                             *** */
/* ******************************************************************* */




/* ******************************************************************* */
/* ***                                                             *** */
/* ***                         Test: main()                        *** */
/* ***                                                             *** */
/* ******************************************************************* */

int main(int agrc, char** argv){

  PSIM p1(5, "logfile");

  try {
    if (p1.getProcessID() == 0)
      p1.log("This is a message - 0");

    if (p1.getProcessID() == 2)
      p1.log("This is a message - 2");

    if (p1.getProcessID() == 1)
      p1.log("This is a message - 1");

    if (p1.getProcessID() == 3)
      p1.log("This is a message - 3");

    if (p1.getProcessID() == 4)
      p1.log("This is a message - 4");

  } catch (string e) {
    cout << "Exception caught: " << e << endl;
  }

  int a=12345;

  p1.broadcast(2, a);
  cout << a << endl;

  p1.close();

  return 0;
}



