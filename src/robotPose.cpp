#include <robotPose.h>
#include <point.h>
#include <limits.h>
//#include <position.h>

using namespace std;

namespace ecn
{

RobotPose RobotPose::goalPose(0,0,0,0,0);
float RobotPose::KEFinal;
vector<float> RobotPose::intervalVector;
float RobotPose::goalRadius = Param::wheelVelocityMax*Param::bigTimeStep;
std::vector<float> RobotPose::radiusRegionLimitVector;
float RobotPose::distToParent(){
    return distance;
}

void RobotPose::setGoalPose(RobotPose _goalPose){
     goalPose = _goalPose;
     pair<float,float> linVel_angVel = robotVelocity(pairVel(goalPose.rightWheelVel, goalPose.leftWheelVel));
     float v = linVel_angVel.first;
     float w = linVel_angVel.second;
     KEFinal = (0.5)*Param::mass*pow(v,2) + (0.5)*Param::inertia*pow(w,2);
     goalPose.fillIntervalVector();
     goalPose.fillRadiusRegionLimitVector();
}



pair<float,float> RobotPose::robotVelocity(RobotPose::pairVel _vel_wheel){
    pair<float,float> returnVel;
    returnVel.first = ((Param::rWheel*_vel_wheel.right + Param::rWheel*_vel_wheel.left)/2); //*(M_PI/180);
    ///////
    returnVel.second = (Param::rWheel*_vel_wheel.right - Param::rWheel*_vel_wheel.left)/(Param::tGauge);
    return returnVel;
}

void RobotPose::print(const RobotPose &_parent)
{
//    std::cout << "******** ((())))))) " << std::endl;
//    std::cout << "parent: " << _parent << std::endl;
//    std::cout << "child: " << *this << std::endl;
//    std::cout << "timestep: " << this->timeStep << std::endl;

    Position tempPosition;
    pair<float,float> linVel_angVel = robotVelocity(pairVel(this->rightWheelVel, this->leftWheelVel));

    float timeIndex = 0;
    while(timeIndex < this->timeStep){
            timeIndex = timeIndex + Param::obstacleCheckInterval;
            tempPosition = getNewStepPosition(_parent , linVel_angVel, timeIndex );
            int xTemp = scaleAndFloor(tempPosition.x);// floor(x/scaleFactor);
            int yTemp = scaleAndFloor(tempPosition.y);// floor(y/scaleFactor);
            Point::maze.passThrough(xTemp,yTemp);

//            std::cout << "********" << std::endl;
//             std::cout << "timeIndex: " << timeIndex << std::endl;
//            std::cout <<"tempPosition"  << tempPosition << std::endl;
        }

////////////////////////////////////////////////////////////////////////////////////////////////////
//    tempPosition = getNewStepPosition(*this , linVel_angVel, this->timeStep);
//    int xTemp = scaleAndFloor(tempPosition.x);// floor(x/scaleFactor);
//    int yTemp = scaleAndFloor(tempPosition.y);// floor(y/scaleFactor);
//    Point::maze.passThrough(xTemp,yTemp);

}

bool isChildHealthy(const RobotPose &_parent, Position _tempChildPosition){

}


bool RobotPose::validPathPosition(const RobotPose &_startPosition, Position _tempChildPosition, pair<float,float> _linVel_angVel, float _timeStep ){

    //NOTE: _time represents the time elapsed from start pos
    float timeIndex = 0;
    // returns false if either the possible child position is blocked OR if the path to that child is blocked
    if(!_tempChildPosition.isFree()){
        return false;
    }
    else{
        while(timeIndex < _timeStep){
            timeIndex = timeIndex + Param::obstacleCheckInterval;
            _tempChildPosition = getNewStepPosition(_startPosition, _linVel_angVel, timeIndex);
            if(!_tempChildPosition.isFree()) return false;
        }
    }
    return true;
}

Position RobotPose::getNewStepPosition(const RobotPose &_startPosition, pair<float,float> _linVel_angVel, float _time ){

    // pair.first = linear velocity
    float v = _linVel_angVel.first;
    float w = _linVel_angVel.second;

    float thetaPos = Position::thetaBound(_startPosition.theta + w*_time);

    float xPos ;
    float yPos ;

    //NOTE: _time represents the time elapsed from start pos
    if(w != 0){
        xPos = _startPosition.x+ ( v*sin( w*_time) )/w;
        yPos = _startPosition.y+ ( v*(1-cos( w*_time)) )/w;
    }
    else{
        xPos = _startPosition.x+ v*cos(thetaPos)*_time;
        yPos = _startPosition.y+ v*sin(thetaPos)*_time;
    }

    // pair.second = angular velocity
    return Position(xPos, yPos, thetaPos);

}

float RobotPose::distTravelled(pairVel _parentVelocityPair, pairVel _wheelVelocityPair, float _timeStep){
    ////// Distance travelled when using the wheelspin heuristic
    ////// returns the distance travelled by the robot
    ////// distance taken as the average of wheel rotations during motion from parent to child
    ////// in 1 time-step
    return (abs((_wheelVelocityPair.left)*_timeStep) + abs((_wheelVelocityPair.right)*_timeStep))/2;

    ////////////////////////////////////////////////////////////////////////
    // Distance travelled when using the energy heuristic
//    pair<float,float> linVel_angVel = robotVelocity(_wheelVelocityPair);
//    pair<float,float> parent_linVel_angVel = robotVelocity(_parentVelocityPair);

//    float linDistance = (linVel_angVel.first)*_timeStep;
//    float thetaDisp = (linVel_angVel.second)*_timeStep;
//    float workDoneForce = force*linDistance;
//    float workDoneMoment = moment*thetaDisp;

////    float KECurrent = (0.5)*mass*pow(linVel_angVel.first,2) + (0.5)*inertia*pow(linVel_angVel.second,2);
////    float KEParent = (0.5)*mass*pow(parent_linVel_angVel.first,2) + (0.5)*inertia*pow(parent_linVel_angVel.second,2);

////    float delKE = abs(KECurrent-KEParent);
////    return delKE + workDoneForce + workDoneMoment;
//    return workDoneForce + workDoneMoment;

    /////////////////////////////////////////////////////////////////////////////
    ////// Euclidean distance
//        pair<float,float> linVel_angVel = robotVelocity(_wheelVelocityPair);
//        float linDistance = (linVel_angVel.first)*_timeStep;
//        return linDistance;

}

void RobotPose::fillIntervalVector(){
    int i = Param::intervalCount;
    float smallestStep = Param::bigTimeStep/Param::intervalCount;
    while(i > 0){
        intervalVector.push_back(float(i)*smallestStep);
        i = i - 1;
    }
}

////// HERE
pair<float,float> RobotPose::calcTimeStep(float _hDistance){
    float noOfWheelRevolution = _hDistance/M_PI;
////    std::cout << "noOfWheelRevolution: " << noOfWheelRevolution << std::endl;
//    pair<float,float> result;
//    if(noOfWheelRevolution >= 12){
//        result.first = 0;
//        result.second = intervalVector[0];
//        return result;
////        return bigTimeStep;
//    }
//    else if (noOfWheelRevolution < 12 && noOfWheelRevolution >= 8){
//        result.first = 1;
//        result.second = intervalVector[1];
//        return result;
////        return ((3*bigTimeStep)/4);
//    }
//    else if (noOfWheelRevolution < 8 && noOfWheelRevolution >= 4 ){
//        result.first = 2;
//        result.second = intervalVector[2];
//        return result;
////        return ((2*bigTimeStep)/4);
//    }
//    else {
//        result.first = 3;
//        result.second = intervalVector[3];
//        return result;
////        return ((1*bigTimeStep)/4);
//}
    /////////////////////From Goal Radius Approach /////////////////////////////////////////
    // returns the timestep corresponding to the heuristic limit
    pair<float,float> result;
    int index = 0;
    while(index < Param::intervalCount){
        if(noOfWheelRevolution >= radiusRegionLimitVector[index]){
            result.first = index;
            result.second = intervalVector[index];
            return result;
        }
        index++;
    }
    result.first = Param::intervalCount - 1;
    result.second = intervalVector[Param::intervalCount - 1];
    return result;
}

vector<RobotPose::pairVel>  RobotPose::generateVelChoices(){
    // creates 2 different vector containing the possible choices that the two wheels can have.
    float scale = -1.0*Param::velocityIncrementLimit;
    vector<float> rightWheelVel_choice;
    vector<float> leftWheelVel_choice;
    float temp_rightWheelVel;
    float temp_leftWheelVel;
    while(scale <= Param::velocityIncrementLimit ){
        temp_rightWheelVel = rightWheelVel+scale;
        temp_leftWheelVel = leftWheelVel+scale;

        if(temp_rightWheelVel > Param::wheelVelocityMax || temp_rightWheelVel < Param::wheelVelocityMin); // do nothing
        else {
            rightWheelVel_choice.push_back(temp_rightWheelVel);
        }
        if(temp_leftWheelVel > Param::wheelVelocityMax || temp_leftWheelVel < Param::wheelVelocityMin); // do nothing
        else {
            leftWheelVel_choice.push_back(temp_leftWheelVel);
        }
        scale += Param::velocityIncrementStep; // increase the scale by velocity step
    }

    // need to create the wheelVelVectorPair which can be directly be used to create valid childrens
    vector<RobotPose::pairVel> ret; // return variable

    for(int i =0; i < rightWheelVel_choice.size(); i++){
        for(int j=0; j < leftWheelVel_choice.size(); j++){
            ret.push_back(RobotPose::pairVel(rightWheelVel_choice[i],leftWheelVel_choice[j]));
        }
    }
    return ret;
}
//// TO-DO: check for timestep during child node creation
std::vector<RobotPose::RobotPosePtr> RobotPose::children()
{
    // this method should return  all positions reachable from this one
    std::vector<RobotPosePtr> generated;

    std::vector<RobotPose> tempChildGenerated; // vector to store the possible child poses
    std::vector<float> tempChildHeuristic; // vector to store corresponding heuristic values of the children
    float bestChildHeuristic = FLT_MAX; // float to store the best possible child

    // based on the vector of acceptable velocity generate different children
    vector<pairVel> velChoices;
    velChoices = generateVelChoices();

    // for all the possible generated combination create children
    for(int i=0; i < velChoices.size(); i++){

        // step1. calculate lin-vel and ang-vel
        pair<float,float> tempVelWheel = robotVelocity(velChoices[i]);

        // step1.2 find the heuristic distance to this goal pose
        float heuristicDistance = this->h(goalPose,true);

        // step1.3 calculate the time step based on the heuristic
        float tempTimeStep = calcTimeStep(heuristicDistance).second;
        float tempTimeIndex = calcTimeStep(heuristicDistance).first;

        // step2. calculate the (x,y,theta) of the possible child node
         Position tempChildPosition = getNewStepPosition(*this,tempVelWheel, tempTimeStep);
        
         //step3. check if the child node is free(valid) or not
         if(!validPathPosition(*this,tempChildPosition,tempVelWheel, tempTimeStep)){ // check with smaller timestep if not free
             while(tempTimeIndex < (Param::intervalCount - 1) && !validPathPosition(*this,tempChildPosition,tempVelWheel, tempTimeStep)){
                tempTimeIndex = tempTimeIndex + 1;
                tempTimeStep = intervalVector[tempTimeIndex];
             }
             if(!validPathPosition(*this,tempChildPosition,tempVelWheel, tempTimeStep)){
                 continue;
             }
             else{
                 tempChildPosition = getNewStepPosition(*this,tempVelWheel, tempTimeStep);
             }

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//             continue;
         }

        // step4. calculate distance of the child from this node(parent)
        float tempdist = distTravelled(pairVel(this->rightWheelVel, this->leftWheelVel), velChoices[i], tempTimeStep);

        // step5. robot pose of the temporary child
        RobotPose tempChildPose = RobotPose(velChoices[i],tempChildPosition,tempdist,tempTimeStep);
        float tempChildHValue = tempChildPose.h(goalPose,true); // heuristic value of the child (health assessment)
        if(bestChildHeuristic > tempChildHValue  ){
            bestChildHeuristic = tempChildHValue; // updating the best heursitic
        }
        tempChildHeuristic.push_back(tempChildHValue);
        tempChildGenerated.push_back(tempChildPose);
    }

    for(int i =0; i < tempChildGenerated.size(); i++){
        if(tempChildHeuristic[i] < Param::childHealthLimit*bestChildHeuristic){
            generated.push_back(std::make_unique <RobotPose>(tempChildGenerated[i]));
        }

    }


    return generated;
}


bool RobotPose::is(const RobotPose &_other)
{
    // check of two RobotPose are same or not
    Position othePosition = _other;
    Position* nodePosition = this;

    bool clauseA = nodePosition->is(othePosition) ;
    bool clauseB = ((_other.leftWheelVel - Param::wheelVelocityTolerance) <= this->leftWheelVel)
            && (this->leftWheelVel <= (_other.leftWheelVel + Param::wheelVelocityTolerance)) ;
    bool clauseC = ((_other.rightWheelVel - Param::wheelVelocityTolerance) <= this->rightWheelVel)
            && (this->rightWheelVel <= (_other.rightWheelVel + Param::wheelVelocityTolerance)) ;

    if(clauseA && clauseB && clauseC)
    {
        return 1;
    }
    return 0;
}


float RobotPose::h(const RobotPose &_goal, bool useManhattan)
{
    // Wheel spin heuristic
    float thetaDispTemp = _goal.theta - theta;
    float thetaDisp = min(abs(thetaDispTemp), float(2*M_PI - abs(thetaDispTemp)));
    float wheelSpinTheta = thetaDisp*Param::tGauge/(2*Param::rWheel);

    float x_dist = (x - _goal.x);
    float y_dist = (y - _goal.y);

    float wheelSpinStraight = sqrt(pow(x_dist,2) + pow(y_dist,2))/Param::rWheel;

    return Param::straightSpinWeight*wheelSpinStraight + Param::thetaSpinWeight*wheelSpinTheta;



//////////////////////////////////////////////////////////////////////
    // Euclidean distance

//        float x_dist = (x - _goal.x);
//        float y_dist = (y - _goal.y);

//        float distance = sqrt(pow(x_dist,2) + pow(y_dist,2));
//        return distance;

//////////////////////////////////////////////////////////////////////
    // Energy based heuristic
//    float thetaDispTemp = _goal.theta - theta;
//    float thetaDisp = min(abs(thetaDispTemp), float(2*M_PI - abs(thetaDispTemp)));

//    float x_dist = (x - _goal.x);
//    float y_dist = (y - _goal.y);

//    float linDistance = sqrt(pow(x_dist,2) + pow(y_dist,2));

//    float workDoneForce = force*linDistance;
//    float workDoneMoment = moment*thetaDisp;

//    pair<float,float> linVel_angVel = robotVelocity(pairVel(this->rightWheelVel, this->leftWheelVel));


/////
////    float KECurrent = (0.5)*mass*pow(linVel_angVel.first,2) + (0.5)*inertia*pow(linVel_angVel.second,2);
////    float delKE = abs(KEFinal - KECurrent);

////    return delKE + workDoneForce + workDoneMoment;
//    return workDoneForce + workDoneMoment;

}

void RobotPose::fillRadiusRegionLimitVector(){
/////////////////////////From Goal Radius Approach /////////////////////////////////////////
//    float smallestStep = Param::goalRadiusMultiplier*goalRadius;
//    for(int i = Param::intervalCount - 1; i >= 0; i--) {
//        radiusRegionLimitVector.push_back(float(i)*smallestStep);
//    }
 /////////////////////////////////////////////////////////////////////////
        float smallestStep = Param::highestThreshold/(Param::intervalCount -1);
        for(int i = Param::intervalCount - 1; i >= 0; i--) {
            radiusRegionLimitVector.push_back(float(i)*smallestStep);
        }
}



} // END of namespace
