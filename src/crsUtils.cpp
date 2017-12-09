//----------------------------------------------------------------------------

#include "crsUtils.h"
#include <regex>

namespace crs {

#define THROW_SYSTEM_FAILURE(errorCode)                                  \
        do                                                               \
          {                                                              \
          std::string msg{strerror((errorCode))};                        \
          throw std::runtime_error(txt("%:%:%() failure --- %",          \
                                       __FILE__,__LINE__,__func__,msg)); \
          } while(0)

#define THROW_NOT_AVAILABLE(msg)                                         \
        do                                                               \
          {                                                              \
          throw std::runtime_error(txt("%:%:%() not available%",         \
                                       __FILE__,__LINE__,__func__,msg)); \
          } while(0)

#define SSL_ERROR_MSG(fnct) \
        (txt("%: %",#fnct,::ERR_error_string(::ERR_get_error(),nullptr)))

#define THROW_SSL_ERROR(fnct)                                      \
        do                                                         \
          {                                                        \
          throw std::runtime_error(txt("%:%:%() %",                \
                                       __FILE__,__LINE__,__func__, \
                                       SSL_ERROR_MSG(fnct)));      \
          } while(0)

#if !defined _WIN32
  static std::map<int,std::function<void(int)>> _sigaction_data{};
  static std::atomic_flag _sigaction_lock=ATOMIC_FLAG_INIT;
  static std::vector<std::mutex> _ssl_locks;
#endif

static bool _staticInitialisation=([]()
  {
  (void)_staticInitialisation; // avoid ``unused variable'' warning
#if defined _WIN32
  ::SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);
  ::_setmode(STDIN_FILENO,_O_BINARY);
  ::_setmode(STDOUT_FILENO,_O_BINARY);
  ::_setmode(STDERR_FILENO,_O_BINARY);
  WSADATA wsaData;
  ::WSAStartup(MAKEWORD(2,2),&wsaData);
#endif
#if defined _MSC_VER // MSVC++ does not show anything
  std::set_terminate([]()
    {
    err("Uncaught exception!\n");
    // FIXME: fails in MSVC++
    // try { throw; }
    // catch(std::exception &e) { err("%\n",e.what()); }
    std::abort();
    });
#endif
#if !defined _WIN32
  sigaction(SIGPIPE,[](int){}); // avoid spurious termination on IO failure
#endif
  return true;
  })();

template<typename Dst>
inline
int // written bytes (dataSize expected)
_writeAll(Dst dst,
          const void *data,
          int dataSize,
          std::function<int(Dst,const void *,int)> writeFnct)
{
const char *ptr{reinterpret_cast<const char *>(data)};
int remaining{dataSize};
while(remaining)
  {
  int r{writeFnct(dst,ptr,remaining)};
  if(!r) { break; }
  ptr+=r;
  remaining-=r;
  }
return dataSize-remaining;
}

template<typename Dst>
inline
int // written bytes (msg.size() expected)
_write(Dst dst,
       const std::string &msg,
       std::function<int(Dst,const void *,int)> writeAllFnct)
{
return writeAllFnct(dst,msg.c_str(),int(msg.size()));
}

template<typename Src>
inline
int // read bytes (bufferCapacity expected) or 0 (EOF)
_readAll(Src src,
         void *buffer,
         int bufferCapacity,
         std::function<int(Src,void *,int)> readFnct)
{
char *ptr{reinterpret_cast<char *>(buffer)};
int remaining{bufferCapacity};
while(remaining)
  {
  int r{readFnct(src,ptr,remaining)};
  if(!r) { break; }
  ptr+=r;
  remaining-=r;
  }
return bufferCapacity-remaining;
}

template<typename Src>
inline
std::string // read text or "" (EOF)
_read(Src src,
      int capacity,
      std::function<int(Src,void *,int)> readFnct)
{
std::string result(capacity,'\0');
result.resize(readFnct(src,&*std::begin(result),capacity));
return result;
}

template<typename Src>
inline
std::string // read text line or "" (EOF)
_readLine(Src src,
          std::function<int(Src,void *,int)> readFnct)
{
std::string result;
char c;
while(readFnct(src,&c,1)==1)
  {
  result+=c;
  if(c=='\n') { break; }
  }
return result;
}

//----------------------------------------------------------------------------

bool // success
parseUri(const std::string &uri,
         std::string &out_protocol,
         std::string &out_hostname,
         uint16_t &out_portNumber,
         std::string &out_resource,
         std::string &out_params)
{
//              1                2       3 4         5      6   7
std::regex re("^([a-zA-Z0-9]+)://([^:/]+)(:([0-9]+))?([^?]*)([?](.*))?$",
              std::regex::extended);
std::smatch m;
if(std::regex_search(uri,m,re))
  {
  out_protocol=m.str(1);
  out_hostname=m.str(2);
  out_portNumber=out_protocol=="http" ? 80 :
                 out_protocol=="https" ? 443 : 0;
  if((!m[3].matched||(extract(m.str(4),out_portNumber)==1))&&
     out_portNumber)
    {
    out_resource=m.str(5);
    if(out_resource.empty()) { out_resource="/"; }
    out_params=m[6].matched ? m.str(7) : "";
    return true;
    }
  }
out_protocol.clear();
out_hostname.clear();
out_portNumber=0;
out_resource.clear();
out_params.clear();
return false;
}

bool // success
parseUri(const std::string &uri,
         std::string &out_protocol,
         std::string &out_hostname,
         uint16_t &out_portNumber,
         std::string &out_resource)
{
std::string params;
auto result=parseUri(uri,out_protocol,out_hostname,out_portNumber,
                     out_resource,params);
if(!params.empty()) { out_resource+="?"+params; }
return result;
}

bool // success
parseProxyUri(const std::string &uri,
              std::string &out_protocol,
              std::string &out_hostname,
              uint16_t &out_portNumber)
{
std::string resource,params;
if(parseUri(uri,out_protocol,out_hostname,out_portNumber,resource,params)&&
   (resource=="/")&&params.empty())
  { return true; }
out_protocol.clear();
out_hostname.clear();
out_portNumber=0;
return false;
}

//----------------------------------------------------------------------------

std::string // text description of error code
strerror(int errorCode)
{
std::string result;
#if defined _WIN32
  char *err;
  if(::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
                     (LPCVOID)0,errorCode,
                     0,// MAKELANGID(LANG_NEUTRAL,SUBLANG_NEUTRAL),
                     (LPTSTR)&err,4096,nullptr))
    {
    for(int i=int(::strlen(err));i--;)
      {
      if((err[i]!='\r')&&(err[i]!='\n')) { break; }
      err[i]='\0';
      }
    result+=err;
    ::LocalFree((HLOCAL)err);
    }
  else
    {
    result+="???";
    }
#else
  result+=::strerror(errorCode);
#endif
result+=txt(" (%)",errorCode);
return result;
}

std::string // value of environment variable or "" (not set)
getenv(const std::string &name)
{
const char *v{::getenv(name.c_str())};
return std::string{v ? v : ""};
}

void
setenv(const std::string &name,
       const std::string &value)
{
#if defined _WIN32
  (void)name; // avoid ``unused parameter'' warning
  (void)value;
  THROW_NOT_AVAILABLE(" on Windows");
#else
  int r;
  if(value.empty())
    { RESTART_SYSCALL(r,::unsetenv(name.c_str())); }
  else
    { RESTART_SYSCALL(r,::setenv(name.c_str(),value.c_str(),1)); }
  if(r==-1) { THROW_SYSTEM_FAILURE(SYSCALL_ERRNO); }
#endif
}

double // seconds since 1970/01/01 00:00:00 UTC
gettimeofday()
{
#if defined _MSC_VER
  FILETIME ft;
  ::GetSystemTimeAsFileTime(&ft);
  int64_t t{(int64_t(ft.dwHighDateTime)<<32)+int64_t(ft.dwLowDateTime)};
  return 1e-6*((t/10LL)-11644473600000000LL); // 100ns to 1us, 1601 to 1970
#else
  struct timeval tv;
  int r;
  RESTART_SYSCALL(r,::gettimeofday(&tv,nullptr));
  if(r==-1) { THROW_SYSTEM_FAILURE(SYSCALL_ERRNO); }
  return double(tv.tv_sec)+1e-6*double(tv.tv_usec);
#endif
}

void
sleep(double seconds)
{
#if defined _MSC_VER
  ::Sleep(DWORD(1e3*seconds));
#else
  struct timespec ts;
  ts.tv_sec=time_t(seconds);
  ts.tv_nsec=int(1e9*(seconds-double(ts.tv_sec)));
  int r;
  RESTART_SYSCALL(r,::nanosleep(&ts,&ts));
  if(r==-1) { THROW_SYSTEM_FAILURE(SYSCALL_ERRNO); }
#endif
}

pid_t // current process identifier
getpid()
{
#if defined _WIN32
  THROW_NOT_AVAILABLE(" on Windows");
  return -1; // never reached
#else
  pid_t p;
  RESTART_SYSCALL(p,::getpid());
  if(p==-1) { THROW_SYSTEM_FAILURE(SYSCALL_ERRNO); }
  return p;
#endif
}

pid_t // parent process identifier
getppid()
{
#if defined _WIN32
  THROW_NOT_AVAILABLE(" on Windows");
  return -1; // never reached
#else
  pid_t p;
  RESTART_SYSCALL(p,::getppid());
  if(p==-1) { THROW_SYSTEM_FAILURE(SYSCALL_ERRNO); }
  return p;
#endif
}

pid_t // new process identifier (in parent process) or 0 (in child process)
fork()
{
#if defined _WIN32
  THROW_NOT_AVAILABLE(" on Windows");
  return -1; // never reached
#else
  pid_t p;
  RESTART_SYSCALL(p,::fork());
  if(p==-1) { THROW_SYSTEM_FAILURE(SYSCALL_ERRNO); }
  return p;
#endif
}

void
exit(int status)
{
if((status<0)||(status>255))
  {
  throw std::runtime_error(txt("%:%:%() status % not in [0;255]",
                               __FILE__,__LINE__,__func__,status));
  }
::exit(status);
}

pid_t // pid of child process or 0 (if nonblocking)
waitpid(pid_t child,
        int &out_status,
        int &out_signal,
        bool nonblocking)
{
#if defined _WIN32
  (void)child; // avoid ``unused parameter'' warning
  (void)out_status;
  (void)out_signal;
  (void)nonblocking;
  THROW_NOT_AVAILABLE(" on Windows");
  return -1; // never reached
#else
  pid_t p;
  int status;
  RESTART_SYSCALL(p,::waitpid(child,&status,nonblocking ? WNOHANG : 0));
  if((p==-1)&&!nonblocking) { THROW_SYSTEM_FAILURE(SYSCALL_ERRNO); }
  out_status=(p>0)&&WIFEXITED(status) ? WEXITSTATUS(status) : -1;
  out_signal=(p>0)&&WIFSIGNALED(status) ? WTERMSIG(status) : -1;
  return std::max(0,p);
#endif
}

pid_t // pid of child process or 0 (if nonblocking)
waitpid(pid_t child,
        bool nonblocking)
{
int status,signal;
return waitpid(child,status,signal,nonblocking);
}

std::string
strsignal(int signal)
{
#if defined _WIN32
  (void)signal; // avoid ``unused parameter'' warning
  THROW_NOT_AVAILABLE(" on Windows");
  return {}; // never reached
#else
  const char *s{signal==-1 ? "none" : ::strsignal(signal)};
  return std::string{s ? s : "unknown"};
#endif
}

#if !defined _WIN32
  static
  void
  _sigaction_handler(int signal)
  {
  std::function<void(int)> action{nullptr};
  while(_sigaction_lock.test_and_set(std::memory_order_acquire)) { }
  auto it=_sigaction_data.find(signal);
  if(it!=std::end(_sigaction_data)) { action=it->second; }
  _sigaction_lock.clear(std::memory_order_release);
  if(action) { action(signal); }
  }
#endif

void
sigaction(int signal,
          std::function<void(int)> action)
{
#if defined _WIN32
  (void)signal; // avoid ``unused parameter'' warning
  (void)action;
  THROW_NOT_AVAILABLE(" on Windows");
#else
  struct sigaction sa;
  ::memset(&sa,0,sizeof(sa));
  sa.sa_handler=SIG_DFL;
  if(action)
    {
    while(_sigaction_lock.test_and_set(std::memory_order_acquire)) { }
    _sigaction_data[signal]=action;
    _sigaction_lock.clear(std::memory_order_release);
    sa.sa_handler=_sigaction_handler;
    }
  int r;
  RESTART_SYSCALL(r,::sigaction(signal,&sa,nullptr));
  if(r==-1) { THROW_SYSTEM_FAILURE(SYSCALL_ERRNO); }
#endif
}

std::array<int,2>
pipe()
{
#if defined _WIN32
  THROW_NOT_AVAILABLE(" on Windows");
  return {}; // never reached
#else
  std::array<int,2> result;
  int r;
  RESTART_SYSCALL(r,::pipe(&result[0]));
  if(r==-1) { THROW_SYSTEM_FAILURE(SYSCALL_ERRNO); }
  return result;
#endif
}

std::array<SOCKET,2>
socketpair(int type)
{
#if defined _WIN32
  (void)type; // avoid ``unused parameter'' warning
  THROW_NOT_AVAILABLE(" on Windows");
  return {}; // never reached
#else
  std::array<SOCKET,2> result;
  int r;
  RESTART_SYSCALL(r,::socketpair(PF_LOCAL,type,0,&result[0]));
  if(r==-1) { THROW_SYSTEM_FAILURE(SYSCALL_ERRNO); }
  return result;
#endif
}

int // new file-descriptor
dup(int fd)
{
int r;
#if defined _MSC_VER
  RESTART_SYSCALL(r,::_dup(fd));
#else
  RESTART_SYSCALL(r,::dup(fd));
#endif
if(r==-1) { THROW_SYSTEM_FAILURE(SYSCALL_ERRNO); }
return r;
}

int // new file-descriptor (newFd)
dup2(int oldFd,
     int newFd)
{
int r;
#if defined _MSC_VER
  RESTART_SYSCALL(r,::_dup2(oldFd,newFd));
#else
  RESTART_SYSCALL(r,::dup2(oldFd,newFd));
#endif
if(r==-1) { THROW_SYSTEM_FAILURE(SYSCALL_ERRNO); }
return r;
}

void // never returns!
exec(const std::vector<std::string> &commandLine)
{
const int count{len(commandLine)};
std::vector<const char *> args;
for(int i{0};i<count;++i)
  { args.emplace_back(commandLine[i].c_str()); }
args.emplace_back(nullptr);
#if defined _MSC_VER
  ::_execvp(args[0],const_cast<char **>(&args[0]));
#else
  ::execvp(args[0],const_cast<char **>(&args[0]));
#endif
THROW_SYSTEM_FAILURE(SYSCALL_ERRNO);
}

void * // shared memory address
mmap_shared(int byteCount)
{
#if defined _WIN32
  (void)byteCount; // avoid ``unused parameter'' warning
  THROW_NOT_AVAILABLE(" on Windows");
  return nullptr; // never reached
#else
  void *p=::mmap((void *)0,byteCount,PROT_READ|PROT_WRITE,
                 MAP_ANONYMOUS|MAP_SHARED,-1,0);
  if(p==MAP_FAILED) { THROW_SYSTEM_FAILURE(SYSCALL_ERRNO); }
  return p;
#endif
}

void 
munmap(void *address,
       int byteCount)
{
#if defined _WIN32
  (void)address; // avoid ``unused parameter'' warning
  (void)byteCount;
  THROW_NOT_AVAILABLE(" on Windows");
#else
  int r;
  RESTART_SYSCALL(r,::munmap(address,byteCount));
  if(r==-1) { THROW_SYSTEM_FAILURE(SYSCALL_ERRNO); }
#endif
}

std::vector<CpuInfo> // detected package and core of each CPU in the system
detectCpuInfo(bool enableSmt)
{
std::vector<CpuInfo> result;
int cpuCount{int(std::thread::hardware_concurrency())};
#if defined __linux__
  for(int cpuId=0;cpuId<cpuCount;++cpuId)
    {
    auto pfx{txt("/sys/devices/system/cpu/cpu%",cpuId)};
    int coreId{-1},pkgId{-1};
    std::ifstream(txt("%/topology/core_id",pfx)) >> coreId;
    std::ifstream(txt("%/topology/physical_package_id",pfx)) >> pkgId;
    if((coreId<0)||(pkgId<0)) { continue; }
    if(enableSmt||
       std::none_of(cbegin(result),cend(result),
                    [pkgId,coreId](const CpuInfo &c)
                    { return (c.pkgId==pkgId)&&(c.coreId==coreId); }))
      {
      result.push_back({pkgId,coreId,cpuId});
      }
    }
#else
  (void)enableSmt; // avoid ``unused parameter'' warning
#endif
if(result.empty())
  {
  // nothing found, assume one package and one core per CPU
  for(int cpuId=0;cpuId<cpuCount;++cpuId)
    { result.push_back({0,cpuId,cpuId}); }
  }
return result;
}

void
bindCurrentThreadToCpu(int cpuId)
{
#if defined __linux__
  cpu_set_t cpuset; CPU_ZERO(&cpuset); CPU_SET(cpuId,&cpuset);
  int r=::pthread_setaffinity_np(::pthread_self(),sizeof(cpuset),&cpuset);
  if(r!=0) { THROW_SYSTEM_FAILURE(r); }
#elif defined _WIN32
  DWORD_PTR r=::SetThreadAffinityMask(::GetCurrentThread(),
                                      DWORD_PTR(1ULL<<cpuId));
  if(!r) { THROW_SYSTEM_FAILURE(SYSCALL_ERRNO); }
#else
  (void)cpuId; // avoid ``unused parameter'' warning
#endif
}

//----------------------------------------------------------------------------

std::vector<std::string> // directory entries (except . and ..)
listDir(const std::string &path)
{
std::vector<std::string> result;
#if defined _MSC_VER
  std::string pattern{path};
  if(pattern.empty()) { pattern='.'; }
  if(pattern.back()!='\\') { pattern+='\\'; }
  pattern+="*.*";
  WIN32_FIND_DATA findData;
  HANDLE findHandle{::FindFirstFile(pattern.c_str(),&findData)};
  if(findHandle==INVALID_HANDLE_VALUE)
    {
    DWORD err{GetLastError()};
    if(err!=ERROR_FILE_NOT_FOUND) { THROW_SYSTEM_FAILURE(err); }
    }
  else
    {
    do
      {
      if(::strcmp(findData.cFileName,".")&&::strcmp(findData.cFileName,".."))
        { result.emplace_back(findData.cFileName); }
      } while(::FindNextFile(findHandle,&findData));
    ::FindClose(findHandle);
    }
#else
  DIR *d{opendir(path.c_str())};
  if(!d) { THROW_SYSTEM_FAILURE(SYSCALL_ERRNO); }
  for(;;)
    {
    struct dirent *e{::readdir(d)};
    if(!e) { break; }
    if(::strcmp(e->d_name,".")&&::strcmp(e->d_name,".."))
      { result.emplace_back(e->d_name); }
    }
  int r;
  RESTART_SYSCALL(r,::closedir(d));
  if(r==-1) { THROW_SYSTEM_FAILURE(SYSCALL_ERRNO); }
#endif
return result;
}

bool // path exists and conforms to mode
access(const std::string &path,
       int mode)
{
int r;
#if defined _WIN32
  if(mode&X_OK) { mode=(mode&~X_OK)|R_OK; } // substitute X with R
#endif
#if defined _MSC_VER
  RESTART_SYSCALL(r,::_access(path.c_str(),mode));
#else
  RESTART_SYSCALL(r,::access(path.c_str(),mode));
#endif
return r!=-1;
}

int // file size or -1 (no file)
fileSize(const std::string &path)
{
#if defined _MSC_VER
  struct _stat st;
  int r;
  RESTART_SYSCALL(r,::_stat(path.c_str(),&st));
  return (r!=-1) ? int(st.st_size) : -1;
#else
  struct stat st;
  int r;
  RESTART_SYSCALL(r,::stat(path.c_str(),&st));
  return (r!=-1) ? int(st.st_size) : -1;
#endif
}

bool // path exists and is a file
isFile(const std::string &path)
{
#if defined _MSC_VER
  struct _stat st;
  int r;
  RESTART_SYSCALL(r,::_stat(path.c_str(),&st));
  return (r!=-1)&&(st.st_mode&_S_IFREG);
#else
  struct stat st;
  int r;
  RESTART_SYSCALL(r,::stat(path.c_str(),&st));
  return (r!=-1)&&((st.st_mode&S_IFMT)==S_IFREG);
#endif
}

bool // path exists and is a directory
isDir(const std::string &path)
{
#if defined _MSC_VER
  struct _stat st;
  int r;
  RESTART_SYSCALL(r,::_stat(path.c_str(),&st));
  return (r!=-1)&&(st.st_mode&_S_IFDIR);
#else
  struct stat st;
  int r;
# if defined _WIN32 // FIXME: ugly bug with trailing '/' or '\\' in mingw!
    if(!path.empty()&&((path.back()=='/')||(path.back()=='\\')))
      {
      std::string tmp(path,0,path.size()-1);
      while(!tmp.empty()&&((tmp.back()=='/')||(tmp.back()=='\\')))
        { tmp.pop_back(); }
      if(tmp.empty()||tmp.back()==':')
        { tmp.push_back(path.back()); }
      RESTART_SYSCALL(r,::stat(tmp.c_str(),&st));
      }
    else // execute the normal syscall
#endif
  RESTART_SYSCALL(r,::stat(path.c_str(),&st));
  return (r!=-1)&&((st.st_mode&S_IFMT)==S_IFDIR);
#endif
}

bool // path exists and is a named pipe
isFifo(const std::string &path)
{
#if defined _WIN32
  (void)path; // avoid ``unused parameter'' warning
  return false; // does not exist under windows
#else
  struct stat st;
  int r;
  RESTART_SYSCALL(r,::stat(path.c_str(),&st));
  return (r!=-1)&&((st.st_mode&S_IFMT)==S_IFIFO);
#endif
}

void
mkdir(const std::string &path)
{
int r;
#if defined _MSC_VER
  RESTART_SYSCALL(r,::_mkdir(path.c_str()));
#elif defined _WIN32
  RESTART_SYSCALL(r,::mkdir(path.c_str()));
#else
  RESTART_SYSCALL(r,::mkdir(path.c_str(),0777));
#endif
if(r==-1) { THROW_SYSTEM_FAILURE(SYSCALL_ERRNO); }
}

void
rmdir(const std::string &path)
{
int r;
#if defined _MSC_VER
  RESTART_SYSCALL(r,::_rmdir(path.c_str()));
#else
  RESTART_SYSCALL(r,::rmdir(path.c_str()));
#endif
if(r==-1) { THROW_SYSTEM_FAILURE(SYSCALL_ERRNO); }
}

void
mkfifo(const std::string &path)
{
#if defined _WIN32
  (void)path; // avoid ``unused parameter'' warning
  THROW_NOT_AVAILABLE(" on Windows");
#else
  int r;
  RESTART_SYSCALL(r,::mkfifo(path.c_str(),0666));
  if(r==-1) { THROW_SYSTEM_FAILURE(SYSCALL_ERRNO); }
#endif
}

static
int // file-descriptor
_open(const std::string &path,
      int mode,
      int rights=0666)
{
int r;
#if defined _MSC_VER
  RESTART_SYSCALL(r,::_open(path.c_str(),mode,rights));
#else
  RESTART_SYSCALL(r,::open(path.c_str(),mode,rights));
#endif
if(r==-1) { THROW_SYSTEM_FAILURE(SYSCALL_ERRNO); }
return r;
}

int // read-only file-descriptor
openR(const std::string &path)
{
return _open(path,O_RDONLY);
}

int // write-only file-descriptor
openW(const std::string &path,
      bool append,
      bool exclusive)
{
int mode{O_WRONLY|O_CREAT};
mode|=(append?O_APPEND:O_TRUNC);
if(exclusive) { mode|=O_EXCL; }
return _open(path,mode);
}

int // read-write file-descriptor
openRW(const std::string &path)
{
return _open(path,O_RDWR|O_CREAT);
}

void
close(int fd)
{
int r;
#if defined _MSC_VER
  RESTART_SYSCALL(r,::_close(fd));
#else
  RESTART_SYSCALL(r,::close(fd));
#endif
if(r==-1) { THROW_SYSTEM_FAILURE(SYSCALL_ERRNO); }
}

void
unlink(const std::string &path)
{
int r;
#if defined _MSC_VER
  RESTART_SYSCALL(r,::_unlink(path.c_str()));
#else
  RESTART_SYSCALL(r,::unlink(path.c_str()));
#endif
if(r==-1) { THROW_SYSTEM_FAILURE(SYSCALL_ERRNO); }
}

int // written bytes
write(int fd,
      const void *data,
      int dataSize)
{
const char *ptr{reinterpret_cast<const char *>(data)};
int r;
#if defined _MSC_VER
  RESTART_SYSCALL(r,int(::_write(fd,ptr,dataSize)));
#else
  RESTART_SYSCALL(r,int(::write(fd,ptr,dataSize)));
#endif
if(r==-1) { THROW_SYSTEM_FAILURE(SYSCALL_ERRNO); }
return r;
}

int // written bytes (dataSize expected)
writeAll(int fd,
         const void *data,
         int dataSize)
{
return _writeAll<int>(fd,data,dataSize,
       static_cast<int (*)(int,const void *,int)>(crs::write));
}

int // written bytes (msg.size() expected)
write(int fd,
      const std::string &msg)
{
return _write<int>(fd,msg,
       static_cast<int (*)(int,const void *,int)>(crs::writeAll));
}

int // read bytes or 0 (EOF)
read(int fd,
     void *buffer,
     int bufferCapacity)
{
char *ptr{reinterpret_cast<char *>(buffer)};
int r;
#if defined _MSC_VER
  RESTART_SYSCALL(r,int(::_read(fd,ptr,bufferCapacity)));
#else
  RESTART_SYSCALL(r,int(::read(fd,ptr,bufferCapacity)));
#endif
if(r==-1) { THROW_SYSTEM_FAILURE(SYSCALL_ERRNO); }
return r;
}

int // read bytes (bufferCapacity expected) or 0 (EOF)
readAll(int fd,
        void *buffer,
        int bufferCapacity)
{
return _readAll<int>(fd,buffer,bufferCapacity,
       static_cast<int (*)(int,void *,int)>(crs::read));
}

std::string // read text or "" (EOF)
read(int fd,
     int capacity)
{
return _read<int>(fd,capacity,
       static_cast<int (*)(int,void *,int)>(crs::read));
}

std::string // read text line or "" (EOF)
readLine(int fd)
{
return _readLine<int>(fd,
       static_cast<int (*)(int,void *,int)>(crs::read));
}

//----------------------------------------------------------------------------

uint32_t // IPv4 address of dotted-decimal text or 0
parseIpv4Address(const std::string &address)
{
uint32_t b3,b2,b1,b0;
if((::sscanf(address.c_str()," %u.%u.%u.%u ",&b3,&b2,&b1,&b0)!=4)||
   (b3>0x000000FF)||(b2>0x000000FF)||(b1>0x000000FF)||(b0>0x000000FF))
  {
  return 0;
  }
return ((b3<<24)|(b2<<16)|(b1<<8)|(b0<<0));
}

std::string // dotted-decimal text of IPv4 address
formatIpv4Address(uint32_t address)
{
return txt("%.%.%.%",
           (address>>24)&0x000000FF,
           (address>>16)&0x000000FF,
           (address>>8)&0x000000FF,
           (address>>0)&0x000000FF);
}

std::string
gethostname()
{
char buffer[0x100];
int r;
RESTART_SYSCALL(r,::gethostname(buffer,sizeof(buffer)));
if(r==-1) { THROW_SYSTEM_FAILURE(SOCKET_ERRNO); }
return std::string{buffer};
}

uint32_t // IPv4 address of host name
gethostbyname(const std::string &hostName)
{
uint32_t addr{0};
struct hostent *host{::gethostbyname(hostName.c_str())};
if(host)
  { addr=ntohl(*reinterpret_cast<uint32_t *>(host->h_addr)); }
else
  { addr=parseIpv4Address(hostName); } // try dotted-decimal notation
if(!addr)
  {
  throw std::runtime_error(txt("%:%:%() unknown host '%'",
                               __FILE__,__LINE__,__func__,hostName));
  }
return addr;
}

SOCKET
socket(int domain,
       int type,
       int protocol)
{
SOCKET s;
RESTART_SYSCALL(s,::socket(domain,type,protocol));
if(s==INVALID_SOCKET) { THROW_SYSTEM_FAILURE(SOCKET_ERRNO); }
return s;
}

#if defined _WIN32
  // Windows sockets are handled through the SOCKET type which actualy is a
  // ``long long int'', thus the previously defined close(int) function will
  // be used for usual file-descriptors whereas this close(SOCKET) function
  // will be used for sockets.
  void
  close(SOCKET s)
  {
  int r;
  RESTART_SYSCALL(r,::closesocket(s));
  if(r==-1) { THROW_SYSTEM_FAILURE(SOCKET_ERRNO); }
  }
#else
  // anywhere else a socket is simply a file-descriptor thus the
  // previously defined close(int) function will be used in both cases.
#endif

void
shutdown(SOCKET s,
         int how)
{
int r;
RESTART_SYSCALL(r,::shutdown(s,how));
if(r==-1) { THROW_SYSTEM_FAILURE(SOCKET_ERRNO); }
}

void
setReuseAddrOption(SOCKET s,
                   bool on)
{
#if defined _WIN32
  BOOL option=on ? TRUE : FALSE;
#else
  int option=on ? 1 : 0;
#endif
const char *opt{reinterpret_cast<const char *>(&option)};
int r;
RESTART_SYSCALL(r,::setsockopt(s,SOL_SOCKET,SO_REUSEADDR,opt,sizeof(option)));
if(r==-1) { THROW_SYSTEM_FAILURE(SOCKET_ERRNO); }
}

void
setBroadcastOption(SOCKET s,
                   bool on)
{
#if defined _WIN32
  BOOL option=on ? TRUE : FALSE;
#else
  int option=on ? 1 : 0;
#endif
const char *opt{reinterpret_cast<const char *>(&option)};
int r;
RESTART_SYSCALL(r,::setsockopt(s,SOL_SOCKET,SO_BROADCAST,opt,sizeof(option)));
if(r==-1) { THROW_SYSTEM_FAILURE(SOCKET_ERRNO); }
}

void
bind(SOCKET s,
     uint32_t address,
     uint16_t port)
{
struct sockaddr_in addr;
socklen_t addrLen{socklen_t(sizeof(addr))};
::memset(&addr,0,addrLen);
addr.sin_family=AF_INET;
addr.sin_port=htons(port);
addr.sin_addr.s_addr=htonl(address);
const struct sockaddr *sa{reinterpret_cast<const struct sockaddr *>(&addr)};
int r;
RESTART_SYSCALL(r,::bind(s,sa,addrLen));
if(r==-1) { THROW_SYSTEM_FAILURE(SOCKET_ERRNO); }
}

void
bind(SOCKET s,
     uint16_t port)
{
return bind(s,INADDR_ANY,port);
}

void
getsockname(SOCKET s,
            uint32_t &out_address,
            uint16_t &out_port)
{
struct sockaddr_in addr;
socklen_t addrLen{socklen_t(sizeof(addr))};
struct sockaddr *sa{reinterpret_cast<struct sockaddr *>(&addr)};
int r;
RESTART_SYSCALL(r,::getsockname(s,sa,&addrLen));
if(r==-1) { THROW_SYSTEM_FAILURE(SOCKET_ERRNO); }
out_address=ntohl(addr.sin_addr.s_addr);
out_port=ntohs(addr.sin_port);
}

void
getpeername(SOCKET s,
            uint32_t &out_address,
            uint16_t &out_port)
{
struct sockaddr_in addr;
socklen_t addrLen{socklen_t(sizeof(addr))};
struct sockaddr *sa{reinterpret_cast<struct sockaddr *>(&addr)};
int r;
RESTART_SYSCALL(r,::getpeername(s,sa,&addrLen));
if(r==-1) { THROW_SYSTEM_FAILURE(SOCKET_ERRNO); }
out_address=ntohl(addr.sin_addr.s_addr);
out_port=ntohs(addr.sin_port);
}

void
listen(SOCKET s,
       int backlog)
{
int r;
RESTART_SYSCALL(r,::listen(s,backlog));
if(r==-1) { THROW_SYSTEM_FAILURE(SOCKET_ERRNO); }
}

SOCKET // dialog socket
accept(SOCKET listenSocket,
       uint32_t &out_address,
       uint16_t &out_port)
{
struct sockaddr_in addr;
socklen_t addrLen{socklen_t(sizeof(addr))};
struct sockaddr *sa{reinterpret_cast<struct sockaddr *>(&addr)};
SOCKET s;
RESTART_SYSCALL(s,::accept(listenSocket,sa,&addrLen));
if(s==INVALID_SOCKET) { THROW_SYSTEM_FAILURE(SOCKET_ERRNO); }
out_address=ntohl(addr.sin_addr.s_addr);
out_port=ntohs(addr.sin_port);
return s;
}

SOCKET // dialog socket
accept(SOCKET listenSocket)
{
uint32_t address;
uint16_t port;
return accept(listenSocket,address,port);
}

void
connect(SOCKET s,
        uint32_t address,
        uint16_t port)
{
struct sockaddr_in addr;
socklen_t addrLen{socklen_t(sizeof(addr))};
::memset(&addr,0,addrLen);
addr.sin_family=AF_INET;
addr.sin_port=htons(port);
addr.sin_addr.s_addr=htonl(address);
const struct sockaddr *sa{reinterpret_cast<const struct sockaddr *>(&addr)};
int r;
RESTART_SYSCALL(r,::connect(s,sa,addrLen));
if(r==-1) { THROW_SYSTEM_FAILURE(SOCKET_ERRNO); }
}

int // sent bytes
send(SOCKET s,
     const void *data,
     int dataSize)
{
const char *ptr{reinterpret_cast<const char *>(data)};
int r;
RESTART_SYSCALL(r,int(::send(s,ptr,dataSize,0)));
#if defined _WIN32
  if((r==-1)&&(SOCKET_ERRNO==WSAECONNRESET)) { r=0; } // ugly hack!
#endif
if(r==-1) { THROW_SYSTEM_FAILURE(SOCKET_ERRNO); }
return r;
}

int // sent bytes (dataSize expected)
sendAll(SOCKET s,
        const void *data,
        int dataSize)
{
return _writeAll<SOCKET>(s,data,dataSize,
       static_cast<int (*)(SOCKET,const void *,int)>(crs::send));
}

int // sent bytes (msg.size() expected)
send(SOCKET s,
     const std::string &msg)
{
return _write<SOCKET>(s,msg,
       static_cast<int (*)(SOCKET,const void *,int)>(crs::sendAll));
}

int // sent bytes
sendto(SOCKET s,
       const void *data,
       int dataSize,
       uint32_t address,
       uint16_t port)
{
const char *ptr{reinterpret_cast<const char *>(data)};
struct sockaddr_in addr;
socklen_t addrLen{socklen_t(sizeof(addr))};
::memset(&addr,0,addrLen);
addr.sin_family=AF_INET;
addr.sin_port=htons(port);
addr.sin_addr.s_addr=htonl(address);
const struct sockaddr *sa{reinterpret_cast<const struct sockaddr *>(&addr)};
int r;
RESTART_SYSCALL(r,int(::sendto(s,ptr,dataSize,0,sa,addrLen)));
if(r==-1) { THROW_SYSTEM_FAILURE(SOCKET_ERRNO); }
return r;
}

int // sent bytes
sendto(SOCKET s,
       const std::string &msg,
       uint32_t address,
       uint16_t port)
{
return sendto(s,msg.c_str(),int(msg.size()),address,port);
}

int // received bytes or 0 (EOF)
recv(SOCKET s,
     void *buffer,
     int bufferCapacity)
{
char *ptr{reinterpret_cast<char *>(buffer)};
int r;
RESTART_SYSCALL(r,int(::recv(s,ptr,bufferCapacity,0)));
#if defined _WIN32
  if((r==-1)&&(SOCKET_ERRNO==WSAECONNRESET)) { r=0; } // ugly hack!
#endif
if(r==-1) { THROW_SYSTEM_FAILURE(SOCKET_ERRNO); }
return r;
}

int // received bytes (bufferCapacity expected) or 0 (EOF)
recvAll(SOCKET s,
        void *buffer,
        int bufferCapacity)
{
return _readAll<SOCKET>(s,buffer,bufferCapacity,
       static_cast<int (*)(SOCKET,void *,int)>(crs::recv));
}

std::string // received text or "" (EOF)
recv(SOCKET s,
     int capacity)
{
return _read<SOCKET>(s,capacity,
       static_cast<int (*)(SOCKET,void *,int)>(crs::recv));
}

std::string // received text line or "" (EOF)
recvLine(SOCKET s)
{
return _readLine<SOCKET>(s,
       static_cast<int (*)(SOCKET,void *,int)>(crs::recv));
}

int // received bytes or 0 (EOF)
recvfrom(SOCKET s,
         void *buffer,
         int bufferCapacity,
         uint32_t &out_address,
         uint16_t &out_port)
{
char *ptr{reinterpret_cast<char *>(buffer)};
struct sockaddr_in addr;
socklen_t addrLen{socklen_t(sizeof(addr))};
struct sockaddr *sa{reinterpret_cast<struct sockaddr *>(&addr)};
int r;
RESTART_SYSCALL(r,int(::recvfrom(s,ptr,bufferCapacity,0,sa,&addrLen)));
if(r==-1) { THROW_SYSTEM_FAILURE(SOCKET_ERRNO); }
out_address=ntohl(addr.sin_addr.s_addr);
out_port=ntohs(addr.sin_port);
return r;
}

std::string // received text or "" (EOF)
recvfrom(SOCKET s,
         int capacity,
         uint32_t &out_address,
         uint16_t &out_port)
{
std::string result(capacity,'\0');
result.resize(recvfrom(s,&*std::begin(result),capacity,out_address,out_port));
return result;
}

int // number of sockets in ready-state
select(std::vector<SOCKET> &inout_readSet,
       std::vector<SOCKET> &inout_writeSet,
       double timeout)
{
fd_set readSet,writeSet;
FD_ZERO(&readSet);
FD_ZERO(&writeSet);
SOCKET maxHandle=-1;
for(auto s: inout_readSet)
  { FD_SET(s,&readSet); maxHandle=std::max(maxHandle,s); }
for(auto s: inout_writeSet)
  { FD_SET(s,&writeSet); maxHandle=std::max(maxHandle,s); }
struct timeval tv;
if(timeout>=0.0)
  {
#if defined _WIN32
  tv.tv_sec=long(timeout);
  tv.tv_usec=long(1e6*(timeout-double(tv.tv_sec)));
#else
  tv.tv_sec=time_t(timeout);
  tv.tv_usec=int(1e6*(timeout-double(tv.tv_sec)));
#endif
  }
int r;
RESTART_SYSCALL(r,::select((int)maxHandle+1,
                           inout_readSet.empty() ? nullptr : &readSet,
                           inout_writeSet.empty() ? nullptr : &writeSet,
                           nullptr,
                           timeout>=0.0 ? &tv : nullptr));
if(r==-1) { THROW_SYSTEM_FAILURE(SOCKET_ERRNO); }
for(int i{int(inout_readSet.size())};i--;)
  {
  if(!FD_ISSET(inout_readSet[i],&readSet))
    { inout_readSet[i]=inout_readSet.back(); inout_readSet.pop_back(); }
  }
for(int i{int(inout_writeSet.size())};i--;)
  {
  if(!FD_ISSET(inout_writeSet[i],&writeSet))
    { inout_writeSet[i]=inout_writeSet.back(); inout_writeSet.pop_back(); }
  }
return r;
}

int // number of sockets in ready-state
select(std::vector<SOCKET> &inout_readSet,
       double timeout)
{
std::vector<SOCKET> writeSet;
return select(inout_readSet,writeSet,timeout);
}

int16_t // value converted to network byte-order
hton_i16(int16_t hostValue)
{
#if BYTE_ORDER==BIG_ENDIAN
  return hostValue;
#else
  union { uint8_t b[2]; int16_t v; } u;
  u.b[0]=uint8_t((hostValue>>8)&0x00FF);
  u.b[1]=uint8_t((hostValue>>0)&0x00FF);
  return u.v;
#endif
}

int16_t // value converted to host byte-order
ntoh_i16(int16_t networkValue)
{
return hton_i16(networkValue);
}

uint16_t // value converted to network byte-order
hton_ui16(uint16_t hostValue)
{
#if BYTE_ORDER==BIG_ENDIAN
  return hostValue;
#else
  union { uint8_t b[2]; uint16_t v; } u;
  u.b[0]=uint8_t((hostValue>>8)&0x00FF);
  u.b[1]=uint8_t((hostValue>>0)&0x00FF);
  return u.v;
#endif
}

uint16_t // value converted to host byte-order
ntoh_ui16(uint16_t networkValue)
{
return hton_ui16(networkValue);
}

int32_t // value converted to network byte-order
hton_i32(int32_t hostValue)
{
#if BYTE_ORDER==BIG_ENDIAN
  return hostValue;
#else
  union { uint8_t b[4]; int32_t v; } u;
  u.b[0]=uint8_t((hostValue>>24)&0x00FF);
  u.b[1]=uint8_t((hostValue>>16)&0x00FF);
  u.b[2]=uint8_t((hostValue>>8)&0x00FF);
  u.b[3]=uint8_t((hostValue>>0)&0x00FF);
  return u.v;
#endif
}

int32_t // value converted to host byte-order
ntoh_i32(int32_t networkValue)
{
return hton_i32(networkValue);
}

uint32_t // value converted to network byte-order
hton_ui32(uint32_t hostValue)
{
#if BYTE_ORDER==BIG_ENDIAN
  return hostValue;
#else
  union { uint8_t b[4]; uint32_t v; } u;
  u.b[0]=uint8_t((hostValue>>24)&0x00FF);
  u.b[1]=uint8_t((hostValue>>16)&0x00FF);
  u.b[2]=uint8_t((hostValue>>8)&0x00FF);
  u.b[3]=uint8_t((hostValue>>0)&0x00FF);
  return u.v;
#endif
}

uint32_t // value converted to host byte-order
ntoh_ui32(uint32_t networkValue)
{
return hton_ui32(networkValue);
}

int64_t // value converted to network byte-order
hton_i64(int64_t hostValue)
{
#if BYTE_ORDER==BIG_ENDIAN
  return hostValue;
#else
  union { uint8_t b[8]; int64_t v; } u;
  u.b[0]=uint8_t((hostValue>>56)&0x00FF);
  u.b[1]=uint8_t((hostValue>>48)&0x00FF);
  u.b[2]=uint8_t((hostValue>>40)&0x00FF);
  u.b[3]=uint8_t((hostValue>>32)&0x00FF);
  u.b[4]=uint8_t((hostValue>>24)&0x00FF);
  u.b[5]=uint8_t((hostValue>>16)&0x00FF);
  u.b[6]=uint8_t((hostValue>>8)&0x00FF);
  u.b[7]=uint8_t((hostValue>>0)&0x00FF);
  return u.v;
#endif
}

int64_t // value converted to host byte-order
ntoh_i64(int64_t networkValue)
{
return hton_i64(networkValue);
}

uint64_t // value converted to network byte-order
hton_ui64(uint64_t hostValue)
{
#if BYTE_ORDER==BIG_ENDIAN
  return hostValue;
#else
  union { uint8_t b[8]; uint64_t v; } u;
  u.b[0]=uint8_t((hostValue>>56)&0x00FF);
  u.b[1]=uint8_t((hostValue>>48)&0x00FF);
  u.b[2]=uint8_t((hostValue>>40)&0x00FF);
  u.b[3]=uint8_t((hostValue>>32)&0x00FF);
  u.b[4]=uint8_t((hostValue>>24)&0x00FF);
  u.b[5]=uint8_t((hostValue>>16)&0x00FF);
  u.b[6]=uint8_t((hostValue>>8)&0x00FF);
  u.b[7]=uint8_t((hostValue>>0)&0x00FF);
  return u.v;
#endif
}

uint64_t // value converted to host byte-order
ntoh_ui64(uint64_t networkValue)
{
return hton_ui64(networkValue);
}

real32_t // value converted to network byte-order
hton_r32(real32_t hostValue)
{
#if BYTE_ORDER==BIG_ENDIAN
  return hostValue;
#else
  union { real32_t v; uint32_t i; } u1,u2;
  u1.v=hostValue;
  u2.i=hton_ui32(u1.i);
  return u2.v;
#endif
}

real32_t // value converted to host byte-order
ntoh_r32(real32_t networkValue)
{
return hton_r32(networkValue);
}

real64_t // value converted to network byte-order
hton_r64(real64_t hostValue)
{
#if BYTE_ORDER==BIG_ENDIAN
  return hostValue;
#else
  union { real64_t v; uint64_t i; } u1,u2;
  u1.v=hostValue;
  u2.i=hton_ui64(u1.i);
  return u2.v;
#endif
}

real64_t // value converted to host byte-order
ntoh_r64(real64_t networkValue)
{
return hton_r64(networkValue);
}

//----------------------------------------------------------------------------

#if USE_SSL
  static
  void
  _ssl_locking_cb(int mode,
                  int n,
                  const char *file,
                  int line)
  {
  (void)file; // avoid ``unused parameter'' warning
  (void)line;
  if(mode&CRYPTO_LOCK) { _ssl_locks[n].lock(); }
  else { _ssl_locks[n].unlock(); }
  }

