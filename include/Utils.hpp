#pragma once
#include "pch.hpp"

namespace Mixijo {
    constexpr std::array NUMBERS{ "0",
          "1",  "2",  "3",  "4",  "5",  "6",  "7",  "8",  "9", "10",
         "11", "12", "13", "14", "15", "16", "17", "18", "19", "20",
         "21", "22", "23", "24", "25", "26", "27", "28", "29", "30",
         "31", "32", "33", "34", "35", "36", "37", "38", "39", "40",
         "41", "42", "43", "44", "45", "46", "47", "48", "49", "50",
         "51", "52", "53", "54", "55", "56", "57", "58", "59", "60",
         "61", "62", "63", "64", "65", "66", "67", "68", "69", "70",
         "71", "72", "73", "74", "75", "76", "77", "78", "79", "80",
         "81", "82", "83", "84", "85", "86", "87", "88", "89", "90",
         "91", "92", "93", "94", "95", "96", "97", "98", "99","100",
        "101","102","103","104","105","106","107","108","109","110",
        "111","112","113","114","115","116","117","118","119","120",
        "121","122","123","124","125","126","127","128","129","130",
        "131","132","133","134","135","136","137","138","139","140",
        "141","142","143","144","145","146","147","148","149","150",
        "151","152","153","154","155","156","157","158","159","160",
        "161","162","163","164","165","166","167","168","169","170",
        "171","172","173","174","175","176","177","178","179","180",
        "181","182","183","184","185","186","187","188","189","190",
        "191","192","193","194","195","196","197","198","199","200"
    };

    constexpr std::vector<std::string_view> split(const std::string_view str, const char delim = ',') {
        std::vector<std::string_view> result;
        int indexCommaToLeftOfColumn = 0;
        int indexCommaToRightOfColumn = -1;
        for (int i = 0; i < static_cast<int>(str.size()); i++) {
            if (str[i] == delim) {
                indexCommaToLeftOfColumn = indexCommaToRightOfColumn;
                indexCommaToRightOfColumn = i;
                int index = indexCommaToLeftOfColumn + 1;
                int length = indexCommaToRightOfColumn - index;
                std::string_view column(str.data() + index, length);
                result.push_back(column);
            }
        }
        const std::string_view finalColumn(str.data() + indexCommaToRightOfColumn + 1, str.size() - indexCommaToRightOfColumn - 1);
        result.push_back(finalColumn);
        return result;
    }

    constexpr std::string_view trim(std::string_view view, const char* t = " \t\n\r\f\v") {
        auto _first = view.find_first_not_of(t);
        if (_first != std::string_view::npos) view = view.substr(_first);
        auto _last = view.find_last_not_of(t);
        if (_last != std::string_view::npos) view = view.substr(0, _last + 1);
        return view;
    }

    template<class Ty>
    constexpr Ty parse(std::string_view view) {
        Ty ty;
        std::from_chars(view.data(), view.data() + view.size(), ty);
        return ty;
    }
}