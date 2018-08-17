// mySQL
var mysql = require("mysql");

// Database object creation
var database = {};

// Using the mySQL
var pool = mysql.createPool({
  connectionLimit:10,
  host:'localhost',
  user:'root',
  password:'root',
  database:'Nodejs_Project',
  debug:false,
  // Authentication error soultion
  insecureAuth : true
});

var callback = function(err, rows){
  return err, rows;
};

function db_connect(){
  console.log("connect() 호출");

  // DB 연결
  if(pool){
    pool.getConnection(function(err, conn){
      if(err){
        if(conn){
          conn.release();
        }
          console.log("DB is not definition");
    //    callback(err, null);
        return;
      }
      console.log("DB Connection Succecss. Thread ID : " + conn.threadId);

    });
  }else{
    console.log("DB object is not definition");
  }
}

db_connect();

module.exports = pool;

console.log("mysql.js 동작 완료");
