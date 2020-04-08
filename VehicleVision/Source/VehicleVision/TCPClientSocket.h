#ifndef _TCPCLIENTSOCKET_H_
#define _TCPCLIENTSOCKET_H_

class TCPClientSocket {
 private:
    uint8* Buffer = NULL;
    int BytesRead = 0;  // size of message in input buffer
    int BufferSize;
    FSocket* Socket = nullptr;
    TSharedPtr<FInternetAddr> RemoteAddressStruct = nullptr;

 public:
    explicit TCPClientSocket(int _BufferSize);

    // Attempts to connect to a server already running at a given
    // address and port. Returns 0 on success and -1 if not
    int MakeConnection(FString Address, int Port);

    // Lower level function which sends an array of bytes over
    // the TCP connection
    int SendBytes(uint8* Message, int OutgoingMessageLength);

    // Higher level function which sends an FString over the TCP connection
    int SendString(FString Message);

    // If not blocking, check if there is available input, and if there
    // is then put it into the buffer. If blocking then pause until input
    // is available
    int ReadMessageIntoBuffer(bool Blocking = true);

    // Higher level function that returns the data in the buffer as an FString. Note that
    // this should almost always be called directly after running ReadMessageIntoBuffer() so
    // that the returned FString is the message most recently received
    FString GetStringFromBuffer();

    // Lower level function that returns the data in the buffer as a raw uint8 array. Note that
    // this should almost always be called directly after running ReadMessageIntoBuffer() so
    // that the returned meesage is the message most recently received. This pointer or array
    // should be cast to match the type expected from the received message.
    uint8* GetBuffer();

    // Close the connection to the server when it is no longer necessary
    void CloseSocket();

    bool IsConnected();

    // Free up memory
    ~TCPClientSocket();
};

#endif
