//YgorFileIndexGPX.cc

#include "YgorFileIndexGPX.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>


namespace {

struct gpx_point {
    double latitude = 0.0;
    double longitude = 0.0;
};

struct coverage_grid {
    double width = 0.0;
    int64_t longitude_cells = 0;
    int64_t latitude_cells = 0;
};

coverage_grid make_grid(double width){
    if(!std::isfinite(width) || (width <= 0.0) || (width > 180.0)){
        throw std::invalid_argument("GPX coverage cell width must be finite and in (0, 180]");
    }

    const auto exact_longitude_cells = 360.0 / width;
    const auto longitude_cells = static_cast<int64_t>(std::llround(exact_longitude_cells));
    const auto tolerance = 32.0 * std::numeric_limits<double>::epsilon()
                         * std::max(1.0, std::abs(exact_longitude_cells));
    if((longitude_cells <= 0)
    || (std::abs(exact_longitude_cells - static_cast<double>(longitude_cells)) > tolerance)){
        throw std::invalid_argument("GPX coverage cell width must divide 360 degrees");
    }

    const auto latitude_cells = static_cast<int64_t>(std::ceil(180.0 / width));
    if((latitude_cells <= 0)
    || (longitude_cells > std::numeric_limits<int32_t>::max())
    || (latitude_cells > (std::numeric_limits<int64_t>::max() / longitude_cells))){
        throw std::invalid_argument("GPX coverage grid is too fine to encode in 64-bit cell identifiers");
    }
    return {width, longitude_cells, latitude_cells};
}

std::string canonical_width(double width){
    make_grid(width); // Validate as part of constructing the measure identity.
    std::ostringstream out;
    out << std::setprecision(std::numeric_limits<double>::max_digits10) << width;
    return out.str();
}

std::string lowercase(std::string in){
    std::transform(in.begin(), in.end(), in.begin(), [](unsigned char c){
        return static_cast<char>(std::tolower(c));
    });
    return in;
}

bool gpx_filename(const std::string &filename){
    const auto dot = filename.find_last_of('.');
    return (dot != std::string::npos) && (lowercase(filename.substr(dot)) == ".gpx");
}

std::string local_tag_name(const std::string &tag, bool &closing){
    std::size_t pos = 0;
    while((pos < tag.size()) && std::isspace(static_cast<unsigned char>(tag[pos]))){
        ++pos;
    }
    closing = (pos < tag.size()) && (tag[pos] == '/');
    if(closing){
        ++pos;
    }
    if((pos >= tag.size()) || (tag[pos] == '!') || (tag[pos] == '?')){
        return {};
    }

    const auto begin = pos;
    while((pos < tag.size())
       && !std::isspace(static_cast<unsigned char>(tag[pos]))
       && (tag[pos] != '/')){
        ++pos;
    }
    auto name = tag.substr(begin, pos - begin);
    const auto colon = name.find_last_of(':');
    if(colon != std::string::npos){
        name = name.substr(colon + 1);
    }
    return name;
}

bool attribute_value(const std::string &tag,
                     const std::string &attribute,
                     std::string &value){
    std::size_t pos = 0;
    while((pos = tag.find(attribute, pos)) != std::string::npos){
        const bool left_ok = (pos == 0)
                          || std::isspace(static_cast<unsigned char>(tag[pos - 1]));
        auto after = pos + attribute.size();
        const bool right_name_ok = (after >= tag.size())
                                || std::isspace(static_cast<unsigned char>(tag[after]))
                                || (tag[after] == '=');
        if(!left_ok || !right_name_ok){
            ++pos;
            continue;
        }
        while((after < tag.size()) && std::isspace(static_cast<unsigned char>(tag[after]))){
            ++after;
        }
        if((after >= tag.size()) || (tag[after] != '=')){
            ++pos;
            continue;
        }
        ++after;
        while((after < tag.size()) && std::isspace(static_cast<unsigned char>(tag[after]))){
            ++after;
        }
        if((after >= tag.size()) || ((tag[after] != '\'') && (tag[after] != '"'))){
            throw std::runtime_error("Malformed GPX attribute: " + attribute);
        }
        const auto quote = tag[after++];
        const auto end = tag.find(quote, after);
        if(end == std::string::npos){
            throw std::runtime_error("Unterminated GPX attribute: " + attribute);
        }
        value = tag.substr(after, end - after);
        return true;
    }
    return false;
}

double parse_coordinate(const std::string &text, const std::string &name){
    std::size_t consumed = 0;
    double value = 0.0;
    try{
        value = std::stod(text, &consumed);
    }catch(...){
        throw std::runtime_error("Unable to parse GPX " + name + " coordinate");
    }
    if((consumed != text.size()) || !std::isfinite(value)){
        throw std::runtime_error("Invalid GPX " + name + " coordinate");
    }
    return value;
}

template <class F>
void for_each_gpx_point(const std::string &filename, F &&visitor){
    std::ifstream input(filename, std::ios::in | std::ios::binary);
    if(!input){
        throw std::runtime_error("Unable to open GPX file");
    }

    bool break_before_next = true;
    bool in_tag = false;
    std::string tag;
    char c = '\0';
    while(input.get(c)){
        if(!in_tag){
            if(c == '<'){
                in_tag = true;
                tag.clear();
            }
            continue;
        }

        if(c != '>'){
            if(tag.size() >= (1024U * 1024U)){
                throw std::runtime_error("Unreasonably large GPX XML tag");
            }
            tag.push_back(c);
            continue;
        }

        in_tag = false;
        bool closing = false;
        const auto name = local_tag_name(tag, closing);
        if((name == "trkseg") || (name == "rte")){
            break_before_next = true;
            continue;
        }
        if(closing || ((name != "trkpt") && (name != "rtept"))){
            continue;
        }

        std::string latitude_text;
        std::string longitude_text;
        if(!attribute_value(tag, "lat", latitude_text)
        || !attribute_value(tag, "lon", longitude_text)){
            throw std::runtime_error("GPX track/route point is missing latitude or longitude");
        }

        gpx_point point{parse_coordinate(latitude_text, "latitude"),
                        parse_coordinate(longitude_text, "longitude")};
        if((point.latitude < -90.0) || (90.0 < point.latitude)
        || (point.longitude < -180.0) || (180.0 < point.longitude)){
            throw std::runtime_error("GPX coordinate is outside the WGS84 latitude/longitude domain");
        }
        visitor(point, break_before_next);
        break_before_next = false;
    }

    if(in_tag){
        throw std::runtime_error("Truncated GPX XML tag");
    }
}

std::vector<file_index_fact> latitude_extent(const std::string &filename){
    bool found = false;
    double min_latitude = 0.0;
    double max_latitude = 0.0;
    for_each_gpx_point(filename, [&](const gpx_point &point, bool){
        if(!found){
            min_latitude = max_latitude = point.latitude;
            found = true;
        }else{
            min_latitude = std::min(min_latitude, point.latitude);
            max_latitude = std::max(max_latitude, point.latitude);
        }
    });
    if(!found){
        return {};
    }
    return {{{file_index_range{min_latitude, max_latitude}}}};
}

void add_longitude_segment(std::vector<file_index_range> &ranges,
                           double a,
                           double b){
    const auto delta = b - a;
    if(std::abs(delta) <= 180.0){
        ranges.push_back({std::min(a, b), std::max(a, b)});
        return;
    }
    const auto low = std::min(a, b);
    const auto high = std::max(a, b);
    ranges.push_back({-180.0, low});
    ranges.push_back({high, 180.0});
}

std::vector<file_index_fact> longitude_extent(const std::string &filename){
    std::vector<file_index_range> ranges;
    bool have_previous = false;
    gpx_point previous;
    for_each_gpx_point(filename, [&](const gpx_point &point, bool break_before){
        if(break_before || !have_previous){
            ranges.push_back({point.longitude, point.longitude});
        }else{
            add_longitude_segment(ranges, previous.longitude, point.longitude);
        }
        previous = point;
        have_previous = true;
    });
    if(ranges.empty()){
        return {};
    }

    std::sort(ranges.begin(), ranges.end(), [](const auto &lhs, const auto &rhs){
        return std::tie(lhs.min, lhs.max) < std::tie(rhs.min, rhs.max);
    });
    std::vector<file_index_range> merged;
    for(const auto &range : ranges){
        if(merged.empty() || (merged.back().max < range.min)){
            merged.push_back(range);
        }else{
            merged.back().max = std::max(merged.back().max, range.max);
        }
    }

    std::vector<file_index_fact> facts;
    facts.reserve(merged.size());
    for(const auto &range : merged){
        facts.push_back({range});
    }
    return facts;
}

int64_t positive_mod(int64_t value, int64_t modulus){
    auto out = value % modulus;
    if(out < 0){
        out += modulus;
    }
    return out;
}

int64_t latitude_cell(const coverage_grid &grid, double latitude){
    auto cell = static_cast<int64_t>(std::floor((latitude + 90.0) / grid.width));
    cell = std::max<int64_t>(0, std::min<int64_t>(grid.latitude_cells - 1, cell));
    return cell;
}

int64_t encoded_cell(const coverage_grid &grid, int64_t longitude_cell, int64_t latitude_cell_in){
    if((latitude_cell_in < 0) || (grid.latitude_cells <= latitude_cell_in)){
        throw std::logic_error("Latitude cell escaped GPX grid");
    }
    const auto x = positive_mod(longitude_cell, grid.longitude_cells);
    return latitude_cell_in * grid.longitude_cells + x;
}

void add_point_cell(const coverage_grid &grid,
                    const gpx_point &point,
                    std::set<int64_t> &cells){
    const auto x = static_cast<int64_t>(std::floor((point.longitude + 180.0) / grid.width));
    const auto y = latitude_cell(grid, point.latitude);
    cells.insert(encoded_cell(grid, x, y));
}

void add_segment_cells(const coverage_grid &grid,
                       const gpx_point &a,
                       const gpx_point &b,
                       std::set<int64_t> &cells){
    double unwrapped_b_longitude = b.longitude;
    const auto longitude_delta = b.longitude - a.longitude;
    if(longitude_delta > 180.0){
        unwrapped_b_longitude -= 360.0;
    }else if(longitude_delta < -180.0){
        unwrapped_b_longitude += 360.0;
    }

    const double x0 = (a.longitude + 180.0) / grid.width;
    const double x1 = (unwrapped_b_longitude + 180.0) / grid.width;
    const double y0 = std::min((a.latitude + 90.0) / grid.width,
                               std::nextafter(static_cast<double>(grid.latitude_cells), 0.0));
    const double y1 = std::min((b.latitude + 90.0) / grid.width,
                               std::nextafter(static_cast<double>(grid.latitude_cells), 0.0));

    int64_t ix = static_cast<int64_t>(std::floor(x0));
    int64_t iy = static_cast<int64_t>(std::floor(y0));
    const int64_t end_x = static_cast<int64_t>(std::floor(x1));
    const int64_t end_y = static_cast<int64_t>(std::floor(y1));
    cells.insert(encoded_cell(grid, ix, iy));

    const double dx = x1 - x0;
    const double dy = y1 - y0;
    const int64_t step_x = (dx > 0.0) ? 1 : ((dx < 0.0) ? -1 : 0);
    const int64_t step_y = (dy > 0.0) ? 1 : ((dy < 0.0) ? -1 : 0);
    const double inf = std::numeric_limits<double>::infinity();
    const double t_delta_x = (step_x == 0) ? inf : (1.0 / std::abs(dx));
    const double t_delta_y = (step_y == 0) ? inf : (1.0 / std::abs(dy));
    double t_max_x = (step_x > 0) ? ((std::floor(x0) + 1.0 - x0) * t_delta_x)
                                  : ((step_x < 0) ? ((x0 - std::floor(x0)) * t_delta_x) : inf);
    double t_max_y = (step_y > 0) ? ((std::floor(y0) + 1.0 - y0) * t_delta_y)
                                  : ((step_y < 0) ? ((y0 - std::floor(y0)) * t_delta_y) : inf);

    const auto close = [](double lhs, double rhs){
        if(!std::isfinite(lhs) || !std::isfinite(rhs)){
            return lhs == rhs;
        }
        const auto scale = std::max({1.0, std::abs(lhs), std::abs(rhs)});
        return std::abs(lhs - rhs) <= (16.0 * std::numeric_limits<double>::epsilon() * scale);
    };

    while((ix != end_x) || (iy != end_y)){
        if(t_max_x < t_max_y && !close(t_max_x, t_max_y)){
            ix += step_x;
            t_max_x += t_delta_x;
            cells.insert(encoded_cell(grid, ix, iy));
        }else if(t_max_y < t_max_x && !close(t_max_x, t_max_y)){
            iy += step_y;
            t_max_y += t_delta_y;
            if((0 <= iy) && (iy < grid.latitude_cells)){
                cells.insert(encoded_cell(grid, ix, iy));
            }
        }else{
            if(step_x != 0){
                cells.insert(encoded_cell(grid, ix + step_x, iy));
            }
            if((step_y != 0) && (0 <= (iy + step_y)) && ((iy + step_y) < grid.latitude_cells)){
                cells.insert(encoded_cell(grid, ix, iy + step_y));
            }
            ix += step_x;
            iy += step_y;
            t_max_x += t_delta_x;
            t_max_y += t_delta_y;
            if((0 <= iy) && (iy < grid.latitude_cells)){
                cells.insert(encoded_cell(grid, ix, iy));
            }
        }
    }
}

std::vector<file_index_fact> coverage_cells(const std::string &filename, double width){
    const auto grid = make_grid(width);
    std::set<int64_t> cells;
    bool have_previous = false;
    gpx_point previous;
    for_each_gpx_point(filename, [&](const gpx_point &point, bool break_before){
        add_point_cell(grid, point, cells);
        if(have_previous && !break_before){
            add_segment_cells(grid, previous, point, cells);
        }
        previous = point;
        have_previous = true;
    });

    std::vector<file_index_fact> facts;
    facts.reserve(cells.size());
    for(const auto cell : cells){
        facts.push_back({cell});
    }
    return facts;
}

} // namespace


