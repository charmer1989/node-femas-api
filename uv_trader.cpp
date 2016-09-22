#include <uv.h>
#include "uv_trader.h"
#include "USTPFtdcTraderApi.h"
#include "USTPFtdcUserApiDataType.h"
#include "USTPFtdcUserApiStruct.h"
#include <cstring>
#include <sstream>
#include "wraper_struct.h"
#include <string>

std::string to_string(int val) {
    std::stringstream ss;
    ss << val;
    return ss.str();
}

std::string charto_string(char val) {
    std::stringstream ss;
    ss << val;
    return ss.str();
}

std::map<int, CbWrap *> uv_trader::cb_map;
int hasConnect = 0;

uv_trader::uv_trader(void) {
    iRequestID = 0;
    uv_async_init(uv_default_loop(), &async_t, NULL);
    logger_cout("uv_trader init");
}

uv_trader::~uv_trader(void) {
    uv_close((uv_handle_t * ) & async_t, NULL);
}

const char *uv_trader::GetVersion() {
    int nMajorVersion = 0;
    int nMinorVersion = 0;

    const char *apiVersion = this->m_pApi->GetVersion(nMajorVersion, nMinorVersion);
    return apiVersion;
}

int uv_trader::On(const char *eName, int cb_type, void(*callback)(CbRtnField *cbResult)) {
    std::string log = "uv_trader On------>";
    std::map<int, CbWrap *>::iterator it = cb_map.find(cb_type);
    if (it != cb_map.end()) {
        logger_cout(log.append(" event id").append(to_string(cb_type)).append(" register repeat").c_str());
        return 1;
    }

    CbWrap *cb_wrap = new CbWrap();
    cb_wrap->callback = callback;
    cb_map[cb_type] = cb_wrap;
    logger_cout(
            log.append(" Event:").append(eName).append(" ID:").append(to_string(cb_type)).append(" register").c_str());
    return 0;
}

void uv_trader::Connect(UVConnectField *pConnectField, void(*callback)(int, void *), int uuid) {
    UVConnectField *_pConnectField = new UVConnectField();
    memcpy(_pConnectField, pConnectField, sizeof(UVConnectField));
    logger_cout("trader Connect this -> invoke");
    this->invoke(_pConnectField, T_CONNECT_RE, callback, uuid);
}

void uv_trader::ReqUserLogin(CUstpFtdcReqUserLoginField *pReqUserLoginField, void(*callback)(int, void *), int uuid) {
    CUstpFtdcReqUserLoginField *_pReqUserLoginField = new CUstpFtdcReqUserLoginField();
    memcpy(_pReqUserLoginField, pReqUserLoginField, sizeof(CUstpFtdcReqUserLoginField));
    this->invoke(_pReqUserLoginField, T_LOGIN_RE, callback, uuid);
}

void uv_trader::ReqOrderInsert(CUstpFtdcInputOrderField *pInputOrder, void(*callback)(int, void *), int uuid) {
    CUstpFtdcInputOrderField *_pInputOrder = new CUstpFtdcInputOrderField();
    memcpy(_pInputOrder, pInputOrder, sizeof(CUstpFtdcInputOrderField));
    this->invoke(_pInputOrder, T_INSERT_RE, callback, uuid);
}


void uv_trader::OnFrontConnected() {
    if (!hasConnect) {
        std::string log = "uv_trader OnFrontConnected !!!!!!!!!!";
        logger_cout(log.c_str());
        CbRtnField *field = new CbRtnField();
        field->eFlag = T_ON_CONNECT;
        field->work.data = field;
        uv_queue_work(uv_default_loop(), &field->work, _on_async, _on_completed);
        hasConnect = 1;
    }
}

void
uv_trader::OnRspUserLogin(CUstpFtdcRspUserLoginField *pRspUserLogin, CUstpFtdcRspInfoField *pRspInfo, int nRequestID,
                          bool bIsLast) {
    CUstpFtdcRspUserLoginField *_pRspUserLogin = NULL;
    if (pRspUserLogin) {
        _pRspUserLogin = new CUstpFtdcRspUserLoginField();
        memcpy(_pRspUserLogin, pRspUserLogin, sizeof(CUstpFtdcRspUserLoginField));
    }
    std::string log = "uv_trader OnRspLogin------>";
    logger_cout(log.append("nRequestID:").append(to_string(nRequestID)).c_str());
    on_invoke(T_ON_RSPUSERLOGIN, _pRspUserLogin, nRequestID);
}

