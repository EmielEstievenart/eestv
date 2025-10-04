#pragma once

#include <iostream>

namespace eestv
{
class WalkingRobot
{
public:
    void walk() { std::cout << "Robot is walking" << '\n'; }

    void move() { std::cout << "Moving the robot \n"; }
};

}
