#ifndef __PIKA_COMMAND_H__
#define __PIKA_COMMAND_H__

#include <deque>
#include <string>
#include <memory>
#include <redis_conn.h>
#include <unordered_map>
#include "slash_string.h"


//Constant for command name
//Admin
const std::string kCmdNameSlaveof = "slaveof";
const std::string kCmdNameTrysync = "trysync";

//Kv
const std::string kCmdNameSet = "set";
const std::string kCmdNameGet = "get";
const std::string kCmdNameDel = "del";
const std::string kCmdNameIncr = "incr";
const std::string kCmdNameIncrby = "incrby";
const std::string kCmdNameIncrbyfloat = "incrbyfloat";
const std::string kCmdNameDecr = "decr";
const std::string kCmdNameDecrby = "decrby";
const std::string kCmdNameGetset = "getset";
const std::string kCmdNameAppend = "append";
const std::string kCmdNameMget = "mget";
//Hash

//List

//Zset

//Set

typedef pink::RedisCmdArgsType PikaCmdArgsType;

enum CmdFlagsMask {
  kCmdFlagsMaskRW         = 1,
  kCmdFlagsMaskType       = 14,
  kCmdFlagsMaskLocal      = 16,
  kCmdFlagsMaskSuspend    = 32,
  kCmdFlagsMaskPrior      = 64
};

enum CmdFlags {
  kCmdFlagsRead           = 0, //default rw
  kCmdFlagsWrite          = 1,
  kCmdFlagsAdmin          = 0, //default type
  kCmdFlagsKv             = 2,
  kCmdFlagsHash           = 4,
  kCmdFlagsList           = 6,
  kCmdFlagsSet            = 8,
  kCmdFlagsZset           = 10,
  kCmdFlagsNoLocal        = 0, //default nolocal
  kCmdFlagsLocal          = 16,
  kCmdFlagsNoSuspend      = 0, //default nosuspend
  kCmdFlagsSuspend        = 32,
  kCmdFlagsNoPrior        = 0, //default noprior
  kCmdFlagsPrior          = 64
};


class CmdInfo {
public:
  CmdInfo(const std::string _name, int _num, uint16_t _flag)
    : name_(_name), arity_(_num), flag_(_flag) {}
  bool CheckArg(int num) const {
    if ((arity_ > 0 && num != arity_) || (arity_ < 0 && num < -arity_)) {
      return false;
    }
    return true;
  }
  bool is_write() const {
    return ((flag_ & kCmdFlagsMaskRW) == kCmdFlagsWrite);
  }
  uint16_t flag_type() const {
    return flag_ & kCmdFlagsMaskType;
  }
  bool is_local() const {
    return ((flag_ & kCmdFlagsMaskLocal) == kCmdFlagsLocal);
  }
  // Others need to be suspended when a suspend command run
  bool is_suspend() const {
    return ((flag_ & kCmdFlagsMaskSuspend) == kCmdFlagsSuspend);
  }
  bool is_prior() const {
    return ((flag_ & kCmdFlagsMaskPrior) == kCmdFlagsPrior);
  }
  std::string name() const {
    return name_;
  }
private:
  std::string name_;
  int arity_;
  uint16_t flag_;

  CmdInfo(const CmdInfo&);
  CmdInfo& operator=(const CmdInfo&);
};

void inline RedisAppendContent(std::string& str, const std::string& value);
void inline RedisAppendLen(std::string& str, int ori, const std::string &prefix);

const std::string kNewLine = "\r\n";

class CmdRes {
public:
  enum CmdRet {
    kNone = 0,
    kOk,
    kSyntaxErr,
    kOutofRange,
    kWrongNum,
    kErrOther,
  };

  CmdRes():ret_(kNone) {}

  bool ok() const {
    return ret_ == kOk || ret_ == kNone;
  }
  void clear() {
    message_.clear();
    ret_ = kNone;
  }
  std::string message() const {
    std::string result;
    switch (ret_) {
    case kNone:
      return message_;
    case kOk:
      return "+OK\r\n";
    case kSyntaxErr:
      return "-ERR syntax error\r\n";
    case kOutofRange:
      return "-ERR value is not an integer or out of range\r\n";
    case kWrongNum:
      result = "-ERR wrong number of arguments for '";
      result.append(message_);
      result.append("' command\r\n");
      break;
    case kErrOther:
      result = "-ERR ";
      result.append(message_);
      result.append(kNewLine);
      break;
    default:
      break;
    }
    return result;
  }

  // Inline functions for Create Redis protocol
  void AppendStringLen(int ori) {
    RedisAppendLen(message_, ori, "$");
  }
  void AppendArrayLen(int ori) {
    RedisAppendLen(message_, ori, "*");
  }
  void AppendInteger(int ori) {
    RedisAppendLen(message_, ori, ":");
  }
  void AppendContent(const std::string &value) {
    RedisAppendContent(message_, value);
  }
  void SetRes(CmdRet _ret, const std::string content = "") {
    ret_ = _ret;
    if (!content.empty()) {
      message_ = content;
    }
  }

private:
  std::string message_;
  CmdRet ret_;
};

class Cmd {
public:
  Cmd() {}
  virtual ~Cmd() {}
  virtual void Do() = 0;
  void Initial(PikaCmdArgsType &argvs, const CmdInfo* const ptr_info) {
    res_.clear(); // Clear res content
    Clear();      // Clear cmd, Derived class can has own implement
    DoInitial(argvs, ptr_info);
  };
  CmdRes& res() {
    return res_;
  }

protected:
  CmdRes res_;

private:
  virtual void DoInitial(PikaCmdArgsType &argvs, const CmdInfo* const ptr_info) = 0;
  virtual void Clear() {};
  Cmd(const Cmd&);
  Cmd& operator=(const Cmd&);
};

// Method for CmdInfo Table
void InitCmdInfoTable();
const CmdInfo* GetCmdInfo(const std::string& opt);
void DestoryCmdInfoTable();
// Method for Cmd Table
void InitCmdTable(std::unordered_map<std::string, Cmd*> *cmd_table);
Cmd* GetCmdFromTable(const std::string& opt, 
    const std::unordered_map<std::string, Cmd*> &cmd_table);
void DestoryCmdTable(std::unordered_map<std::string, Cmd*> &cmd_table);

void inline RedisAppendContent(std::string& str, const std::string& value) {
  str.append(value.data(), value.size());
  str.append(kNewLine);
}
void inline RedisAppendLen(std::string& str, int ori, const std::string &prefix) {
  char buf[32];
  slash::ll2string(buf, 32, static_cast<long long>(ori));
  str.append(prefix);
  str.append(buf);
  str.append(kNewLine);
}
#define RedisappendInter RedisAppendLen
#endif