void uv_trader::OnRspOrderInsert(CUstpFtdcInputOrderField *pInputOrder, CUstpFtdcRspInfoField *pRspInfo, int nRequestID,
                                 bool bIsLast) {
    CUstpFtdcRspInfoField *_pRspInfo = NULL;
    if (pRspInfo) {
        _pRspInfo = new CUstpFtdcRspInfoField();
        memcpy(_pRspInfo, pRspInfo, sizeof(CUstpFtdcRspInfoField));
    }
    std::string log = "uv_trader OnRspOrderInsert------>";
    logger_cout(log.c_str());
    on_invoke(T_ON_RSPINSERT, _pRspInfo, nRequestID);
}

void uv_trader::OnErrRtnOrderInsert(CUstpFtdcInputOrderField *pInputOrder, CUstpFtdcRspInfoField *pRspInfo) {
    CUstpFtdcRspInfoField *_pRspInfo = NULL;
    if (pRspInfo) {
        _pRspInfo = new CUstpFtdcRspInfoField();
        memcpy(_pRspInfo, pRspInfo, sizeof(CUstpFtdcRspInfoField));
    }
    std::string log = "uv_trader OnErrRtnOrderInsert------>";
    logger_cout(log.c_str());
    on_invoke(T_ON_ERRINSERT, _pRspInfo, 0);
};

void uv_trader::OnRspError(CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    CUstpFtdcRspInfoField *_pRspInfo = NULL;
    if (pRspInfo) {
        _pRspInfo = new CUstpFtdcRspInfoField();
        memcpy(_pRspInfo, pRspInfo, sizeof(CUstpFtdcRspInfoField));
    }
    std::string log = "uv_trader OnRspError------>";
    logger_cout(log.c_str());
    on_invoke(T_ON_RSPERROR, _pRspInfo, 0);
};

void uv_trader::_async(uv_work_t *work) {
    LookupCtpApiBaton *baton = static_cast<LookupCtpApiBaton *>(work->data);
    uv_trader *uv_trader_obj = static_cast<uv_trader *>(baton->uv_trader_obj);
    std::string log = "uv_trader _async------>";
    logger_cout(log.append(to_string(baton->fun)).c_str());
    switch (baton->fun) {
        case T_CONNECT_RE: {
            UVConnectField *_pConnectF = static_cast<UVConnectField *>(baton->args);
            uv_trader_obj->m_pApi = CUstpFtdcTraderApi::CreateFtdcTraderApi("");
            uv_trader_obj->m_pApi->RegisterFront(_pConnectF->front_ip);
            uv_trader_obj->m_pApi->RegisterSpi(uv_trader_obj);
            uv_trader_obj->m_pApi->Init();

            logger_cout(log.append("invoke connect,the result is 0 | szPath is ").append(_pConnectF->front_ip).c_str());
            break;
        }
        case T_LOGIN_RE: {
            CUstpFtdcReqUserLoginField *reqUserLogin = static_cast<CUstpFtdcReqUserLoginField *>(baton->args);
            baton->nResult = uv_trader_obj->m_pApi->ReqUserLogin(reqUserLogin, baton->iRequestID); // uuid
            logger_cout(log.append("invoke ReqUserLogin,the result:").append(to_string(baton->nResult)).c_str());
            break;
        }
        case T_INSERT_RE: {
            CUstpFtdcInputOrderField *_pInputOrder = static_cast<CUstpFtdcInputOrderField *>(baton->args);
            baton->nResult = uv_trader_obj->m_pApi->ReqOrderInsert(_pInputOrder, baton->iRequestID);
            logger_cout(log.append("invoke ReqOrderInsert,the result:").append(to_string(baton->nResult)).c_str());
            break;
        }
        default: {
            logger_cout(log.append("No case event:").append(to_string(baton->fun)).c_str());
            break;
        }
    }
}