  static
  unsigned long
  _ssl_id_cb()
  {
  return (unsigned long)::pthread_self();
  }
#endif

SSL_CTX *
sslInit(const std::string &caCertPath,
        const std::string &certPath,
        const std::string &keyPath)
{
#if !USE_SSL
  (void)caCertPath; // avoid ``unused parameter'' warning
  (void)certPath;
  (void)keyPath;
  THROW_NOT_AVAILABLE("");
  return nullptr; // never reached
#else
  ::SSL_library_init();
  ::SSL_load_error_strings();
  ::OpenSSL_add_all_algorithms();
  SSL_CTX *ctx{::SSL_CTX_new(::SSLv23_method())};
  // provide SSL with the list of known CA
  if(!caCertPath.empty()&&
     !::SSL_CTX_load_verify_locations(ctx,caCertPath.c_str(),nullptr))
    {
    THROW_SSL_ERROR(SSL_CTX_load_verify_locations);
    }
  // provide SSL with a certificate/key pair
  if(!certPath.empty()&&!keyPath.empty())
    {
    ::SSL_CTX_use_certificate_file(ctx,certPath.c_str(),SSL_FILETYPE_PEM);
    ::SSL_CTX_use_PrivateKey_file(ctx,keyPath.c_str(),SSL_FILETYPE_PEM);
    if(!::SSL_CTX_check_private_key(ctx))
      { THROW_SSL_ERROR(SSL_CTX_check_private_key); }
    }
  // provide SSL with multithreading/locking
  (std::vector<std::mutex>(CRYPTO_num_locks())).swap(_ssl_locks); // ugly
  CRYPTO_set_locking_callback(_ssl_locking_cb);
  CRYPTO_set_id_callback(_ssl_id_cb);
  (void)_ssl_locking_cb; // avoid warning with OpenSSL 1.1
  (void)_ssl_id_cb;
  return ctx;
#endif
}

void
sslDestroy(SSL_CTX *ctx)
{
#if !USE_SSL
  (void)ctx; // avoid ``unused parameter'' warning
  THROW_NOT_AVAILABLE("");
#else
  ::SSL_CTX_free(ctx);
  _ssl_locks.clear();
#endif
}

SSL *
sslConnect(SOCKET s,
           SSL_CTX *ctx,
           const std::string &hostname)
{
#if !USE_SSL
  (void)s; // avoid ``unused parameter'' warning
  (void)ctx;
  (void)hostname;
  THROW_NOT_AVAILABLE("");
  return nullptr; // never reached
#else
  SSL *ssl{::SSL_new(ctx)};
  ::SSL_set_mode(ssl,SSL_MODE_AUTO_RETRY);
  ::SSL_set_fd(ssl,s);
  if(::SSL_connect(ssl)!=1)
    { THROW_SSL_ERROR(SSL_connect); }
  if(::SSL_get_verify_result(ssl)!=X509_V_OK)
    { err("!!! Warning !!! peer certificate not trusted\n"); }
  if(!hostname.empty())
    {
    X509 *cert{::SSL_get_peer_certificate(ssl)};
    if(!cert)
      { err("!!! Warning !!! %\n",SSL_ERROR_MSG(SSL_get_peer_certificate)); }
    else
      {
      char commonName[0x100]{""};
      ::X509_NAME_get_text_by_NID(::X509_get_subject_name(cert),
                                  NID_commonName,
                                  commonName,sizeof(commonName));
      if(hostname!=commonName)
        {
        err("!!! Warning !!! Common name '%' != host name '%'\n",
            commonName,hostname);
        }
      ::X509_free(cert);
      }
    }
  return ssl;
#endif
}

SSL *
sslAccept(SOCKET s,
          SSL_CTX *ctx)
{
#if !USE_SSL
  (void)s; // avoid ``unused parameter'' warning
  (void)ctx;
  THROW_NOT_AVAILABLE("");
  return nullptr; // never reached
#else
  SSL *ssl{SSL_new(ctx)};
  ::SSL_set_mode(ssl,SSL_MODE_AUTO_RETRY);
  ::SSL_set_fd(ssl,s);
  if(::SSL_accept(ssl)!=1)
    { THROW_SSL_ERROR(SSL_accept); }
  if(::SSL_get_verify_result(ssl)!=X509_V_OK)
    { err("!!! Warning !!! peer certificate not trusted\n"); }
  return ssl;
#endif
}

void
sslClose(SSL *ssl)
{
#if !USE_SSL
  (void)ssl; // avoid ``unused parameter'' warning
  THROW_NOT_AVAILABLE("");
#else
  ::SSL_free(ssl);
#endif
}

int // sent bytes
sslSend(SSL *ssl,
        const void *data,
        int dataSize)
{
#if !USE_SSL
  (void)ssl; // avoid ``unused parameter'' warning
  (void)data;
  (void)dataSize;
  THROW_NOT_AVAILABLE("");
  return 0; // never reached
#else
  const char *ptr{reinterpret_cast<const char *>(data)};
  int r=::SSL_write(ssl,ptr,dataSize);
  if(r<0) { THROW_SSL_ERROR(SSL_write); }
  return r;
#endif
}

int // sent bytes (dataSize expected)
sslSendAll(SSL *ssl,
           const void *data,
           int dataSize)
{
return _writeAll<SSL *>(ssl,data,dataSize,
       static_cast<int (*)(SSL *,const void *,int)>(crs::sslSend));
}

int // sent bytes (msg.size() expected)
sslSend(SSL *ssl,
        const std::string &msg)
{
return _write<SSL *>(ssl,msg,
       static_cast<int (*)(SSL *,const void *,int)>(crs::sslSendAll));
}

bool // some bytes are immediately available
sslPending(SSL *ssl)
{
#if !USE_SSL
  (void)ssl; // avoid ``unused parameter'' warning
  THROW_NOT_AVAILABLE("");
  return false; // never reached
#else
  return ::SSL_pending(ssl)>0;
#endif
}

int // received bytes or 0 (EOF)
sslRecv(SSL *ssl,
        void *buffer,
        int bufferCapacity)
{
#if !USE_SSL
  (void)ssl; // avoid ``unused parameter'' warning
  (void)buffer;
  (void)bufferCapacity;
  THROW_NOT_AVAILABLE("");
  return 0; // never reached
#else
  char *ptr{reinterpret_cast<char *>(buffer)};
  int r=::SSL_read(ssl,ptr,bufferCapacity);
  if(r<0) { THROW_SSL_ERROR(SSL_read); }
  return r;
#endif
}

int // received bytes (bufferCapacity expected) or 0 (EOF)
sslRecvAll(SSL *ssl,
           void *buffer,
           int bufferCapacity)
{
return _readAll<SSL *>(ssl,buffer,bufferCapacity,
       static_cast<int (*)(SSL *,void *,int)>(crs::sslRecv));
}

std::string // received text or "" (EOF)
sslRecv(SSL *ssl,
        int capacity)
{
return _read<SSL *>(ssl,capacity,
       static_cast<int (*)(SSL *,void *,int)>(crs::sslRecv));
}

std::string // received text line or "" (EOF)
sslRecvLine(SSL *ssl)
{
return _readLine<SSL *>(ssl,
       static_cast<int (*)(SSL *,void *,int)>(crs::sslRecv));
}

} // namespace crs

//----------------------------------------------------------------------------
