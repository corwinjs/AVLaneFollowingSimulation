
#include "TCPClientSocket.h"
#include "Sockets.h"
#include "SocketTypes.h"
#include "SocketSubsystem.h"
#include "Interfaces/IPv4/IPv4Address.h"

TCPClientSocket::TCPClientSocket(int _BufferSize) {
    BufferSize = _BufferSize;
    Buffer = new uint8[BufferSize];
    memset(Buffer, 0, BufferSize);
}

int TCPClientSocket::MakeConnection(FString Address, int Port) {
    // The address used by the lower level SetIP function needs to be in the form of four
    // uint8 values. Most people are more used to seeing it written like "127.0.0.1", so
    // the input to this function is an FString which then needs to be converted to those
    // four uint8 values.
    TArray<FString> AddressStringArray;
    Address.ParseIntoArray(AddressStringArray, TEXT("."), true);
    if (AddressStringArray.Num() != 4) {
        UE_LOG(LogTemp, Warning, TEXT("incorrect ip address format"));;
        return -1;
    }
    TArray<uint32> AddressNumArray;
    for (int i = 0; i < 4; i++) {
        AddressNumArray.Add((uint32)FCString::Atoi(*AddressStringArray[i]));
    }
    FIPv4Address FormattedAddress(AddressNumArray[0], AddressNumArray[1], AddressNumArray[2],
        AddressNumArray[3]);

    // Set up a remote address struct with the provided address and port information
    RemoteAddressStruct = ISocketSubsystem::Get(
        PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
    RemoteAddressStruct->SetIp(FormattedAddress.Value);
    RemoteAddressStruct->SetPort(Port);

    // Initialize the Socket and attempt a connection to the specified server
    Socket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_Stream,
        TEXT("default"), false);
    bool connected = Socket->Connect(*RemoteAddressStruct);
    if (!connected) {
        UE_LOG(LogTemp, Warning, TEXT("failed to connect to server"));
        CloseSocket();
        return -1;
    }

    UE_LOG(LogTemp, Warning, TEXT("successful connection to server"));
    return 0;
}

int TCPClientSocket::SendBytes(uint8* Message, int OutgoingMessageSize) {
    // Don't allow an attempt to send if no connection has been established
    if (!Socket) {
        UE_LOG(LogTemp, Warning, TEXT("cannot send - not connected to server"));
        return -1;
    }

    int BytesSent = 0;
    bool Success = Socket->Send(Message, OutgoingMessageSize, BytesSent);

    if (!Success) {
        UE_LOG(LogTemp, Warning, TEXT("failed to send to server"));
        return -1;
    }

    return BytesSent;
}

int TCPClientSocket::SendString(FString Message) {
    // Convert the FString to a uint8 array and get its size in order to make a call
    // to the SendBytes() function
    TCHAR *MessageChar = Message.GetCharArray().GetData();
    int32 OutgoingMessageSize = FCString::Strlen(MessageChar);
    int BytesSent = SendBytes((uint8*)TCHAR_TO_UTF8(MessageChar), OutgoingMessageSize);

    return BytesSent;
}

int TCPClientSocket::ReadMessageIntoBuffer(bool Blocking) {
    // Don't allow an attempt to read if no connection has been established
    if (!Socket) {
        UE_LOG(LogTemp, Warning, TEXT("cannot read - not connected to server"));
        return -1;
    }

    // If Blocking, wait in an infinite while loop until data is available on the connection
    uint32 DataAvailable = 0;
    Socket->HasPendingData(DataAvailable);
    if (Blocking && !DataAvailable) {
        // UE_LOG(LogTemp, Warning, TEXT("TCP Read Message Blocking - data not available"));
    }
    while (Blocking && !DataAvailable) {
        Socket->HasPendingData(DataAvailable);
    }

    // Read the message into the buffer
    memset(Buffer, 0, BufferSize);
    ESocketReceiveFlags::Type BlockFlag = ESocketReceiveFlags::Type::None;
    bool Success = Socket->Recv((uint8*)Buffer, BufferSize, BytesRead, BlockFlag);

    if (!Success) {
        UE_LOG(LogTemp, Warning, TEXT("failed to receive from server"));
        return -1;
    }

    // Return the length of the message now in the buffer
    return BytesRead;
}

FString TCPClientSocket::GetStringFromBuffer() {
    // Convert the contents of the buffer into an FString to return
    FString BufferString((char*)Buffer);
    return BufferString;
}

uint8* TCPClientSocket::GetBuffer() {
    return Buffer;
}

bool TCPClientSocket::IsConnected() {
    return Socket->GetConnectionState() == ESocketConnectionState::SCS_Connected;
}

void TCPClientSocket::CloseSocket() {
    Socket->Close();
    delete Socket;
    Socket = nullptr;
    RemoteAddressStruct = nullptr;
}

TCPClientSocket::~TCPClientSocket() {
    delete [] Buffer;
    Buffer = NULL;
}