///uv_queue_work
void uv_trader::_completed(uv_work_t *work, int) {
    LookupCtpApiBaton *baton = static_cast<LookupCtpApiBaton *>(work->data);
    baton->callback(baton->nResult, baton);
    delete baton->args;
    delete baton;
}

void uv_trader::_on_async(uv_work_t *work) {
    //do nothing
}

void uv_trader::_on_completed(uv_work_t *work, int) {
    std::string head = "uv_trader _on_completed  ==> ";

    CbRtnField *cbTrnField = static_cast<CbRtnField *>(work->data);
    std::map<int, CbWrap *>::iterator it = cb_map.find(cbTrnField->eFlag);
    if (it != cb_map.end()) {
        logger_cout(head.append("has return").c_str());
        cb_map[cbTrnField->eFlag]->callback(cbTrnField);
    } else {
        logger_cout(head.append("none").c_str());
    }
    if (cbTrnField->rtnField)
        delete cbTrnField->rtnField;
    // if (cbTrnField->errorCode)
    // 	delete cbTrnField->errorCode;
    delete cbTrnField;
}

void uv_trader::invoke(void *field, int ret, void(*callback)(int, void *), int uuid) {
    LookupCtpApiBaton *baton = new LookupCtpApiBaton();
    baton->work.data = baton;
    baton->uv_trader_obj = this;
    baton->callback = callback;
    baton->args = field;
    baton->fun = ret;
    baton->uuid = uuid;

    iRequestID = iRequestID + 1;
    baton->iRequestID = iRequestID;
    std::string head = "uv_trader invoke------>uuid:";
    logger_cout(head.append(to_string(uuid)).append(",requestid:").append(to_string(baton->iRequestID)).c_str());
    uv_queue_work(uv_default_loop(), &baton->work, _async, _completed);
}

void uv_trader::on_invoke(int event_type, void *_stru, int nRequestID) {
    CbRtnField *field = new CbRtnField();
    field->work.data = field;
    field->eFlag = event_type;
    field->rtnField = _stru;
    field->nRequestID = nRequestID;
    uv_queue_work(uv_default_loop(), &field->work, _on_async, _on_completed);
}


void uv_trader::OnFrontDisconnected(int nReason) {
    logger_cout("-----------uv OnFrontDisconnected");
    logger_cout(to_string(nReason).c_str());
};

void uv_trader::OnHeartBeatWarning(int nTimeLapse) {
    logger_cout("-----------uv OnHeartBeatWarning");
};

void uv_trader::OnPackageStart(int nTopicID, int nSequenceNo) {
    logger_cout("-----------uv OnPackageStart");
};

void uv_trader::OnPackageEnd(int nTopicID, int nSequenceNo) {
    logger_cout("-----------uv OnPackageEnd");
};

void
uv_trader::OnRspUserLogout(CUstpFtdcRspUserLogoutField *pRspUserLogout, CUstpFtdcRspInfoField *pRspInfo, int nRequestID,
                           bool bIsLast) {
    logger_cout("-----------uv OnRspUserLogout");
};

void
uv_trader::OnRspUserPasswordUpdate(CUstpFtdcUserPasswordUpdateField *pUserPasswordUpdate,
                                   CUstpFtdcRspInfoField *pRspInfo,
                                   int nRequestID, bool bIsLast) {
    logger_cout("-----------uv OnRspUserPasswordUpdate");
};

void
uv_trader::OnRspOrderAction(CUstpFtdcOrderActionField *pOrderAction, CUstpFtdcRspInfoField *pRspInfo, int nRequestID,
                            bool bIsLast) {
    logger_cout("-----------uv OnRspOrderAction");
};

void uv_trader::OnRtnFlowMessageCancel(CUstpFtdcFlowMessageCancelField *pFlowMessageCancel) {
    logger_cout("-----------uv OnRtnFlowMessageCancel");
};

void uv_trader::OnRtnTrade(CUstpFtdcTradeField *pTrade) {
    logger_cout("-----------uv OnRtnTrade");
};

void uv_trader::OnRtnOrder(CUstpFtdcOrderField *pOrder) {
    logger_cout("-----------uv OnRtnOrder");
};

