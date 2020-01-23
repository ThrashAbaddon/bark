// Copyright (c) 2019 fortiss GmbH, Julian Bernhard, Klemens Esterle, Patrick Hart, Tobias Kessler
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.


#ifndef MODULES_WORLD_OPENDRIVE_ROAD_HPP_
#define MODULES_WORLD_OPENDRIVE_ROAD_HPP_

#include <string>
#include <memory>
#include <vector>
#include <map>

#include "modules/world/opendrive/plan_view.hpp"
#include "modules/world/opendrive/lane_section.hpp"
#include "modules/world/opendrive/commons.hpp"

namespace modules {
namespace world {
namespace opendrive {

using XodrLaneSections = std::vector<XodrLaneSectionPtr>;

class XodrRoad {
 public:
  XodrRoad(const std::string& name, XodrRoadId id) : id_(id), name_(name) {}

  explicit XodrRoad(const std::shared_ptr<XodrRoad>& road) :
    id_(road->id_),
    name_(road->name_),
    link_(road->link_),
    reference_(road->reference_),
    lane_sections_(road->lane_sections_) {}
  
  XodrRoad() {}
  virtual ~XodrRoad() {}

  //! getter
  std::shared_ptr<PlanView> get_plan_view() const { return reference_; }
  std::string get_name() const { return name_; }
  XodrRoadId get_id() const { return id_; }
  XodrRoadLink get_link() const { return link_; }
  XodrLaneSections get_lane_sections() const { return lane_sections_; }

  // TODO (@hart): implement function get_next_roads()
  // either one road from successor or multiple roads from junction

  XodrLanes get_lanes() const {
    XodrLanes lanes;
    for (auto& lane_section : lane_sections_) {
      XodrLanes section_lanes = lane_section->get_lanes();
      lanes.insert(section_lanes.begin(), section_lanes.end());
    }
    return lanes;
  }


  //! setter
  void set_id(XodrRoadId id) { id_ = id; }
  void set_name(const std::string& name) { name_ = name; }
  void set_plan_view(PlanViewPtr p) { reference_ = p; }
  void set_link(XodrRoadLink l) { link_ = l; }

  void add_lane_section(XodrLaneSectionPtr l) {
    // additionally we need lane 0 in XodrLaneSection
    // XodrLane lane_0 = create_lane();
    lane_sections_.push_back(l);
  }

 private:
  XodrRoadId id_;
  std::string name_;
  XodrRoadLink link_;
  PlanViewPtr reference_;
  XodrLaneSections lane_sections_;
};

using XodrRoadPtr = std::shared_ptr<XodrRoad>;
using XodrRoads = std::map<XodrRoadId, XodrRoadPtr>;
using XodrRoadSequence = std::vector<XodrRoadId>;

}  // namespace opendrive
}  // namespace world
}  // namespace modules

#endif  // MODULES_WORLD_OPENDRIVE_ROAD_HPP_
