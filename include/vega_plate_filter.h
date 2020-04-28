#include "interface_base.h"
#include "dg_types.h"
#include "jsoncpp/json/json.h"
#include <iostream>
#include <fstream>
#include <numeric>

#if USE_DGJSON
namespace Json = dgJson;
#endif

namespace vega {

    const wchar_t  DEFAULT_LOCAL_HEAD_CHAR =  L' ';

    class PlateMatcher;

    /**
     * Processing sequence:
     * 1. correct
     * 2. get category
     * 3. filter by location
     * 4. filter as scenario request.
     */
    class PlateFilter {
    public:
        PlateFilter() = default;
        ~PlateFilter() = default;

    public:

        /**
         * V1 Rule
         * @param plates
         */
        void filterOldRule(std::vector<PlateRecogData> &plates, wchar_t local, const cv::Rect &roi);
        /**
         * Loading rule definition.
         * Json file must be UTF-8 without BOM.
         *
         * You need to load rule file before call @getCategory
         * @param path json file path of plate matching rules
         */
        DgError loadRule(const std::string &path);

        /**
         * Get plate category id, current it's matrix id
         * @param plate plate recognition data
         * @return -1 if category can not be identified, otherwise GB1400 ID
         */
        int getCategory(PlateRecogData &plate);

    public:
        enum class PetroStationPattern {
            /** average confidence of all characters */
            avg_conf = 1,
            /** average confidence of characters except leading char(mostly it's province) */
            avg_letter_conf = 2,
            min_letter_conf = 3,
            avg_max_six_letter_conf = 4
        };

        /**
         * Filter a single plate for petro station.
         * Plate with confidence calculated by algorithm pattern defined will pass
         * the filtering only it's greater than threshold
         * @param plate plate recognition data
         * @param pattern filter pattern
         * @param threshold filter threshold
         * @return true if plate passes filtering, false if not.
         */
        bool filterPetroStation(PlateRecogData &plate,
                                PetroStationPattern pattern,
                                float threshold) const;
        /**
         * Scenario filtering: Petro Station
         * Plates
         * @param plates
         * @param pattern
         * @param threshold
         */
        void filterPetroStation(std::vector<PlateRecogData> &plates, PetroStationPattern pattern, float threshold) const;
        /**
         * Scenario filtering: Security
         */
        void filterSecurity(std::vector<PlateRecogData> &plates) const;
    public:
        /**
         * Location filtering.
         * @param objs Detected plates in a car
         * @param roi roi of car in the same coordinates of objs.
         */
        void filterByLocation (std::vector<BBox> &objs, const cv::Rect &roi) const;
        void filterByLocation (std::vector<PlateRecogData> &objs, const cv::Rect &roi) const;

    public:
        /**
         * Correction plate data.
         *
         * if local is set to DEFAULT_LOCAL_HEAD_CHAR, no local replacement will be executed.
         * otherwise, if first char confidence < local_thres, it will be replaced with #local.
         *
         * If any char is replaced, it will be replaced with a score #replace_score.
         *
         * @param plate plate data
         * @param local set to DEFAULT_LOCAL_HEAD_CHAR to avoid local char replacement
         * @param local_thres threshold to determine if first char should be replaced
         * @param replace_score confidence for those chars be replaced.
         * @return true if plate can not be corrected(invalid plate)
         */
        bool correctPlate(PlateRecogData &plate, wchar_t local, float local_thres, float replace_score);

    protected:
        typedef enum {
            PLATE_HDR,
            PLATE_SEQ,
            PLATE_TAIL,
            PLATE_MAX
        } PlatePart;
        void eraseMinConf(int iletternum, std::vector<PlateChar> &chars);

    protected:

        std::shared_ptr<PlateMatcher> matcher_;
    public:
        static inline bool isChinese(wchar_t ch) {
            const std::wstring ch_list = L"京津沪渝冀豫云辽黑湘皖闽鲁新苏浙赣鄂桂甘晋蒙陕吉贵粤青藏川宁琼军使空海北沈兰济南广成海口领学警港挂澳";
            if (ch_list.find(ch) != std::wstring::npos) {
                return true;
            }
            return false;
        }

