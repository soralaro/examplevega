export LD_LIBRARY_PATH=./lib/:$LD_LIBRARY_PATH
export VEGA_HOST_MODEL_PATH=/opt/deepglint/ivs_test/va/model/hiai_c32080/
export GLOG_minloglevel=1
export GLOG_v=26
export VEGA_MAX_TEST_IMAGE_WIDTH=4000
export VEGA_MAX_TEST_IMAGE_HEIGHT=4000
export VEGA_THREAD_POOL_CAPACITY=1
mkdir ./result
./bin/test_sample 0 ./imglist/1280x720.jpg
mkdir ./result/h264_en/
./bin/test_video_encoder 0 ./imglist/page_number_h264.list ./result/h264_en h264 1
mkdir ./result/h264_deco
./bin/test_video_decoder 0 ./imglist/h264.list 1 25 ./result/h264_deco
