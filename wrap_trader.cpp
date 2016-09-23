#include <node.h>
#include "wrap_trader.h"
#include <cstring>
#include <nan.h>

Persistent <Function> WrapTrader::constructor;
int WrapTrader::s_uuid;
std::map<std::string, int> WrapTrader::event_map;
std::map<int, Persistent<Function> > WrapTrader::callback_map;
std::map<int, Persistent<Function> > WrapTrader::fun_rtncb_map;


void logger_cout(const char *content) {
    using namespace std;
    if (islog) {
        cout << content << endl;
    }
}

WrapTrader::WrapTrader(void) {
    logger_cout("wrap_trader------>object start init");
    uvTrader = new uv_trader();
    logger_cout("wrap_trader------>object init successed");
}

WrapTrader::~WrapTrader(void) {
    if (uvTrader) {
        delete uvTrader;
    }
    logger_cout("wrap_trader------>object destroyed");
}

void WrapTrader::Init(Isolate *isolate) {
    // Prepare constructor template
    Local <FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
    tpl->SetClassName(String::NewFromUtf8(isolate, "WrapTrader"));
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    // Prototype
    NODE_SET_PROTOTYPE_METHOD(tpl, "getVersion", GetVersion);
    NODE_SET_PROTOTYPE_METHOD(tpl, "connect", Connect);
    NODE_SET_PROTOTYPE_METHOD(tpl, "on", On);
    NODE_SET_PROTOTYPE_METHOD(tpl, "reqUserLogin", ReqUserLogin);
    NODE_SET_PROTOTYPE_METHOD(tpl, "reqOrderInsert", ReqOrderInsert);

    constructor.Reset(isolate, tpl->GetFunction());
}

void WrapTrader::New(const FunctionCallbackInfo <Value> &args) {
    Isolate *isolate = args.GetIsolate();

    if (event_map.size() == 0)
        initEventMap();

    if (args.IsConstructCall()) {
        // Invoked as constructor: `new MyObject(...)`
        WrapTrader *wTrader = new WrapTrader();
        wTrader->Wrap(args.This());
        args.GetReturnValue().Set(args.This());
    } else {
        // Invoked as plain function `MyObject(...)`, turn into construct call.
        const int argc = 1;
        Local <Value> argv[argc] = {Number::New(isolate, 0)};
        Local <Function> cons = Local<Function>::New(isolate, constructor);
        Local <Context> context = isolate->GetCurrentContext();
        Local <Object> instance = cons->NewInstance(context, argc, argv).ToLocalChecked();
        args.GetReturnValue().Set(instance);
    }
}

void WrapTrader::NewInstance(const FunctionCallbackInfo <Value> &args) {
    Isolate *isolate = args.GetIsolate();
    const unsigned argc = 1;
    Local <Value> argv[argc] = {Number::New(isolate, 0)};
    Local <Function> cons = Local<Function>::New(isolate, constructor);
    Local <Context> context = isolate->GetCurrentContext();
    Local <Object> instance = cons->NewInstance(context, argc, argv).ToLocalChecked();
    args.GetReturnValue().Set(instance);
}

