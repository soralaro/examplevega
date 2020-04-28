//
// Created by chenzhenxiong on 4/22/20.
//

#ifndef VEGACUDA_READ_IMAGE_LIST_H
#define VEGACUDA_READ_IMAGE_LIST_H

#include "dg_types.h"
//#include "vega_memory.h"
namespace vega {
    class ReadImageList {
    public:
        ReadImageList() {};

        virtual ~ReadImageList() {};
    private:
        static std::string &ltrim(std::string &str) {
            auto it2 = std::find_if(str.begin(), str.end(),
                                    [](char ch) { return !std::isspace<char>(ch, std::locale::classic()); });
            str.erase(str.begin(), it2);
            return str;
        }

        static std::string &rtrim(std::string &str) {
            auto it1 = std::find_if(str.rbegin(), str.rend(),
                                    [](char ch) { return !std::isspace<char>(ch, std::locale::classic()); });
            str.erase(it1.base(), str.end());
            return str;
        }

       static  std::string &trim(std::string &str) {
            return ltrim(rtrim(str));
        }

        static bool endwith(const std::string &str, const std::string &substr) {
            if (str.length() < substr.length()) return false;

            return str.substr(str.length() - substr.length()) == substr;
        }

    public:
        static SdkImage read_list(std::vector<std::string> &vlist, const std::string &vdpath) {

            SdkImage vtype = SdkImage::H264;
            std::ifstream fp(vdpath);
            CHECK(fp) << "Image list open fail: " << vdpath;
            std::string line;

            while (std::getline(fp, line)) {
                trim(line);
                if (line.empty() || line[0] == '#') {
                    continue;
                }

                bool esc = false;
                if (line[0] == '"') {
                    auto p = line.find('"', 1);
                    CHECK(p != line.npos) << "line start with \" but not end with it: " << line;
                    for (int i = 1; i < (int) p; i++) {
                        if (line[i] == ',') line[i] = '?';
                    }
                    esc = true;
                }
                std::vector<std::string> vec;
                boost::split(vec, line, boost::is_any_of(","));

                for (auto &s : vec) {
                    trim(s);
                }

                CHECK(!vec.empty());

                if (esc) {
                    for (auto &c : vec[0]) {
                        if (c == '?') c = ',';
                    }
                    for (auto it = vec[0].begin(); it != vec[0].end();) {
                        if (*it == '"') {
                            it = vec[0].erase(it);
                        } else {
                            it++;
                        }
                    }
                }

                if (endwith(vec[0], ".h264")) {
                    CHECK(vtype == SdkImage::IMAGE || vtype == SdkImage::H264);
                    vtype = SdkImage::H264;
                    vlist.push_back(vec[0]);
                } else if (endwith(vec[0], ".h265")) {
                    vtype = SdkImage::H265;
                    vlist.push_back(vec[0]);
                } else if (endwith(vec[0], ".jpg")) {
                    vtype = SdkImage::JPEG;
                    vlist.push_back(vec[0]);
                } else {
                    CHECK(false) << "Only supported h264/h265, got" << vec[0];
                }
            }

            return vtype;
        }
    };

}

#endif //VEGACUDA_READ_IMAGE_LIST_H