void uv_trader::OnErrRtnOrderAction(CUstpFtdcOrderActionField *pOrderAction, CUstpFtdcRspInfoField *pRspInfo) {
    logger_cout("-----------uv OnErrRtnOrderAction");
};

void uv_trader::OnRtnInstrumentStatus(CUstpFtdcInstrumentStatusField *pInstrumentStatus) {
    logger_cout("-----------uv OnRtnInstrumentStatus");
};

void uv_trader::OnRtnInvestorAccountDeposit(CUstpFtdcInvestorAccountDepositResField *pInvestorAccountDepositRes) {
    logger_cout("-----------uv OnRtnInvestorAccountDeposit");
};

void
uv_trader::OnRspQryOrder(CUstpFtdcOrderField *pOrder, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    logger_cout("-----------uv OnRspQryOrder");
};

void
uv_trader::OnRspQryTrade(CUstpFtdcTradeField *pTrade, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    logger_cout("-----------uv OnRspQryTrade");
};

void uv_trader::OnRspQryUserInvestor(CUstpFtdcRspUserInvestorField *pRspUserInvestor, CUstpFtdcRspInfoField *pRspInfo,
                                     int nRequestID, bool bIsLast) {
    logger_cout("-----------uv OnRspQryUserInvestor");
};

void
uv_trader::OnRspQryTradingCode(CUstpFtdcRspTradingCodeField *pRspTradingCode, CUstpFtdcRspInfoField *pRspInfo,
                               int nRequestID,
                               bool bIsLast) {
    logger_cout("-----------uv OnRspQryTradingCode");
};

void
uv_trader::OnRspQryInvestorAccount(CUstpFtdcRspInvestorAccountField *pRspInvestorAccount,
                                   CUstpFtdcRspInfoField *pRspInfo,
                                   int nRequestID, bool bIsLast) {
    logger_cout("-----------uv OnRspQryInvestorAccount");
};

void
uv_trader::OnRspQryInstrument(CUstpFtdcRspInstrumentField *pRspInstrument, CUstpFtdcRspInfoField *pRspInfo,
                              int nRequestID,
                              bool bIsLast) {
    logger_cout("-----------uv OnRspQryInstrument");
};

void
uv_trader::OnRspQryExchange(CUstpFtdcRspExchangeField *pRspExchange, CUstpFtdcRspInfoField *pRspInfo, int nRequestID,
                            bool bIsLast) {
    logger_cout("-----------uv OnRspQryExchange");
};

void
uv_trader::OnRspQryInvestorPosition(CUstpFtdcRspInvestorPositionField *pRspInvestorPosition,
                                    CUstpFtdcRspInfoField *pRspInfo,
                                    int nRequestID, bool bIsLast) {
    logger_cout("-----------uv OnRspQryInvestorPosition");
};

void
uv_trader::OnRspSubscribeTopic(CUstpFtdcDisseminationField *pDissemination, CUstpFtdcRspInfoField *pRspInfo,
                               int nRequestID,
                               bool bIsLast) {
    logger_cout("-----------uv OnRspSubscribeTopic");
};

void
uv_trader::OnRspQryComplianceParam(CUstpFtdcRspComplianceParamField *pRspComplianceParam,
                                   CUstpFtdcRspInfoField *pRspInfo,
                                   int nRequestID, bool bIsLast) {
    logger_cout("-----------uv OnRspQryComplianceParam");
};

void
uv_trader::OnRspQryTopic(CUstpFtdcDisseminationField *pDissemination, CUstpFtdcRspInfoField *pRspInfo, int nRequestID,
                         bool bIsLast) {
    logger_cout("-----------uv OnRspQryTopic");
};

void
uv_trader::OnRspQryInvestorFee(CUstpFtdcInvestorFeeField *pInvestorFee, CUstpFtdcRspInfoField *pRspInfo, int nRequestID,
                               bool bIsLast) {
    logger_cout("-----------uv OnRspQryInvestorFee");
};

void uv_trader::OnRspQryInvestorMargin(CUstpFtdcInvestorMarginField *pInvestorMargin, CUstpFtdcRspInfoField *pRspInfo,
                                       int nRequestID, bool bIsLast) {
    logger_cout("-----------uv OnRspQryInvestorMargin");
};