void WrapTrader::initEventMap() {
    event_map["connect"] = T_ON_CONNECT;
    event_map["rspUserLogin"] = T_ON_RSPUSERLOGIN;
    event_map["rspInsert"] = T_ON_RSPINSERT;
    event_map["errInsert"] = T_ON_ERRINSERT;
    event_map["rspError"] = T_ON_RSPERROR;


    event_map["apiReady"] = T_ON_API_READY;
    event_map["rspOrderAction"] = T_ON_RSP_ORDER_ACTION;
    event_map["rtnFund"] = T_ON_RTN_FUND;
    event_map["disconnected"] = T_ON_DISCONNECTED;
    event_map["rspUserLogin"] = T_ON_RSPUSERLOGIN;
    event_map["rspUserLogout"] = T_ON_RSPUSERLOGOUT;
    event_map["rspInfoconfirm"] = T_ON_RSPINFOCONFIRM;
    event_map["rspAction"] = T_ON_RSPACTION;
    event_map["errAction"] = T_ON_ERRACTION;
    event_map["rqOrder"] = T_ON_RQORDER;
    event_map["rtnOrder"] = T_ON_RTNORDER;
    event_map["rqTrade"] = T_ON_RQTRADE;
    event_map["rtnTrade"] = T_ON_RTNTRADE;
    event_map["rqInvestorPosition"] = T_ON_RQINVESTORPOSITION;
    event_map["rqInvestorPositionDetail"] = T_ON_RQINVESTORPOSITIONDETAIL;
    event_map["rqTradingAccount"] = T_ON_RQTRADINGACCOUNT;
    event_map["rqInstrument"] = T_ON_RQINSTRUMENT;
    event_map["rqDdpthmarketData"] = T_ON_RQDEPTHMARKETDATA;
    event_map["rqSettlementInfo"] = T_ON_RQSETTLEMENTINFO;
}

void WrapTrader::GetVersion(const FunctionCallbackInfo <Value> &args) {
    Isolate *isolate = args.GetIsolate();

    WrapTrader *wTrader = ObjectWrap::Unwrap<WrapTrader>(args.Holder());
    const char *apiVersion = wTrader->uvTrader->GetVersion();
    logger_cout("result:");
    logger_cout(apiVersion);

    args.GetReturnValue().Set(String::NewFromUtf8(isolate, apiVersion));
}