file_index_measure File_Index_GPX_Latitude_Extent_Measure(){
    return {"gpx.latitude_extent", 1, file_index_value_kind::range, {}};
}

file_index_measure File_Index_GPX_Longitude_Extent_Measure(){
    return {"gpx.longitude_extent", 1, file_index_value_kind::range, {}};
}

file_index_measure File_Index_GPX_Coverage_Cell_Measure(double cell_width_degrees){
    return {"gpx.coverage_cell", 1, file_index_value_kind::integer,
            "cell_width_degrees=" + canonical_width(cell_width_degrees)};
}

void Register_GPX_File_Index_Measures(sqlite_file_index &index,
                                      double cell_width_degrees){
    const auto applicable = [](const std::string &filename, const file_index_measure &){
        return gpx_filename(filename);
    };

    index.register_measure(File_Index_GPX_Latitude_Extent_Measure(),
        [](const std::string &filename, const file_index_measure &){
            return latitude_extent(filename);
        }, applicable);

    index.register_measure(File_Index_GPX_Longitude_Extent_Measure(),
        [](const std::string &filename, const file_index_measure &){
            return longitude_extent(filename);
        }, applicable);

    index.register_measure(File_Index_GPX_Coverage_Cell_Measure(cell_width_degrees),
        [cell_width_degrees](const std::string &filename, const file_index_measure &){
            return coverage_cells(filename, cell_width_degrees);
        }, applicable);
}