        static inline bool isProvice(wchar_t ch) {
            const static std::wstring province = L"京津沪渝冀豫云辽黑湘皖闽鲁新苏浙赣鄂桂甘晋蒙陕吉贵粤青藏川宁琼";
            if (province.find(ch) != std::wstring::npos) {
                return true;
            }
            return false;
        }

        static inline bool isDigital(wchar_t ch) {
            const static std::wstring digital = L"0123456789";
            if (digital.find(ch) != std::wstring::npos) {
                return true;
            }
            return false;
        }

        static inline bool isLetter(wchar_t ch) {
            const static std::wstring letter = L"ABCDEFGHJKLMNPQRSTUVWXYZ";
            if (letter.find(ch) != std::wstring::npos) {
                return true;
            }
            return false;
        }

        static inline bool isMilitaryPlate(wchar_t ch1, wchar_t ch2) {
            std::wstring s1 = L"VZKHEBSLJNGC";
            std::wstring s2 = L"ABCDEFGHJKLMNOPRSTVY";

            return (s1.find(ch1) != std::wstring::npos && s2.find(ch2) != std::wstring::npos);
        }
        static inline bool isMilitaryHdr(wchar_t ch) {
            const static std::wstring jun = L"空海北沈兰济南广成";
            if (jun.find(ch) != std::wstring::npos) {
                return true;
            }
            return false;
        }
        static Confidence getAvgCharConfExcepCh(PlateRecogData &plate) {
            Confidence total = 0;
            auto cnt = plate.wide_literal.length();

            for (auto &f : plate.literal_confidence) {
                total += f;
            }

            if (isChinese(plate.wide_literal[0])) {
                total -= plate.literal_confidence[0];
                cnt--;
            }

            return total / cnt;
        }
        static Confidence getAvgCharConfExcepCap(PlateRecogData &plate) {
            Confidence total = 0;
            auto cnt = plate.wide_literal.length() - 1;

            for (auto i = 1u; i < cnt + 1; i++) {
                auto &f = plate.literal_confidence[i];
                total += f;
            }

            return total / cnt;
        }
        static Confidence getMinCharConfExcepCap(PlateRecogData &plate) {
            Confidence conf = 1.0;
            auto len = plate.wide_literal.length();

            for (auto i = 1u; i < len; i++) {
                auto &f = plate.literal_confidence[i];
                if (f < conf) {
                    conf = f;
                }
            }

            return conf;
        }
        static Confidence getAvgCharConf(PlateRecogData &plate) {
            Confidence total = 0;
            auto cnt = plate.wide_literal.length();

            for (auto &f : plate.literal_confidence) {
                total += f;
            }

            return total / cnt;
        }
        static Confidence  getAvgCharConfMaxSix(PlateRecogData &plate) {
            unsigned max_n = 6;
            Confidence average = 0.0f;

            std::vector<Confidence> temp(plate.literal_confidence);
            std::sort(temp.begin(), temp.end(), [](Confidence &a, Confidence &b) {
                return a > b;
            });

            if(temp.size() >= max_n) {
                average = (float)std::accumulate(temp.begin(), temp.begin()+max_n, 0.0)/max_n;
            } else {
                average = 0.0f;
            }

            return average;
        }
        static int getPosMaxPlateConfLimit(std::vector<PlateRecogData> &plates) {
            Confidence conf = 0.0f;
            int posmax = -1;

            for (unsigned i = 0; i < plates.size(); i++) {
                Confidence f = getAvgCharConfExcepCh(plates[i]);
                if (f > conf && f > 0.8) {
                    conf = f;
                    posmax = i;
                }
            }
            return posmax;
        }
        inline cv::Point getPlatCenter(const cv::Rect &box) const {
            return cv::Point(box.x + box.width / 2, box.y + box.height / 2);
        }
    };
}
