var femas = require('./build/Release/femas.node');

console.log(typeof femas);
femas.settings({log:true});
var hasConnect = false;
var trader = femas.createTrader();
console.log("JS____ trader.getVersion()=>",trader.getVersion());
trader.connect("tcp://118.126.16.227:17086", 0, function (result, reqId) { //   tcp://116.228.53.149:6666 tcp://118.126.16.227:17086
    console.log("JS____ trader->connect->result:", result, "reqId:", reqId);
});

trader.on('connect', function(){
    console.log("JS____       ON CONNECT");
    if (hasConnect) return ;
    hasConnect = true;
    trader.reqUserLogin("8001036327", "123456", "88888", function (result) {
        console.log("JS____ trader->reqUserLogin->result:", result);
    });
});

trader.on("rspUserLogin", function (errCode, userInfo) {
    console.log("JS____ ON rspUserLogin, errCode:", errCode, "userInfo:",userInfo);
    setTimeout(function () {
        trader.reqOrderInsert({
            brokerID: "88888",///经纪公司编号
            exchangeID: "CFFEX",///交易所代码
            investorID: "8001036327",///投资者编号
            userID: "8001036327",///用户代码
            instrumentID: "au1612",///合约代码
            userOrderLocalID: "000000000001",///用户本地报单号
            direction: "0",///买卖方向 买0  卖1
            limitPrice: 293,///价格
            volume: 1,///数量

            orderSysID: "",///系统报单编号
            seatNo: "",///指定通过此席位序号下单
            // 未使用
            gtdDate: "",///GTD日期
            businessUnit: "",///业务单元
        }, function (result) {
            console.log("JS____ trader->reqOrderInsert->result:", result);
        });
    }, 3000);
});
trader.on("rspInsert", function (errCode, rspInfo) {
    rspInfo.ErrorMsg = rspInfo.ErrorMsg.toString('utf8');
    console.log("JS____ ON rspInsert, errCode:", errCode, "rspInfo:",rspInfo);
});
trader.on("errInsert", function (errCode, rspInfo) {
    rspInfo.ErrorMsg = rspInfo.ErrorMsg.toString('utf8');
    console.log("JS____ ON errInsert, errCode:", errCode, "rspInfo:",rspInfo);
});
trader.on('rspError', function (errCode, rspInfo) {
    rspInfo.ErrorMsg = rspInfo.ErrorMsg.toString('utf8');
    console.log("JS____ ON rspError, errCode:", errCode, "rspInfo:",rspInfo);
});
