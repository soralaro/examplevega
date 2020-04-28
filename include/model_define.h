//
// Created by gqjiang on 3/14/19.
//

#ifndef VEGA_OP_DEFINE_H
#define VEGA_OP_DEFINE_H


#pragma once

#include <string>

namespace vega {

    class Model {
    public:
        /**
         * Interfaces that not depend on models
         */
        static const std::string decode_video; // decode h264 or h265
        static const std::string decode_frame; // decode images
        static const std::string fetch_frame ; // Include image encode functions
        static const std::string encode_video;
        static const std::string delete_frame ;
        static const std::string delete_stream ;
        static const std::string ai_image;

        /**
         * Detector
         */
        static const std::string vehicle_detect_;
        static const std::string vehicle_window_detect;
        static const std::string vehicle_antenna_detect;
        static const std::string vehicle_marker_detect;
        static const std::string vehicle_redrope_detect;
        static const std::string temp_plate_detect;
        static const std::string dangerous_car_detect;
        static const std::string plate_cover_detect;
        static const std::string face_detect_ ;
        static const std::string face_license_detect_ ;
        static const std::string traffic_lights_detect;
        static const std::string crosswalk_detect;
        static const std::string roadconstruct_detect;
        static const std::string slaghead_detect;
        static const std::string trafficsign_detect;
        static const std::string face_detect;
        static const std::string traffic_lane_line;


        /**
         * Classifier
         */
        static const std::string belt_phone_classifier;
        static const std::string car_damage_classifier;
        static const std::string vehicle_color_classifier;
        static const std::string face_cover_classifier;
        static const std::string plate_cover_classifier;
        static const std::string slag_car_classifier;
        static const std::string yellow_mark_classifier;
        static const std::string dangerous_car_classifier;
        static const std::string pedestrian_attr_classifier;
        static const std::string non_vehicle_attr_classifier;
        static const std::string skin_color_classifier;
        static const std::string second_classifier;
        static const std::string face_quality_ ;
        static const std::string face_attribute_ ;
        static const std::string face_pose_ ;
        static const std::string face_blur_ ;
        static const std::string face_occlusion_ ;
        static const std::string plate_color_classifier ;
        static const std::string helmet_classifier;
        static const std::string lightscolor_classifier;
        static const std::string lightsclass_classifier;
        static const std::string lineclass_classifier;
        static const std::string imagecombination_classifier;
        static const std::string faceblur_classifier;
        static const std::string face_occlusion_classifier;
        static const std::string face_attr_incar_classifier;
        static const std::string face_pose_classifier;
        static const std::string specialusecar_classifier;
        static const std::string slagcover_classifier;
        static const std::string trafficsign_classifier;
        static const std::string face_expression_classifier;
        static const std::string face_attribute_classifier;
        static const std::string face_classification;
        static const std::string plate_quality_classifier;


        /**
         * Data Flow
         */
        static const std::string reid_car_back;
        static const std::string reid_car_front;
        static const std::string reid_person;
        static const std::string reid_non_vehicle;
        static const std::string reclassify;
        static const std::string face_align1_;
        static const std::string face_align2_;
        static const std::string face_feature_;
        static const std::string malebeauty;
        static const std::string femalebeauty;


        /**
         * Keypoint
         */
        static const std::string carlandmark;
        static const std::string trafficline_keypoint;
        static const std::string PersonPose;


        /**
         * Others
         */
        static const std::string plate_detect;
        static const std::string plate;
        static const std::string vehicle_brand;
        static const std::string face_align_graph_; // face align1, align2, transform
        static const std::string plate_rectify;
        static const std::string plate_quality_rectify;
        static const std::string plate_char;
        static const std::string plate_recog_graph; // plate recognition, input: plate box, output plate data
        static const std::string plate_quality_graph; // plate recognition, input: plate box, output plate quality/blur
        static const std::string landmark_pose;
        static const std::string face_svm_score;
        static const std::string img_transform_;
    };


}

#endif //VEGA_OP_DEFINE_H
