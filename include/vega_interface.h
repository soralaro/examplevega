//
// Created by JiangGuoqing on 2019-04-28.
//

#ifndef VEGA_INTERFACE_H
#define VEGA_INTERFACE_H

#include "interface_base.h"

/**
 *  Vega is designed as an SDK to deploy inference models and codec functions on different
 *  platforms with unified APIs.
 *
 *  Before any Vega SDK call, you need call SDKInit() to initialize Vega.
 *
 *  Vega SDK provides services thru several interfaces. Each interface is an Executable<TASK>,
 *  that is, an entity provides an interface Executable::execute() to process a batch of tasks,
 *  each of which is a shared_ptr<TASK> and contains both input and output data. Tasks are
 *  organized in a vector, whose size should not exceed Executable::getBatchSize().
 *
 *  To use interface, you need to create tasks vector first. Each interface has its own requirement
 *  of how to fill in the task although some rules are shared in common. You need to review
 *  the task definition comments for more details.
 *
 *  @see SdkTaskBase and SdkTask for detailed information.
 *
 *  All interfaces can be created by a group of functions provided by Vega SDK, which shares
 *  almost same declaration. Assuming interface to be created is Executable<TASK>, the interface
 *  will be:
 *
 *  std::shared_ptr<Executable<TASK>>  createXXXXInterface(
 *      int deviceId, const std::string &cfgPath, const std::string &modelName,
 *      ModelDecryptor decryptor, typename Executable<TASK>::AsyncCallback callback);
 *
 *  Interface itself just identify what kind of <TASK>, or even more precisely, what kind of
 *  returning data type it supports. SO interface with different functions can be created
 *  by same interface, for those interfaces Vega SDK differs actual functionality by
 *  modelName and cfgPath, which are all optional.
 *
 *  modelName and cfgPath should be set with regard of interface type. There are 2 main types
 *  of interface:
 *  1. Inference interfaces:
 *      inference interfaces must be created with valid cfgPath, inside which at least a config.json
 *      must provide. Vega SDK will read config.json to determine which model should be loaded.
 *      So modelName is not required.
 *
 *      Caller should make sure all and only required files are in this directory.
 *
 *  2. Other interfaces
 *     These interfaces includes codec/frame control functions. They does not need a cfgPath.
 *     Most of these interfaces do not require a modelName except few multiple function
 *     combined interfaces like Decode interface, whose function must be specified explicitly
 *     by modelName.
 *
 *  see interface creator for more information about modelName or cfgPath.
 *
 *  Vega SDK now supports model decryption internally. So decryptor, which is used to decrypt model,
 *  is not required any more, but if you provide one instead of nullptr, Vega SDK will prefer
 *  yours. Vega SDK will read config.json to determine which method should be used to decrypt if
 *  you set decryptor to nullptr.
 *
 *  deviceId is the ID of computing device, for CUDA its GPU ID, for HIAI its Davinci device ID.
 *  The deviceId on HI35XX is not yet defined, just use 0.
 *
 *  Interface behaviors as an async call. When execution request is finished, callback will
 *  be called with tasks containing result and an error code will be delivered to caller.
 *  An error other than DG_OK implies there is at least one in these tasks are incorrectly
 *  processed, and you should check SdkTaskBase::error_ of each task.
 *
 *  Although  async call can be manipulated as sync call, to improve performance, interfaces
 *  MUST be used asynchronously. DO NOT send next batch tasks until previous ends.
 *  Another element effects performance is batch size(tasks size). A batch size less than
 *  interface's max batch size(Get by Executable::getBatchSize()) will reduce performance
 *  of execution dramatically. Caller who cares about performance must make sure they use
 *  as bigger batch size as they can.
 *
 *  Although interface behavior same on different platform, they have subtle difference on
 *  deploying, for example, the number of instance. You may call multiple createXXXInterface
 *  to create multiple interfaces for a single functionality, but it won't improve any
 *  performance on HIAI or HISI, but it MAY improve performance on CUDA, especially on
 *  JPEG decoding or encoding. On CUDA, one single instance of interface is incapable of
 *  maximizing performance of GPU, but thru multiple instances it might be. But keep in
 *  mind that this only takes effect when GPU still not reaching it's max power.
 *
 *  All interfaces are maintained by caller by shared pointer. Vega won't keep any instance
 *  of that interface. So to destroy interface, just clear it by shared_ptr::reset().
 *
 *  When SDK should be destroied, it's caller's responsibility to ensure that all interfaces
 *  are destroyed before call SDKDestory(), which must be called explicitly before exiting
 *  program. On some platforms, it may produce a segfault on exiting if it's not called.
 */

