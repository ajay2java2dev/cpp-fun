//
// Created by Ajay on 10/20/19.
//

#ifndef AIRCRAFTPROJECT_FIGHTER_H
#define AIRCRAFTPROJECT_FIGHTER_H

#endif //AIRCRAFTPROJECT_FIGHTER_H

#include "Aircraft.h"

class Fighter: public Aircraft {

private:
    string rangeAndSpeedDescription;

public:
    Fighter(int numOfEngines, int seatCapacity, string rangAndSpeed);

    void setSpeedAndRangeDesc(string speedAndRangeDesc);
    string getSpeedAndRangeDesc();

    void printCharacteristics();
};
