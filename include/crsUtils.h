//----------------------------------------------------------------------------

#ifndef CRSUTILS_HPP
#define CRSUTILS_HPP 1

#include "platformAdapter.h"

#if defined _WIN32
# define SIGUSR1 10 // signals are not supported on Windows
# define SIGUSR2 12 // ensure compilation succeeds 
# define SIGCHLD 17 // (will fail at run-time)
#endif

#if USE_SSL
# include <openssl/ssl.h>
# include <openssl/err.h>
#else
  struct SSL_CTX; // dummy types for prototypes
  struct SSL;
#endif

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <array>
#include <vector>
#include <map>
#include <algorithm>
#include <numeric>
#include <functional>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace crs {

//----------------------------------------------------------------------------
// Various utilities: input/output, text manipulation...
//----------------------------------------------------------------------------

bool // success
parseUri(const std::string &uri,
         std::string &out_protocol,
         std::string &out_hostname,
         uint16_t &out_portNumber,
         std::string &out_resource,
         std::string &out_params);

bool // success
parseUri(const std::string &uri,
         std::string &out_protocol,
         std::string &out_hostname,
         uint16_t &out_portNumber,
         std::string &out_resource);

bool // success
parseProxyUri(const std::string &uri,
              std::string &out_protocol,
              std::string &out_hostname,
              uint16_t &out_portNumber);

template<typename Container>
int // number of elements in c (as an int, whatever c::size_type is)
len(const Container &c);

template<typename Container>
int // position of e in c or -1
find(const Container &c,
     const typename Container::value_type &e);

int // position of searched in s or -1
find(const std::string &s,
     const std::string &searched);

bool // success
startsWith(const std::string &s,
           const std::string &prefix);

bool // success
endsWith(const std::string &s,
         const std::string &suffix);

std::string // s without leading/trailing separators
strip(const std::string &s);

template<typename First,
         typename ...Args>
std::ostream &
txt(std::ostream &output,
    const char *format,
    First first,
    Args &&...args) noexcept;

template<typename ...Args>
std::string
txt(const char *format,
    Args &&...args) noexcept;

template<typename ...Args>
int // written bytes
out(const char *format,
    Args &&...args) noexcept;

template<typename ...Args>
int // written bytes
out(const std::string &format,
    Args &&...args) noexcept;

template<typename ...Args>
int // written bytes
err(const char *format,
    Args &&...args) noexcept;

template<typename ...Args>
int // written bytes
err(const std::string &format,
    Args &&...args) noexcept;

template<typename ...Args>
int // extraction count
extract(std::istream &input,
        Args &&...args) noexcept;

template<typename ...Args>
int // extraction count
extract(const std::string &input,
        Args &&...args) noexcept;

int // split count
split(const std::string &str,
      const char *charSet,
      std::function<void(int,int)> fnct,
      bool keepEmpty=false,
      int maxSplitCount=-1) noexcept;

std::vector<std::string> // split str
split(const std::string &str,
      const char *charSet,
      bool keepEmpty=false,
      int maxSplitCount=-1) noexcept;

int // split count
split(const std::string &str,
      std::function<void(int,int)> fnct,
      bool keepEmpty=false,
      int maxSplitCount=-1) noexcept;

std::vector<std::string> // split str
split(const std::string &str,
      bool keepEmpty=false,
      int maxSplitCount=-1) noexcept;

int // split count
split_on_word(const std::string &str,
              const std::string &word,
              std::function<void(int,int)> fnct,
              bool keepEmpty=true,
              int maxSplitCount=-1) noexcept;

std::vector<std::string> // split str
split_on_word(const std::string &str,
              const std::string &word,
              bool keepEmpty=true,
              int maxSplitCount=-1) noexcept;

template<typename Container>
std::string & // inout_result
join(std::string &inout_result,
     const Container &strings,
     std::function<void()> fnct) noexcept;

template<typename Container>
std::string // joined strings
join(const Container &strings,
     const std::string &separator={}) noexcept;

//----------------------------------------------------------------------------
// Usual system-programming operations
//----------------------------------------------------------------------------

std::string // text description of error code
strerror(int errorCode);

std::string // value of environment variable or "" (not set)
getenv(const std::string &name);

