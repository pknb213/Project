var Calc = require('./Cal_Moudule');

var calc = new Calc();
calc.emit('stop');

console.log(Calc.title + '에 Stop 이벤트 전달');
