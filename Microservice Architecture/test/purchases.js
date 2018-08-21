const mysql = require("mysql");
const conn = {
  host: 'localhost',
  user: 'root',
  password: 'root',
  database: 'micro_project'
};

/*
  구매 관리의 각 기능별로 분기
*/
exports.onRequest = function(res,method,pathname,params, cb){

  switch(method){
    case "POST":
      return register(res, method, pathname, params, (response) => {
        process.nextTick(cb, res, response);
      });
    case "GET":
      return inquiry(resm, method, pathname, params, (response) => {
        process.nextTick(cb, res, response);
      });
    case "DELETE":
      return unregister(res, method, pathname, params, (response) => {
        process.nextTick(cb, res, response);
      });
    default:
        return process.nextTick(cb, res, null);
  }
}
/*
  구매 기능
  @params method      메서드
  @params pathname    URI
  @params Parameters  입력 입력파라미터
  @params cb          콜백
*/
function register(method, pathname, params, cb){
  var response = {
    key: params.key,
    errorcode: 0,
    errormessage: "Success"
  };

  if(params.userid == null || params.goodsid == null){
    response.errorcode = 1;
    response.errormessage = "Invalid Parameters";
    cb(response);
  }else{
    var connection = mysql.createConnection(conn);
    connection.connection();
    connection.query("insert into purchases(userid, goodsid) values(?, ? )", [params.userid, params.goodsid], (error, results, fields) => {
      if(error){
        response.errorcode = 1;
        response.errormessage = error;
      }
      cb(response);
    });
    connection.end();
  }
}

/*
  구매 내역 조회 기능
  @params method      메서드
  @params pathname    URI
  @params Parameters  입력 입력파라미터
  @params cb          콜백
*/
function inquiry(method, pathname, params, cb){
  var response = {
    key: params.key,
    errorcode: 0,
    errormessage: "Success"
  };

  if(params.userid == null){
    response.errorcode = 1;
    response.errormessage = "Invalid Parameters";
    cb(response);
  }else{
    var connection = mysql.createConnection(conn);
    connection.connect();
    connection.query("select id, goodsid, date from purchases where userid = ?", [params.userid], (error,results,fields) => {
      if(error){
        response.errorcode = 1;
        response.errormessage = error;
      }else{
        response.results = results;
      }
      cb(response);
    });
    connection.end();
  }
}

console.log("purchases.js");
