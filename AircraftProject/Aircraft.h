//
// Created by Ajay on 10/20/19.
//

#ifndef AIRCRAFTPROJECT_AIRCRAFT_H
#define AIRCRAFTPROJECT_AIRCRAFT_H

#endif //AIRCRAFTPROJECT_AIRCRAFT_H

#include <string>
#include <iostream>


using namespace std;


class Aircraft {
private:
    int numOfEngines;
    int seatCapacity;

public:
    Aircraft(int numOfEngines, int seatCapacity);

    //setter/getters
    void setNumOfEngines(int numOfEgnines);
    void setSeatCapacity(int seatCapacity);
    int getNumOfEngines();
    int getSeatCapactity();

    //print details -- Note in base class this is declared virtual - meaning this method is ready to be overridden
    // Read more : https://www.geeksforgeeks.org/virtual-function-cpp/
    virtual void printCharacteristics();

};