void WrapTrader::On(const FunctionCallbackInfo <Value> &args) {
    Isolate *isolate = args.GetIsolate();

    if (args[0]->IsUndefined() || args[1]->IsUndefined()) {
        logger_cout("Wrong arguments->event name or function");
        isolate->ThrowException(
                Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments->event name or function")));
        return;
    }

    WrapTrader *obj = ObjectWrap::Unwrap<WrapTrader>(args.Holder());

    Local <String> eventName = args[0]->ToString();
    Local <Function> cb = Local<Function>::Cast(args[1]);

    String::Utf8Value eNameAscii(eventName);

    std::map<std::string, int>::iterator eIt = event_map.find((std::string) * eNameAscii);
    if (eIt == event_map.end()) {
        logger_cout("System has not register this event");
        isolate->ThrowException(
                Exception::TypeError(String::NewFromUtf8(isolate, "System has no register this event")));
        return;
    }

    std::map<int, Persistent <Function>>::iterator cIt = callback_map.find(eIt->second);
    if (cIt != callback_map.end()) {
        logger_cout("Callback is defined before");
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Callback is defined before")));
        return;
    }

    callback_map[eIt->second].Reset(isolate, cb);
    obj->uvTrader->On(*eNameAscii, eIt->second, FunCallback);
    return args.GetReturnValue().Set(String::NewFromUtf8(isolate, "finish exec on"));
}

void WrapTrader::Connect(const FunctionCallbackInfo <Value> &args) {
    Isolate *isolate = args.GetIsolate();

    if (args[0]->IsUndefined()) {
        logger_cout("Wrong arguments->front addr");
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments->front addr")));
        return;
    }
    if (!args[1]->IsNumber()) {
        logger_cout("Wrong arguments->public or private topic type");
        isolate->ThrowException(
                Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments->public or private topic type")));
        return;
    }
    int uuid = -1;
    WrapTrader *obj = ObjectWrap::Unwrap<WrapTrader>(args.Holder());
    if (!args[2]->IsUndefined() && args[2]->IsFunction()) {
        uuid = ++s_uuid;
        fun_rtncb_map[uuid].Reset(isolate, Local<Function>::Cast(args[2]));
        logger_cout(to_string(uuid).append("|uuid").c_str());
    }

    Local <String> frontIp = args[0]->ToString();
    int frontPort = args[1]->Int32Value();
    String::Utf8Value frontIpUtf8(frontIp);

    UVConnectField pConnectField;
    memset(&pConnectField, 0, sizeof(pConnectField));
    strcpy(pConnectField.front_ip, ((std::string) * frontIpUtf8).c_str());
    pConnectField.front_port = frontPort;
    logger_cout(((std::string) * frontIpUtf8).append("|addrUtf8").c_str());
    logger_cout(to_string(frontPort).append("|frontPort").c_str());

    obj->uvTrader->Connect(&pConnectField, FunRtnCallback, uuid);
    return args.GetReturnValue().Set(String::NewFromUtf8(isolate, "finish exec connect"));
}

void WrapTrader::ReqUserLogin(const FunctionCallbackInfo <Value> &args) {
    Isolate *isolate = args.GetIsolate();

    std::string log = "wrap_trader ReqUserLogin------>";
    if (args[0]->IsUndefined() || args[1]->IsUndefined()) {
        std::string _head = std::string(log);
        logger_cout(_head.append(" Wrong arguments").c_str());
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments")));
        return;
    }

    int uuid = -1;
    WrapTrader *obj = ObjectWrap::Unwrap<WrapTrader>(args.Holder());
    if (!args[3]->IsUndefined() && args[3]->IsFunction()) {
        uuid = ++s_uuid;
        fun_rtncb_map[uuid].Reset(isolate, Local<Function>::Cast(args[3]));
        std::string _head = std::string(log);
        logger_cout(_head.append(" uuid is ").append(to_string(uuid)).c_str());
    }

    Local <String> userNo = args[0]->ToString();
    Local <String> pwd = args[1]->ToString();
    Local <String> brokerId = args[2]->ToString();
    String::Utf8Value userNoUtf8(userNo);
    String::Utf8Value pwdUtf8(pwd);
    String::Utf8Value brokerIdUtf8(brokerId);

    char* g_pProductInfo="XZ_TX";
    CUstpFtdcReqUserLoginField reqUserLogin;
    memset(&reqUserLogin,0,sizeof(CUstpFtdcReqUserLoginField));
    strcpy(reqUserLogin.BrokerID, ((std::string) * brokerIdUtf8).c_str());
    strcpy(reqUserLogin.UserID, ((std::string) * userNoUtf8).c_str());
    strcpy(reqUserLogin.Password, ((std::string) * pwdUtf8).c_str());
    strcpy(reqUserLogin.UserProductInfo,g_pProductInfo);

    logger_cout(log.append(" ").append((std::string) * userNoUtf8).append("|").append((std::string) * pwdUtf8).c_str());
    obj->uvTrader->ReqUserLogin(&reqUserLogin, FunRtnCallback, uuid);
    return args.GetReturnValue().Set(String::NewFromUtf8(isolate, "finish exec reqUserlogin"));
}

void WrapTrader::ReqOrderInsert(const FunctionCallbackInfo <Value> &args) {
    Isolate *isolate = args.GetIsolate();
    std::string log = "wrap_trader ReqOrderInsert------>";

    if (args[0]->IsUndefined() || !args[0]->IsObject()) {
        std::string _head = std::string(log);
        logger_cout(_head.append(" Wrong arguments").c_str());
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments")));
        return;
    }
    int uuid = -1;
    WrapTrader *obj = ObjectWrap::Unwrap<WrapTrader>(args.Holder());
    if (!args[1]->IsUndefined() && args[1]->IsFunction()) {
        uuid = ++s_uuid;
        fun_rtncb_map[uuid].Reset(isolate, Local<Function>::Cast(args[1]));
        std::string _head = std::string(log);
        logger_cout(_head.append(" uuid is ").append(to_string(uuid)).c_str());
    }
    Local <Object> jsonObj = args[0]->ToObject();

    Local <Value> brokerID = jsonObj->Get(v8::String::NewFromUtf8(isolate, "brokerID"));
    if (brokerID->IsUndefined()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments->brokerID")));
        return;
    }
    String::Utf8Value brokerID_(brokerID->ToString());

    Local <Value> exchangeID = jsonObj->Get(v8::String::NewFromUtf8(isolate, "exchangeID"));
    if (exchangeID->IsUndefined()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments->exchangeID")));
        return;
    }
    String::Utf8Value exchangeID_(exchangeID->ToString());

    Local <Value> orderSysID = jsonObj->Get(v8::String::NewFromUtf8(isolate, "orderSysID"));
    if (orderSysID->IsUndefined()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments->orderSysID")));
        return;
    }
    String::Utf8Value orderSysID_(orderSysID->ToString());

    Local <Value> investorID = jsonObj->Get(v8::String::NewFromUtf8(isolate, "investorID"));
    if (investorID->IsUndefined()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments->investorID")));
        return;
    }
    String::Utf8Value investorID_(investorID->ToString());

    Local <Value> userID = jsonObj->Get(v8::String::NewFromUtf8(isolate, "userID"));
    if (userID->IsUndefined()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments->userID")));
        return;
    }
    String::Utf8Value userID_(userID->ToString());

    Local <Value> seatNo = jsonObj->Get(v8::String::NewFromUtf8(isolate, "seatNo"));
    if (seatNo->IsUndefined()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments->seatNo")));
        return;
    }
    double seatNo_ = seatNo->NumberValue();

    Local <Value> instrumentID = jsonObj->Get(v8::String::NewFromUtf8(isolate, "instrumentID"));
    if (instrumentID->IsUndefined()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments->instrumentID")));
        return;
    }
    String::Utf8Value instrumentID_(instrumentID->ToString());

    Local <Value> userOrderLocalID = jsonObj->Get(v8::String::NewFromUtf8(isolate, "userOrderLocalID"));
    if (userOrderLocalID->IsUndefined()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments->userOrderLocalID")));
        return;
    }
    String::Utf8Value userOrderLocalID_(userOrderLocalID->ToString());

    Local <Value> direction = jsonObj->Get(v8::String::NewFromUtf8(isolate, "direction"));
    if (direction->IsUndefined()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments->direction")));
        return;
    }
    String::Utf8Value direction_(direction->ToString());

    Local <Value> gtdDate = jsonObj->Get(v8::String::NewFromUtf8(isolate, "gtdDate"));
    if (gtdDate->IsUndefined()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments->gtdDate")));
        return;
    }
    String::Utf8Value gtdDate_(gtdDate->ToString());

    Local <Value> businessUnit = jsonObj->Get(v8::String::NewFromUtf8(isolate, "businessUnit"));
    if (businessUnit->IsUndefined()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments->businessUnit")));
        return;
    }
    String::Utf8Value businessUnit_(businessUnit->ToString());

    Local <Value> limitPrice = jsonObj->Get(v8::String::NewFromUtf8(isolate, "limitPrice"));
    if (limitPrice->IsUndefined()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments->limitPrice")));
        return;
    }
    double limitPrice_ = limitPrice->NumberValue();

    Local <Value> volume = jsonObj->Get(v8::String::NewFromUtf8(isolate, "volume"));
    if (volume->IsUndefined()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments->volume")));
        return;
    }
    double volume_ = volume->NumberValue();

    CUstpFtdcInputOrderField req;
    memset(&req, 0, sizeof(req));

    strcpy(req.BrokerID, ((std::string) * brokerID_).c_str());
    strcpy(req.ExchangeID, ((std::string) * exchangeID_).c_str());
    strcpy(req.OrderSysID, ((std::string) * orderSysID_).c_str());
    strcpy(req.InvestorID, ((std::string) * investorID_).c_str());
    strcpy(req.UserID, ((std::string) * userID_).c_str());
    strcpy(req.InstrumentID, ((std::string) * instrumentID_).c_str());
    strcpy(req.UserOrderLocalID, ((std::string) * userOrderLocalID_).c_str());

    std::string s("0");
    req.Direction = s == (std::string) * direction_ ? USTP_FTDC_D_Buy : USTP_FTDC_D_Sell;
    std::cout << "----Direction:" << req.Direction << std::endl;

    strcpy(req.GTDDate, ((std::string) * gtdDate_).c_str());
    strcpy(req.BusinessUnit, ((std::string) * businessUnit_).c_str());
    req.LimitPrice = limitPrice_;
    req.Volume = volume_;
    req.SeatNo = seatNo_;

    req.OrderPriceType = USTP_FTDC_OPT_LimitPrice;
    req.OffsetFlag = USTP_FTDC_OF_Open;
    req.HedgeFlag = USTP_FTDC_CHF_Speculation;
    req.TimeCondition = USTP_FTDC_TC_IOC;
    req.VolumeCondition = USTP_FTDC_VC_AV;
    req.MinVolume = 1;
    req.StopPrice = 0;
    req.ForceCloseReason = USTP_FTDC_FCR_NotForceClose;
    req.IsAutoSuspend = 0;
    strcpy(req.UserCustom, "");

    obj->uvTrader->ReqOrderInsert(&req, FunRtnCallback, uuid);
    return;
}

void WrapTrader::FunCallback(CbRtnField *data) {
    Isolate *isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);

    std::map < int, Persistent < Function >> ::iterator
    cIt = callback_map.find(data->eFlag);
    if (cIt == callback_map.end())
        return;

    switch (data->eFlag) {
        case T_ON_CONNECT: {
            Local <Value> argv[1] = {Undefined(isolate)};
            Local <Function> fn = Local<Function>::New(isolate, cIt->second);
            fn->Call(isolate->GetCurrentContext()->Global(), 1, argv);
            break;
        }
        case T_ON_RSPUSERLOGIN: {
            Local <Value> argv[2];
            pkg_cb_userlogin(data, argv);
            Local <Function> fn = Local<Function>::New(isolate, cIt->second);
            fn->Call(isolate->GetCurrentContext()->Global(), 2, argv);
            break;
        }
        case T_ON_RSPINSERT: {
            Local <Value> argv[2];
            pkg_cb_rspinsert(data, argv);
            Local <Function> fn = Local<Function>::New(isolate, cIt->second);
            fn->Call(isolate->GetCurrentContext()->Global(), 2, argv);
            break;
        }
        case T_ON_ERRINSERT: {
            Local <Value> argv[2];
            pkg_cb_errinsert(data, argv);
            Local <Function> fn = Local<Function>::New(isolate, cIt->second);
            fn->Call(isolate->GetCurrentContext()->Global(), 2, argv);
            break;
        }
        case T_ON_RSPERROR: {
            Local <Value> argv[2];
            pkg_cb_errinsert(data, argv);
            Local <Function> fn = Local<Function>::New(isolate, cIt->second);
            fn->Call(isolate->GetCurrentContext()->Global(), 2, argv);
            break;
        }
    }
}

void WrapTrader::FunRtnCallback(int result, void *baton) {
    Isolate *isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);

    LookupCtpApiBaton *tmp = static_cast<LookupCtpApiBaton *>(baton);
    if (tmp->uuid != -1) {
        std::map<int, Persistent < Function >> ::iterator it = fun_rtncb_map.find(tmp->uuid);

        const unsigned argc = 2;
        Local <Value> argv[argc] = {Integer::New(isolate, tmp->nResult), Integer::New(isolate, tmp->iRequestID)};

        Local <Function> fn = Local<Function>::New(isolate, it->second);
        fn->Call(isolate->GetCurrentContext()->Global(), argc, argv);
        it->second.Reset();
        fun_rtncb_map.erase(tmp->uuid);
    }
}

