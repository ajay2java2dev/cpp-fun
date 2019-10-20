//
// Created by Ajay on 10/20/19.
//
#include "Aircraft.h"

Aircraft::Aircraft(int numOfEngines, int seatCapacity) : numOfEngines(numOfEngines), seatCapacity(seatCapacity) {}

//implement what the setters/getters are suppose to do
void Aircraft:: setNumOfEngines(int numOfEngines) { numOfEngines = numOfEngines;}
void Aircraft:: setSeatCapacity(int seatCapacity) { seatCapacity = seatCapacity;}
int Aircraft:: getNumOfEngines() { return numOfEngines; }
int Aircraft:: getSeatCapactity() { return seatCapacity; }

void Aircraft::printCharacteristics() {
    cout << "This Aircraft has " << numOfEngines << " engines and a seating capacity of " << seatCapacity << endl;
}
