#include <Arduino.h>
#include <Wire.h>

// Easy PID Motor Control i2c communication library
#include <EPMC_I2C_Client.h>

uint8_t i2c_address = 0x55; // set this address to the same address you have during setup via the GUI app
EPMC_I2C_Client controller(i2c_address);
// OR
// EPMC_I2C_Client controller(i2c_address, SupportedNumOfMotors::TWO);

float targetVel[] = {1.571, 3.142, 6.284}; // [4 rev/sec, 2 rev/sec, 1 rev/sec]
float vel = targetVel[1]; // in rad/sec
float v = 0.0;

float pos0, pos1;
float vel0, vel1;

bool sendHigh = true;

long readTime;
long readTimeInterval = 20; // millisec (50Hz)

long cmdTime;
long cmdTimeInterval = 5000; // millisec

void setup()
{
  // setup serial communication to print result on serial minitor
  Serial.begin(115200);

  // start i2c communication
  Wire.begin();
  bool connect_success = controller.begin(); // takes about 3 to 4 secs to connect
  if (!connect_success) {
    Serial.println("Error Connecting to EPMC. Probably due to Supported NumOfMotor Mismatch");
    while(true);
  }

  controller.clearDataBuffer();
  controller.writeSpeed(v, v);

  int cmd_vel_timeout = 10000; // 0 to deactivate.
  controller.setCmdTimeout(cmd_vel_timeout); // set motor command velocity timeout
  cmd_vel_timeout = controller.getCmdTimeout(); // get the stored command velocity timeout
  Serial.print("motor command vel timeout in ms: ");
  Serial.println(cmd_vel_timeout);

  sendHigh = true;

  readTime = millis();
  cmdTime = millis();
}

void loop()
{
  if ((millis() - cmdTime) >= cmdTimeInterval)
  {
    if (sendHigh)
    {
      v = vel;
      controller.writeSpeed(v, v);
      vel *= -1;
      sendHigh = false;
    }
    else
    {
      v = 0.0;
      controller.writeSpeed(v, v);
      sendHigh = true;
    }
    cmdTime = millis();
  }

  if ((millis() - readTime) >= readTimeInterval)
  {
    // controller.writeSpeed(v, v);
    controller.readPos(pos0, pos1);
    controller.readVel(vel0, vel1);

    // Print results
    Serial.println("-----------------------------------");
    Serial.print("Motor 0: ");
    Serial.print(pos0, 2); Serial.print("\t"); Serial.println(vel0, 2);
    Serial.print("Motor 1: ");
    Serial.print(pos1, 2); Serial.print("\t"); Serial.println(vel1, 2);
    Serial.println("------------------------------------");
    
    // Serial.println();

    readTime = millis();
  }
}