void
setenv(const std::string &name,
       const std::string &value={});

double // seconds since 1970/01/01 00:00:00 UTC
gettimeofday();

void
sleep(double seconds);

pid_t // current process identifier
getpid();

pid_t // parent process identifier
getppid();

pid_t // new process identifier (in parent process) or 0 (in child process)
fork();

void
exit(int status);

pid_t // pid of child process or 0 (if nonblocking)
waitpid(pid_t child,
        int &out_status,
        int &out_signal,
        bool nonblocking=false);

pid_t // pid of child process or 0 (if nonblocking)
waitpid(pid_t child,
        bool nonblocking=false);

std::string // text description of signal number
strsignal(int signal);

void
sigaction(int signal,
          std::function<void(int)> action=nullptr);

std::array<int,2>
pipe();

std::array<SOCKET,2>
socketpair(int type);

int // new file-descriptor 
dup(int fd);

int // new file-descriptor (newFd)
dup2(int oldFd,
     int newFd);

void // never returns!
exec(const std::vector<std::string> &commandLine);

template<typename T>
T * // shared memory address
mmap_shared(int elementCount);

void * // shared memory address
mmap_shared(int byteCount);

template<typename T>
void
munmap(T *address,
       int elementCount);

void
munmap(void *address,
       int byteCount);

struct CpuInfo { int pkgId, coreId, cpuId; };

std::vector<CpuInfo> // detected package and core of each CPU in the system
detectCpuInfo(bool enableSmt=true);

void
bindCurrentThreadToCpu(int cpu);

//----------------------------------------------------------------------------
// Usual operations on files and directories
//----------------------------------------------------------------------------

std::vector<std::string> // directory entries (except . and ..)
listDir(const std::string &path);

bool // path exists and conforms to mode
access(const std::string &path,
       int mode);

int // file size or -1 (no file)
fileSize(const std::string &path);

bool // path exists and is a file
isFile(const std::string &path);

bool // path exists and is a directory
isDir(const std::string &path);

bool // path exists and is a named pipe
isFifo(const std::string &path);

void
mkdir(const std::string &path);

void
rmdir(const std::string &path);

void
mkfifo(const std::string &path);

int // read-only file-descriptor
openR(const std::string &path);

int // write-only file-descriptor
openW(const std::string &path,
      bool append=false,
      bool exclusive=false);

int // read-write file-descriptor
openRW(const std::string &path);

void
close(int fd);

void
unlink(const std::string &path);

int // written bytes
write(int fd,
      const void *data,
      int dataSize);

int // written bytes (dataSize expected)
writeAll(int fd,
         const void *data,
         int dataSize);

int // written bytes (msg.size() expected)
write(int fd,
      const std::string &msg);

int // read bytes or 0 (EOF)
read(int fd,
     void *buffer,
     int bufferCapacity);

int // read bytes (bufferCapacity expected) or 0 (EOF)
readAll(int fd,
        void *buffer,
        int bufferCapacity);

std::string // read text or "" (EOF)
read(int fd,
     int capacity);

std::string // read text line or "" (EOF)
readLine(int fd);

//----------------------------------------------------------------------------
// Usual operations on IPv4 sockets
//----------------------------------------------------------------------------

uint32_t // IPv4 address of dotted-decimal text or 0
parseIpv4Address(const std::string &address);

std::string // dotted-decimal text of IPv4 address
formatIpv4Address(uint32_t address);

std::string
gethostname();

uint32_t // IPv4 address of host name
gethostbyname(const std::string &hostName);

SOCKET
socket(int domain,
       int type,
       int protocol);

void
close(SOCKET s);

void
shutdown(SOCKET s,
         int how);

void
setReuseAddrOption(SOCKET s,
                   bool on);

void
setBroadcastOption(SOCKET s,
                   bool on);

void
bind(SOCKET s,
     uint32_t address,
     uint16_t port);

void
bind(SOCKET s,
     uint16_t port);

void
getsockname(SOCKET s,
            uint32_t &out_address,
            uint16_t &out_port);

void
getpeername(SOCKET s,
            uint32_t &out_address,
            uint16_t &out_port);

void
listen(SOCKET s,
       int backlog=10);

SOCKET // dialog socket
accept(SOCKET listenSocket,
       uint32_t &out_address,
       uint16_t &out_port);

