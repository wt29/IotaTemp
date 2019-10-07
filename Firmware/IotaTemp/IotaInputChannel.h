#ifndef IotaInputChannel_h
#define IotaInputChannel_h

enum channelTypes:byte {channelTypeUndefined=0,
                        channelTypeTemperature=1,
                        channelTypeHumidity=2};
						
union dataBuckets {
	  struct {
        double value1;
        double value2;
        double accum1;
        double accum2;
		    uint32 timeThen;
        
      };
      struct {
        double  temperature;
        double  humidity;
      };

      dataBuckets()
      :value1(0)
      ,value2(0)
      ,accum1(0)
      ,accum2(0)
      ,timeThen(millis()){}
      };
	
class IotaInputChannel {
  public:
    dataBuckets  dataBucket;
    char*        _name;                       // External name
	  char* 		   _model;					            // Future support for other sensors
    uint8_t      _channel;                    // Internal identifying number
    channelTypes _type;                       // Humidity, Temp maybe windspeed?.
	  bool		     _active;	
    bool         _reversed;                   // True if negative power in being made positive (reversed CT)
    bool         _signed;                     // True if channel should not be reversed when negative (net metered main)
    uint8_t      _tchannel;
    
    IotaInputChannel(uint8_t channel)
    :_name(nullptr)
	  ,_model(nullptr)
    ,_channel(channel)
    ,_active(false)
    ,_reversed(false)
    ,_signed(false)
   {}
	~IotaInputChannel(){
		
	}

	void reset(){
    delete[] _name;
	  _name = nullptr;
    delete[] _model;
	  _model = nullptr;
	  _active = false;
    _reversed = false;
    _signed = false;
	}
	
  void ageBuckets(uint32_t timeNow) {
		double elapsedHrs = double((uint32_t)(timeNow - dataBucket.timeThen)) / 3600000E0;
		dataBucket.accum1 += dataBucket.value1 * elapsedHrs;
		dataBucket.accum2 += dataBucket.value2 * elapsedHrs;
		dataBucket.timeThen = timeNow;    
  }

  void setTemperature(float temp){
		if(_type != channelTypeTemperature) return;
		ageBuckets(millis());
		dataBucket.temperature = temp;
  }
	
  void setHumidity(float humid){
		if(_type != channelTypeTemperature) return;
		ageBuckets(millis());
		dataBucket.humidity = humid;
  }
		
	bool isActive(){return _active;}
	void active(bool _active_){_active = _active_;}
	
	double getTemperature(){return dataBucket.temperature;}	
	double getHumidity(){return dataBucket.humidity;}	
	
  private:
};
#endif
