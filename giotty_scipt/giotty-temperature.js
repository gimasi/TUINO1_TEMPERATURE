var _data = raw_data;
var _params = schema;

var payload = _data.payload;


function hexToBytes(hex) {
    for (var bytes = [], c = 0; c < hex.length; c += 2)
        bytes.push(parseInt(hex.substr(c, 2), 16));
    return bytes;
}

log("Starting");
log( payload );
byte_data = hexToBytes( payload );
log( _data );

save_time_series = {}

if (byte_data[0] == 0x02 ) {
  var temperature;

  // read sensor temperature
  temperature = byte_data[1] << 8;
  temperature = temperature + byte_data[2];
    temperature = temperature / 100;
          
    log("Temperature=");
    log(temperature);
  
    _params.temperature.value = temperature; 
}

update_schema = _params;