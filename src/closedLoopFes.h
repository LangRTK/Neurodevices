#ifndef closedLoopFes_h
#define closedLoopFes_h
#include "openLoopFes.h"
#include "SistemasdeControle/embeddedTools/sensors/sensorfusion.h"

LinAlg::Matrix<double> closedLoop_gyData(3,1);

volatile uint64_t closedLoop_Counter, closedLoopIMU_Counter; volatile bool closedLoop_flag = false;
volatile ControlHandler::PID<long double> pid[2]; volatile long double ref1, ref2;
esp_timer_create_args_t closedLoop_periodic_timer_args, IMUClosedLoop_periodic_timer_args;
esp_timer_handle_t closedLoop_periodic_timer = nullptr, IMUClosedLoop_periodic_timer = nullptr;

void IRAM_ATTR ClosedLoop_IMUDataLoop(void *param){
  LinAlg::Matrix<double> gyDataTemp;
  closedLoopIMU_Counter++;
  closedLoop_gyData += 0.05*(sensors.update()-closedLoop_gyData);

  if(closedLoopIMU_Counter==(int)param)
  {
    printf("stop\r\n"); //Print information
    ESP_ERROR_CHECK(esp_timer_stop(IMUClosedLoop_periodic_timer)); //Timer pause
    ESP_ERROR_CHECK(esp_timer_delete(IMUClosedLoop_periodic_timer)); //Timer delete
    IMUClosedLoop_periodic_timer = nullptr;
  }
}

void IRAM_ATTR closedLoopControlUpdate(void *param){
    closedLoop_Counter++;
    LinAlg::Matrix<double> U = dispositivo.performOneControlStep(ref1, ref2, closedLoop_gyData);

    std::stringstream ss;
    ss << closedLoop_Counter << " , " << closedLoop_gyData(0,0)  << " , " << closedLoop_gyData(1,0) << " , " << closedLoop_gyData(2,0) 
                             << " , " << U(0,0) << " , " << U(0,1) << "\r\n";
    client->write(ss.str().c_str());

  if(closedLoop_Counter==(int)param)
  {
    printf("stop\r\n"); //Print information
    ESP_ERROR_CHECK(esp_timer_stop(closedLoop_periodic_timer)); //Timer pause
    ESP_ERROR_CHECK(esp_timer_delete(closedLoop_periodic_timer)); //Timer delete
    closedLoop_periodic_timer = nullptr;
    closedLoop_flag = false;
  }
}

String closedLoopFesUpdate(void* data, size_t len) {
  char* d = reinterpret_cast<char*>(data); String msg,answer;
  for (size_t i = 0; i < len; ++i) msg += d[i];
  uint16_t index = msg.indexOf('?'); String op = msg.substring(0,index);
  msg = msg.substring(index+1,msg.length());
  LinAlg::Matrix<double> code = msg.c_str();

  if (op.toInt() == 3){
    Serial.print("Oeration 3, received data: "); Serial.println(msg);
    closedLoop_flag = true;

    openLoopFesInit();
    dispositivo.getPID(0).setParams(1,0.1,0); dispositivo.getPID(0).setSampleTime(1); dispositivo.getPID(0).setLimits(1.1,1.5);
    dispositivo.getPID(1).setParams(1,0.1,0); dispositivo.getPID(1).setSampleTime(1); dispositivo.getPID(1).setLimits(1.1,1.8);
    dispositivo.fes[0].setPowerLevel(0); 
    dispositivo.fes[1].setPowerLevel(0); 
    dispositivo.fes[2].setPowerLevel(0); 
    dispositivo.fes[3].setPowerLevel(0); 

    closedLoop_periodic_timer_args.callback = &closedLoopControlUpdate;
    closedLoop_periodic_timer_args.name = "closedLoopControlUpdate";
    closedLoop_periodic_timer_args.arg = (void*)msg.toInt();
    ESP_ERROR_CHECK(esp_timer_create(&closedLoop_periodic_timer_args, &closedLoop_periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(closedLoop_periodic_timer, 10000));
    closedLoop_Counter = 0;

    sensors.init();
    IMUClosedLoop_periodic_timer_args.callback = &ClosedLoop_IMUDataLoop;
    IMUClosedLoop_periodic_timer_args.name = "imuSendInit";
    IMUClosedLoop_periodic_timer_args.arg = (void*)msg.toInt();
    ESP_ERROR_CHECK(esp_timer_create(&IMUClosedLoop_periodic_timer_args, &IMUClosedLoop_periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(IMUClosedLoop_periodic_timer, 1000));
    closedLoopIMU_Counter = 0;

    answer += "Loop para estimulacao em malha fechada\r\n";
  }
  else
    answer += "";
  return answer;
}

String closedLoopFesReferenceUpdate(void* data, size_t len) {
  char* d = reinterpret_cast<char*>(data); String msg,answer;
  for (size_t i = 0; i < len; ++i) msg += d[i];
  uint16_t index = msg.indexOf('?'); String op = msg.substring(0,index);
  msg = msg.substring(index+1,msg.length());
  LinAlg::Matrix<double> code = msg.c_str();

  if (op.toInt() == 4){
    Serial.print("Oeration 4, received data: "); Serial.println(msg);
    closedLoop_flag = true;
    ref1 = code(0,0); ref2 = code(0,1);
    answer += "Referencias dos controladores PID atualizadas\r\n";
  }
  else
    answer += "";
  return answer;
}

#endif