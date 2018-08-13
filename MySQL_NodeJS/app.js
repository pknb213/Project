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

// Crypto 모듈 사용
var crypto = require('crypto');

// mySQL 모듈 사용
var mysql = require('mysql');

// Express 객체 생성
var app = express();

// DB 객체를 위한 변수 선언
var database;

// DB Schema 객체를 위한 변수 선언
var UserSchema;

// DB 모델 객체를 위한 변수 선언
var UserModel;

// DB 연결
/* Using the Mongo Database Method
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

    createUserSchema();
  });

  // 연결 끊어졌을 때 5초 후 재연결
  database.on('disconnected', function(){
    console.log('연결이 끊어졌습니다. 5초 후 다시 연결합니다');
    setInterval(connectDB,5000);
  });
}
*/
// Using the mySQL method
var pool = mysql.createPool({
  connectionLimit : 10,
  host : 'localhost',
  user : 'root',
  password : 'root',
  database : 'test',
  debug : false
});

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
/*
router.route('/process/login').post(function(req,res){
  console.log('/process/login 호출됨');

  //  var paramId = req.param('id');
  //  var paramPassword = req.param('password');
  var paramId = req.body.id || req.query.id;
  var paramPassword = req.body.password || req.query.password;

  console.log('요청 파라미터 : ' + paramId + ', ' + paramPassword);

  if(database){
    authUser(database, paramId, paramPassword, function(err,docs){
      if(err) {
        console.error('사용자 로그인 중 에러 발생 : ' + err.stack);
        res.writeHead('200', {'Content-Type':'text/html;charset=utf8'});
				res.write('<h2>사용자 로그인 중 에러 발생</h2>');
        res.write('<p>' + err.stack + '</p>');
				res.end();
        return;
      }

      if(docs){
        console.dir(docs);
        var username = docs[0].name;
        res.writeHead('200', {'Content-Type':'text/html;charset=utf8'});
        res.write('<h1>로그인 성공</h1>');
        res.write('<div><p>사용자 I D : ' + paramId + '</p></div>');
        res.write('<div><p>사용자 Password : ' + paramPassword + '</p></div>');
        res.write('<div><p>사용자 : ' + username + '</p></div>');
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
*/
// mySQL Dababase version
router.route('/process/login').post(function(req,res){
  console.log('/process/login 호출됨');

  // Request parameter confirm
  var paramId = req.body.id || req.query.id;
  var paramPassword = req.body.password || req.query.password;

  console.log("요청 파라미터 : " + paramId + ', ' + paramPassword);
  if(pool)
  {
    authUser(paramId, paramPassword, function(err,rows){
      if(err){
        console.error('로그인 중 오류 발생: ' + err.stack);
        res.writeHead('200', {'Content-Type':'text/html;charset=utf8'});
        res.write('<h2>사용자 로그인 중 오류 발생</h2>');
        res.write('<p>' + err.stack + '</p>');
        res.end();
        return;
      }
      if(rows){
        console.dir(rows);
        var username = rows[0].name;
        res.writeHead('200', {'Content-Type':'text/html;charset=utf8'});
        res.write('<h1>로그인 중 성공</h1>');
        res.write('<div><p>사용자 ID : ' + paramId + '</p></div>');
        res.write('<div><p>사용자 Name : ' + username + '</p></div>');
        res.write("<br><br><a href = '/public/login2.html'>다시 로그인하기</a>");
        res.end();
      }
    });
  }
});

