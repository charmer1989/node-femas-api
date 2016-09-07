var femas = require('./build/Release/femas.node');

console.log(typeof femas);
femas.settings({log:true});
var trader = femas.createTrader();
console.log("trader.getVersion()=>",trader.getVersion());
trader.connect("tcp://116.228.53.149:6666", 0, function (result) {
    console.log("trader->connect->result:", result);
});

trader.on('connect', function(){
    console.log("ON CONNECT");
    trader.reqUserLogin("username", "pwd", "brokerid", function (result) {
        console.log("trader->reqUserLogin->result:", result);
    });
});
trader.on("rspUserLogin", function () {
    console.log("ON rspUserLogin");
});
trader.on("rspInsert", function () {
    console.log("ON rspInsert");
});
trader.on("errInsert", function () {
    console.log("ON errInsert");
});