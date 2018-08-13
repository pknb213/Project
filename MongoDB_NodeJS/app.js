// Express 기본 모듈
var express = require('express'), http = require('http'), path = require('path');

// Express의 미들웨어
var bodyParser = require('body-parser'), cookieParser = require('cookie-parser'), static = require('serve-static'), errorHandler = require('errorHandler');

// 오류 핸들러 모듈 사용
var expressErrorHandler = require('express-error-handler');

// Session 미들웨어 불러오기
var expressSession = require('express-session');

// Mongo DB 모듈 사용
var MongoClient = require('mongodb').MongoClient;

// Mongoose 모듈 사용
var mongoose = require('mongoose');

// Express 객체 생성
var app = express();

// DB 객체를 위한 변수 선언
var database;

// DB Schema 객체를 위한 변수 선언
var UserSchema;

// DB 모델 객체를 위한 변수 선언
var UserModel;

// DB 연결
function connectDB(){
  // DB 연결 정보
  //var databaseUrl = 'mongodb://localhost:27017';
  var databaseUrl = 'mongodb://localhost:27017/local';

  //DB연결
  console.log('DB 연결을 시도합니다');
  mongoose.Promise = global.Promise;
  mongoose.connect(databaseUrl);
  database = mongoose.connection;

  database.on('error', console.error.bind(console,'mongoose connection error'));
  database.on('open', function(){
    console.log('DB 연결 됨 : ' + databaseUrl);

    // Schema 정의
    UserSchema = mongoose.Schema({
      id:String,
      name:String,
      password:String
    });
    console.log('UserSchema 정의함');

    // UserModel 모델 정의
    UserModel = mongoose.model("users", UserSchema);
    console.log('UserModel 정의함');
  });

  // 연결 끊어졌을 때 5초 후 재연결
  database.on('disconnected', function(){
    console.log('연결이 끊어졌습니다. 5초 후 다시 연결합니다');
    setInterval(connectDB,5000);
  });
/*
// Basic method
  MongoClient.connect(databaseUrl, function(err,client){
    if(err) throw err;

    console.log('데이터 베이스에 연결되었습니다 : ' + databaseUrl);

    // DB 변수 할당
    database = client.db('local');
  });
  */
}


// Initialization
// 기본 속성 설정
app.set('port',process.env.PORT || 3000);

// body-parser를 하용해 application/x-www-form-urlencoded 파싱
app.use(bodyParser.urlencoded({extended: false}));

// body-parser를 사용해 application/json 파싱
app.use(bodyParser.json());

// public 폴더를 static으로 오픈
app.use('/public', static(path.join(__dirname, 'public')));

// cookie-parser 설정
app.use(cookieParser());

// 세션 설정
app.use(expressSession({
  secret: 'my key',
  resave: true,
  saveUninitialized:true
}));

// 라우터 객체 참조
var router = express.Router();

// 로그인 라우팅 함수 - 데이터베이스의 정보와 비교
router.route('/process/login').post(function(req,res){
  console.log('/process/login 호출됨');

  //  var paramId = req.param('id');
  //  var paramPassword = req.param('password');
  var paramId = req.body.id || req.query.id;
  var paramPassword = req.body.password || req.query.password;

  console.log('요청 파라미터 : ' + paramId + ', ' + paramPassword);

  if(database){
    authUser(database, paramId, paramPassword, function(err,docs){
      if(err) { throw err;}

      if(docs){
        console.dir(docs);
        var username = docs[0].name;
        res.writeHead('200', {'Content-Type':'text/html;charset=utf8'});
        res.write('<h1>로그인 성공</h1>');
        res.write('<div><p>사용자 아이디 : ' + paramId + '</p></div>');
        res.write('<div><p>사용자 이름 : ' + paramPassword + '</p></div>');
        res.write("<br><br><a href='/public/login.html'>다시 로그인하기 </a>");
        res.end();
      }else{
        res.writeHead('200', {'Content-Type':'text/html;charset=utf8'});
        res.write('<h1>로그인 실패</h1>');
        res.write('<div><p>아이디와 비밀번호를 다시 확인하십시오</p></div>');
        res.write("<br><br><a href = '/public/login.html'>다시 로그인하기</a>");
        res.end();
      }
    });
  }else{
    res.writeHead('200', {'Content-Type':'text/html;charset=utf8'});
    res.write('<h2>데이터베이스 연결 실패</h2>');
    res.write('<div><p>데이터베이스에 연결을 하지 못했습니다</p></div>');
    res.end();
  }
});