// 사용자 추가 라우터 함수 (클라이언트에서 보내온 데이터를 이용해 DB 추가)
/* Mongo Database Vertion
router.route('/process/adduser').post(function(req,res){
  console.log('/process/adduser 호출됨');

  var paramId = req.body.id || req.query.id;
  var paramPassword = req.body.password || req.query.password;
  var paramName = req.body.name || req.query.name;

    console.log('요청 파라미터 : ' + paramId + ', ' + paramPassword + ' , ' + paramName);

    // DB 객체가 초기화된 경우, adduser 함수 호출하여 사용자 추가
    if(database){
      addUser(database, paramId, paramPassword, paramName, function(err, result){
        if(err){
          console.error('사용자 추가 중 오류 발생 : ' + err.stack);

          res.writeHead('200', {'Content-Type':'text/html; charset=utf8'});
          res.write('<h2>사용자 추가 중 오류 발생</h2>');
          res.write('<p>' + err.stack + '</p>');
          res.end();

          return;
        }

        console.log('result : %s  count : %d', result, result.insertedCount);
        // 결과 객체 확인하여 추가된 데이터가 있으면 성공 응답 전송
        if(addUser){
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
*/
// mySQL version
router.route('/process/adduser').post(function(req,res){
  console.log('/process/adduser 호출');

  var paramId = req.body.id || req.query.id;
  var paramPassword = req.body.password || req.query.password;
  var paramName = req.body.name || req.query.name;
  var paramAge = req.body.age || req.query.age;

  console.log('Request Parameter  : ' + paramId + ', ' + paramPassword + ', ' + paramName + ', ' + paramAge);

  // pool 객체가 초기화된 경우, addUser 함수 호출하여 사용자 추가
  if(pool){
    adduser(paramId,paramName,paramAge,paramPassword,function(err,addedUser){
      if(err){
        console.error('사용자 추가 중 오류 발생 : ' + err.stack);

        res.writeHead('200', {'Content-Type':'text/html;charset=utf8'});
        res.write('<h2>사용자 추가 중 오류 발생</h2>');
        res.write('<p>' + err.stack + '<p>');
        res.end();

        return;
      }

      if(addedUser){ // 결과 객체 있으면 성공 응답 전송
        console.dir(addedUser);
        console.log('inserted' + addedUser.affectedRows + 'rows');

        var insertId = addedUser.insertId;
        console.log('추가한 레코드의 아이디 : ' + insertId);

        res.writeHead('200', {'Content-Type':'text/html;charset=utf8'});
        res.write('<h2>사용자 추가 성공 !!!</h2>');
        res.end();
      }else{
        res.writeHead('200', {'Content-Type':'text/html;charset=utf8'});
        res.write('<h2>사용자 추가 실패...</h2>');
        res.end();
      }
    });
  }else{ // DB 객체가 초기화되지 않은 경우 실패 응답 전송
    res.writeHead('200', {'Content-Type':'text/html;charset=utf8'});
    res.write('<h2>DB Connection Fail...</h2>');
    res.end();
  }
});

// 사용자 리스트 라우터 함수
router.route('/process/listuser').post(function(req,res){
  console.log('/process/listuser 호출됨');

  // DB 객체가 초기화 된 경우, 모델 객체의 findAll 메소드 호출
  if(database){
    // 1. 모든 사용자 검색
    UserModel.findAll(function(err,results){
      // 오류가 발생했을 때 클라이언트 오류 전송
      if(err){
        console.error('사용자 리스트 조회 중 오류 발생 : ' + err.stack);

        res.writeHead('200', {'Content-Type':'text/html; charset=utf8'});
        res.write('<h2>사용자 리스트 조회 중 오류 발생</h2>');
        res.write('<p>' + err.stack + '</p>');
        res.end();

        return;
      }

      if(results){ // 결과 객체가 있으면 리스트 전송
        console.dir(results);

        res.writeHead('200', {'Content-Type':'text/html; charset=utf8'});
        res.write('<h2>사용자 리스트</h2>');
        res.write('<div><ul>');

        for(var i = 0; i < results.length; i++){
          var curId = results[i]._doc.id;
          var curName = results[i]._doc.name;
          var curPass = results[i]._doc.hashed_password;
          res.write(' <li>#' + i + ' : ' + curId + ', ' + curName + ', ' + curPass + '</li>');
        }
        res.write('</ul></div>');
        res.end();
      }else{ // 결과 개게가 없으면 실패 응답 전송
        res.writeHead('200', {'Content-Type':'text/html; charset=utf8'});
        res.write('<h2>사용자 리스트 실패</h2>');
        res.end();
      }
    });
  }else{ // DB 객체가 초기화되지 않았을 때 실패 응답 전송
    res.writeHead('200', {'Content-Type':'text/html; charset=utf8'});
    res.write('<h2> DB 연결 실패</h2>');
    res.end();
  }
});

// User Schema 및 Model 객체 생성
function createUserSchema(){
  userSchema = mongoose.Schema({
    id:{type:String, required:true,unique:true},
    hashed_password:{type:String, required:true, 'default': ' '},
    salt:{type:String, required:true},
    name:{type:String, index:'hased', 'default':''},
    age:{type:Number, 'default':-1},
    created_at:{type:Date, index:{unique:false},'defalut':Date.now},
    updated_at:{type:Date, index:{unique:false}, 'defalut':Date.now}
  });

  userSchema
    .virtual('password')
    .set(function(password){
      this._password = password;
      this.salt = this.makeSalt();
      this.hashed_password = this.encryptPassword(password);
      console.log('virtual password 설정함 : ' + this.hashed_password);
    })
    .get(function(){return this.password;});

    userSchema.method('encryptPassword', function(plainText, inSalt){
      if(inSalt){
        return crypto.createHmac('sha1', inSalt).update(plainText).digest('hex');
      }else{
        return crypto.createHmac('sha1', this.salt).update(plainText).digest('hex');
      }
    });

    userSchema.method('makeSalt', function(){
      return Math.round((new Date().valueOf()*Math.random())) + '';
    });

    userSchema.method('authenticate', function(plainText, inSalt, hashed_password){
      if(inSalt){
        console.log('authenticate 호출됨 : %s ->  %s : %s', plainText,
                    this.encryptPassword(plainText, inSalt), hashed_password);
        return this.encryptPassword(plainText, inSalt) == hashed_password;
      }else{
        console.log('authenticate 호출됨 : %s ->  %s : %s', plainText,
                    this.encryptPassword(plainText), this.hashed_password);
        return this.encryptPassword(plainText) == this.hashed_password;
      }
    });

    userSchema.path('id').validate(function(id){
      return id.length;
    }, 'id 칼럼의 값이 없습니다.');
    userSchema.path('name').validate(function(name){
      return name.length;
    }, 'name 칼럼의 값이 없습니다.');

    userSchema.static('findById', function(id, callback){
      return this.find({id:id}, callback);
    });
    userSchema.static('findAll', function(callback){
      return this.find({}, callback);
    });

    UserModel = mongoose.model('user5',userSchema);

}

