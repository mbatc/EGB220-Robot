#ifndef RoadRunner_h__
#define RoadRunner_h__

#include "PIDController.h"
#include "SensorArray.h"
#include "Motors.h"

void InitRoadRunner();

/**
 * This function is called when a line is detected.
 * 
 * The position of the line along the sensor array is passed as a parameter.
 **/
void OnLineDetected(float linePos, unsigned long detectedMillis);

/**
 * This function is called when no line is detected
 **/
void OnLineMissing(int missingMillis, unsigned long detectedMillis)

/**
 * This function is called when a horizontal line is detected.
 **/
void OnHorizontalDetected();

/**
 * This function is called when a track marker is detected.
 * 
 * The side which the marker was detected is passed as a paramter.
 **/
void OnMarker(MarkerSensor sensorSide);

/**
 * Set the speed of the motors using a value between 0 and 255 for each motor.
 **/
void SetMotorSpeed(int left, int right);

extern SensorArray   g_sensors;
extern PIDController g_controlSystem;
extern unsigned long g_prevTime = 0;
extern int m_leftMotorPin;
extern int m_rightMotorPin;

#endif