SOCKET // dialog socket
accept(SOCKET listenSocket);

void
connect(SOCKET s,
        uint32_t address,
        uint16_t port);

int // sent bytes
send(SOCKET s,
     const void *data,
     int dataSize);

int // sent bytes (dataSize expected)
sendAll(SOCKET s,
        const void *data,
        int dataSize);

int // sent bytes (msg.size() expected)
send(SOCKET s,
     const std::string &msg);

int // sent bytes
sendto(SOCKET s,
       const void *data,
       int dataSize,
       uint32_t address,
       uint16_t port);

int // sent bytes
sendto(SOCKET s,
       const std::string &msg,
       uint32_t address,
       uint16_t port);

int // received bytes or 0 (EOF)
recv(SOCKET s,
     void *buffer,
     int bufferCapacity);

int // received bytes (bufferCapacity expected) or 0 (EOF)
recvAll(SOCKET s,
        void *buffer,
        int bufferCapacity);

std::string // received text or "" (EOF)
recv(SOCKET s,
     int capacity);

std::string // received text line or "" (EOF)
recvLine(SOCKET s);

int // received bytes or 0 (EOF)
recvfrom(SOCKET s,
         void *buffer,
         int bufferCapacity,
         uint32_t &out_address,
         uint16_t &out_port);

std::string // received text or "" (EOF)
recvfrom(SOCKET s,
         int capacity,
         uint32_t &out_address,
         uint16_t &out_port);

int // number of sockets in ready-state
select(std::vector<SOCKET> &inout_readSet,
       std::vector<SOCKET> &inout_writeSet,
       double timeout=-1.0);

int // number of sockets in ready-state
select(std::vector<SOCKET> &inout_readSet,
       double timeout=-1.0);

int16_t // value converted to network byte-order
hton_i16(int16_t hostValue);

int16_t // value converted to host byte-order
ntoh_i16(int16_t networkValue);

uint16_t // value converted to network byte-order
hton_ui16(uint16_t hostValue);

uint16_t // value converted to host byte-order
ntoh_ui16(uint16_t networkValue);

int32_t // value converted to network byte-order
hton_i32(int32_t hostValue);

int32_t // value converted to host byte-order
ntoh_i32(int32_t networkValue);

uint32_t // value converted to network byte-order
hton_ui32(uint32_t hostValue);

uint32_t // value converted to host byte-order
ntoh_ui32(uint32_t networkValue);

int64_t // value converted to network byte-order
hton_i64(int64_t hostValue);

int64_t // value converted to host byte-order
ntoh_i64(int64_t networkValue);

uint64_t // value converted to network byte-order
hton_ui64(uint64_t hostValue);

uint64_t // value converted to host byte-order
ntoh_ui64(uint64_t networkValue);

real32_t // value converted to network byte-order
hton_r32(real32_t hostValue);

real32_t // value converted to host byte-order
ntoh_r32(real32_t networkValue);

real64_t // value converted to network byte-order
hton_r64(real64_t hostValue);

real64_t // value converted to host byte-order
ntoh_r64(real64_t networkValue);

//----------------------------------------------------------------------------
// Usual operations on SSL
//----------------------------------------------------------------------------

SSL_CTX *
sslInit(const std::string &caCertPath={},
        const std::string &certPath={},
        const std::string &keyPath={});

void
sslDestroy(SSL_CTX *ctx);

SSL *
sslConnect(SOCKET s,
           SSL_CTX *ctx,
           const std::string &hostname={});

SSL *
sslAccept(SOCKET s,
          SSL_CTX *ctx);

void
sslClose(SSL *ssl);

int // sent bytes
sslSend(SSL *ssl,
        const void *data,
        int dataSize);

int // sent bytes (dataSize expected)
sslSendAll(SSL *ssl,
           const void *data,
           int dataSize);

int // sent bytes (msg.size() expected)
sslSend(SSL *ssl,
        const std::string &msg);

bool // some bytes are immediately available
sslPending(SSL *ssl);

int // received bytes or 0 (EOF)
sslRecv(SSL *ssl,
        void *buffer,
        int bufferCapacity);