std::vector<int64_t> GPX_Coverage_Cells_For_Bounds(double min_latitude,
                                                   double max_latitude,
                                                   double min_longitude,
                                                   double max_longitude,
                                                   double cell_width_degrees){
    if(!std::isfinite(min_latitude) || !std::isfinite(max_latitude)
    || !std::isfinite(min_longitude) || !std::isfinite(max_longitude)
    || (min_latitude < -90.0) || (90.0 < max_latitude)
    || (min_longitude < -180.0) || (180.0 < min_longitude)
    || (max_longitude < -180.0) || (180.0 < max_longitude)
    || (max_latitude < min_latitude)){
        throw std::invalid_argument("Invalid GPX coverage query bounds");
    }

    const auto grid = make_grid(cell_width_degrees);
    const auto min_y = latitude_cell(grid, min_latitude);
    const auto max_y = latitude_cell(grid, max_latitude);
    std::set<int64_t> cells;

    const auto add_longitude_interval = [&](double lo, double hi){
        const auto first_x = static_cast<int64_t>(std::floor((lo + 180.0) / grid.width));
        const auto last_x = static_cast<int64_t>(std::floor((hi + 180.0) / grid.width));
        for(int64_t y = min_y; y <= max_y; ++y){
            for(int64_t x = first_x; x <= last_x; ++x){
                cells.insert(encoded_cell(grid, x, y));
            }
        }
    };

    if(min_longitude <= max_longitude){
        add_longitude_interval(min_longitude, max_longitude);
    }else{
        add_longitude_interval(min_longitude, 180.0);
        add_longitude_interval(-180.0, max_longitude);
    }

    return {cells.begin(), cells.end()};
}

file_index_predicate GPX_Coverage_Predicate(double min_latitude,
                                            double max_latitude,
                                            double min_longitude,
                                            double max_longitude,
                                            double cell_width_degrees){
    const auto cells = GPX_Coverage_Cells_For_Bounds(min_latitude, max_latitude,
                                                     min_longitude, max_longitude,
                                                     cell_width_degrees);
    std::vector<file_index_value> values;
    values.reserve(cells.size());
    for(const auto cell : cells){
        values.emplace_back(cell);
    }
    return File_Index_Any_Of(File_Index_GPX_Coverage_Cell_Measure(cell_width_degrees),
                             std::move(values));
}