void WrapTrader::pkg_cb_userlogin(CbRtnField *data, Local <Value> *cbArray) {
    Isolate *isolate = Isolate::GetCurrent();

    *cbArray = Local<Value>::New(isolate, Number::New(isolate, 0));
    if (data->rtnField) {
        CUstpFtdcRspUserLoginField *pRspUserLogin = static_cast<CUstpFtdcRspUserLoginField *>(data->rtnField);
        Local <Object> jsonRtn = Object::New(isolate);
        jsonRtn->Set(String::NewFromUtf8(isolate, "TradingDay"),
                     String::NewFromUtf8(isolate, pRspUserLogin->TradingDay));
        jsonRtn->Set(String::NewFromUtf8(isolate, "BrokerID"),
                     String::NewFromUtf8(isolate, pRspUserLogin->BrokerID));
        jsonRtn->Set(String::NewFromUtf8(isolate, "UserID"),
                     String::NewFromUtf8(isolate, pRspUserLogin->UserID));
        jsonRtn->Set(String::NewFromUtf8(isolate, "MaxOrderLocalID"),
                     String::NewFromUtf8(isolate, pRspUserLogin->MaxOrderLocalID));
        jsonRtn->Set(String::NewFromUtf8(isolate, "TradingSystemName"),
                     String::NewFromUtf8(isolate, pRspUserLogin->TradingSystemName));
        jsonRtn->Set(String::NewFromUtf8(isolate, "DataCenterID"),
                     Number::New(isolate, pRspUserLogin->DataCenterID));

        *(cbArray + 1) = jsonRtn;
    } else {
        *(cbArray + 1) = Local<Value>::New(isolate, Undefined(isolate));
    }
    return;
}