int // received bytes (bufferCapacity expected) or 0 (EOF)
sslRecvAll(SSL *ssl,
           void *buffer,
           int bufferCapacity);

std::string // received text or "" (EOF)
sslRecv(SSL *ssl,
        int capacity);

std::string // received text line or "" (EOF)
sslRecvLine(SSL *ssl);

//----------------------------------------------------------------------------
// Inline implementation details (don't look below!)
//----------------------------------------------------------------------------

template<typename Container>
inline
int // number of elements in c (as an int, whatever c::size_type is)
len(const Container &c)
{
return int(c.size());
}

template<typename Container>
inline
int // position of e in c or -1
find(const Container &c,
     const typename Container::value_type &e)
{
auto i=std::find(std::begin(c),std::end(c),e);
return i==std::end(c) ? -1 : int(i-std::begin(c));
}

inline
int // position of searched in s or -1
find(const std::string &s,
     const std::string &searched)
{
auto i=s.find(searched);
return i==std::string::npos ? -1 : int(i);
}

inline
bool // success
startsWith(const std::string &s,
           const std::string &prefix)
{
return (s.size()>=prefix.size())&&
       !s.compare(0,prefix.size(),prefix);
}

inline
bool // success
endsWith(const std::string &s,
         const std::string &suffix)
{
return (s.size()>=suffix.size())&&
       !s.compare(s.size()-suffix.size(),suffix.size(),suffix);
}

inline
std::string // s without leading/trailing separators
strip(const std::string &s)
{
auto notSpace=[](char c){ return !::isspace(c); };
auto b=std::find_if(std::begin(s),std::end(s),notSpace);
auto e=std::find_if(std::rbegin(s),std::rend(s),notSpace).base();
return std::string{b,std::max(b,e)};
}

inline
std::ostream &
txt(std::ostream &output,
    const char *format) noexcept
{
return output << format;
}

template<typename First,
         typename ...Args>
inline
std::ostream &
txt(std::ostream &output,
    const char *format,
    First first,
    Args &&...args) noexcept
{
while(*format)
  {
  if(*format=='%')
    { return txt(output<<first,++format,std::forward<Args>(args)...); }
  output << *format++;
  }
return output;
}

template<typename ...Args>
inline
std::string
txt(const char *format,
    Args &&...args) noexcept
{
std::ostringstream output;
txt(output,format,std::forward<Args>(args)...);
return output.str();
}

template<typename ...Args>
inline
int // written bytes
out(const char *format,
    Args &&...args) noexcept
{
return write(STDOUT_FILENO,txt(format,std::forward<Args>(args)...));
}

template<typename ...Args>
inline
int // written bytes
out(const std::string &format,
    Args &&...args) noexcept
{
return out(format.c_str(),std::forward<Args>(args)...);
}

template<typename ...Args>
inline
int // written bytes
err(const char *format,
    Args &&...args) noexcept
{
return write(STDERR_FILENO,txt(format,std::forward<Args>(args)...));
}

template<typename ...Args>
inline
int // written bytes
err(const std::string &format,
    Args &&...args) noexcept
{
return err(format.c_str(),std::forward<Args>(args)...);
}

inline
void
_extract_arg(std::istream &input,
             int &count,
             const char &literalChar) noexcept
{
if(!input.fail())
  {
  char buffer;
  input >> buffer;
  if(buffer!=literalChar)
    { input.setstate(input.rdstate()|std::ios::failbit); }
  }
if(!input.fail()) { ++count; }
}

template<int N>
void
_extract_arg(std::istream &input,
             int &count,
             const char(&literalString)[N]) noexcept
{
if(!input.fail())
  {
  std::array<char,N-1> buffer;
  input >> buffer[0]; // only first char to skip leading separators
  if(N>2) { input.read(&buffer[1],N-2); } // remaining of literal string
  if(::strncmp(&buffer[0],literalString,N-1))
    { input.setstate(input.rdstate()|std::ios::failbit); }
  }
if(!input.fail()) { ++count; }
}

template<typename T>
inline
void
_extract_arg(std::istream &input,
             int &count,
             T &arg) noexcept
{
if(!input.fail()) { input >> arg; }
if(!input.fail()) { ++count; }
}

struct _variadic_pass { template<typename ...T> _variadic_pass(T...) {} };

