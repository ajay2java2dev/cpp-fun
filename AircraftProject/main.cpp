#include "iostream"
#include "Fighter.h"
#include "vector"

int main() {
    std::cout << "Hello, Aircraft project!" << std::endl;

    vector<Aircraft *> aircraftList;

    //Aircraft passengerAircraft1(2,6);
    aircraftList.push_back(new Aircraft(2,6));

    //Aircraft passengerAircraft2(4,200);
    aircraftList.push_back(new Aircraft(4,200));


    //Fighter fighterAircraft1(1,1, "Top Speed: Mach 1.2, Max Range: 400 Miles");
    aircraftList.push_back(new Fighter(1,1, "Top Speed: Mach 1.2, Max Range: 400 Miles"));

    //Fighter fighterAircraft2(2,2, "Top Speed: Mach 2, Max Range: 1200 Miles");
    aircraftList.push_back(new Fighter(2,2, "Top Speed: Mach 2, Max Range: 1200 Miles"));

    int p = 0; // this value would set from the file but change the value here to see the difference.

    if (p == 0) {
        for (int index = 0; index < aircraftList.size(); index++) {
            //aircraftList[index].printCharacteristics(); //this wont work now since its now Aircraft pointers

            aircraftList[index] -> printCharacteristics(); // this works since we are storing now Aircraft pointers
        }
    } else {
        //aircraftList[p-1].printCharacteristics();

        aircraftList[p-1] -> printCharacteristics();
    }


    return 0;
}