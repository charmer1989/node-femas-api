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
    event_map["rspError"] = T_ON_RSPERROR;
}

void WrapTrader::GetVersion(const FunctionCallbackInfo <Value> &args) {
    Isolate *isolate = args.GetIsolate();

    WrapTrader *wTrader = ObjectWrap::Unwrap<WrapTrader>(args.Holder());
    const char *apiVersion = wTrader->uvTrader->GetVersion();
    logger_cout("调用结果:");
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
        fun_rtncb_map[uuid].Reset(isolate, Local<Function>::Cast(args[2]));
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

//    Local <Value> hedgeFlag2 = jsonObj->Get(v8::String::NewFromUtf8(isolate, "hedgeFlag2"));
//    if (hedgeFlag2->IsUndefined()) {
//        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments->hedgeFlag2")));
//        return;
//    }
//    String::Utf8Value hedgeFlag2_(hedgeFlag2->ToString());
//
//    Local <Value> marketLevel = jsonObj->Get(v8::String::NewFromUtf8(isolate, "marketLevel"));
//    if (marketLevel->IsUndefined()) {
//        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments->marketLevel")));
//        return;
//    }
//    double marketLevel_ = marketLevel->NumberValue();
//
//    Local <Value> orderDeleteByDisConnFlag = jsonObj->Get(v8::String::NewFromUtf8(isolate, "orderDeleteByDisConnFlag"));
//    if (orderDeleteByDisConnFlag->IsUndefined()) {
//        isolate->ThrowException(
//                Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments->orderDeleteByDisConnFlag")));
//        return;
//    }
//    String::Utf8Value orderDeleteByDisConnFlag_(orderDeleteByDisConnFlag->ToString());
//
//    TapAPINewOrder req;
//    memset(&req, 0, sizeof(req));
//
//    strcpy(req.AccountNo, ((std::string) * accountNo_).c_str());
//    strcpy(req.ExchangeNo, ((std::string) * exchangeNo_).c_str());
//    req.CommodityType = TAPI_COMMODITY_TYPE_FUTURES;
//    strcpy(req.CommodityNo, ((std::string) * commodityNo_).c_str());
//    strcpy(req.ContractNo, ((std::string) * contractNo_).c_str());
//    std::string s("S");
//    req.OrderSide = s == (std::string) * orderSide_ ? TAPI_SIDE_SELL : TAPI_SIDE_BUY;
//
//    std::string log2 = "strcmp:";
//    logger_cout(log2.append(to_string(s == (std::string) * orderSide_)).c_str());
//
//    strcpy(req.StrikePrice, "");
//    req.CallOrPutFlag = TAPI_CALLPUT_FLAG_NONE;
//    strcpy(req.ContractNo2, "");
//    strcpy(req.StrikePrice2, "");
//    req.CallOrPutFlag2 = TAPI_CALLPUT_FLAG_NONE;
//    req.OrderType = TAPI_ORDER_TYPE_LIMIT;
//    req.OrderSource = TAPI_ORDER_SOURCE_ESUNNY_API;
//    req.TimeInForce = TAPI_ORDER_TIMEINFORCE_FAK;
//    strcpy(req.ExpireTime, "");
//    req.IsRiskOrder = APIYNFLAG_NO;
//    req.PositionEffect = TAPI_PositionEffect_OPEN;
//    req.PositionEffect2 = TAPI_PositionEffect_NONE;
//    strcpy(req.InquiryNo, "");
//    req.HedgeFlag = TAPI_HEDGEFLAG_T;
//    req.OrderPrice = orderPrice_;
//    req.OrderPrice2;
//    req.StopPrice;
//    req.OrderQty = orderQty_;
//    req.OrderMinQty;
//    req.MinClipSize;
//    req.MaxClipSize;
//    req.RefInt;
//    req.RefString;
//    req.TacticsType = TAPI_TACTICS_TYPE_NONE;
//    req.TriggerCondition = TAPI_TRIGGER_CONDITION_NONE;
//    req.TriggerPriceType = TAPI_TRIGGER_PRICE_NONE;
//    req.AddOneIsValid = APIYNFLAG_NO;
//    req.OrderQty2;
//    req.HedgeFlag2 = TAPI_HEDGEFLAG_NONE;
//    req.MarketLevel = TAPI_MARKET_LEVEL_0;
//    req.OrderDeleteByDisConnFlag = APIYNFLAG_NO;
//
//    obj->uvTrader->ReqOrderInsert(&req, FunRtnCallback, uuid);
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
    }
}

