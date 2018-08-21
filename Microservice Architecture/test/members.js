const mysql = require('mysql');
const conn = {
  host: 'localhost',
  user: 'root',
  password: 'root',
  database: 'micro_project'
};

/*
  회원 관리의 각 기능별로 분기
*/
exports.onRequest = function(res,method,pathname,params, cb){

  switch(method){
    case "POST":
      return register(method, pathname, params, (response) => {
        process.nextTick(cb, res, response);
      });
    case "GET":
      return inquiry(method, pathname, params, (response) => {
        process.nextTick(cb, res, response);
      });
    case "DELETE":
      return unregister(method, pathname, params, (response) => {
        process.nextTick(cb, res, response);
      });
    default:
        return process.nextTick(cb, res, null);
  }
}

/*
  회원 등록 가능
  @params method      메서드
  @params pathname    URI
  @params Parameters  입력 파라미터
  @params cb          콜백
*/
function register(method, pathname, params, cb){
  var response = {
    key: params.key,
    errorcode: 0,
    errormessage: "Success"
  };

  if(params.username == null || params.password == null){
    response.errorcode = 1;
    response.errormessage = "Invalid Parameters";
    cb(response);
  }else{
    var connection = mysql.createConnection(conn);
    connection.connect();
    connection.query("insert into members(username, password) values('" + params.username + "', password('"+params.password + "');", (error, results, fields) => {
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
  회원 인증 기능
  @params method      메서드
  @params pathname    URI
  @params params      입력 입력파라미터
  @params cb          콜백
*/
function inquiry(method, pathname, params, cb){
  var response = {
    key:params.key,
    errorcode:0,
    errormessage:"Success"
  };
  if(params.username == null || params.passwrod == null){
    response.errorcode = 1;
    response.errormessage = "Invalid Parameters";
    cb(response);
  }else{
    var connection = mysql.createConnection(conn);
    connection.connect();
    connection.query("selet id from members where username = '"+ params.username+"' and password = password('" + params.password + "');", (error, results, fields) => {
      if(error || results.length == 0){
        response.errorcode = 1;
        response.errormessage = error ? error : "invalid password";
      }else{
        response.userid = results[0].id;
      }
      cb(response);
    });
    connection.end();
  }
}

/*
  회원 탈퇴 기능
  @params method    메서드
  @params pathname  URI
  @Parame params    입력 입력파라미터
  @params cb        콜백
*/
function unregister(method, pathname, params, cb){
  var response = {
    key: params.key,
    errorcode: 0,
    errormessage: "Success"
  };
  if(params.username == null){
    response.errorcode = 1;
    response.errormessage = "Invalid Parameters";
    cb(response);
  }else{
    var connection = mysql.createConnection(conn);
    connection.connect();
    connecttion.query("delete from members where username = '" + params.username + "';", (error, results, fields) => {
      if(error){
        response.errorcode = 1;
        response.errormessage = error;
      }
      cb(response);
    });
    connection.end();
  }
}
console.log("members.js");