namespace vega {

    /**
     * Image or video decode interface.
     * Decoded frame will be saved in matrix pool, and identified by returned stream id + frame id.
     *
     * @modelName must be @Model::decode_frame or @Model::decode_video
     *
     * for CUDA:
     *  - create an independant decode interface for each stream will improve performance
     */
    extern std::shared_ptr<DecodeInterface>
    createDecodeInterface(int deviceId, const std::string &cfgPath, const std::string &modelName,
                          ModelDecryptor decryptor, typename Executable<DecodeTask>::AsyncCallback callback);

    /**
     * Remove stream and all frames on this stream.
     * @modelName will be ignored
     */
    extern std::shared_ptr<RemoveStreamInterface>
    createRemoveStreamInterface(int deviceId, const std::string &cfgPath, const std::string &modelName,
                                ModelDecryptor decryptor, typename Executable<RemoveStreamTask>::AsyncCallback callback);

    /**
     * Fetch data of a frame.
     * @modelName will be ignored
     */
    extern std::shared_ptr<FetchFrameInterface>
    createFetchFrameInterface(int deviceId, const std::string &cfgPath, const std::string &modelName,
                              ModelDecryptor decryptor, typename Executable<FetchFrameTask>::AsyncCallback callback);

    /**
      * video encode interface.
      * @modelName will be ignored
      * for HIAI:
      *  - One device can only encoding one H264/H265 video stream
      */
    extern std::shared_ptr<EncodeInterface>
    createEncodeInterface(int deviceId, const std::string &cfgPath, const std::string &modelName,
                          ModelDecryptor decryptor, typename Executable<EncodeTask>::AsyncCallback callback);


    /**
     * Delete a frame.
     * @modelName must be @Model::delete_frame
     */
    extern std::shared_ptr<FreeFrameInterface>
    createFreeFrameInterface(int deviceId, const std::string &cfgPath, const std::string &modelName,
                             ModelDecryptor decryptor, typename Executable<FreeFrameTask>::AsyncCallback callback);
    /**
     * Create a detection interface.
     * @modelName can be ""
     */
    extern std::shared_ptr<DetectInterface>
    createDetectInterface(int deviceId, const std::string &cfgPath, const std::string &modelName,
                          ModelDecryptor decryptor, typename Executable<DetectTask>::AsyncCallback callback);
    /**
     * Create classification interface
     * @modelName can be ""
     */
    extern std::shared_ptr<ClassifierInterface>
    createClassifierInterface(int deviceId, const std::string &cfgPath, const std::string &modelName,
                              ModelDecryptor decryptor, typename Executable<ClassifierTask>::AsyncCallback callback);
    /**
     * Create a data flow interface.
     * @modelName can be ""
     */
    extern std::shared_ptr<DataFlowInterface>
    createDataFlowInterface(int deviceId, const std::string &cfgPath, const std::string &modelName,
                            ModelDecryptor decryptor, typename Executable<DataFlowTask>::AsyncCallback callback);
    /**
     * Create Vehicle Brand interface
     * @modelName will be ignored
     */
    extern std::shared_ptr<VehicleBrandInterface>
    createVehicleBrandInterface(int deviceId, const std::string &cfgPath, const std::string &modelName,
                                ModelDecryptor decryptor, typename Executable<VehicleBrandTask>::AsyncCallback callback);
    /**
     * Create Face align transform interface.
     * @modelName will be ignored
     * if only transform is applied, you may set @cfgPath to "" or "-"(without giving an dir containing config.json)
     */
    extern std::shared_ptr<FaceAlignTransformInterface>
    createFaceAlignTransformInterface(int deviceId, const std::string &cfgPath, const std::string &modelName,
                                      ModelDecryptor decryptor, typename Executable<FaceAlignTransformTask>::AsyncCallback callback);
    /**
     * Create Face landmark & pose interface.
     * @modelName will be ignored
     */
    extern std::shared_ptr<LandmarkPoseInterface>
    createLandmarkPoseInterface(int deviceId, const std::string &cfgPath, const std::string &modelName,
                                ModelDecryptor decryptor, typename Executable<LandmarkPoseTask>::AsyncCallback callback);
    /**
     * Create Plate rectify interface
     * @modelName will be ignored
     */
    extern std::shared_ptr<PlateRectifyInterface>
    createPlateRectifyInterface(int deviceId, const std::string &cfgPath, const std::string &modelName,
                                      ModelDecryptor decryptor, typename Executable<PlateRectifyTask>::AsyncCallback callback);
    /**
     * Create Plate char interface
     * @modelName will be ignored
     */
    extern std::shared_ptr<PlateCharInterface>
    createPlateCharInterface(int deviceId, const std::string &cfgPath, const std::string &modelName,
                                ModelDecryptor decryptor, typename Executable<PlateCharTask>::AsyncCallback callback);
    /**
     * Create plate recognition interface
     * To get category of plate, rule file named as "plate_rule.json" must be placed in @cfgPath
     * @modelName will be ignored
     */
    extern std::shared_ptr<PlateRecogInterface>
    createPlateRecogInterface(int deviceId, const std::string &cfgPath, const std::string &modelName,
                         ModelDecryptor decryptor, typename Executable<PlateRecogTask>::AsyncCallback callback);

