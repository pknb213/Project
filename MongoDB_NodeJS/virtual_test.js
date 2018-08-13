var mongodb = require('mongodb');
var mongoose = require('mongoose');

var database;
var userSchema;
var userModel;

function connectDB(){
  var databaseUrl = 'mongodb://localhost:27017/local';

  mongoose.connect(databaseUrl);
  database = mongoose.connection;

  database.on('error', console.error.bind(console, 'mongoose connection Error'));
  database.on('open', function(){
    console.log('DB에 연결되었습니다. : ' + databaseUrl);
    createUserSchema();
    doTest();
  });
  database.on('disconnected', connectDB);
}

function createUserSchema(){
  userSchema = mongoose.Schema({
    id:{type:String, required:true,unique:true},
    name:{type:String, index:'hased', 'default':''},
    age:{type:Number, 'default':-1},
    created_at:{type:Date, index:{unique:false},'defalut':Date.now},
    updated_at:{type:Date, index:{unique:false}, 'defalut':Date.now}
  });

  userSchema
    .virtual('info')
    .set(function(info){
      var splitted = info.split(' ');
      this.id = splitted[0];
      this.name = splitted[1];
      console.log('virtual info 설정함 : %s, %s', this.id, this.name);
    })
    .get(function(){return this.id + ' ' + this.name;});

  console.log('UserSchema 정의함');

  userModel = mongoose.model('users4', userSchema);
  console.log('UserModel 정의함');
}

function doTest(){
  var user = new userModel({'info' : 'test05 반도리'});

  user.save(function(err){
    if(err){
        console.error('Do Test 중 오류 발생 : ' + err.stack);
        res.writeHead('200', {'Content-Type':'text/html; charset=utf8'});
        res.write('<h2>Do Test 중 오류 발생</h2>');
        res.write('<p>' + err.stack + '</p>');
        res.end();
        return;
      }
    console.log('[Do Test Executing] 사용자 데이터를 추가함');
    findAll();
  });
  console.log('info 속성에 값 할당함');
  console.log('id : %s, name : %s', user.id, user.name);
}

function findAll(){
    userModel.find({}, function(err,result){
      if(err){
        console.error('사용자 찾기 중 오류 발생 : ' + err.stack);
        res.writeHead('200', {'Content-Type':'text/html; charset=utf8'});
        res.write('<h2>사용자 찾기 중 오류 발생</h2>');
        res.write('<p>' + err.stack + '</p>');
        res.end();
        return;
      }
      if(result){
        for(var i=0; i<result.length;i++){
          console.log('[Find All] 조회된 user 문서 객체 #%d -> id : %s, name : %s',
                      i, result[i]._doc.id, result[i]._doc.name);
        }
      }
    });
}


connectDB();
