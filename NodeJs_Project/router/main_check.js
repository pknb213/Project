var router = require("../config/config.js")(router);

router.route('/process/main_check').post(function(req,res){
  console.log("/process/main_check 호출");

});

console.log("main_check.js 동작 완료");
