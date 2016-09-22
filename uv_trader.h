#ifndef UV_TRADER_H_
#define UV_TRADER_H_

#include <iostream>
#include <string>
#include <map>
#include <fstream>
#include "USTPFtdcTraderApi.h"
#include "USTPFtdcUserApiDataType.h"
#include "USTPFtdcUserApiStruct.h"
#include <uv.h>
#include <node.h>
#include "wraper_struct.h"
#include <node_object_wrap.h>

extern bool islog;
extern std::string authCodeStr;
// extern ITapTradeAPI * CreateTapTradeAPI(const TapAPIApplicationInfo *appInfo, TAPIINT32 &iResult);

void logger_cout(const char *content);

std::string to_string(int val);

std::string charto_string(char val);

class uv_trader : public CUstpFtdcTraderSpi {
public:
    uv_trader(void);

    virtual ~uv_trader(void);

    const char *GetVersion();

    int On(const char *eName, int cb_type, void(*callback)(CbRtnField *cbResult));

    void Connect(UVConnectField *pConnectField, void(*callback)(int, void *), int uuid);

    void ReqUserLogin(CUstpFtdcReqUserLoginField *pReqUserLoginField, void(*callback)(int, void *), int uuid);

    void ReqOrderInsert(CUstpFtdcInputOrderField *pInputOrder, void(*callback)(int, void *), int uuid);

    virtual void OnFrontConnected();

    virtual void
    OnRspUserLogin(CUstpFtdcRspUserLoginField *pRspUserLogin, CUstpFtdcRspInfoField *pRspInfo, int nRequestID,
                   bool bIsLast);

    virtual void
    OnRspOrderInsert(CUstpFtdcInputOrderField *pInputOrder, CUstpFtdcRspInfoField *pRspInfo, int nRequestID,
                     bool bIsLast);

    virtual void OnErrRtnOrderInsert(CUstpFtdcInputOrderField *pInputOrder, CUstpFtdcRspInfoField *pRspInfo);



    virtual void OnFrontDisconnected(int nReason) ;

    virtual void OnHeartBeatWarning(int nTimeLapse) ;

    virtual void OnPackageStart(int nTopicID, int nSequenceNo) ;

    virtual void OnPackageEnd(int nTopicID, int nSequenceNo) ;

    virtual void OnRspError(CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    virtual void
    OnRspUserLogout(CUstpFtdcRspUserLogoutField *pRspUserLogout, CUstpFtdcRspInfoField *pRspInfo, int nRequestID,
                    bool bIsLast) ;

    virtual void
    OnRspUserPasswordUpdate(CUstpFtdcUserPasswordUpdateField *pUserPasswordUpdate, CUstpFtdcRspInfoField *pRspInfo,
                            int nRequestID, bool bIsLast) ;

    virtual void
    OnRspOrderAction(CUstpFtdcOrderActionField *pOrderAction, CUstpFtdcRspInfoField *pRspInfo, int nRequestID,
                     bool bIsLast) ;

    virtual void OnRtnFlowMessageCancel(CUstpFtdcFlowMessageCancelField *pFlowMessageCancel) ;

    virtual void OnRtnTrade(CUstpFtdcTradeField *pTrade) ;

    virtual void OnRtnOrder(CUstpFtdcOrderField *pOrder) ;

    virtual void OnErrRtnOrderAction(CUstpFtdcOrderActionField *pOrderAction, CUstpFtdcRspInfoField *pRspInfo) ;

    virtual void OnRtnInstrumentStatus(CUstpFtdcInstrumentStatusField *pInstrumentStatus) ;

    virtual void OnRtnInvestorAccountDeposit(CUstpFtdcInvestorAccountDepositResField *pInvestorAccountDepositRes) ;

    virtual void
    OnRspQryOrder(CUstpFtdcOrderField *pOrder, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    virtual void
    OnRspQryTrade(CUstpFtdcTradeField *pTrade, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    virtual void OnRspQryUserInvestor(CUstpFtdcRspUserInvestorField *pRspUserInvestor, CUstpFtdcRspInfoField *pRspInfo,
                                      int nRequestID, bool bIsLast) ;

    virtual void
    OnRspQryTradingCode(CUstpFtdcRspTradingCodeField *pRspTradingCode, CUstpFtdcRspInfoField *pRspInfo, int nRequestID,
                        bool bIsLast) ;

    virtual void
    OnRspQryInvestorAccount(CUstpFtdcRspInvestorAccountField *pRspInvestorAccount, CUstpFtdcRspInfoField *pRspInfo,
                            int nRequestID, bool bIsLast) ;

    virtual void
    OnRspQryInstrument(CUstpFtdcRspInstrumentField *pRspInstrument, CUstpFtdcRspInfoField *pRspInfo, int nRequestID,
                       bool bIsLast) ;

    virtual void
    OnRspQryExchange(CUstpFtdcRspExchangeField *pRspExchange, CUstpFtdcRspInfoField *pRspInfo, int nRequestID,
                     bool bIsLast) ;

    virtual void
    OnRspQryInvestorPosition(CUstpFtdcRspInvestorPositionField *pRspInvestorPosition, CUstpFtdcRspInfoField *pRspInfo,
                             int nRequestID, bool bIsLast) ;

    virtual void
    OnRspSubscribeTopic(CUstpFtdcDisseminationField *pDissemination, CUstpFtdcRspInfoField *pRspInfo, int nRequestID,
                        bool bIsLast) ;

    virtual void
    OnRspQryComplianceParam(CUstpFtdcRspComplianceParamField *pRspComplianceParam, CUstpFtdcRspInfoField *pRspInfo,
                            int nRequestID, bool bIsLast) ;

    virtual void
    OnRspQryTopic(CUstpFtdcDisseminationField *pDissemination, CUstpFtdcRspInfoField *pRspInfo, int nRequestID,
                  bool bIsLast) ;

    virtual void
    OnRspQryInvestorFee(CUstpFtdcInvestorFeeField *pInvestorFee, CUstpFtdcRspInfoField *pRspInfo, int nRequestID,
                        bool bIsLast) ;

    virtual void OnRspQryInvestorMargin(CUstpFtdcInvestorMarginField *pInvestorMargin, CUstpFtdcRspInfoField *pRspInfo,
                                        int nRequestID, bool bIsLast) ;


private:
    static void _async(uv_work_t *work);

    static void _completed(uv_work_t *work, int);

    static void _on_async(uv_work_t *work);

    static void _on_completed(uv_work_t *work, int);

    void invoke(void *field, int ret, void(*callback)(int, void *), int uuid);

    void on_invoke(int event_type, void *_stru, int nRequestID);

    CUstpFtdcTraderApi *m_pApi;
    int iRequestID;
    uv_async_t async_t;
    static std::map<int, CbWrap *> cb_map;
    int hasConnect;
};

#endif