    /**
     * Create plate quality interface
     */
    extern std::shared_ptr<PlateQualityInterface>
    createPlateQualityInterface(int deviceId, const std::string &cfgPath, const std::string &modelName,
                              ModelDecryptor decryptor, typename Executable<PlateQualityTask>::AsyncCallback callback);
    /**
     * Create KeyPoint interface
     * @modelName can be ""
     */
    extern std::shared_ptr<KeyPointInterface>
    createKeyPointInterface(int deviceId, const std::string &cfgPath, const std::string &modelName,
                            ModelDecryptor decryptor, typename Executable<KeyPointTask>::AsyncCallback callback);
    /**
 * Create AIimage interface
 * @modelName can be ""
 */
    extern std::shared_ptr<AIimageInterface>
    createAIimageInterface(int deviceId, const std::string &cfgPath, const std::string &modelName,
                            ModelDecryptor decryptor, typename Executable<AIimageTask>::AsyncCallback callback);

    /**
     * Create plate rectify of quality interface
     */
    extern std::shared_ptr<PlateQualityRectifyInterface> createPlateQualityRectifyInterface(int deviceId, const std::string &cfgPath, const std::string &modelName,
                             ModelDecryptor decryptor, typename Executable<PlateQualityRectifyTask>::AsyncCallback callback);
        /**
     * Query system attribute
     *
     * key is supported for:
     * - SysAttribute::arch_: arch string like hiai/pascal/HI3559A...
     *
     * @param device Device id to query on
     * @param key System attribute
     * @param value output value corresponding to key
     * @return DG_OK if query succeeds
     */
        extern DgError SDKQuery(int device, const std::string &key, std::string &value);
    /**
     * Init SDK with given config file path.
     * Currently config is not defined, so set cfgFile to "".
     */
    extern DgError SDKInit(const std::string &cfgFile);
    /**
     * Destroy SDK.
     * Call this to destroy SDK explicitly before exiting, and must make sure all interfaces
     * are released before this call.
     */
    extern void SDKDestroy();

    inline
    float calculate_face_score(float pitch, float yal, float is_face) {
        return (float)(is_face*(0.5*cos(yal*3.14159265/180)+0.5*cos(pitch*3.14159265/180)));
    }

    /*
     * Vega 2.0 model interface, used to replace all model interfaces in vega 1.0
     */
    extern std::shared_ptr<ModelInterface>
    createModelInterface(int deviceId, const std::string &modelPath,
                         typename Executable<ModelTask>::AsyncCallback callback);

    extern std::shared_ptr<ImgTransformInterface>
    createImgTransformInterface(int deviceId, typename Executable<ImgTransformTask>::AsyncCallback callback);
}
#endif //VEGA_INTERFACE_H
