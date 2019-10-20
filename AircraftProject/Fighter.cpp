//
// Created by Ajay on 10/20/19.
//

#include "Fighter.h"

Fighter::Fighter(int numOfEngines, int seatCapacity, string rangAndSpeed): Aircraft(numOfEngines,seatCapacity)
    ,rangeAndSpeedDescription(rangAndSpeed) {}

void Fighter::printCharacteristics() {
    cout << "This Aircraft has " << getNumOfEngines() << " engines, a seating capacity of " << getSeatCapactity()
         << " and its details are " << rangeAndSpeedDescription << endl;
}