// 사용자 추가 라우터 함수 (클라이언트에서 보내온 데이터를 이용해 DB 추가)
router.route('/process/adduser').post(function(req,res){
  console.log('/process/adduser 호출됨');

  var paramId = req.body.id || req.query.id;
  var paramPassword = req.body.password || req.query.password;
  var paramName = req.body.name || req.query.name;

    console.log('요청 파라미터 : ' + paramId + ', ' + paramPassword + ' , ' + paramName);

    // DB 객체가 초기화된 경우, adduser 함수 호출하여 사용자 추가
    if(database){
      addUser(database, paramId, paramPassword, paramName, function(err, result){
        if(err){throw err;}

        console.log('result : %s  count : %d', result, result.insertedCount);
        // 결과 객체 확인하여 추가된 데이터가 있으면 성공 응답 전송
        if(result && result.insertedCount > 0){
          console.dir(result);
          res.writeHead('200', {'Content-Type':'text/html;charset=utf8'});
          res.write('<h2>사용자 추가 성공</h2>');
          res.end();
        }else{ // 결과 객체가 없으면 실패 응답 전송
          res.writeHead('200', {'Content-Type':'text/html;charset=utf8'});
          res.write('<h2>사용자 추가 실패</h2>');
          res.end();
        }
      });
    }else{// DB 객체가 초기화되지 않은 경우 실패 응답전송
      res.writeHead('200', {'Content-Type':'text/html;charset=utf8'});
      res.write('<h2>DB 연결 실패</h2>');
      res.end();
    }
});

// 사용자를 인증하는 함수
var authUser = function(database, id, password, callback){
  console.log('authUser 호출됨');

  // users 컬렉션 참조
  //var users = database.collection('users');

  // 아이디와 비밀번호를 사용해 검색
  UserModel.find({"id":id,"password":password,}, function(err,results){
    if(err){
      callback(err,null);
      return;
    }
    console.log('아이디 [%s], 비밀번호 [%s] 사용자 검색 결과', id, password);
    console.dir(results);
    if(results.length>0){
      console.log("일치하는 사용자 찾음 %s  %s", id, password);
      callback(null,results);
    }else{
      console.log("일치하는 사용자를 찾지 못함");
      callback(null,null);
    }
  });

/*
  // Basic method
  users.find({"id" : id, "password" : password}).toArray(function(err,docs){
    if(err){
      callback(err, null);
      return;
    }
    if(docs.length > 0){
      console.log('아이디 [%s], 비밀번호 [%s]가 일치하는 사용자 찾음', id, password);
      callback(null, docs);
    }else{
      console.log("일치하는 사용자를 찾지 못함");
      callback(null,null);
    }
  });
  */
};

// 사용자 추가 함수
var addUser = function(database, id, password, name, callback){
  console.log('addUser 호출됨 : ' + id + ' , ' + password + ' , ' + name);

  // users 컬렉션 참조
  //var users = database.collection('users');

  // UserModel의 인스턴스 생성
  var user = new UserModel({"id":id, "password":password, "name":name});

  // save()로 저장
  user.save(function(err){
    if(err){
      callback(err,null);
      return;
    }
    console.log("사용자 데이터 추가함");
    callback(null, user);
  });
/*
  // id, password, username을 사용해 사용자 추가
  users.insertMany([{"id":id, "password":password, "name":name}], function(err, result){
    // 오류 발생시 콜백 함수를 호출하면서 오류 객체 전달
    if(err){ callback(err,null); return; }
    // 오류가 아닌 경우 콜백 함수를 호출하면서 결과 객체 전달
    if(result.insertedCount > 0){
      console.log("사용자 레코드 추가됨 : " + result.insertedCount);
    }else{
      console.log("추가된 레코드가 없음");
    }
    callback(null,result);
  });
  */
};

// 라우터 객체 등록
app.use('/', router);

// 404 오류 처리
var errorHandler = expressErrorHandler({
  static: {
    '404': '/public/404.html'
  }
});

app.use(expressErrorHandler.httpError(404));
app.use(errorHandler);

// ==== 서버 시작 ====
http.createServer(app).listen(app.get('port'), function(){
  console.log('서버가 시작되었습니다. Port : ' + app.get('port'));

  // DB 연결
  connectDB();

});
