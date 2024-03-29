#include <avr/wdt.h>
#include "DeviceDriverSet_xxx0.h"
#include "ApplicationFunctionSet_xxx0.cpp"
#include <Arduino.h>

extern DeviceDriverSet_Motor AppMotor;
extern Application_xxx Application_ConquerorCarxxx0;
extern MPU6050_getdata AppMPU6050getdata;

static bool debug_yaw = true;
const float adjust_threshold = 2.5;
const int lowest_speed = 40;

class SmartCar
{

private:
    // Call this after a turn is completed
    void updateYawReference()
    {
        AppMPU6050getdata.MPU6050_dveGetEulerAngles(&Yaw);
        yaw_So = Yaw; // Set the new reference yaw
    }

    int calculateGradualSpeed(float currentYaw, float targetYaw, int initialSpeed){
        // Calculate the remaining angle to turn
        float remainingAngle = abs(currentYaw - targetYaw);

        // Gradually reduce speed based on the remaining angle
        int speed = initialSpeed * (remainingAngle / 90); // Assuming 90 is the max turn angle
        if (speed < lowest_speed) speed = lowest_speed; // Enforce a minimum speed of 50

        // Turn logic
        return speed;
    }

    float closestMultipleOf90(float number) {
        return round(number / 90.0f) * 90.0f;
    }

    void recarlibrate(){
        this->stop();
        AppMPU6050getdata.MPU6050_calibration();
        this->updateYawReference();
    }


    void turnTillTarget(int speed, float target, bool turnLeft, bool (*condition)(float yaw, float target)){
        updateYawReference();
        AppMPU6050getdata.MPU6050_dveGetEulerAngles(&Yaw);
        while (condition(Yaw, target))
        {
            this->updateYawReference();
            // AppMPU6050getdata.MPU6050_dveGetEulerAngles(&Yaw);
            if (turnLeft) this->moveLeft(calculateGradualSpeed(Yaw, target, speed));
            else this->moveRight(calculateGradualSpeed(Yaw, target, speed));
            if (debug_yaw)
                Serial.println("Current Yaw: " + String(Yaw) + " | target: " + String(target));
        }

        this->stop();
    }

public:
    void init()
    {
        // Serial.begin(9600);
        AppMotor.DeviceDriverSet_Motor_Init();
        AppMPU6050getdata.MPU6050_dveInit();
        // delay(2000);
        AppMPU6050getdata.MPU6050_calibration();
    }

    void printAngle(){
        AppMPU6050getdata.MPU6050_dveGetEulerAngles(&Yaw);
        Serial.println("Current Angle: " + String(Yaw));
    }

    void moveForwardForSeconds(int speed, float ms){
        for (int i = 0; i < ms/10; i ++){
            this->moveForward(speed);
            delay(10);
        }
        this->stop();
    }

    void moveBackwardForSeconds(int speed, float ms){
        for (int i = 0; i < ms/10; i ++){
            this->moveBackward(speed);
            delay(10);
        }
        this->stop();
    }

    void moveForward(int speed)
    {
        // this->adjust(10);
        this->stop();
        updateYawReference();
        ApplicationFunctionSet_ConquerorCarMotionControl(
            ConquerorCarMotionControl::Forward,
            speed // 0-255
        );
    }

    void moveBackward(int speed)
    {
        this->stop();
        // this->adjust(speed);
        updateYawReference();
        ApplicationFunctionSet_ConquerorCarMotionControl(
            ConquerorCarMotionControl::Backward,
            speed // 0-255
        );
    }

    void moveLeft(int speed)
    {
        ApplicationFunctionSet_ConquerorCarMotionControl(
            ConquerorCarMotionControl::Left,
            speed // 0-255
        );
    }

    void turnLeft(int speed)
    {
        this->stop();
        AppMPU6050getdata.MPU6050_dveGetEulerAngles(&Yaw);
        float target = closestMultipleOf90(Yaw - 90);
        turnTillTarget(speed, target, true, [](float Yaw, float target) { return Yaw > target; });
        this->adjust(speed);
    }

    void moveRight(int speed)
    {
        ApplicationFunctionSet_ConquerorCarMotionControl(
            ConquerorCarMotionControl::Right,
            speed // 0-255
        );
    }

    void turnRight(int speed)
    {
        this->stop();
        AppMPU6050getdata.MPU6050_dveGetEulerAngles(&Yaw);
        float target = closestMultipleOf90(Yaw + 90);
        turnTillTarget(speed, target, false, [](float Yaw, float target) { return Yaw < target; });
        this->adjust(speed);
    }

    void adjust(int speed){
        
        this->stop();
        AppMPU6050getdata.MPU6050_dveGetEulerAngles(&Yaw);
        float target = closestMultipleOf90(Yaw);
        if (debug_yaw) Serial.println("Offset Angle: " + String(Yaw-target));
        // delay(5000);
        if (abs(Yaw - target) <= adjust_threshold) return;

        Serial.println("Adjust Target | Current Yaw: " + String(Yaw) + " | target: " + String(target));

        if (Yaw > target){ // too far right, turn left
            turnTillTarget(speed, target, true, [](float Yaw, float target) { return Yaw > target; });
        }else{ // too far left, turn right
            turnTillTarget(speed, target, false, [](float Yaw, float target) { return Yaw < target; });
        }
        this->stop();
        AppMPU6050getdata.resetYawAtIntervals();
    }

    void stop()
    {
        ApplicationFunctionSet_ConquerorCarMotionControl(
            ConquerorCarMotionControl::stop_it,
            0);
    }
};