template<typename ...Args>
inline
int // extraction count
extract(std::istream &input,
        Args &&...args) noexcept
{
int count{0};
_variadic_pass{(_extract_arg(input,count,std::forward<Args>(args)),1)...};
return count;
}

template<typename ...Args>
inline
int // extraction count
extract(const std::string &input,
        Args &&...args) noexcept
{
std::istringstream inputStream{input};
return extract(inputStream,std::forward<Args>(args)...);
}

inline
int // split count
split(const std::string &str,
      const char *charSet,
      std::function<void(int,int)> fnct,
      bool keepEmpty,
      int maxSplitCount) noexcept
{
using size_type=std::string::size_type;
size_type lastPos{0};
int splitCount{0};
bool stop{false};
do
  {
  size_type pos{splitCount==maxSplitCount
                ? std::string::npos
                : str.find_first_of(charSet,lastPos)};
  if(pos==std::string::npos)
    { pos=str.size(); stop=true; }
  if(keepEmpty||(pos!=lastPos))
    {
    ++splitCount;
    fnct(int(lastPos),int(pos));
    }
  lastPos=pos+1;
  } while(!stop);
return splitCount;
}

inline
std::vector<std::string> // split str
split(const std::string &str,
      const char *charSet,
      bool keepEmpty,
      int maxSplitCount) noexcept
{
std::vector<std::string> result;
split(str,charSet,
  [&str,&result](int begin,int end)
    { result.emplace_back(str.c_str()+begin,end-begin); },
  keepEmpty,maxSplitCount);
return result;
}

inline
int // split count
split(const std::string &str,
      std::function<void(int,int)> fnct,
      bool keepEmpty,
      int maxSplitCount) noexcept
{
return split(str," \t\r\n",fnct,keepEmpty,maxSplitCount);
}

inline
std::vector<std::string> // split str
split(const std::string &str,
      bool keepEmpty,
      int maxSplitCount) noexcept
{
std::vector<std::string> result;
split(str,
  [&str,&result](int begin,int end)
    { result.emplace_back(str.c_str()+begin,end-begin); },
  keepEmpty,maxSplitCount);
return result;
}

inline
int // split count
split_on_word(const std::string &str,
              const std::string &word,
              std::function<void(int,int)> fnct,
              bool keepEmpty,
              int maxSplitCount) noexcept
{
using size_type=std::string::size_type;
const size_type skip{word.size()};
size_type lastPos{0};
int splitCount{0};
bool stop{false};
do
  {
  size_type pos{splitCount==maxSplitCount
                ? std::string::npos
                : str.find(word,lastPos)};
  if(pos==std::string::npos)
    { pos=str.size(); stop=true; }
  if(keepEmpty||(pos!=lastPos))
    {
    ++splitCount;
    fnct(int(lastPos),int(pos));
    }
  lastPos=pos+skip;
  } while(!stop);
return splitCount;
}

inline
std::vector<std::string> // split str
split_on_word(const std::string &str,
              const std::string &word,
              bool keepEmpty,
              int maxSplitCount) noexcept
{
std::vector<std::string> result;
split_on_word(str,word,
  [&str,&result](int begin,int end)
    { result.emplace_back(str.c_str()+begin,end-begin); },
  keepEmpty,maxSplitCount);
return result;
}

template<typename Container>
inline
std::string & // inout_result
join(std::string &inout_result,
     const Container &strings,
     std::function<void()> fnct) noexcept
{
bool first{true};
for(const auto &s : strings)
  {
  if(!first) { fnct(); }
  first=false;
  inout_result+=s;
  }
return inout_result;
}

template<typename Container>
inline
std::string // joined strings
join(const Container &strings,
     const std::string &separator) noexcept
{
std::string result;
join(result,strings,[&result,&separator](){ result+=separator; });
return result;
}

template<typename T>
inline
T * // shared memory address
mmap_shared(int elementCount)
{
return static_cast<T *>(mmap_shared(int(elementCount*sizeof(T))));
}

template<typename T>
inline
void
munmap(T *address,
       int elementCount)
{
munmap(static_cast<void *>(address),int(elementCount*sizeof(T)));
}

} // namespace crs

#endif // CRSUTILS_HPP

//----------------------------------------------------------------------------