void WrapTrader::FunRtnCallback(int result, void *baton) {
    Isolate *isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);

    LookupCtpApiBaton *tmp = static_cast<LookupCtpApiBaton *>(baton);
    if (tmp->uuid != -1) {
        std::map < int, Persistent < Function > > ::iterator
        it = fun_rtncb_map.find(tmp->uuid);

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
//        jsonRtn->Set(String::NewFromUtf8(isolate, "UserNo"),
//                     String::NewFromUtf8(isolate, pRspUserLogin->UserNo));
//
//        Local <Value> str = Nan::Encode(pRspUserLogin->UserName, strlen(pRspUserLogin->UserName),
//                                        Nan::Encoding::BUFFER);
//        jsonRtn->Set(String::NewFromUtf8(isolate, "UserName"), str);
//        jsonRtn->Set(String::NewFromUtf8(isolate, "QuoteTempPassword"),
//                     String::NewFromUtf8(isolate, pRspUserLogin->QuoteTempPassword));
//        jsonRtn->Set(String::NewFromUtf8(isolate, "ReservedInfo"),
//                     String::NewFromUtf8(isolate, pRspUserLogin->ReservedInfo));
//        jsonRtn->Set(String::NewFromUtf8(isolate, "LastLoginIP"),
//                     String::NewFromUtf8(isolate, pRspUserLogin->LastLoginIP));
//        jsonRtn->Set(String::NewFromUtf8(isolate, "LastLoginTime"),
//                     String::NewFromUtf8(isolate, pRspUserLogin->LastLoginTime));
//        jsonRtn->Set(String::NewFromUtf8(isolate, "TradeDate"), String::NewFromUtf8(isolate, pRspUserLogin->TradeDate));
//        jsonRtn->Set(String::NewFromUtf8(isolate, "LastSettleTime"),
//                     String::NewFromUtf8(isolate, pRspUserLogin->LastSettleTime));
//        jsonRtn->Set(String::NewFromUtf8(isolate, "StartTime"), String::NewFromUtf8(isolate, pRspUserLogin->StartTime));
//        jsonRtn->Set(String::NewFromUtf8(isolate, "InitTime"), String::NewFromUtf8(isolate, pRspUserLogin->InitTime));
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
        CUstpFtdcInputOrderField *pRspUserLogin = static_cast<CUstpFtdcInputOrderField *>(data->rtnField);
        Local <Object> jsonRtn = Object::New(isolate);
        logger_cout("---1");
//        jsonRtn->Set(String::NewFromUtf8(isolate, "UserNo"),
//                     String::NewFromUtf8(isolate, pRspUserLogin->UserNo));
//        logger_cout("---2");
//
//        Local <Value> str = Nan::Encode(pRspUserLogin->UserName, strlen(pRspUserLogin->UserName),
//                                        Nan::Encoding::BUFFER);
//        jsonRtn->Set(String::NewFromUtf8(isolate, "UserName"), str);
//        logger_cout("---3");
//        jsonRtn->Set(String::NewFromUtf8(isolate, "QuoteTempPassword"),
//                     String::NewFromUtf8(isolate, pRspUserLogin->QuoteTempPassword));
//        logger_cout("---4");
//        jsonRtn->Set(String::NewFromUtf8(isolate, "ReservedInfo"),
//                     String::NewFromUtf8(isolate, pRspUserLogin->ReservedInfo));
//        logger_cout("---5");
//        jsonRtn->Set(String::NewFromUtf8(isolate, "LastLoginIP"),
//                     String::NewFromUtf8(isolate, pRspUserLogin->LastLoginIP));
//        logger_cout("---6");
//        jsonRtn->Set(String::NewFromUtf8(isolate, "LastLoginTime"),
//                     String::NewFromUtf8(isolate, pRspUserLogin->LastLoginTime));
//        logger_cout("---7");
//        jsonRtn->Set(String::NewFromUtf8(isolate, "TradeDate"), String::NewFromUtf8(isolate, pRspUserLogin->TradeDate));
//        logger_cout("---8");
//        jsonRtn->Set(String::NewFromUtf8(isolate, "LastSettleTime"),
//                     String::NewFromUtf8(isolate, pRspUserLogin->LastSettleTime));
//        logger_cout("---9");
//        jsonRtn->Set(String::NewFromUtf8(isolate, "StartTime"), String::NewFromUtf8(isolate, pRspUserLogin->StartTime));
//        logger_cout("---10");
//        jsonRtn->Set(String::NewFromUtf8(isolate, "InitTime"), String::NewFromUtf8(isolate, pRspUserLogin->InitTime));
//        logger_cout("---11");
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
        CUstpFtdcInputOrderField *pRspUserLogin = static_cast<CUstpFtdcInputOrderField *>(data->rtnField);
        Local <Object> jsonRtn = Object::New(isolate);
//        jsonRtn->Set(String::NewFromUtf8(isolate, "ActionType"),
//                     String::NewFromUtf8(isolate, (char *) pRspUserLogin->ActionType));
//        jsonRtn->Set(String::NewFromUtf8(isolate, "AccountNo"),
//                     String::NewFromUtf8(isolate, pRspUserLogin->OrderInfo->AccountNo));
//        jsonRtn->Set(String::NewFromUtf8(isolate, "ExchangeNo"),
//                     String::NewFromUtf8(isolate, pRspUserLogin->OrderInfo->ExchangeNo));
//        jsonRtn->Set(String::NewFromUtf8(isolate, "RefInt"), Number::New(isolate, pRspUserLogin->OrderInfo->RefInt));

//        Local <Value> str = Nan::Encode(pRspUserLogin->UserName, strlen(pRspUserLogin->UserName),
//                                        Nan::Encoding::BUFFER);
//        jsonRtn->Set(String::NewFromUtf8(isolate, "UserName"), str);
        *(cbArray + 1) = jsonRtn;
    } else {
        *(cbArray + 1) = Local<Value>::New(isolate, Undefined(isolate));
    }
    return;
}