void WrapTrader::pkg_cb_rspinsert(CbRtnField *data, Local <Value> *cbArray) {
    Isolate *isolate = Isolate::GetCurrent();

    *cbArray = Local<Value>::New(isolate, Number::New(isolate, 0));
    if (data->rtnField) {
        CUstpFtdcRspInfoField *pRspInfo = static_cast<CUstpFtdcRspInfoField *>(data->rtnField);
        Local <Object> jsonRtn = Object::New(isolate);
        jsonRtn->Set(String::NewFromUtf8(isolate, "ErrorID"),
                     Number::New(isolate, pRspInfo->ErrorID));
        Local <Value> str = Nan::Encode(pRspInfo->ErrorMsg, strlen(pRspInfo->ErrorMsg),
                                        Nan::Encoding::BUFFER);
        jsonRtn->Set(String::NewFromUtf8(isolate, "ErrorMsg"), str);

        *(cbArray + 1) = jsonRtn;
    } else {
        *(cbArray + 1) = Local<Value>::New(isolate, Undefined(isolate));
    }
    return;
}

void WrapTrader::pkg_cb_errinsert(CbRtnField *data, Local <Value> *cbArray) {
    Isolate *isolate = Isolate::GetCurrent();

    *cbArray = Local<Value>::New(isolate, Number::New(isolate, data->errorCode));
    if (data->rtnField) {
        CUstpFtdcRspInfoField *pRspInfo = static_cast<CUstpFtdcRspInfoField *>(data->rtnField);
        Local <Object> jsonRtn = Object::New(isolate);
        jsonRtn->Set(String::NewFromUtf8(isolate, "ErrorID"),
                     Number::New(isolate, pRspInfo->ErrorID));
        Local <Value> str = Nan::Encode(pRspInfo->ErrorMsg, strlen(pRspInfo->ErrorMsg),
                                        Nan::Encoding::BUFFER);
        jsonRtn->Set(String::NewFromUtf8(isolate, "ErrorMsg"), str);

        *(cbArray + 1) = jsonRtn;
    } else {
        *(cbArray + 1) = Local<Value>::New(isolate, Undefined(isolate));
    }
    return;
}