// 사용자를 인증하는 함수
/*
var authUser = function(database, id, password, callback){
  console.log('authUser 호출됨');

  // 1. 아이디를 사용해 검색
  UserModel.findById(id, function(err,results){
    if(err){
      callback(err,null);
      return;
    }
    console.log('아이디 [%s]로 사용자 검색 결과', id);
    console.dir(results);

    if(results.length > 0){
      console.log('아이디와 일치하는 사용자 찾음');

      // 2. 비밀번호를 확인
      var user = new UserModel({id : id});
      var authenticated = user.authenticate(password, results[0]._doc.salt,
                                            results[0]._doc.hashed_password);

      if(authenticated){
        console.log('비밀번호 일치함');
        callback(null, results);
      }else{
        console.log('비밀번호 불일치');
        callback(null,null);
      }
    }else{
      console.log("아이디와 일치하는 사용자를 찾지못함");
      callback(null,null);
    }
  });

};
*/
// mySQL Database version
var authUser = function(id, password, callback){
  console.log('authUser 호출됨');
  pool.getConnection(function(err,conn){
    if(err){
      if(conn){
        conn.release();
      }
      callback(err,null);
      return;
    }
    console.log('DB Connection Thread Id : ' + conn.threadId);

    var columns = ['id','name','age'];
    var tablename = 'users';

    // SQL 실행
    var exec = conn.query("select ?? from ?? where id = ? and password = ?",
                          [columns, tablename, id, password], function(err,rows){
      conn.release();
      console.log('실행 대상 SQL : ' + exec.sql);
      console.dir(rows);
      if(rows.length > 0){
        console.log('ID : [%s], Pass : [%s] 가 일치 하는 사용자 찾음', id, password);

        callback(null, rows);
      }else{
        console.log("일치하는 사용자를 찾지 못함");
        callback(null,null);
      }
    });
  });
};

// 사용자 추가 함수
/* Mongo Database vertion
var addUser = function(database, id, password, name, callback){
  console.log('addUser 호출됨 : ' + id + ' , ' + password + ' , ' + name);

  // UserModel의 인스턴스 생성
  var user = new UserModel({"id":id, "password":password, "name":name});

  // save()로 저장
  user.save(function(err,addedUser){
    if(err){
      callback(err,null);
      return;
    }
    console.log("사용자 데이터 추가함");
    callback(null, addedUser);
  });
};
*/

// mySQL Database vertion
var adduser = function(id, name, age, password, callback){
  console.log('Add user 호출됨');

  pool.getConnection(function(err, conn){
    if(err){
      if(conn){
        conn.release(); // 반드시 해제 해야함
      }
      callback(err, null);
      return;
    }
    console.log('DB Connection 스레드 ID : ' + conn.threadId);

    // 데이터를 객체로 만듭니다
    var data = {id:id, name:name, age:age, password:password};

    // SQL문 실행
    var exec = conn.query('insert into users set ?', data, function(err,results){
      conn.release(); // 반드시 해제
      console.log('실행 대상 SQL : ' + exec.sql);
      console.log('Result : ');
      console.dir(results);
      console.log('Data : ');
      console.dir(data);
      if(err){
        console.log('SQL 실행시 오류 발생');
        console.dir(err);

        callback(err,null);

        return;
      }
      callback(null,results);
    });
  });
};


// 라우터 객체 등록
app.use('/', router);

// 404 오류 처리
var errorHandler = expressErrorHandler({
  static: {
    '404': './public/404.html'
  }
});

app.use(expressErrorHandler.httpError(404));
app.use(errorHandler);

// ==== 서버 시작 ====
http.createServer(app).listen(app.get('port'), function(){
  console.log('서버가 시작되었습니다. Port : ' + app.get('port'));

  // DB 연결
  //connectDB();

});
