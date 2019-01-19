const i2c = require('i2c-bus');

const SENSOR_ADDR  = 0x23;
const CMD_Reset    = 0x07;
const CMD_Highmode = 0x11;
const CMD_Init     = 0x20;

function sleep(ms){
  return new Promise(resolve => {
    setTimeout(resolve, ms);
  })
}

 let i2c1 = i2c.openSync(1);

 const i2c_init = () => {
     if(i2c1.readByteSync(SENSOR_ADDR,CMD_Reset,1))
     {
       console.log("Failed the Reset register");
       i2c1 = i2c1.closeSync();
     }
     if(i2c1.readByteSync(SENSOR_ADDR,CMD_Highmode,1))
     {
       console.log("Failed the Continuous High Res mode2");
       i2c1 = i2c1.closeSync();
     }
     let a = i2c1.readByteSync(SENSOR_ADDR,CMD_Init,1);
     display();
 }

 const display = /*async*/ () => {
  //let buffer = new Buffer( [0,1]);
  //console.log(buffer);
  //let buf_size = buffer.length;
  //console.log(buf_size);

  //let scan = i2c1.scanSync(0x03,0x77);
  //console.log(scan);

  //await sleep(2000);

  const loop = setInterval( () => {
    if(i2c1.readByteSync(SENSOR_ADDR,CMD_Init,1))
    {
      console.log("Failed Reading the Data");
      i2c1 = i2c1.closeSync();
      clearIntserval(loop);
    }
    let val = i2c1.readWordSync(SENSOR_ADDR, CMD_Init);
    val = ((val * 10) + 5 ) / 12;
    val = Math.round(val);
    console.log("Lux : " + val);
  }, 500)
}

i2c_init();
