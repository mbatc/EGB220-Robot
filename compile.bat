@echo off

arduino-cli core update-index
arduino-cli core install arduino:avr
arduino-cli core install JDLuck:avr
arduino-cli compile --fqbn JDLuck:avr:spark2 %1