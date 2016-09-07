#ifndef WRAP_TRADER_H
#define WRAP_TRADER_H

#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <fstream>
#include <node.h>
#include "USTPFtdcTraderApi.h"
#include "USTPFtdcUserApiDataType.h"
#include "USTPFtdcUserApiStruct.h"
#include <uv.h>
#include <node_object_wrap.h>
#include "uv_trader.h"

using namespace v8;

extern bool islog;
extern void logger_cout(const char* content);
extern std::string to_string(int val);
extern std::string charto_string(char val);

class WrapTrader : public node::ObjectWrap {
public:
  WrapTrader(void);
  ~WrapTrader(void);

  static void Init(Isolate* isolate);
  static void NewInstance(const FunctionCallbackInfo<Value>& args);
  static void On(const FunctionCallbackInfo<Value>& args);
  static void Connect(const FunctionCallbackInfo<Value>& args);
  static void GetVersion(const FunctionCallbackInfo<Value>& args);

	static void ReqUserLogin(const FunctionCallbackInfo<Value>& args);
	///报单录入请求
	static void ReqOrderInsert(const FunctionCallbackInfo<Value>& args);


private:
  static void initEventMap();
  static void New(const FunctionCallbackInfo<Value>& args);

	static void pkg_cb_userlogin(CbRtnField* data, Local<Value>*cbArray);
	static void pkg_cb_rspinsert(CbRtnField* data, Local<Value>*cbArray);
	static void pkg_cb_errinsert(CbRtnField* data, Local<Value>*cbArray);

  uv_trader* uvTrader;
  static int s_uuid;
	static void FunCallback(CbRtnField *data);
	static void FunRtnCallback(int result, void* baton);
	static Persistent<Function> constructor;
	static std::map<std::string, int> event_map;
	static std::map<int, Persistent<Function> > callback_map;
	static std::map<int, Persistent<Function> > fun_rtncb_map;
};

#endif
