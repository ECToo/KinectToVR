#pragma once
#include "stdafx.h"
#include <iostream>
#include "KinectSettings.h"
#include "KinectJoint.h"
#include "IETracker.h"
#include <vrinputemulator.h>
#include <SFML/System/Vector3.hpp>
#include <openvr_math.h>
#include "VectorMath.h"

enum class KinectDeviceRole {
    Unassigned,
    LeftFoot,
    RightFoot,
    Hip,
    KinectSensor,
    LeftHand,
    RightHand
};

class KinectTrackedDevice {
public:
    KinectTrackedDevice(
        vrinputemulator::VRInputEmulator& inputEmulator,
        KVR_Joint::KinectJointType j0,
        KVR_Joint::KinectJointType j1,
        KinectDeviceRole r)
        :
        inputEmulatorRef(inputEmulator),
        joint0(j0),
        joint1(j1),
        trackedPositionVROffset({ 0,0,0 }),
        role(r)
    {
        deviceId = initTracker(inputEmulator, true);
    }

    void update(vr::HmdVector3_t additionalOffset, vr::HmdVector3_t rawJointPos, vr::HmdQuaternion_t rawJointRotation) {
        lastRawPos = rawJointPos;
        auto pose = inputEmulatorRef.getVirtualDevicePose(deviceId);
        //JOINT POSITION
        //Perspective Correction for changing coord systems
        //Convert to sf vector for rotation fn
        sf::Vector3f jPosition = { rawJointPos.v[0],rawJointPos.v[1] ,rawJointPos.v[2] };

        //Rotate the position around the kinect rep's rot
        if (isSensor()) {
            //Rotate arrow by 180
            rawJointRotation = rawJointRotation * vrmath::quaternionFromRotationY(PI);
        }
        sf::Vector3f laterallyRotatedPos = KMath::rotate(jPosition, { 0,1,0 },  KinectSettings::kinectRadRotation.v[1]);
        sf::Vector3f tiltRotatedPos = KMath::rotate(laterallyRotatedPos, { 1,0,0 }, KinectSettings::kinectRadRotation.v[0]);
        vr::HmdVector3_t pos = { { tiltRotatedPos.x,tiltRotatedPos.y,tiltRotatedPos.z } };

        //Adjust this position by the Kinect's VR pos offset
        pos.v[0] += KinectSettings::kinectRepPosition.v[0];
        pos.v[1] += KinectSettings::kinectRepPosition.v[1];
        pos.v[2] += KinectSettings::kinectRepPosition.v[2];

        //std::cerr << "jPOS:" << pos.v[0] << ", " << pos.v[1] << ", " << pos.v[2] << "\n";
        //JOINT ROTATION
        if (!isSensor()) {
            //std::cerr << "KROT:" << KinectSettings::kinectRepRotation.w << ", " << KinectSettings::kinectRepRotation.x << ", " << KinectSettings::kinectRepRotation.y << ", " << KinectSettings::kinectRepRotation.z << "\n";
            //std::cerr << "RAW:" << rawJointRotation.w << ", " << rawJointRotation.x << ", " << rawJointRotation.y << ", " << rawJointRotation.z << "\n";


            rawJointRotation =  KinectSettings::kinectRepRotation * rawJointRotation;


            //std::cerr << "ADJ:" << rawJointRotation.w << ", " << rawJointRotation.x << ", " << rawJointRotation.y << ", " << rawJointRotation.z << "\n";
        }
        pose.qRotation.w = rawJointRotation.w;
        pose.qRotation.x = rawJointRotation.x;
        pose.qRotation.y = rawJointRotation.y;
        pose.qRotation.z = rawJointRotation.z;
        pose.vecPosition[0] = pos.v[0] + trackedPositionVROffset.v[0];
        pose.vecPosition[1] = pos.v[1] + trackedPositionVROffset.v[1];
        pose.vecPosition[2] = pos.v[2] + trackedPositionVROffset.v[2];
        //Debug
        /*
        if (isKinectRepresentation) {
            
            std::cerr << "KMOV: " << pose.vecPosition[0] << ", " << pose.vecPosition[1] << ", " << pose.vecPosition[2] << '\n';
            std::cerr << "KROT: " << KinectSettings::kinectRadRotation.v[0] << ", " << KinectSettings::kinectRadRotation.v[1] << ", " << KinectSettings::kinectRadRotation.v[2]  << '\n';
        }
        */
        pose.poseIsValid = true;

        pose.result = vr::TrackingResult_Running_OK;
        inputEmulatorRef.setVirtualDevicePose(deviceId, pose);
    }
    /*
    void destroyTrackers(vrinputemulator::VRInputEmulator& inputEmulator, std::vector<KinectTrackedDevice> trackers) {
        for (auto device : trackers) {
            auto pose = inputEmulator.getVirtualDevicePose(device.deviceId);
            if (pose.deviceIsConnected) {
                pose.deviceIsConnected = false;
                pose.poseIsValid = false;
                inputEmulator.setVirtualDevicePose(device.deviceId, pose);
            }
        }
    }
    */
    bool isSensor() {
        return role == KinectDeviceRole::KinectSensor;
    }

    ~KinectTrackedDevice() {
        
    }
    void destroy() {
        destroyTracker(inputEmulatorRef, deviceId);
    }

    vrinputemulator::VRInputEmulator &inputEmulatorRef;
    uint32_t deviceId;

    KVR_Joint::KinectJoint joint0;
    KVR_Joint::KinectJoint joint1;

    vr::HmdVector3_t trackedPositionVROffset;
    vr::HmdVector3_t lastRawPos{ 0,0,0 };

    KinectDeviceRole role;
};

